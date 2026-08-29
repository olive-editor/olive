


#ifndef _ofxhTimeLine_h_
#define _ofxhTimeLine_h_

#include "ofxTimeLine.h"


  
    namespace OFX::Host::TimeLine {
      
      /// Things that implement timeline controls derive from this ABC and implement the following
      /// functions.
      class TimeLineI {
      public :
        virtual ~TimeLineI() = default;

        /// get the current time on the timeline. This is not necessarily the same
        /// time as being passed to an action (eg render)
        virtual double timeLineGetTime() = 0;

        /// set the timeline to a specific time
        virtual void timeLineGotoTime(double t) = 0;

        /// get the first and last times available on the effect's timeline
        virtual void timeLineGetBounds(double &t1, double &t2) = 0;        
      };

    } // namespace progress
  


#endif
