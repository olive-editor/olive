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

#ifndef COLORPROCESSOR_H
#define COLORPROCESSOR_H

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE::v1;

#include "codec/frame.h"
#include "common/constructors.h"
#include "render/color.h"

OLIVE_NAMESPACE_ENTER

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

  Color ConvertColor(Color in);

private:
  OCIO::ConstProcessorRcPtr processor;

};

OLIVE_NAMESPACE_EXIT

#endif // COLORPROCESSOR_H
