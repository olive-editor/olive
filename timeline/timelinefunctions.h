#ifndef TIMELINEFUNCTIONS_H
#define TIMELINEFUNCTIONS_H

#include <QVector>

#include "timeline/clip.h"

namespace olive {
namespace timeline {

enum CreateObjects {
  ADD_OBJ_TITLE,
  ADD_OBJ_SOLID,
  ADD_OBJ_BARS,
  ADD_OBJ_TONE,
  ADD_OBJ_NOISE,
  ADD_OBJ_AUDIO
};

enum TrimType {
  TRIM_NONE,
  TRIM_IN,
  TRIM_OUT
};

enum Alignment {
  kAlignmentTop,
  kAlignmentBottom,
  kAlignmentSingle
};

void RelinkClips(QVector<Clip*>& pre_clips, QVector<ClipPtr> &post_clips);

}
}

#endif // TIMELINEFUNCTIONS_H
