/*
    Copyright (C) 2012 Carl Hetherington <cth@carlh.net>

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

#include <stdexcept>

class StringError : public std::exception
{
public:
	/** @param w Error message */
	StringError (std::string w) {
		_what = w;
	}

	virtual ~StringError () throw () {}

	/** @return error message */
	char const * what () const throw () {
		return _what.c_str ();
	}

private:
	/** error message */
	std::string _what;
};

/** A low-level problem with the decoder (possibly due to the nature
 *  of a source file).
 */
class DecodeError : public StringError
{
public:
	DecodeError (std::string s)
		: StringError (s)
	{}
};

/** A low-level problem with an encoder */
class EncodeError : public StringError
{
public:
	EncodeError (std::string s)
		: StringError (s)
	{}
};

/** Parent class for file-related errors */
class FileError : public StringError
{
public:
	FileError (std::string m, std::string f)
		: StringError (m)
		, _file (f)
	{}

	virtual ~FileError () throw () {}

	std::string file () const {
		return _file;
	}

private:
	std::string _file;
};
	

/** Some error occurred when trying to open a file */
class OpenFileError : public FileError
{
public:
	OpenFileError (std::string f)
		: FileError ("could not open file " + f, f)
	{}
};

/** Some error occurred when trying to create a file */
class CreateFileError : public FileError
{
public:
	/** @param f File that we were trying to create */
	CreateFileError (std::string f)
		: FileError ("could not create file " + f, f)
	{}
};

/** Some error occurred when trying to write to a file */
class WriteFileError : public FileError
{
public:
	/** @param f File that we were trying to write to */
	WriteFileError (std::string f)
		: FileError ("could not write to file " + f, f)
	{}
};

/** A Film is missing a setting that is required for some operation */
class MissingSettingError : public StringError
{
public:
	/** @param s Name of setting that was required */
	MissingSettingError (std::string s)
		: StringError ("missing required setting " + s)
		, _setting (s)
	{}

	virtual ~MissingSettingError () throw () {}

	std::string setting () const {
		return _setting;
	}

private:
	std::string _setting;
};
	

/** A content location outside the Film's directory has been specified */
class BadContentLocationError : public std::exception
{
public:
	~BadContentLocationError () throw () {}
	
	char const * what () const throw () {
		return "bad content location";
	}
};

/** Some problem with communication between an encode client and server */
class NetworkError : public StringError
{
public:
	NetworkError (std::string s)
		: StringError (s)
	{}
};
