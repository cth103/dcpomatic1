#include <sstream>
#include <stdexcept>
#include <iomanip>
#include <iostream>
#include <sndfile.h>
#include <openjpeg.h>
#include "j2k_wav_transcoder.h"
#include "film.h"
#include "lut.h"

using namespace std;

J2KWAVTranscoder::J2KWAVTranscoder (Film* f, Progress* p, int w, int h)
	: Transcoder (f, p, w, h)
	, _deinterleave_buffer_size (8192)
	, _deinterleave_buffer (0)
{
	/* Create sound output files */
	for (int i = 0; i < audio_channels(); ++i) {
		stringstream wav_path;
		wav_path << _film->dir ("wavs") << "/" << (i + 1) << ".wav";
		SF_INFO sf_info;
		sf_info.samplerate = audio_sample_rate();
		/* We write mono files */
		sf_info.channels = 1;
		sf_info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_24;
		SNDFILE* f = sf_open (wav_path.str().c_str(), SFM_WRITE, &sf_info);
		if (f == 0) {
			throw runtime_error ("Could not create audio output file");
		}
		_sound_files.push_back (f);
	}

	/* Create buffer for deinterleaving audio */
	_deinterleave_buffer = new uint8_t[_deinterleave_buffer_size];
}

J2KWAVTranscoder::~J2KWAVTranscoder ()
{
	delete[] _deinterleave_buffer;

	for (vector<SNDFILE*>::iterator i = _sound_files.begin(); i != _sound_files.end(); ++i) {
		sf_close (*i);
	}
}	

