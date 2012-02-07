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

#include <AS_DCP.h>
#include <KM_fileio.h>
#include <KM_prng.h>
#include <KM_memio.h>
#include <KM_util.h>
#include <WavFileWriter.h>
#include <openssl/sha.h>

#include <iostream>
#include <assert.h>

#include "asdcp_intf.h"
#include "opendcp.h"

using namespace ASDCP;

const ui32_t FRAME_BUFFER_SIZE = 4 * Kumu::Megabyte;

/* generate a random UUID */
extern "C" void uuid_random(char *uuid) {
    char buffer[64];
    Kumu::UUID TmpID;
    Kumu::GenRandomValue(TmpID);
    sprintf(uuid,"%.36s", TmpID.EncodeHex(buffer, 64));
}

/* calcuate the SHA1 digest of a file */
extern "C" int calculate_digest(const char *filename, char *digest)
{
    using namespace Kumu;

    FileReader    reader;
    SHA_CTX       sha_context;
    ByteString    read_buffer(16384);
    ui32_t        read_length;
    const ui32_t  sha_length = 20;
    byte_t        byte_buffer[sha_length];
    char          sha_buffer[64];
    Result_t      result = RESULT_OK;

    result = reader.OpenRead(filename);
    SHA1_Init(&sha_context);

    while (ASDCP_SUCCESS(result)) {
        read_length = 0;
        result = reader.Read(read_buffer.Data(), read_buffer.Capacity(), &read_length);

        if (ASDCP_SUCCESS(result)) {
            SHA1_Update(&sha_context, read_buffer.Data(), read_length);
        }
    }

    if (result == RESULT_ENDOFFILE) {
        result = RESULT_OK;
    }

    if (ASDCP_SUCCESS(result)) {
        SHA1_Final(byte_buffer, &sha_context);
        sprintf(digest,"%.36s",base64encode(byte_buffer, sha_length, sha_buffer, 64));
    }

    if (ASDCP_SUCCESS(result)) {
        return DCP_SUCCESS;
    } else {
        return MXF_CALC_DIGEST_FAILED;
    }
}

/* get wav duration */
extern "C" int get_wav_duration(const char *filename, int frame_rate) {
    PCM::WAVParser       pcm_parser;
    PCM::AudioDescriptor audio_desc;
    Result_t             result = RESULT_OK;
    ui32_t               duration;

    Rational edit_rate(frame_rate,1);

    result = pcm_parser.OpenRead(filename, edit_rate);

    if (ASDCP_SUCCESS(result)) {
        pcm_parser.FillAudioDescriptor(audio_desc);
        duration = audio_desc.ContainerDuration; 
    } else {
        duration = 0;
    }

    return duration;
}

/* get the essence class of a file */
extern "C" int get_file_essence_class(char *filename) {
    Result_t      result = RESULT_OK;
    EssenceType_t essence_type;
    int essence_class = -1;

    result = ASDCP::EssenceType(filename, essence_type);

    if (ASDCP_FAILURE(result)) {
        return MXF_COULD_NOT_DETECT_ESSENCE_TYPE;
    }

    /* If type is unknown, try reading raw */
    if (essence_type == ESS_UNKNOWN) {
       result = ASDCP::RawEssenceType(filename, essence_type);
    }

    if (ASDCP_FAILURE(result)) {
        return MXF_COULD_NOT_DETECT_ESSENCE_TYPE;
    }

    switch (essence_type) {
        case ESS_JPEG_2000:
        case ESS_JPEG_2000_S:
        case ESS_MPEG2_VES:
            essence_class = ACT_PICTURE;
            break;
        case ESS_PCM_24b_48k:
        case ESS_PCM_24b_96k:
            essence_class = ACT_SOUND;
            break;
        case ESS_TIMED_TEXT:
            essence_class = ACT_TIMED_TEXT;
            break;
        default:
            essence_class = ACT_UNKNOWN;
            break;
    }

    return essence_class;
}

