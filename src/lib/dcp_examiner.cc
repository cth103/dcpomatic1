/*
    Copyright (C) 2014 Carl Hetherington <cth@carlh.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include <dcp/dcp.h>
#include <dcp/cpl.h>
#include <dcp/reel.h>
#include <dcp/reel_picture_asset.h>
#include <dcp/reel_sound_asset.h>
#include <dcp/sound_mxf.h>
#include "dcp_examiner.h"
#include "dcp_content.h"
#include "exceptions.h"

#include "i18n.h"

using std::list;
using boost::shared_ptr;

DCPExaminer::DCPExaminer (shared_ptr<const DCPContent> content)
{
	dcp::DCP dcp (content->directory ());
	dcp.read ();

	if (dcp.cpls().size() == 0) {
		throw DCPError ("No CPLs found in DCP");
	} else if (dcp.cpls().size() > 1) {
		throw DCPError ("Multiple CPLs found in DCP");
	}

	list<shared_ptr<dcp::Reel> > reels = dcp.cpls().front()->reels ();
	for (list<shared_ptr<dcp::Reel> >::const_iterator i = reels.begin(); i != reels.end(); ++i) {

		if ((*i)->main_picture ()) {
			dcp::Fraction const frac = (*i)->main_picture()->frame_rate ();
			float const fr = float(frac.numerator) / frac.denominator;
			if (!_video_frame_rate) {
				_video_frame_rate = fr;
			} else if (_video_frame_rate.get() != fr) {
				throw DCPError (_("Mismatched frame rates in DCP"));
			}

			shared_ptr<dcp::PictureMXF> mxf = (*i)->main_picture()->mxf ();
			if (!_video_size) {
				_video_size = mxf->size ();
			} else if (_video_size.get() != mxf->size ()) {
				throw DCPError (_("Mismatched video sizes in DCP"));
			}

			_video_length += ContentTime::from_frames ((*i)->main_picture()->duration(), _video_frame_rate.get ());
		}
			
		if ((*i)->main_sound ()) {
			shared_ptr<dcp::SoundMXF> mxf = (*i)->main_sound()->mxf ();

			if (!_audio_channels) {
				_audio_channels = mxf->channels ();
			} else if (_audio_channels.get() != mxf->channels ()) {
				throw DCPError (_("Mismatched audio channel counts in DCP"));
			}

			if (!_audio_frame_rate) {
				_audio_frame_rate = mxf->sampling_rate ();
			} else if (_audio_frame_rate.get() != mxf->sampling_rate ()) {
				throw DCPError (_("Mismatched audio frame rates in DCP"));
			}
		}
	}
}
