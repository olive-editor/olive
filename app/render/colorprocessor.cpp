/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "colorprocessor.h"

#include "common/define.h"
#include "colormanager.h"

OLIVE_NAMESPACE_ENTER

ColorProcessor::ColorProcessor(ColorManager *config, const QString &input, const ColorTransform &transform)
{
  const QString& output = (transform.output().isEmpty()) ? config->GetDefaultDisplay() : transform.output();

  if (transform.is_display()) {

    const QString& view = (transform.view().isEmpty()) ? config->GetDefaultView(output) : transform.view();

    OCIO::DisplayTransformRcPtr display_transform = OCIO::DisplayTransform::Create();

    display_transform->setInputColorSpaceName(input.toUtf8());
    display_transform->setDisplay(output.toUtf8());
    display_transform->setView(view.toUtf8());

    if (!transform.look().isEmpty()) {
      display_transform->setLooksOverride(transform.look().toUtf8());
      display_transform->setLooksOverrideEnabled(true);
    }

    OCIO_SET_C_LOCALE_FOR_SCOPE;
    processor_ = config->GetConfig()->getProcessor(display_transform);

  } else {

    OCIO_SET_C_LOCALE_FOR_SCOPE;
    processor_ = config->GetConfig()->getProcessor(input.toUtf8(),
                                                   output.toUtf8());

  }
}

void ColorProcessor::ConvertFrame(Frame *f)
{
  OCIO::PackedImageDesc img(reinterpret_cast<float*>(f->data()),
                            f->width(),
                            f->height(),
                            PixelFormat::ChannelCount(f->format()),
                            OCIO::AutoStride,
                            OCIO::AutoStride,
                            f->linesize_bytes());

  processor_->apply(img);
}

Color ColorProcessor::ConvertColor(Color in)
{
  processor_->applyRGBA(in.data());
  return in;
}

ColorProcessorPtr ColorProcessor::Create(ColorManager *config, const QString& input, const ColorTransform &transform)
{
  return std::make_shared<ColorProcessor>(config, input, transform);
}

OCIO::ConstProcessorRcPtr ColorProcessor::GetProcessor()
{
  return processor_;
}

void ColorProcessor::ConvertFrame(FramePtr f)
{
  ConvertFrame(f.get());
}

OLIVE_NAMESPACE_EXIT
