#ifndef COLORSERVICE_H
#define COLORSERVICE_H

#include <memory>
#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE::v1;

#include "decoder/frame.h"

class ColorService
{
public:
  ColorService();

  void ConvertFrame(FramePtr f);

private:
  OCIO::ConstProcessorRcPtr processor;
};

using ColorServicePtr = std::shared_ptr<ColorService>;

#endif // COLORSERVICE_H