/* get the essence type of an asset */
extern "C" int get_file_essence_type(char *filename) {
    Result_t result = RESULT_OK;
    EssenceType_t essence_type;

    result = ASDCP::RawEssenceType(filename, essence_type);

    if (ASDCP_FAILURE(result)) {
        return MXF_COULD_NOT_DETECT_ESSENCE_TYPE;
    }

    switch(essence_type) {
        case ESS_MPEG2_VES:
            return AET_MPEG2_VES;
            break;
        case ESS_JPEG_2000:
            return AET_JPEG_2000;
            break;
        case ESS_PCM_24b_48k:
            return AET_PCM_24b_48k;
            break;
        case ESS_PCM_24b_96k:
            return AET_PCM_24b_96k;
            break;
        case ESS_TIMED_TEXT:
            return AET_TIMED_TEXT;
            break;
        default:
            return AET_UNKNOWN;
            break;
    }

    return 0;
}

/* read asset file information */
extern "C" int read_asset_info(asset_t *asset)
{
    EssenceType_t essence_type;
    WriterInfo info;
    Result_t result = RESULT_OK;
    Kumu::UUID uuid;
    char uuid_buffer[64];
    
    result = ASDCP::EssenceType(asset->filename, essence_type);

    if (ASDCP_FAILURE(result)) {
        return MXF_COULD_NOT_DETECT_ESSENCE_TYPE;
    }

    switch (essence_type) {
        case ESS_MPEG2_VES:
        {
            MPEG2::MXFReader reader;
            MPEG2::VideoDescriptor desc;
            result = reader.OpenRead(asset->filename);

            if (ASDCP_FAILURE(result)) {
                return MXF_MPEG2_FILE_OPEN_ERROR;
            }

	    reader.FillVideoDescriptor(desc);
            reader.FillWriterInfo(info);

            asset->essence_type       = essence_type;
            asset->essence_class      = ACT_PICTURE;
            asset->duration           = desc.ContainerDuration;
            asset->intrinsic_duration = desc.ContainerDuration;
            asset->entry_point    = 0;
            asset->xml_ns         = info.LabelSetType;
            sprintf(asset->uuid,"%.36s", Kumu::bin2UUIDhex(info.AssetUUID,16,uuid_buffer, 64));
            sprintf(asset->aspect_ratio,"%d %d",desc.AspectRatio.Numerator,desc.AspectRatio.Denominator);
            sprintf(asset->edit_rate,"%d %d",desc.EditRate.Numerator,desc.EditRate.Denominator);
            sprintf(asset->sample_rate,"%d %d",desc.SampleRate.Numerator,desc.SampleRate.Denominator);
            sprintf(asset->frame_rate,"%d",desc.FrameRate);
        }
        break;
        case ESS_JPEG_2000_S:
        {        
            JP2K::MXFSReader reader;
            JP2K::PictureDescriptor desc;
            result = reader.OpenRead(asset->filename);

            if (ASDCP_FAILURE(result)) {
                return MXF_J2K_FILE_OPEN_ERROR;
            }

            reader.FillPictureDescriptor(desc);
            reader.FillWriterInfo(info);

            asset->stereoscopic       = 1;
            asset->essence_type       = essence_type;
            asset->essence_class      = ACT_PICTURE;
            asset->duration           = desc.ContainerDuration;
            asset->intrinsic_duration = desc.ContainerDuration;
            asset->entry_point    = 0;
            asset->xml_ns         = info.LabelSetType;
            sprintf(asset->uuid,"%.36s", Kumu::bin2UUIDhex(info.AssetUUID,16,uuid_buffer, 64));
            sprintf(asset->aspect_ratio,"%d %d",desc.AspectRatio.Numerator,desc.AspectRatio.Denominator);
            sprintf(asset->edit_rate,"%d %d",desc.EditRate.Numerator,desc.EditRate.Denominator);
            sprintf(asset->sample_rate,"%d %d",desc.SampleRate.Numerator,desc.SampleRate.Denominator);
            sprintf(asset->frame_rate,"%d %d",desc.SampleRate.Numerator,desc.SampleRate.Denominator);
        }
        break;
        case ESS_JPEG_2000:
        {
            JP2K::MXFReader reader;
            JP2K::PictureDescriptor desc;
            result = reader.OpenRead(asset->filename);

            /* Try 3D MXF Interop */
            if (ASDCP_FAILURE(result)) {
                JP2K::MXFSReader reader;
                result = reader.OpenRead(asset->filename);
                asset->stereoscopic   = 1;
                if ( ASDCP_FAILURE(result) ) {
                    return MXF_J2K_FILE_OPEN_ERROR;
                }
                reader.FillPictureDescriptor(desc);
                reader.FillWriterInfo(info);
            } else {
                reader.FillPictureDescriptor(desc);
                reader.FillWriterInfo(info);
            }

            asset->essence_type        = essence_type;
            asset->essence_class       = ACT_PICTURE;
            asset->duration            = desc.ContainerDuration;
            asset->intrinsic_duration  = desc.ContainerDuration;
            asset->entry_point    = 0;
            asset->xml_ns         = info.LabelSetType;
            sprintf(asset->uuid,"%.36s", Kumu::bin2UUIDhex(info.AssetUUID,16,uuid_buffer, 64));
            sprintf(asset->aspect_ratio,"%d %d",desc.AspectRatio.Numerator,desc.AspectRatio.Denominator);
            sprintf(asset->edit_rate,"%d %d",desc.EditRate.Numerator,desc.EditRate.Denominator);
            sprintf(asset->sample_rate,"%d %d",desc.SampleRate.Numerator,desc.SampleRate.Denominator);
            sprintf(asset->frame_rate,"%d %d",desc.SampleRate.Numerator,desc.SampleRate.Denominator);
        }
        break;
        case ESS_PCM_24b_48k:
        case ESS_PCM_24b_96k:
        {
            PCM::MXFReader reader;
            PCM::AudioDescriptor desc;
            result = reader.OpenRead(asset->filename);

            if (ASDCP_FAILURE(result)) {
                return MXF_WAV_FILE_OPEN_ERROR;
            }

            reader.FillAudioDescriptor(desc);
            reader.FillWriterInfo(info);

            asset->essence_type       = essence_type;
            asset->essence_class      = ACT_SOUND;
            asset->duration           = desc.ContainerDuration;
            asset->intrinsic_duration = desc.ContainerDuration;
            asset->entry_point    = 0;
            asset->xml_ns         = info.LabelSetType;
            sprintf(asset->uuid,"%.36s", Kumu::bin2UUIDhex(info.AssetUUID,16,uuid_buffer, 64));
            sprintf(asset->edit_rate,"%d %d",desc.EditRate.Numerator,desc.EditRate.Denominator);
            sprintf(asset->sample_rate,"%d %d",desc.AudioSamplingRate.Numerator,desc.AudioSamplingRate.Denominator);
        }
        break;
        case ESS_TIMED_TEXT:
        { 
            TimedText::MXFReader reader;
            TimedText::TimedTextDescriptor desc;
            result = reader.OpenRead(asset->filename);

            if (ASDCP_FAILURE(result)) {
                return MXF_TT_FILE_OPEN_ERROR;
            }
  
            reader.FillTimedTextDescriptor(desc);
            reader.FillWriterInfo(info);
  
            asset->essence_type       = essence_type;
            asset->essence_class      = ACT_TIMED_TEXT;
            asset->duration           = desc.ContainerDuration;
            asset->intrinsic_duration = desc.ContainerDuration;
            asset->entry_point    = 0;
            asset->xml_ns         = info.LabelSetType;
            sprintf(asset->uuid,"%.36s", Kumu::bin2UUIDhex(info.AssetUUID,16,uuid_buffer, 64));
            sprintf(asset->edit_rate,"%d %d",desc.EditRate.Numerator,desc.EditRate.Denominator);
      }
      break;
      default:
            return MXF_UNKOWN_ESSENCE_TYPE;
    }
 
    return DCP_SUCCESS;
}

