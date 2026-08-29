/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2023 Olive Studios LLC
  Modifications Copyright (C) 2025 mikesolar

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

***/

#ifndef LIBOLIVECORE_PIXELFORMAT_H
#define LIBOLIVECORE_PIXELFORMAT_H
#include "ofxCore.h"
#include <string>
namespace olive::core
{

class PixelFormat {
public:
	enum Format { INVALID = -1, U8, U10, U16, F16, F32, COUNT };

	PixelFormat(Format f = INVALID)
	{
		f_ = f;
	}

	operator Format() const
	{
		return f_;
	}

	static PixelFormat from_ofx(std::string ofxFormat){
		if(ofxFormat == kOfxBitDepthByte){
			return PixelFormat::U8;
		}
		else if (ofxFormat == kOfxBitDepthShort){
			return PixelFormat::U16;
		}
		else if(ofxFormat == kOfxBitDepthHalf){
			return PixelFormat::F16;
		}
		else if(ofxFormat == kOfxBitDepthFloat){
			return PixelFormat::F32;
		}
		return PixelFormat::INVALID;
	}
	static int byte_count(Format f)
	{
		switch (f) {
		case INVALID:
		case COUNT:
			break;
		case U8:
			return 1;
		case U10:
			return 4; // packed RGBA10A2, treated as 4 bytes per pixel
		case U16:
		case F16:
			return 2;
		case F32:
			return 4;
		}

		return 0;
	}

	const char *to_string() const
	{
		switch (f_) {
		case U8:
			return "u8";
		case U10:
			return "u10";
		case U16:
			return "u16";
		case F16:
			return "f16";
		case F32:
			return "f32";
		case INVALID:
		case COUNT:
			break;
		}

		return "";
	}

	int byte_count() const
	{
		return byte_count(f_);
	}

	static bool is_float(Format f)
	{
		switch (f) {
		case INVALID:
		case COUNT:
		case U8:
		case U10:
		case U16:
			break;
		case F16:
		case F32:
			return true;
		}

		return false;
	}

	bool is_float() const
	{
		return is_float(f_);
	}

private:
	Format f_;
};

}

#endif // LIBOLIVECORE_PIXELFORMAT_H
