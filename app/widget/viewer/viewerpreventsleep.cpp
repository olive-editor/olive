#include "viewerpreventsleep.h"

#include <QtGlobal>

#if defined(Q_OS_WINDOWS)
#include <winbase.h>
#elif defined(Q_OS_MAC)
#include <IOKit/pwr_mgt/IOPMLib.h>
#endif

namespace olive {

#if defined(Q_OS_MAC)
IOPMAssertionID assertionID = 0;
#endif

void PreventSleep(bool on)
{
#if defined(Q_OS_WINDOWS)
  SetThreadExecutionState(on ? ES_DISPLAY_REQUIRED | ES_CONTINUOUS : ES_CONTINUOUS);
#elif defined(Q_OS_MAC)
  if (on) {
    static const CFStringRef reasonForActivity = CFSTR("Video Playback");

    IOPMAssertionCreateWithName(kIOPMAssertionTypeNoDisplaySleep,
                                kIOPMAssertionLevelOn, reasonForActivity, &assertionID);
  } else if (assertionID) {
    IOPMAssertionRelease(assertionID);
    assertionID = 0;
  }
#endif
}

}
