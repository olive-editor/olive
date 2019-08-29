#ifndef COLORSERVICE_H
#define COLORSERVICE_H

#include <memory>
#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE::v1;

#include "decoder/frame.h"
#include "render/gl/shadergenerators.h"

class ColorService;
using ColorServicePtr = std::shared_ptr<ColorService>;

class ColorService
{
public:
  ColorService(const char *source_space, const char *dest_space);

  static void Init();

  static ColorServicePtr Create(const char *source_space, const char *dest_space);

  void ConvertFrame(FramePtr f);

  OCIO::ConstProcessorRcPtr GetProcessor();

private:
  OCIO::ConstProcessorRcPtr processor;
};

#endif // COLORSERVICE_H
