#ifndef RATIONAL_H
#define RATIONAL_H

#include <libavformat/avformat.h>

namespace olive {
  /**
   * Rational type for timeline values. Currently we just use FFmpeg's AVRational for ease but we can change this
   * later if we want.
   */
  using rational = AVRational;
};

#endif // RATIONAL_H