/* write the asset to an mxf file */
extern "C" int write_mxf(opendcp_t *opendcp, filelist_t *filelist, char *output_file) {
    Result_t      result = RESULT_OK; 
    int           rc = 0;
    EssenceType_t essence_type;

    result = ASDCP::RawEssenceType(filelist->in[0], essence_type);

    if (ASDCP_FAILURE(result)) {
        return MXF_COULD_NOT_DETECT_ESSENCE_TYPE;
    }

    switch (essence_type) {
        case ESS_JPEG_2000:
            if ( opendcp->stereoscopic )
                rc = write_j2k_s_mxf(opendcp,filelist,output_file);
            else
                rc = write_j2k_mxf(opendcp,filelist,output_file);
            break;
        case ESS_JPEG_2000_S:
                rc = write_j2k_s_mxf(opendcp,filelist,output_file);
            break;
        case ESS_PCM_24b_48k:
        case ESS_PCM_24b_96k:
            rc = write_pcm_mxf(opendcp,filelist,output_file);
            break;
        case ESS_MPEG2_VES:
            rc = write_mpeg2_mxf(opendcp,filelist,output_file);
            break;
        case ESS_TIMED_TEXT:
            rc = write_tt_mxf(opendcp,filelist,output_file);
            break;
        case ESS_UNKNOWN:
            rc = MXF_UNKOWN_ESSENCE_TYPE;
            break;
    }
 

    return rc; 
}

