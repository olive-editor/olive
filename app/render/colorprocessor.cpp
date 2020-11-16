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
#include "common/ocioutils.h"
#include "colormanager.h"

OLIVE_NAMESPACE_ENTER

ColorProcessor::ColorProcessor(ColorManager *config, const QString &input, const ColorTransform &transform)
{
  const QString& output = (transform.output().isEmpty()) ? config->GetDefaultDisplay() : transform.output();

  if (transform.is_display()) {

    const QString& view = (transform.view().isEmpty()) ? config->GetDefaultView(output) : transform.view();

    auto display_transform = OCIO::DisplayViewTransform::Create();

    display_transform->setSrc(input.toUtf8());
    display_transform->setDisplay(output.toUtf8());
    display_transform->setView(view.toUtf8());

    OCIO_SET_C_LOCALE_FOR_SCOPE;

    if (transform.look().isEmpty()) {
      processor_ = config->GetConfig()->getProcessor(display_transform);
    } else {
      auto group = OCIO::GroupTransform::Create();

      const char* out_cs = OCIO::LookTransform::GetLooksResultColorSpace(config->GetConfig(),
                                                                         config->GetConfig()->getCurrentContext(),
                                                                         transform.look().toUtf8());

      auto lt = OCIO::LookTransform::Create();
      lt->setSrc(input.toUtf8());
      lt->setDst(out_cs);
      lt->setLooks(transform.look().toUtf8());
      lt->setSkipColorSpaceConversion(false);
      group->appendTransform(lt);

      display_transform->setSrc(out_cs);
      group->appendTransform(display_transform);

      processor_ = config->GetConfig()->getProcessor(group);
    }

  } else {

    OCIO_SET_C_LOCALE_FOR_SCOPE;
    processor_ = config->GetConfig()->getProcessor(input.toUtf8(),
                                                   output.toUtf8());

  }

  cpu_processor_ = processor_->getDefaultCPUProcessor();
  id_ = GenerateID(config, input, transform);
}

void ColorProcessor::ConvertFrame(Frame *f)
{
  OCIO::BitDepth ocio_bit_depth = OCIOUtils::GetOCIOBitDepthFromPixelFormat(f->format());

  if (ocio_bit_depth == OCIO::BIT_DEPTH_UNKNOWN) {
    qCritical() << "Tried to color convert frame with no format";
    return;
  }

  OCIO::PackedImageDesc img(f->data(),
                            f->width(),
                            f->height(),
                            VideoParams::kRGBAChannelCount,
                            ocio_bit_depth,
                            OCIO::AutoStride,
                            OCIO::AutoStride,
                            f->linesize_bytes());

  cpu_processor_->apply(img);
}

Color ColorProcessor::ConvertColor(const Color& in)
{
  // I've been bamboozled
  float c[4] = {float(in.red()), float(in.green()), float(in.blue()), float(in.alpha())};

  cpu_processor_->applyRGBA(c);

  return Color(c[0], c[1], c[2], c[3]);
}

QString ColorProcessor::GenerateID(ColorManager *config, const QString &input, const ColorTransform &transform)
{
  return QStringLiteral("%1:%2:%3:%4:%5").arg(config->GetConfigFilename(),
                                              input,
                                              transform.display(),
                                              transform.view(),
                                              transform.look());
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
