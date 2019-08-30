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

  static void DisassociateAlpha(FramePtr f);

  static void AssociateAlpha(FramePtr f);

  static void ReassociateAlpha(FramePtr f);

  OCIO::ConstProcessorRcPtr GetProcessor();

private:
  OCIO::ConstProcessorRcPtr processor;

  enum AlphaAction {
    kAssociate,
    kDisassociate,
    kReassociate
  };

  static void AssociateAlphaPixFmtFilter(AlphaAction action, FramePtr f);

  template<typename T>
  static void AssociateAlphaInternal(AlphaAction action, T* data, int pix_count);
};

#endif // COLORSERVICE_H