typedef struct {
    WriterInfo    info;
    AESEncContext *aes_context;
    HMACContext   *hmac_context;
} writer_info_t;

Result_t fill_writer_info(opendcp_t *opendcp, writer_info_t *writer_info) {
    Kumu::FortunaRNG        rng;
    byte_t                  iv_buf[CBC_BLOCK_SIZE];
    Result_t                result = RESULT_OK; 

    writer_info->info.ProductVersion = OPENDCP_VERSION;
    writer_info->info.CompanyName = OPENDCP_NAME;
    writer_info->info.ProductName = OPENDCP_NAME;

    /* set the label type */
    if (opendcp->ns == XML_NS_INTEROP) {
        writer_info->info.LabelSetType = LS_MXF_INTEROP;
    } else if (opendcp->ns == XML_NS_SMPTE) {
        writer_info->info.LabelSetType = LS_MXF_SMPTE;
    } else {
        writer_info->info.LabelSetType = LS_MXF_UNKNOWN;
    }

    /* generate a random UUID for this essence */
    Kumu::GenRandomUUID(writer_info->info.AssetUUID);

    /* start encryption, if set */
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
            return result;
        }

        result = writer_info->aes_context->SetIVec(rng.FillRandom(iv_buf, CBC_BLOCK_SIZE));

        if (ASDCP_FAILURE(result)) {
            return result;
        }

        if (opendcp->write_hmac) {
            writer_info->info.UsesHMAC = true;
            writer_info->hmac_context = new HMACContext;
            result = writer_info->hmac_context->InitKey(opendcp->key_value, writer_info->info.LabelSetType);

            if (ASDCP_FAILURE(result)) {
                return result;
            }
        }
    }

    return result;
}

/* write out j2k mxf file */
int write_j2k_mxf(opendcp_t *opendcp, filelist_t *filelist, char *output_file) {
    JP2K::MXFWriter         mxf_writer;
    JP2K::PictureDescriptor picture_desc;
    JP2K::CodestreamParser  j2k_parser;
    JP2K::FrameBuffer       frame_buffer(FRAME_BUFFER_SIZE);
    writer_info_t           writer_info;
    Result_t                result = RESULT_OK; 
    ui32_t                  start_frame;
    ui32_t                  mxf_duration;

    /* set the starting frame */
    if (opendcp->mxf.start_frame && filelist->file_count >= (opendcp->mxf.start_frame-1)) {
        start_frame = opendcp->mxf.start_frame - 1; // adjust for zero base
    } else {
        start_frame = 0;
    }

    result = j2k_parser.OpenReadFrame(filelist->in[start_frame], frame_buffer);

    if (ASDCP_FAILURE(result)) {
        return MXF_J2K_FILE_OPEN_ERROR;
    }

    Rational edit_rate(opendcp->frame_rate,1);
    j2k_parser.FillPictureDescriptor(picture_desc);
    picture_desc.EditRate = edit_rate; 

    fill_writer_info(opendcp, &writer_info);

    result = mxf_writer.OpenWrite(output_file, writer_info.info, picture_desc);

    if (ASDCP_FAILURE(result)) {
        return MXF_FILE_WRITE_ERROR;
    }
   
    /* set the duration of the output mxf */
    if (opendcp->mxf.duration && filelist->file_count >= opendcp->mxf.duration) {
        mxf_duration = opendcp->mxf.duration;
    } else {
        mxf_duration = filelist->file_count;
    }

    ui32_t i = start_frame;
    /* read each input frame and write to the output mxf until duration is reached */
    while (ASDCP_SUCCESS(result) && mxf_duration--) {
        if (opendcp->slide == 0 || i == start_frame) {
            result = j2k_parser.OpenReadFrame(filelist->in[i], frame_buffer);

            if (opendcp->delete_intermediate) {
                unlink(filelist->in[i]);
            }

            if (ASDCP_FAILURE(result)) {
                return MXF_J2K_FILE_OPEN_ERROR;
            }

            if (opendcp->encrypt_header_flag) {
                frame_buffer.PlaintextOffset(0);
            }
            i++;
        }
        result = mxf_writer.WriteFrame(frame_buffer, writer_info.aes_context, writer_info.hmac_context);
    }

    if (result == RESULT_ENDOFFILE) {
        result = RESULT_OK;
    }

    if (ASDCP_FAILURE(result)) {
        return MXF_FILE_WRITE_ERROR;
    }

    result = mxf_writer.Finalize();

    if (ASDCP_FAILURE(result)) {
        return MXF_FILE_FINALIZE_ERROR;
    }

    return DCP_SUCCESS;
}

