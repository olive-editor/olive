#ifndef TIMELINEFUNCTIONS_H
#define TIMELINEFUNCTIONS_H

#include <QVector>

#include "timeline/clip.h"
#include "timeline/mediaimportdata.h"
#include "ghost.h"

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

enum Alignment {
  kAlignmentTop,
  kAlignmentBottom,
  kAlignmentSingle
};

void RelinkClips(QVector<Clip*>& pre_clips, QVector<ClipPtr> &post_clips);

bool SnapToPoint(long point, long* l, double zoom);

QVector<Ghost> CreateGhostsFromMedia(Sequence *seq, long entry_point, QVector<olive::timeline::MediaImportData> &media_list);

// snapping
extern bool snapping;
extern bool snapped;
extern long snap_point;

}
}

#endif // TIMELINEFUNCTIONS_H
