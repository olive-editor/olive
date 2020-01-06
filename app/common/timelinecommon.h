#ifndef TIMELINECOMMON_H
#define TIMELINECOMMON_H

class Timeline {
public:
  enum MovementMode {
    kNone,
    kMove,
    kTrimIn,
    kTrimOut
  };

  enum TrackType {
    kTrackTypeNone = -1,
    kTrackTypeVideo,
    kTrackTypeAudio,
    kTrackTypeSubtitle,
    kTrackTypeCount
  };

  static bool IsATrimMode(MovementMode mode) {return mode == kTrimIn || mode == kTrimOut;}
};

#endif // TIMELINECOMMON_H