/* write out 3D j2k mxf file */
int write_j2k_s_mxf(opendcp_t *opendcp, filelist_t *filelist, char *output_file) {
    JP2K::MXFSWriter         mxf_writer;
    JP2K::PictureDescriptor picture_desc;
    JP2K::CodestreamParser  j2k_parser_left;
    JP2K::CodestreamParser  j2k_parser_right;
    JP2K::FrameBuffer       frame_buffer_left(FRAME_BUFFER_SIZE);
    JP2K::FrameBuffer       frame_buffer_right(FRAME_BUFFER_SIZE);
    writer_info_t           writer_info;
    Result_t                result = RESULT_OK;
    ui32_t                  mxf_duration;

    result = j2k_parser_left.OpenReadFrame(filelist->in[0], frame_buffer_left);

    if (ASDCP_FAILURE(result)) {
        return MXF_J2K_FILE_OPEN_ERROR;
    }

    result = j2k_parser_right.OpenReadFrame(filelist->in[1], frame_buffer_right);

    if (ASDCP_FAILURE(result)) {
        return MXF_J2K_FILE_OPEN_ERROR;
    }

    Rational edit_rate(opendcp->frame_rate,1);
    j2k_parser_left.FillPictureDescriptor(picture_desc);
    picture_desc.EditRate = edit_rate;

    fill_writer_info(opendcp, &writer_info);

    result = mxf_writer.OpenWrite(output_file, writer_info.info, picture_desc);

    if (ASDCP_FAILURE(result)) {
        return MXF_FILE_WRITE_ERROR;
    }
     
    /* set the duration of the output mxf, set to half the filecount since it is 3D */
    if ((filelist->file_count/2) < opendcp->duration || !opendcp->duration) {
        mxf_duration = filelist->file_count/2;
    } else {
        mxf_duration = opendcp->duration;
    }

    ui32_t i = 0;
    /* read each input frame and write to the output mxf until duration is reached */
    while (ASDCP_SUCCESS(result) && mxf_duration--) {
        if (opendcp->slide == 0 || i == 0) {
            result = j2k_parser_left.OpenReadFrame(filelist->in[i], frame_buffer_left);

            if (opendcp->delete_intermediate) {
                unlink(filelist->in[i]);
            }

            if (ASDCP_FAILURE(result)) {
                return MXF_J2K_FILE_OPEN_ERROR;
            }
            i++;

            result = j2k_parser_right.OpenReadFrame(filelist->in[i], frame_buffer_right);

            if (opendcp->delete_intermediate) {
                unlink(filelist->in[i]);
            }

            if (ASDCP_FAILURE(result)) {
                return MXF_J2K_FILE_OPEN_ERROR;
            }
            i++;

            if (opendcp->encrypt_header_flag) {
                frame_buffer_left.PlaintextOffset(0);
            }

            if (opendcp->encrypt_header_flag) {
                frame_buffer_right.PlaintextOffset(0);
            }
        }
        result = mxf_writer.WriteFrame(frame_buffer_left, JP2K::SP_LEFT, writer_info.aes_context, writer_info.hmac_context);
        result = mxf_writer.WriteFrame(frame_buffer_right, JP2K::SP_RIGHT, writer_info.aes_context, writer_info.hmac_context);
    }

    if (result == RESULT_ENDOFFILE) {
        result = RESULT_OK;
    }

    if (ASDCP_FAILURE(result)) {
        return MXF_FILE_WRITE_ERROR;
    }

    result = mxf_writer.Finalize();

    if (ASDCP_FAILURE(result)) {
        return MXF_FILE_FINALIZE_ERROR;
    }

    return DCP_SUCCESS;
}

