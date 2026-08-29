

#ifndef _ofxTimeLine_h_
#define _ofxTimeLine_h_
#include "ofxCore.h"
/** @brief Name of the time line suite */
#define kOfxTimeLineSuite "OfxTimeLineSuite"
/** @brief Suite to control timelines 

    This suite is used to enquire and control a timeline associated with a plug-in
    instance.

    This is an optional suite in the Image Effect API.
*/
typedef struct OfxTimeLineSuiteV1 {
  /** @brief Get the time value of the timeline that is controlling to the indicated effect.
      
      \arg \c instance is the instance of the effect changing the timeline, cast to a void *
      \arg \c time pointer through which the timeline value should be returned
       
      This function returns the current time value of the timeline associated with the effect instance.

      @returns
      - ::kOfxStatOK - the time enquiry was sucessful
      - ::kOfxStatFailed - the enquiry failed for some host specific reason
      - ::kOfxStatErrBadHandle - the effect handle was invalid
  */
  OfxStatus (*getTime)(void *instance, double *time);

  /** @brief Move the timeline control to the indicated time.
      
      \arg \c instance is the instance of the effect changing the timeline, cast to a void *
      \arg \c time is the time to change the timeline to. This is in the temporal coordinate system of the effect.
       
      This function moves the timeline to the indicated frame and returns. Any side effects of the timeline
      change are also triggered and completed before this returns (for example instance changed actions and renders
      if the output of the effect is being viewed).

      @returns
      - ::kOfxStatOK - the time was changed sucessfully, will all side effects if the change completed
      - ::kOfxStatFailed - the change failed for some host specific reason
      - ::kOfxStatErrBadHandle - the effect handle was invalid
      - ::kOfxStatErrValue - the time was an illegal value       
  */
  OfxStatus (*gotoTime)(void *instance, double time);

  /** @brief Get the current bounds on a timeline
      
      \arg \c instance is the instance of the effect changing the timeline, cast to a void *
      \arg \c firstTime is the first time on the timeline. This is in the temporal coordinate system of the effect.
      \arg \c lastTime is last time on the timeline. This is in the temporal coordinate system of the effect.
       
      This function

      @returns
      - ::kOfxStatOK - the time enquiry was sucessful
      - ::kOfxStatFailed - the enquiry failed for some host specific reason
      - ::kOfxStatErrBadHandle - the effect handle was invalid
  */
  OfxStatus (*getTimeBounds)(void *instance, double *firstTime, double *lastTime);
} OfxTimeLineSuiteV1;

#endif
