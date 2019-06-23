#ifndef VIDEOSTREAM_H
#define VIDEOSTREAM_H

#include "rational.h"

class Footage;

class VideoStream
{
public:
  VideoStream(Footage* footage, const int& index, const int& width, const int& height, const rational& timebase);

  Footage* footage();
  void set_footage(Footage* f);

  const int& index();
  void set_index(const int& index);

  const int& width();
  void set_width(const int& width);

  const int& height();
  void set_height(const int& height);

  const rational& timebase();
  void set_timebase(const rational& timebase);

private:
  Footage* footage_;

  int index_;
  int width_;
  int height_;
  rational timebase_;
};

#endif // VIDEOSTREAM_H
