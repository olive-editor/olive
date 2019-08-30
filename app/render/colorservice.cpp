#include "colorservice.h"

#include <QFloat16>

#include "common/define.h"

ColorService::ColorService(const char* source_space, const char* dest_space)
{
  OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();

  processor = config->getProcessor(source_space,
                                   dest_space);
}

void ColorService::Init()
{
  try {
    // FIXME: Hardcoded values for testing purposes
    OCIO::ConstConfigRcPtr config = OCIO::Config::CreateFromFile("/run/media/matt/Home/OpenColorIO/ocio.configs.0.7v4/nuke-default/config.ocio");

    OCIO::SetCurrentConfig(config);
  } catch (OCIO::Exception& exception) {
    qWarning() << "OpenColorIO Error:" << exception.what();
  }
}

ColorServicePtr ColorService::Create(const char *source_space, const char *dest_space)
{
  return std::make_shared<ColorService>(source_space, dest_space);
}

void ColorService::ConvertFrame(FramePtr f)
{
  OCIO::PackedImageDesc img(reinterpret_cast<float*>(f->data()), f->width(), f->height(), kRGBAChannels);

  processor->apply(img);
}

void ColorService::DisassociateAlpha(FramePtr f)
{
  AssociateAlphaPixFmtFilter(kDisassociate, f);
}

void ColorService::AssociateAlpha(FramePtr f)
{
  AssociateAlphaPixFmtFilter(kAssociate, f);
}

void ColorService::ReassociateAlpha(FramePtr f)
{
  AssociateAlphaPixFmtFilter(kReassociate, f);
}

OpenColorIO::v1::ConstProcessorRcPtr ColorService::GetProcessor()
{
  return processor;
}

void ColorService::AssociateAlphaPixFmtFilter(ColorService::AlphaAction action, FramePtr f)
{
  int pixel_count = f->width() * f->height() * kRGBAChannels;

  switch (static_cast<olive::PixelFormat>(f->format())) {
  case olive::PIX_FMT_INVALID:
  case olive::PIX_FMT_COUNT:
    qWarning() << "Alpha association functions received an invalid pixel format";
    break;
  case olive::PIX_FMT_RGBA8:
  case olive::PIX_FMT_RGBA16:
    qWarning() << "Alpha association functions only works on float-based pixel formats at this time";
    break;
  case olive::PIX_FMT_RGBA16F:
  {
    AssociateAlphaInternal<qfloat16>(action, reinterpret_cast<qfloat16*>(f->data()), pixel_count);
    break;
  }
  case olive::PIX_FMT_RGBA32F:
  {
    AssociateAlphaInternal<float>(action, reinterpret_cast<float*>(f->data()), pixel_count);
    break;
  }
  }
}

template<typename T>
void ColorService::AssociateAlphaInternal(ColorService::AlphaAction action, T *data, int pix_count)
{
  for (int i=0;i<pix_count;i+=kRGBAChannels) {
    T alpha = data[i+kRGBChannels];

    // FIXME: Is qIsNull the correct approach here or should it literally be an != 0?
    if (action == kAssociate || !qIsNull(alpha)) {
      for (int j=0;j<kRGBChannels;j++) {
        if (action == kDisassociate) {
          data[i+j] /= alpha;
        } else {
          data[i+j] *= alpha;
        }
      }
    }
  }
}
