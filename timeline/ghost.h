#ifndef GHOST_H
#define GHOST_H

#include "effects/transition.h"
#include "timelinefunctions.h"

struct Ghost {
  int clip;
  long in;
  long out;
  int track;
  long clip_in;

  long old_in;
  long old_out;
  int old_track;
  long old_clip_in;

  // importing variables
  Media* media;
  int media_stream;

  // other variables
  long ghost_length;
  long media_length;
  olive::timeline::TrimType trim_type;

  // transition trimming
  TransitionPtr transition;
};

#endif // GHOST_H
