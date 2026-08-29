

#include "ofxCore.h"
#include "ofxhUtilities.h"

namespace OFX {

  /// get me deepest bit depth 
  std::string FindDeepestBitDepth(const std::string &s1, const std::string &s2)
  {
    if(s1 == kOfxBitDepthNone) {
      return s2;
    }
    else if(s1 == kOfxBitDepthByte) {
      if(s2 == kOfxBitDepthShort || s2 == kOfxBitDepthFloat)
        return s2;
      return s1;
    }
    else if(s1 == kOfxBitDepthShort) {
      if(s2 == kOfxBitDepthFloat)
        return s2;
      return s1;
    }
    else if(s1 == kOfxBitDepthHalf) {
      if(s2 == kOfxBitDepthFloat)
        return s2;
      return s1;
    }
    else if(s1 == kOfxBitDepthFloat) {
      return s1;
    }
    else {
      return s2; // oooh this might be bad dad.
    }
  }

}
