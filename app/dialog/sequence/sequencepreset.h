#ifndef SEQUENCEPARAM_H
#define SEQUENCEPARAM_H

#include "common/rational.h"
#include "render/pixelformat.h"

OLIVE_NAMESPACE_ENTER

struct SequencePreset {
  QString name;
  int width;
  int height;
  rational frame_rate;
  int sample_rate;
  uint64_t channel_layout;
  int preview_divider;
  PixelFormat::Format preview_format;
};

OLIVE_NAMESPACE_EXIT

#endif // SEQUENCEPARAM_H
