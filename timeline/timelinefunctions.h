#ifndef TIMELINEFUNCTIONS_H
#define TIMELINEFUNCTIONS_H

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

}
}

class TimelineFunctions
{
public:
  TimelineFunctions();
};

#endif // TIMELINEFUNCTIONS_H
