/*
    Copyright (C) 2013-2014 Carl Hetherington <cth@carlh.net>

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

#include <libcxml/cxml.h>
#include "image_content.h"
#include "image_examiner.h"
#include "config.h"
#include "compose.hpp"
#include "film.h"
#include "job.h"
#include "frame_rate_change.h"
#include "safe_stringstream.h"

#include "i18n.h"

#include "image_filename_sorter.cc"

using std::string;
using std::cout;
using boost::shared_ptr;

ImageContent::ImageContent (shared_ptr<const Film> f, boost::filesystem::path p)
	: Content (f)
	, VideoContent (f)
{
	if (boost::filesystem::is_regular_file (p) && valid_image_file (p)) {
		_paths.push_back (p);
	} else {
		for (boost::filesystem::directory_iterator i(p); i != boost::filesystem::directory_iterator(); ++i) {
			if (boost::filesystem::is_regular_file (i->path()) && valid_image_file (i->path())) {
				_paths.push_back (i->path ());
			}
		}

		if (_paths.empty()) {
			throw FileError (_("No valid image files were found in the folder."), p);
		}

		sort (_paths.begin(), _paths.end(), ImageFilenameSorter ());
	}
}


ImageContent::ImageContent (shared_ptr<const Film> f, shared_ptr<const cxml::Node> node, int version)
	: Content (f, node)
	, VideoContent (f, node, version)
{

}

string
ImageContent::summary () const
{
	string s = path_summary () + " ";
	/* Get the string() here so that the name does not have quotes around it */
	if (still ()) {
		s += _("[still]");
	} else {
		s += _("[moving images]");
	}

	return s;
}

string
ImageContent::technical_summary () const
{
	string s = Content::technical_summary() + " - "
		+ VideoContent::technical_summary() + " - ";

	if (still ()) {
		s += N_("still");
	} else {
		s += N_("moving");
	}

	return s;
}

void
ImageContent::as_xml (xmlpp::Node* node) const
{
	node->add_child("Type")->add_child_text ("Image");
	Content::as_xml (node);
	VideoContent::as_xml (node);
}

void
ImageContent::examine (shared_ptr<Job> job)
{
	Content::examine (job);

	shared_ptr<const Film> film = _film.lock ();
	DCPOMATIC_ASSERT (film);

	shared_ptr<ImageExaminer> examiner (new ImageExaminer (film, shared_from_this(), job));

	take_from_video_examiner (examiner);
	set_video_length (examiner->video_length ());
}

void
ImageContent::set_video_length (VideoContent::Frame len)
{
	{
		boost::mutex::scoped_lock lm (_mutex);
		_video_length = len;
	}

	signal_changed (ContentProperty::LENGTH);
}

Time
ImageContent::full_length () const
{
	shared_ptr<const Film> film = _film.lock ();
	DCPOMATIC_ASSERT (film);

	FrameRateChange frc (video_frame_rate(), film->video_frame_rate ());
	return video_length_after_3d_combine() * frc.factor() * TIME_HZ / video_frame_rate();
}

string
ImageContent::identifier () const
{
	SafeStringStream s;
	s << VideoContent::identifier ();
	s << "_" << video_length();
	return s.str ();
}

bool
ImageContent::still () const
{
	return number_of_paths() == 1;
}

void
ImageContent::set_video_frame_rate (float r)
{
	{
		boost::mutex::scoped_lock lm (_mutex);
		if (_video_frame_rate == r) {
			return;
		}

		_video_frame_rate = r;
	}

	signal_changed (VideoContentProperty::VIDEO_FRAME_RATE);
}

void
ImageContent::set_default_colour_conversion ()
{
	bool const s = still ();

	boost::mutex::scoped_lock lm (_mutex);

	if (s) {
		_colour_conversion = PresetColourConversion::from_id ("srgb").conversion;
	} else {
		_colour_conversion = PresetColourConversion::from_id ("rec709").conversion;
	}
}
