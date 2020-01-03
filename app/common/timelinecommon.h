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

  static bool IsATrimMode(MovementMode mode) {return mode == kTrimIn || mode == kTrimOut;}
};

#endif // TIMELINECOMMON_H