/* write out pcm audio mxf file */
int write_pcm_mxf(opendcp_t *opendcp, filelist_t *filelist, char *output_file) {
    PCM::FrameBuffer     frame_buffer;
    PCM::AudioDescriptor audio_desc;
    PCM::WAVParser       pcm_parser_channel[filelist->file_count];
    PCM::FrameBuffer     frame_buffer_channel[filelist->file_count];
    PCM::AudioDescriptor audio_desc_channel[filelist->file_count];
    PCM::MXFWriter       mxf_writer;
    writer_info_t        writer_info;
    Result_t             result = RESULT_OK; 
    ui32_t               mxf_duration;
    ui32_t               file_count = 0;

    Rational edit_rate(opendcp->frame_rate,1);

    /* read first file */
    result = pcm_parser_channel[0].OpenRead(filelist->in[0], edit_rate);

    if (ASDCP_FAILURE(result)) {
        return MXF_WAV_FILE_OPEN_ERROR;
    }

    /* read audio descriptot */
    pcm_parser_channel[0].FillAudioDescriptor(audio_desc);

    for (file_count = 0; file_count < filelist->file_count; file_count++) {
        result = pcm_parser_channel[file_count].OpenRead(filelist->in[file_count], edit_rate);
        if (ASDCP_FAILURE(result)) {
            dcp_log(LOG_ERROR,"Could not open %s",filelist->in[file_count]);
            return MXF_WAV_FILE_OPEN_ERROR;
        }
        pcm_parser_channel[file_count].FillAudioDescriptor(audio_desc_channel[file_count]);
        if (audio_desc_channel[file_count].AudioSamplingRate != audio_desc.AudioSamplingRate) {
            dcp_log(LOG_ERROR,"Mismatched sampling rate");
            return MXF_WAV_FILE_OPEN_ERROR;
        }
        if (audio_desc_channel[file_count].QuantizationBits != audio_desc.QuantizationBits) {
            dcp_log(LOG_ERROR,"Mismatched bit rate");
            return MXF_WAV_FILE_OPEN_ERROR;
        }
        if (audio_desc_channel[file_count].ContainerDuration != audio_desc.ContainerDuration) {
            dcp_log(LOG_ERROR,"Mismatched duration");
            return MXF_WAV_FILE_OPEN_ERROR;
        }
        frame_buffer_channel[file_count].Capacity(PCM::CalcFrameBufferSize(audio_desc_channel[file_count]));
    }

    if (ASDCP_FAILURE(result)) {
        return MXF_WAV_FILE_OPEN_ERROR;
    }

    /*  set total audio characteristics */
    audio_desc.EditRate = edit_rate;
    audio_desc.ChannelCount = filelist->file_count;
    audio_desc.BlockAlign  = audio_desc.BlockAlign * filelist->file_count;
    audio_desc.AvgBps = audio_desc.AvgBps * filelist->file_count; 

    /* set total frame buffer size */
    frame_buffer.Capacity(PCM::CalcFrameBufferSize(audio_desc));
    frame_buffer.Size(PCM::CalcFrameBufferSize(audio_desc));

    /* fill write info */
    fill_writer_info(opendcp, &writer_info);
    result = mxf_writer.OpenWrite(output_file, writer_info.info, audio_desc);

    if (ASDCP_FAILURE(result)) {
        return MXF_FILE_WRITE_ERROR;
    }

    /* set duration */
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

        /* read a frame from each file */
        for (file_count = 0; file_count < filelist->file_count; file_count++) {
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

        /* write sample from each frame to output buffer */
        if (ASDCP_SUCCESS(result)) {
            while (data_s < data_e) {
                for (file_count = 0; file_count < filelist->file_count; file_count++) {
                    byte_t *frame = frame_buffer_channel[file_count].Data()+offset;
                    memcpy(data_s,frame,sample_size);
                    data_s += sample_size;
                }
                offset += sample_size;
            }

            /* write the frame */
            result = mxf_writer.WriteFrame(frame_buffer, writer_info.aes_context, writer_info.hmac_context);
        }
    }

    if (result == RESULT_ENDOFFILE) {
        result = RESULT_OK;
    }

    if (ASDCP_FAILURE(result)) {
        return MXF_FILE_WRITE_ERROR;
    }

    /* write footer information */
    result = mxf_writer.Finalize();

    if (ASDCP_FAILURE(result)) {
        return MXF_FILE_FINALIZE_ERROR;
    }
 
    return DCP_SUCCESS;
}

