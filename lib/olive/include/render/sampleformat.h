/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2023 Olive Studios LLC

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

#ifndef LIBOLIVECORE_SAMPLEFORMAT_H
#define LIBOLIVECORE_SAMPLEFORMAT_H

#include <QString>
#include <stdexcept>

namespace olive {

class SampleFormat
{
public:
  enum Format {
    INVALID = -1,

    U8P,
    S16P,
    S32P,
    S64P,
    F32P,
    F64P,

    U8,
    S16,
    S32,
    S64,
    F32,
    F64,

    COUNT,

    PLANAR_START = U8P,
    PACKED_START = U8,
    PLANAR_END = PACKED_START,
    PACKED_END = COUNT,
  };

  SampleFormat(Format f = INVALID) { f_ = f; }

  operator Format() const { return f_; }

  static int byte_count(Format f)
  {
    switch (f) {
    case U8:
    case U8P:
      return 1;
    case S16:
    case S16P:
      return 2;
    case S32:
    case F32:
    case S32P:
    case F32P:
      return 4;
    case S64:
    case F64:
    case S64P:
    case F64P:
      return 8;
    case INVALID:
    case COUNT:
      break;
    }

    return 0;
  }

  int byte_count() const
  {
    return byte_count(f_);
  }

  static QString to_string(Format f)
  {
    switch (f) {
    case INVALID:
    case COUNT:
      break;
    case U8: return "u8";
    case S16: return "s16";
    case S32: return "s32";
    case S64: return "s64";
    case F32: return "f32";
    case F64: return "f64";
    case U8P: return "u8p";
    case S16P: return "s16p";
    case S32P: return "s32p";
    case S64P: return "s64p";
    case F32P: return "f32p";
    case F64P: return "f64p";
    }

    return "";
  }

  QString to_string() const
  {
    return to_string(f_);
  }

  static SampleFormat from_string(const QString &s)
  {
    if (s.isEmpty()) {
      return INVALID;
    } else if (s == "u8") {
      return U8;
    } else if (s == "s16") {
      return S16;
    } else if (s == "s32") {
      return S32;
    } else if (s == "s64") {
      return S64;
    } else if (s == "f32") {
      return F32;
    } else if (s == "f64") {
      return F64;
    } else if (s == "u8p") {
      return U8P;
    } else if (s == "s16p") {
      return S16P;
    } else if (s == "s32p") {
      return S32P;
    } else if (s == "s64p") {
      return S64P;
    } else if (s == "f32p") {
      return F32P;
    } else if (s == "f64p") {
      return F64P;
    } else {
      // Deprecated: sample formats used to be serialized as an integer. Handle that here, but we'll
      //             probably remove that eventually.
      bool ok;
      int i = s.toInt(&ok);
      if (ok && i > INVALID && i < COUNT) {
        return static_cast<Format>(i);
      } else {
        // Failed to deserialize from string
        return INVALID;
      }
    }
  }

  static bool is_packed(Format f)
  {
    return f >= PACKED_START && f < PACKED_END;
  }

  bool is_packed() const { return is_packed(f_); }

  static bool is_planar(Format f)
  {
    return f >= PLANAR_START && f < PLANAR_END;
  }

  bool is_planar() const { return is_planar(f_); }

  static SampleFormat to_packed_equivalent(SampleFormat fmt)
  {
    switch (fmt) {

    // For packed input, just return input
    case U8:
    case S16:
    case S32:
    case S64:
    case F32:
    case F64:
      return fmt;

    // Convert to packed
    case U8P:
      return U8;
    case S16P:
      return S16;
    case S32P:
      return S32;
    case S64P:
      return S64;
    case F32P:
      return F32;
    case F64P:
      return F64;

    case INVALID:
    case COUNT:
      break;
    }

    return INVALID;
  }

  SampleFormat to_packed_equivalent() const
  {
    return to_packed_equivalent(f_);
  }

  static SampleFormat to_planar_equivalent(SampleFormat fmt)
  {
    switch (fmt) {

    // Convert to planar
    case U8:
      return U8P;
    case S16:
      return S16P;
    case S32:
      return S32P;
    case S64:
      return S64P;
    case F32:
      return F32P;
    case F64:
      return F64P;

    // For planar input, just return input
    case U8P:
    case S16P:
    case S32P:
    case S64P:
    case F32P:
    case F64P:
      return fmt;

    case INVALID:
    case COUNT:
      break;
    }

    return INVALID;
  }

  SampleFormat to_planar_equivalent() const
  {
    return to_planar_equivalent(f_);
  }

private:
  Format f_;

};

}

#endif // LIBOLIVECORE_SAMPLEFORMAT_H
