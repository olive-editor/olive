#ifndef COLORSERVICE_H
#define COLORSERVICE_H

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

#include "decoder/frame.h"

class ColorService
{
public:
  ColorService();

  static void ConvertFrame(FramePtr f);

private:

};

#endif // COLORSERVICE_H
