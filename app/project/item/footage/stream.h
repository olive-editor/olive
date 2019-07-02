/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

#ifndef STREAM_H
#define STREAM_H

#include "rational.h"

class Footage;

/**
 * @brief The Stream class
 *
 * A base class for keeping metadata about a media stream. A Stream can contain video data, audio data, subtitle data,
 * etc. and a Stream object stores metadata about it.
 *
 * The Stream class is fairly simple and is intended to be subclassed for data that pertains specifically to one
 * Stream::Type. \see VideoStream and \see AudioStream.
 */
class Stream
{
public:
  enum Type {
    kUnknown,
    kVideo,
    kAudio,
    kData,
    kSubtitle,
    kAttachment
  };

  /**
   * @brief Stream constructor
   */
  Stream();

  /**
   * @brief Required virtual destructor, serves no purpose
   */
  virtual ~Stream();

  const Type& type();
  void set_type(const Type& type);

  Footage* footage();
  void set_footage(Footage* f);

  const rational& timebase();
  void set_timebase(const rational& timebase);

  const int& index();
  void set_index(const int& index);

  const int64_t& duration();
  void set_duration(const int64_t& duration);

private:
  Footage* footage_;

  rational timebase_;

  int64_t duration_;

  int index_;

  Type type_;

};

#endif // STREAM_H
