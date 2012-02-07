/*
     OpenDCP: Builds Digital Cinema Packages
     Copyright (c) 2010-2011 Terrence Meiczinger, All Rights Reserved

     This program is free software: you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published by
     the Free Software Foundation, either version 3 of the License, or
     (at your option) any later version.

     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <QMessageBox>
#include <QInputDialog>
#include <QFileInfo>

#include <AS_DCP.h>
#include <KM_fileio.h>
#include <KM_prng.h>
#include <KM_memio.h>
#include <KM_util.h>
#include <openssl/sha.h>

#include "opendcp.h"
#include "mxf-writer.h"
#include "dialogmxfconversion.h"

using namespace ASDCP;

MxfWriter::MxfWriter(QObject *parent)
    : QThread(parent)
{
    reset();
}

MxfWriter::~MxfWriter()
{

}

void MxfWriter::reset()
{
    cancelled = 0;
    success = 0;
}

Result_t MxfWriter::fillWriterInfo(opendcp_t *opendcp, writer_info_t *writer_info) {
    Kumu::FortunaRNG        rng;
    byte_t                  iv_buf[CBC_BLOCK_SIZE];
    Result_t                result = RESULT_OK;

    writer_info->info.ProductVersion = OPENDCP_VERSION;
    writer_info->info.CompanyName    = OPENDCP_NAME;
    writer_info->info.ProductName    = OPENDCP_NAME;

    // set the label type
    if (opendcp->ns == XML_NS_INTEROP) {
        writer_info->info.LabelSetType = LS_MXF_INTEROP;
    } else if (opendcp->ns == XML_NS_SMPTE) {
        writer_info->info.LabelSetType = LS_MXF_SMPTE;
    } else {
        writer_info->info.LabelSetType = LS_MXF_UNKNOWN;
    }

    // generate a random UUID for this essence
    Kumu::GenRandomUUID(writer_info->info.AssetUUID);

    // start encryption, if set
    if (opendcp->key_flag) {
        Kumu::GenRandomUUID(writer_info->info.ContextID);
        writer_info->info.EncryptedEssence = true;

        if (opendcp->key_id) {
            memcpy(writer_info->info.CryptographicKeyID, opendcp->key_id, UUIDlen);
        } else {
            rng.FillRandom(writer_info->info.CryptographicKeyID, UUIDlen);
        }

        writer_info->aes_context = new AESEncContext;
        result = writer_info->aes_context->InitKey(opendcp->key_value);

        if (ASDCP_FAILURE(result)) {
            delete[] writer_info->aes_context;
            return result;
        }

        result = writer_info->aes_context->SetIVec(rng.FillRandom(iv_buf, CBC_BLOCK_SIZE));

        if (ASDCP_FAILURE(result)) {
            delete[] writer_info->aes_context;
            return result;
        }

        if (opendcp->write_hmac) {
            writer_info->info.UsesHMAC = true;
            writer_info->hmac_context = new HMACContext;
            result = writer_info->hmac_context->InitKey(opendcp->key_value, writer_info->info.LabelSetType);

            if (ASDCP_FAILURE(result)) {
                delete[] writer_info->aes_context;
                delete[] writer_info->hmac_context;
                return result;
            }
        }
    }

    return result;
}

void MxfWriter::run()
{
    Result_t result = writeMxf();

    if (result == RESULT_OK) {
        success = 1;
    } else {
        success = 0;
    }

    emit finished();
}

void MxfWriter::setMxfInputs(opendcp_t *opendcp, QFileInfoList fileList, QString outputFile)
{
    opendcpMxf    = opendcp;
    mxfFileList   = fileList;
    mxfOutputFile = outputFile;
}

Result_t MxfWriter::writeMxf()
{
    Result_t      result = RESULT_OK;
    EssenceType_t essence_type;

    result = ASDCP::RawEssenceType(mxfFileList.at(0).absoluteFilePath().toAscii().constData(), essence_type);

    if (ASDCP_FAILURE(result)) {
        printf("Could determine essence type of  %s\n",mxfFileList.at(0).absoluteFilePath().toAscii().constData());
        result = RESULT_FAIL;
    }

    switch (essence_type) {
        case ESS_JPEG_2000:
        case ESS_JPEG_2000_S:
            if ( opendcpMxf->stereoscopic ) {
                result = writeJ2kStereoscopicMxf(opendcpMxf, mxfFileList, mxfOutputFile);
            } else {
                result = writeJ2kMxf(opendcpMxf, mxfFileList, mxfOutputFile);
            }
            break;
        case ESS_PCM_24b_48k:
        case ESS_PCM_24b_96k:
            result = writePcmMxf(opendcpMxf, mxfFileList, mxfOutputFile);
            break;
        case ESS_MPEG2_VES:
            result = writeMpeg2Mxf(opendcpMxf, mxfFileList, mxfOutputFile);
            break;
        case ESS_TIMED_TEXT:
            result = writeTTMxf(opendcpMxf, mxfFileList, mxfOutputFile);
            break;
        case ESS_UNKNOWN:
            result = RESULT_FAIL;
            break;
    }

    if (ASDCP_FAILURE(result)) {
        return RESULT_FAIL;
    }

    return RESULT_OK;
}

Result_t MxfWriter::writeJ2kMxf(opendcp_t *opendcp, QFileInfoList mxfFileList, QString mxfOuputFile) {
    JP2K::MXFWriter         mxf_writer;
    JP2K::PictureDescriptor picture_desc;
    JP2K::CodestreamParser  j2k_parser;
    JP2K::FrameBuffer       frame_buffer(FRAME_BUFFER_SIZE);
    writer_info_t           writer_info;
    Result_t                result = RESULT_OK;
    ui32_t                  start_frame;
    ui32_t                  mxf_duration;
    ui32_t                  slide_duration;

    // set the starting frame
    if (opendcp->mxf.start_frame && mxfFileList.size() >= (opendcp->mxf.start_frame-1)) {
        start_frame = opendcp->mxf.start_frame - 1; // adjust for zero base
    } else {
        start_frame = 0;
    }

    result = j2k_parser.OpenReadFrame(mxfFileList.at(start_frame).absoluteFilePath().toAscii().constData(), frame_buffer);

    if (ASDCP_FAILURE(result)) {
        printf("Failed to open file %s\n",mxfFileList.at(start_frame).absoluteFilePath().toAscii().constData());
        return result;
    }

    Rational edit_rate(opendcp->frame_rate,1);
    j2k_parser.FillPictureDescriptor(picture_desc);
    picture_desc.EditRate = edit_rate;

    fillWriterInfo(opendcp, &writer_info);

    result = mxf_writer.OpenWrite(mxfOutputFile.toAscii().constData(), writer_info.info, picture_desc);

    if (ASDCP_FAILURE(result)) {
        printf("failed to open output file %s\n",mxfOutputFile.toAscii().constData());
        return result;
    }

    // set the duration of the output mxf
    if (opendcp->slide) {
        mxf_duration = opendcp->mxf.duration;
        slide_duration = mxf_duration / mxfFileList.size();
    } else if (opendcp->mxf.duration && (mxfFileList.size() >= opendcp->mxf.duration)) {
        mxf_duration = opendcp->mxf.duration;
    } else {
        mxf_duration = mxfFileList.size();
    }

    ui32_t i = start_frame;
    ui32_t read = 1;

    // read each input frame and write to the output mxf until duration is reached
    while (!cancelled && ASDCP_SUCCESS(result) && mxf_duration--) {
        if (read) {
            result = j2k_parser.OpenReadFrame(mxfFileList.at(i).absoluteFilePath().toAscii().constData(), frame_buffer);

            if (opendcp->delete_intermediate) {
                unlink(mxfFileList.at(i).absoluteFilePath().toAscii().constData());
            }

            if (ASDCP_FAILURE(result)) {
                printf("Failed to open file %s\n",mxfFileList.at(i).absoluteFilePath().toAscii().constData());
                return result;
            }

            if (opendcp->encrypt_header_flag) {
                frame_buffer.PlaintextOffset(0);
            }

            if (opendcp->slide) {
                read = 0;
            }
        }

        if (opendcp->slide) {
            if (mxf_duration % slide_duration == 0) {
                i++;
                read = 1;
            }
        } else {
            i++;
        }

        result = mxf_writer.WriteFrame(frame_buffer, writer_info.aes_context, writer_info.hmac_context);
        emit frameDone();
    }

    if (result == RESULT_ENDOFFILE) {
        result = RESULT_OK;
    }

    if (ASDCP_FAILURE(result)) {
        printf("not end of file\n");
        return result;
    }

    result = mxf_writer.Finalize();

    if (ASDCP_FAILURE(result)) {
        printf("failed to finalize\n");
        return result;
    }

    return result;
}

Result_t MxfWriter::writeJ2kStereoscopicMxf(opendcp_t *opendcp, QFileInfoList mxfFileList, QString mxfOutputFile)
{
    JP2K::MXFSWriter        mxf_writer;
    JP2K::PictureDescriptor picture_desc;
    JP2K::CodestreamParser  j2k_parser_left;
    JP2K::CodestreamParser  j2k_parser_right;
    JP2K::FrameBuffer       frame_buffer_left(FRAME_BUFFER_SIZE);
    JP2K::FrameBuffer       frame_buffer_right(FRAME_BUFFER_SIZE);
    writer_info_t           writer_info;
    Result_t                result = RESULT_OK;
    ui32_t                  mxf_duration;
    ui32_t                  slide_duration;
    ui32_t                  start_frame;

    // set the starting frame
    if (opendcp->mxf.start_frame && (mxfFileList.size()/2) >= (opendcp->mxf.start_frame-1)) {
        start_frame = opendcp->mxf.start_frame - 1; // adjust for zero base
    } else {
        start_frame = 0;
    }

    result = j2k_parser_left.OpenReadFrame(mxfFileList.at(start_frame).absoluteFilePath().toAscii().constData(), frame_buffer_left);

    if (ASDCP_FAILURE(result)) {
        printf("Failed to open file %s\n",mxfFileList.at(start_frame).absoluteFilePath().toAscii().constData());
        return result;
    }

    result = j2k_parser_right.OpenReadFrame(mxfFileList.at(start_frame+1).absoluteFilePath().toAscii().constData(), frame_buffer_right);

    if (ASDCP_FAILURE(result)) {
        printf("Failed to open file %s\n",mxfFileList.at(start_frame+1).absoluteFilePath().toAscii().constData());
        return result;
    }

    Rational edit_rate(opendcp->frame_rate,1);
    j2k_parser_left.FillPictureDescriptor(picture_desc);
    picture_desc.EditRate = edit_rate;

    fillWriterInfo(opendcp, &writer_info);

    result = mxf_writer.OpenWrite(mxfOutputFile.toAscii().constData(), writer_info.info, picture_desc);

    if (ASDCP_FAILURE(result)) {
        printf("failed to open output file %s\n",mxfOutputFile.toAscii().constData());
        return result;
    }

    // set the duration of the output mxf
    if (opendcp->slide) {
        mxf_duration = opendcp->mxf.duration;
        slide_duration = mxf_duration / (mxfFileList.size() / 2);
    } else if (opendcp->mxf.duration && (mxfFileList.size()/2 >= opendcp->mxf.duration)) {
        mxf_duration = opendcp->mxf.duration;
    } else {
        mxf_duration = mxfFileList.size()/2;
    }

    ui32_t i = 0;
    ui32_t read = 1;

    // read each input frame and write to the output mxf until duration is reached 
    while (!cancelled & ASDCP_SUCCESS(result) && mxf_duration--) {
        if (read) {
            result = j2k_parser_left.OpenReadFrame(mxfFileList.at(i).absoluteFilePath().toAscii().constData(), frame_buffer_left);

            if (opendcp->delete_intermediate) {
                unlink(mxfFileList.at(i).absoluteFilePath().toAscii().constData());
            }

            if (ASDCP_FAILURE(result)) {
                printf("Failed to open file %s\n",mxfFileList.at(i).absoluteFilePath().toAscii().constData());
                return result;
            }

            i++;

            result = j2k_parser_right.OpenReadFrame(mxfFileList.at(i).absoluteFilePath().toAscii().constData(), frame_buffer_right);

            if (opendcp->delete_intermediate) {
                unlink(mxfFileList.at(i).absoluteFilePath().toAscii().constData());
            }

            if (ASDCP_FAILURE(result)) {
                printf("Failed to open file %s\n",mxfFileList.at(i).absoluteFilePath().toAscii().constData());
                return result;
            }

            if (opendcp->encrypt_header_flag) {
                frame_buffer_left.PlaintextOffset(0);
            }

           if (opendcp->encrypt_header_flag) {
                frame_buffer_right.PlaintextOffset(0);
            }

            if (opendcp->slide) {
                read = 0;
            }
        }

        if (opendcp->slide) {
            if (mxf_duration % slide_duration == 0) {
                i++;
                read = 1;
            }
        } else {
            i++;
        }

        result = mxf_writer.WriteFrame(frame_buffer_left, JP2K::SP_LEFT, writer_info.aes_context, writer_info.hmac_context);
        result = mxf_writer.WriteFrame(frame_buffer_right, JP2K::SP_RIGHT, writer_info.aes_context, writer_info.hmac_context);
        emit frameDone();
    }

    if (result == RESULT_ENDOFFILE) {
        result = RESULT_OK;
    }


    if (ASDCP_FAILURE(result)) {
        printf("not end of file\n");
        return result;
    }

    result = mxf_writer.Finalize();

    if (ASDCP_FAILURE(result)) {
        printf("failed to finalize\n");
        return result;
    }

    return result;
}

void filelistFree(filelist_t *filelist) {
    int x;

    for (x=0;x<filelist->file_count;x++) {
        if (filelist->in[x]) {
            free(filelist->in[x]);
        }
        if (filelist->out[x]) {
            free(filelist->out[x]);
        }
    }

    if (filelist->in) {
        free(filelist->in);
    }

    if (filelist->out) {
        free(filelist->out);
    }

    if (filelist) {
        free(filelist);
    }

    return;
}

Result_t MxfWriter::writePcmMxf(opendcp_t *opendcp, QFileInfoList mxfFileList, QString mxfOutputFile)
{
    PCM::FrameBuffer     frame_buffer;
    PCM::AudioDescriptor audio_desc;
    PCM::WAVParser       pcm_parser_channel[mxfFileList.size()];
    PCM::FrameBuffer     frame_buffer_channel[mxfFileList.size()];
    PCM::AudioDescriptor audio_desc_channel[mxfFileList.size()];
    PCM::MXFWriter       mxf_writer;
    writer_info_t        writer_info;
    Result_t             result = RESULT_OK;
    ui32_t               mxf_duration;
    ui32_t               file_count = 0;
  
    Rational edit_rate(opendcp->frame_rate,1);

    // read first file
    result = pcm_parser_channel[0].OpenRead(mxfFileList.at(0).absoluteFilePath().toStdString().c_str(), edit_rate);

    if (ASDCP_FAILURE(result)) {
        dcp_log(LOG_ERROR,"Could not open %s",mxfFileList.at(0).absoluteFilePath().toStdString().c_str());
        return RESULT_FILEOPEN;
    }

    // read audio descriptor 
    pcm_parser_channel[0].FillAudioDescriptor(audio_desc);

    for (file_count = 0; file_count < mxfFileList.size(); file_count++) {
        result = pcm_parser_channel[file_count].OpenRead(
                 mxfFileList.at(file_count).absoluteFilePath().toStdString().c_str(), edit_rate);
        if (ASDCP_FAILURE(result)) {
            dcp_log(LOG_ERROR,"Could not open %s",mxfFileList.at(0).absoluteFilePath().toStdString().c_str());
            return RESULT_FILEOPEN;
        }
        pcm_parser_channel[file_count].FillAudioDescriptor(audio_desc_channel[file_count]);
        if (audio_desc_channel[file_count].AudioSamplingRate != audio_desc.AudioSamplingRate) {
            dcp_log(LOG_ERROR,"Mismatched sampling rate");
            return RESULT_FILEOPEN;
        }
        if (audio_desc_channel[file_count].QuantizationBits != audio_desc.QuantizationBits) {
            dcp_log(LOG_ERROR,"Mismatched bit rate");
            return RESULT_FILEOPEN;
        }
        if (audio_desc_channel[file_count].ContainerDuration != audio_desc.ContainerDuration) {
            dcp_log(LOG_ERROR,"Mismatched duration");
            return RESULT_FILEOPEN;
        }
        frame_buffer_channel[file_count].Capacity(PCM::CalcFrameBufferSize(audio_desc_channel[file_count]));
    }

    if (ASDCP_FAILURE(result)) {
        return RESULT_FILEOPEN;
    }

    //  set total audio characteristics 
    audio_desc.EditRate     = edit_rate;
    audio_desc.ChannelCount = mxfFileList.size();
    audio_desc.BlockAlign   = audio_desc.BlockAlign * mxfFileList.size();
    audio_desc.AvgBps       = audio_desc.AvgBps * mxfFileList.size();

    // set total frame buffer size 
    frame_buffer.Capacity(PCM::CalcFrameBufferSize(audio_desc));
    frame_buffer.Size(PCM::CalcFrameBufferSize(audio_desc));

    // fill write info 
    fillWriterInfo(opendcp, &writer_info);
    result = mxf_writer.OpenWrite(mxfOutputFile.toAscii().constData(), writer_info.info, audio_desc);

    if (ASDCP_FAILURE(result)) {
        return RESULT_FILEOPEN;
    }

    // set duration
    if (!opendcp->duration) {
        mxf_duration = 0xffffffff;
    } else {
        mxf_duration = opendcp->duration;
    }

    while (ASDCP_SUCCESS(result) && mxf_duration--) {
        byte_t *data_s = frame_buffer.Data();
        byte_t *data_e = data_s + frame_buffer.Capacity();
        byte_t sample_size = PCM::CalcSampleSize(audio_desc_channel[0]);
        int    offset = 0;

        // read a frame from each file
        for (file_count = 0; file_count < mxfFileList.size(); file_count++) {
            memset(frame_buffer_channel[file_count].Data(), 0, frame_buffer_channel[file_count].Capacity());
            result = pcm_parser_channel[file_count].ReadFrame(frame_buffer_channel[file_count]);
            if (ASDCP_FAILURE(result)) {
                continue;
            }
            if (frame_buffer_channel[file_count].Size() != frame_buffer_channel[file_count].Capacity()) {
                dcp_log(LOG_INFO,"frame was short, expect size: %d actual size: %d. MXF Duration will be reduced by one frame",
                                  frame_buffer_channel[file_count].Size(), frame_buffer_channel[file_count].Capacity());
                result = RESULT_ENDOFFILE;
                continue;
            }
        }

        // write sample from each frame to output buffer
        if (ASDCP_SUCCESS(result)) {
            while (data_s < data_e) {
                for (file_count = 0; file_count < mxfFileList.size(); file_count++) {
                    byte_t *frame = frame_buffer_channel[file_count].Data()+offset;
                    memcpy(data_s,frame,sample_size);
                    data_s += sample_size;
                }
                offset += sample_size;
            }

            // write the frame
            result = mxf_writer.WriteFrame(frame_buffer, writer_info.aes_context, writer_info.hmac_context);
            emit frameDone();
        }
    }

    if (result == RESULT_ENDOFFILE) {
        result = RESULT_OK;
    }

    if (ASDCP_FAILURE(result)) {
        return RESULT_WRITEFAIL;
    }

    // write footer information
    result = mxf_writer.Finalize();

    if (ASDCP_FAILURE(result)) {
        return RESULT_WRITEFAIL;
    }

    return result;
}

Result_t MxfWriter::writeMpeg2Mxf(opendcp_t *opendcp, QFileInfoList mxfFileList, QString mxfOutputFile)
{
    MPEG2::FrameBuffer     frame_buffer(FRAME_BUFFER_SIZE);
    MPEG2::Parser          mpeg2_parser;
    MPEG2::MXFWriter       mxf_writer;
    MPEG2::VideoDescriptor video_desc;
    writer_info_t          writer_info;
    Result_t               result = RESULT_OK;
    ui32_t                 mxf_duration;

    result = mpeg2_parser.OpenRead(mxfFileList.at(0).absoluteFilePath().toAscii().constData());

    if (ASDCP_FAILURE(result)) {
        printf("Failed to open file %s\n",mxfFileList.at(0).absoluteFilePath().toAscii().constData());
        return result;
    }

    mpeg2_parser.FillVideoDescriptor(video_desc);

    fillWriterInfo(opendcp, &writer_info);

    result = mxf_writer.OpenWrite(mxfOutputFile.toAscii().constData(), writer_info.info, video_desc);

    if (ASDCP_FAILURE(result)) {
        printf("failed to open output file %s\n",mxfOutputFile.toAscii().constData());
        return result;
    }

    result = mpeg2_parser.Reset();

    if (ASDCP_FAILURE(result)) {
        printf("parser reset failed\n");
        return result;
    }

    if (!opendcp->duration) {
        mxf_duration = 0xffffffff;
    } else {
        mxf_duration = opendcp->duration;
    }

    while (ASDCP_SUCCESS(result) && mxf_duration-- && cancelled == 0) {
        result = mpeg2_parser.ReadFrame(frame_buffer);

        if (ASDCP_FAILURE(result)) {
            continue;
        }

        if (opendcp->encrypt_header_flag) {
            frame_buffer.PlaintextOffset(0);
        }

        result = mxf_writer.WriteFrame(frame_buffer, writer_info.aes_context, writer_info.hmac_context);
    }

    if (result == RESULT_ENDOFFILE) {
        result = RESULT_OK;
    }

    if (ASDCP_FAILURE(result)) {
        printf("not end of file\n");
        return result;
    }

    result = mxf_writer.Finalize();

    if (ASDCP_FAILURE(result)) {
        printf("failed to finalize\n");
        return result;
    }

    return result;
}

Result_t MxfWriter::writeTTMxf(opendcp_t *opendcp, QFileInfoList mxfFileList, QString mxfOutputFile)
{
    TimedText::DCSubtitleParser    tt_parser;
    TimedText::MXFWriter           mxf_writer;
    TimedText::FrameBuffer         frame_buffer(FRAME_BUFFER_SIZE);
    TimedText::TimedTextDescriptor tt_desc;
    TimedText::ResourceList_t::const_iterator resource_iterator;
    writer_info_t                  writer_info;
    std::string                    xml_doc;
    Result_t                       result = RESULT_OK;

    result = tt_parser.OpenRead(mxfFileList.at(0).absoluteFilePath().toAscii().constData());

    if (ASDCP_FAILURE(result)) {
        printf("Failed to open file %s\n",mxfFileList.at(0).absoluteFilePath().toAscii().constData());
        return result;
    }

    tt_parser.FillTimedTextDescriptor(tt_desc);

    fillWriterInfo(opendcp, &writer_info);

    result = mxf_writer.OpenWrite(mxfOutputFile.toAscii().constData(), writer_info.info, tt_desc);
    result = tt_parser.ReadTimedTextResource(xml_doc);

    if (ASDCP_FAILURE(result)) {
        printf("Could not read Time Text Resource\n");
        return result;
    }

    result = mxf_writer.WriteTimedTextResource(xml_doc, writer_info.aes_context, writer_info.hmac_context);

    if (ASDCP_FAILURE(result)) {
        printf("Could not write Time Text Resource\n");
        return result;
    }

    resource_iterator = tt_desc.ResourceList.begin();

    while (ASDCP_SUCCESS(result) && resource_iterator != tt_desc.ResourceList.end() && cancelled == 0) {
        result = tt_parser.ReadAncillaryResource((*resource_iterator++).ResourceID, frame_buffer);

        if (ASDCP_FAILURE(result)) {
          printf("Could not read Time Text Resource\n");
          return result;
        }

        result = mxf_writer.WriteAncillaryResource(frame_buffer, writer_info.aes_context, writer_info.hmac_context);
    }

    if (result == RESULT_ENDOFFILE) {
        result = RESULT_OK;
    }

    if (ASDCP_FAILURE(result)) {
        printf("not end of file\n");
        return result;
    }

    result = mxf_writer.Finalize();

    if (ASDCP_FAILURE(result)) {
        printf("failed to finalize\n");
        return result;
    }

    return result;
}
