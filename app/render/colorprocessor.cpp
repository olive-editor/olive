#include "colorprocessor.h"

#include "common/define.h"

ColorProcessor::ColorProcessor(const QString& source_space, const QString& dest_space)
{
  OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();

  processor = config->getProcessor(source_space.toUtf8(),
                                   dest_space.toUtf8());
}

ColorProcessor::ColorProcessor(const QString& source_space,
                               QString display,
                               QString view,
                               const QString& look)
{
  OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();

  if (display.isEmpty()) {
    display = config->getDefaultDisplay();
  }

  if (view.isEmpty()) {
    view = config->getDefaultView(display.toUtf8());
  }

  // Get current display stats
  OCIO::DisplayTransformRcPtr transform = OCIO::DisplayTransform::Create();
  transform->setInputColorSpaceName(source_space.toUtf8());
  transform->setDisplay(display.toUtf8());
  transform->setView(view.toUtf8());

  if (!look.isEmpty()) {
    transform->setLooksOverride(look.toUtf8());
    transform->setLooksOverrideEnabled(true);
  }

  processor = config->getProcessor(transform);
}

void ColorProcessor::ConvertFrame(FramePtr f)
{
  OCIO::PackedImageDesc img(reinterpret_cast<float*>(f->data()), f->width(), f->height(), kRGBAChannels);

  processor->apply(img);
}

ColorProcessorPtr ColorProcessor::Create(const QString& source_space, const QString& dest_space)
{
  return std::make_shared<ColorProcessor>(source_space, dest_space);
}

ColorProcessorPtr ColorProcessor::Create(const QString &source_space, const QString &display, const QString &view, const QString &look)
{
  return std::make_shared<ColorProcessor>(source_space, display, view, look);
}

OpenColorIO::v1::ConstProcessorRcPtr ColorProcessor::GetProcessor()
{
  return processor;
}
