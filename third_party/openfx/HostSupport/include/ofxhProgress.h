


#ifndef _ofxhProgress_h_
#define _ofxhProgress_h_

#include "ofxProgress.h"

namespace OFX {
  namespace Host {
    namespace Progress {
      
      /// Things that display progress derive from this ABC and implement the following
      /// functions.
      class ProgressI {
      public :
        virtual ~ProgressI() {}

        /// Start doing progress. 
        virtual void progressStart(const std::string &message, const std::string &messageid) = 0;

        /// finish yer progress
        virtual void progressEnd() = 0;

        /// set the progress to some level of completion, returns
        /// false if you should abandon processing, true to continue
        virtual bool progressUpdate(double t) = 0;        
      };

    } // namespace progress
  } // namespace Host
} // namespace OFX

#endif
