#ifndef SNAPSERVICE_H
#define SNAPSERVICE_H

#include "common/rational.h"

namespace olive {

class SnapService
{
public:
  SnapService() = default;

  enum SnapPoints {
    kSnapToClips = 0x1,
    kSnapToPlayhead = 0x2,
    kSnapToMarkers = 0x4,
    kSnapAll = 0xFF
  };

  /**
   * @brief Snaps point `start_point` that is moving by `movement` to currently existing clips
   */
  virtual bool SnapPoint(QList<rational> start_times, rational *movement, int snap_points = kSnapAll) = 0;

  virtual void HideSnaps() = 0;

};

}

#endif // SNAPSERVICE_H