void
J2KWAVTranscoder::process_video (uint8_t* rgb, int line_size)
{
	/* Create libopenjpeg image container */
	
	opj_image_cmptparm_t cmptparm[3];

	memset (&cmptparm[0], 0, 3 * sizeof (opj_image_cmptparm_t));
	for (int i = 0; i < 3; ++i) {
		cmptparm[i].dx = 1;
		cmptparm[i].dy = 1;
		cmptparm[i].w = out_width ();
		cmptparm[i].h = out_height ();
		cmptparm[i].x0 = 0;
		cmptparm[i].y0 = 0;
		cmptparm[i].prec = 12;
		cmptparm[i].bpp = 12;
		cmptparm[i].sgnd = 0;
	}

	opj_image* opj = opj_image_create (3, &cmptparm[0], CLRSPC_SRGB);
	if (opj == 0) {
		throw runtime_error ("Could not create libopenjpeg image");
	}

	opj->x0 = 0;
	opj->y0 = 0;
	opj->x1 = out_width ();
	opj->y1 = out_height ();

	int size = out_width() * out_height();

	struct {
		float r, g, b;
	} s;

	struct {
		float x, y, z;
	} d;

	/* Copy our RGB into it, converting to XYZ in the process */

	uint8_t* p = rgb;

	int lut_index = 0;
	
	for (int i = 0; i < size; ++i) {
		/* In gamma LUT (converting 8-bit input to 12-bit) */
		s.r = lut_in[lut_index][*p++ << 4];
		s.g = lut_in[lut_index][*p++ << 4];
		s.b = lut_in[lut_index][*p++ << 4];

		/* RGB to XYZ Matrix */
		d.x = ((s.r * color_matrix[lut_index][0][0]) + (s.g * color_matrix[lut_index][0][1]) + (s.b * color_matrix[lut_index][0][2]));
		d.y = ((s.r * color_matrix[lut_index][1][0]) + (s.g * color_matrix[lut_index][1][1]) + (s.b * color_matrix[lut_index][1][2]));
		d.z = ((s.r * color_matrix[lut_index][2][0]) + (s.g * color_matrix[lut_index][2][1]) + (s.b * color_matrix[lut_index][2][2]));

		/* DCI companding */
		d.x = d.x * DCI_COEFFICENT * (DCI_LUT_SIZE - 1);
		d.y = d.y * DCI_COEFFICENT * (DCI_LUT_SIZE - 1);
		d.z = d.z * DCI_COEFFICENT * (DCI_LUT_SIZE - 1);
		
		/* Out gamma LUT */
		opj->comps[0].data[i] = lut_out[LO_DCI][(int)d.x];
		opj->comps[1].data[i] = lut_out[LO_DCI][(int)d.y];
		opj->comps[2].data[i] = lut_out[LO_DCI][(int)d.z];
	}

	/* Encode */

	/* XXX: bandwidth */
	/* Maximum DCI compliant bitrate for JPEG2000 */
	int const bw = 250000000;
//	int const bw = 125 * 1000000;

	/* Set the max image and component sizes based on frame_rate */
	int const max_cs_len = ((float) bw) / 8 / _film->frames_per_second ();
 
	int const max_comp_size = max_cs_len / 1.25;

	/* Set encoding parameters to default values */
	opj_cparameters_t parameters;
	opj_set_default_encoder_parameters (&parameters);

	/* Set default cinema parameters */
	parameters.tile_size_on = false;
	parameters.cp_tdx = 1;
	parameters.cp_tdy = 1;
	
	/* Tile part */
	parameters.tp_flag = 'C';
	parameters.tp_on = 1;
	
	/* Tile and Image shall be at (0,0) */
	parameters.cp_tx0 = 0;
	parameters.cp_ty0 = 0;
	parameters.image_offset_x0 = 0;
	parameters.image_offset_y0 = 0;

	/* Codeblock size = 32*32 */
	parameters.cblockw_init = 32;
	parameters.cblockh_init = 32;
	parameters.csty |= 0x01;
	
	/* The progression order shall be CPRL */
	parameters.prog_order = CPRL;
	
	/* No ROI */
	parameters.roi_compno = -1;
	
	parameters.subsampling_dx = 1;
	parameters.subsampling_dy = 1;
	
	/* 9-7 transform */
	parameters.irreversible = 1;
	
	parameters.tcp_rates[0] = 0;
	parameters.tcp_numlayers++;
	parameters.cp_disto_alloc = 1;
	parameters.cp_rsiz = CINEMA2K;
	parameters.cp_comment = strdup ("OpenDCP");
	parameters.cp_cinema = CINEMA2K_24;

	/* 3 components, so use MCT */
	parameters.tcp_mct = 1;
	
	/* set max image */
	parameters.max_comp_size = max_comp_size;
	parameters.tcp_rates[0] = ((float) (3 * opj->comps[0].w * opj->comps[0].h * opj->comps[0].prec)) / (max_cs_len * 8);

	/* get a J2K compressor handle */
	opj_cinfo_t* cinfo = opj_create_compress (CODEC_J2K);

	/* Set event manager to null (openjpeg 1.3 bug) */
	cinfo->event_mgr = 0;

	/* Setup the encoder parameters using the current image and user parameters */
	opj_setup_encoder (cinfo, &parameters, opj);

	opj_cio_t *cio = opj_cio_open ((opj_common_ptr) cinfo, 0, 0);

	int const r = opj_encode (cinfo, cio, opj, 0);
	if (r == 0) {
		opj_cio_close (cio);
		opj_destroy_compress (cinfo);
		throw runtime_error ("jpeg2000 encoding failed");
	}

	int const codestream_length = cio_tell (cio);

	stringstream j2c;
	j2c << _film->dir ("j2c") << "/";
	j2c.width (8);
	j2c << setfill('0') << video_frame() << ".j2c";
	FILE* f = fopen (j2c.str().c_str (), "wb");
	
	if (!f) {
		throw runtime_error ("Unable to create jpeg2000 file for writing");
		opj_cio_close(cio);
		opj_destroy_compress(cinfo);
	}

	fwrite (cio->buffer, 1, codestream_length, f);
	fclose (f);

	opj_image_destroy (opj);

	/* Free openjpeg structure */
	opj_cio_close (cio);
	opj_destroy_compress (cinfo);
	
	/* Free user parameters structure */
	free (parameters.cp_comment);
	free (parameters.cp_matrice);
}

void
J2KWAVTranscoder::process_audio (uint8_t* data, int channels, int data_size)
{
	/* Size of a sample in bytes */
	int const sample_size = 2;
	
	/* XXX: we are assuming that sample_size is right, the _deinterleave_buffer_size is a multiple
	   of the sample size and that data_size is a multiple of channels * sample_size.
	*/
	
	/* XXX: this code is very tricksy and it must be possible to make it simpler ... */
	
	/* Number of bytes left to read this time */
	int remaining = data_size;
	/* Our position in the output buffers, in bytes */
	int position = 0;
	while (remaining > 0) {
		/* How many bytes of the deinterleaved data to do this time */
		int this_time = min (remaining / channels, _deinterleave_buffer_size);
		for (int i = 0; i < channels; ++i) {
			for (int j = 0; j < this_time; j += sample_size) {
				for (int k = 0; k < sample_size; ++k) {
					int const to = j + k;
					int const from = position + (i * sample_size) + (j * channels) + k;
					_deinterleave_buffer[to] = data[from];
				}
			}
			
			switch (audio_sample_format ()) {
			case AV_SAMPLE_FMT_S16:
				sf_write_short (_sound_files[i], (const short *) _deinterleave_buffer, this_time / sample_size);
				break;
			default:
				throw runtime_error ("Unknown audio sample format");
			}
		}
		
		position += this_time;
		remaining -= this_time * channels;
	}
}