/* write out timed text mxf file */
int write_tt_mxf(opendcp_t *opendcp, filelist_t *filelist, char *output_file) {
    TimedText::DCSubtitleParser    tt_parser;
    TimedText::MXFWriter           mxf_writer;
    TimedText::FrameBuffer         frame_buffer(FRAME_BUFFER_SIZE);
    TimedText::TimedTextDescriptor tt_desc;
    TimedText::ResourceList_t::const_iterator resource_iterator;
    writer_info_t                  writer_info;
    std::string                    xml_doc;
    Result_t                       result = RESULT_OK;

    result = tt_parser.OpenRead(filelist->in[0]);
 
    if (ASDCP_FAILURE(result)) {
        return MXF_TT_FILE_OPEN_ERROR;
    }

    tt_parser.FillTimedTextDescriptor(tt_desc);

    fill_writer_info(opendcp, &writer_info);

    result = mxf_writer.OpenWrite(output_file, writer_info.info, tt_desc);
    result = tt_parser.ReadTimedTextResource(xml_doc);

    if (ASDCP_FAILURE(result)) {
        return MXF_TT_FILE_OPEN_ERROR;
    }

    result = mxf_writer.WriteTimedTextResource(xml_doc, writer_info.aes_context, writer_info.hmac_context);

    if (ASDCP_FAILURE(result)) {
        return MXF_FILE_WRITE_ERROR;
    }

    resource_iterator = tt_desc.ResourceList.begin();

    while (ASDCP_SUCCESS(result) && resource_iterator != tt_desc.ResourceList.end()) {
        result = tt_parser.ReadAncillaryResource((*resource_iterator++).ResourceID, frame_buffer);

        if (ASDCP_FAILURE(result)) {
          return MXF_TT_FILE_OPEN_ERROR;
        }

        result = mxf_writer.WriteAncillaryResource(frame_buffer, writer_info.aes_context, writer_info.hmac_context);
    }

    if (result == RESULT_ENDOFFILE) {
        result = RESULT_OK;
    }

    if (ASDCP_FAILURE(result)) {
        return MXF_FILE_WRITE_ERROR;
    }

    result = mxf_writer.Finalize();

    if (ASDCP_FAILURE(result)) {
        return MXF_FILE_FINALIZE_ERROR;
    }

    return DCP_SUCCESS;
}

/* write out mpeg2 mxf file */
int write_mpeg2_mxf(opendcp_t *opendcp, filelist_t *filelist, char *output_file) {
    MPEG2::FrameBuffer     frame_buffer(FRAME_BUFFER_SIZE);
    MPEG2::Parser          mpeg2_parser;
    MPEG2::MXFWriter       mxf_writer;
    MPEG2::VideoDescriptor video_desc;
    writer_info_t          writer_info;
    Result_t               result = RESULT_OK; 
    ui32_t                 mxf_duration;

    result = mpeg2_parser.OpenRead(filelist->in[0]);

    if (ASDCP_FAILURE(result)) {
        return MXF_MPEG2_FILE_OPEN_ERROR;
    }

    mpeg2_parser.FillVideoDescriptor(video_desc);

    fill_writer_info(opendcp, &writer_info);

    result = mxf_writer.OpenWrite(output_file, writer_info.info, video_desc);

    if (ASDCP_FAILURE(result)) {
        return MXF_FILE_WRITE_ERROR;
    }

    result = mpeg2_parser.Reset();
  
    if (ASDCP_FAILURE(result)) {
        return MXF_PARSER_RESET_ERROR;
    }

    if (!opendcp->duration) {
        mxf_duration = 0xffffffff;
    } else {
        mxf_duration = opendcp->duration;
    }

    while (ASDCP_SUCCESS(result) && mxf_duration--) {
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
        return MXF_FILE_WRITE_ERROR;
    }

    result = mxf_writer.Finalize();

    if (ASDCP_FAILURE(result)) {
        return MXF_FILE_FINALIZE_ERROR;
    }

    return DCP_SUCCESS;
}
