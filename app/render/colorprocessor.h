#ifndef COLORPROCESSOR_H
#define COLORPROCESSOR_H

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE::v1;

#include "common/constructors.h"
#include "decoder/frame.h"


class ColorProcessor;
using ColorProcessorPtr = std::shared_ptr<ColorProcessor>;

class ColorProcessor
{
public:
  ColorProcessor(OCIO::ConstConfigRcPtr config, const QString &source_space, const QString &dest_space);

  ColorProcessor(OCIO::ConstConfigRcPtr config, const QString& source_space,
                 QString display,
                 QString view,
                 const QString& look);

  DISABLE_COPY_MOVE(ColorProcessor)

  static ColorProcessorPtr Create(OCIO::ConstConfigRcPtr config, const QString& source_space, const QString& dest_space);

  static ColorProcessorPtr Create(OCIO::ConstConfigRcPtr config,
                                  const QString& source_space,
                                  const QString& display,
                                  const QString& view,
                                  const QString& look);

  OCIO::ConstProcessorRcPtr GetProcessor();

  void ConvertFrame(FramePtr f);

private:
  OCIO::ConstProcessorRcPtr processor;

};

#endif // COLORPROCESSOR_H
