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

#include "codec/frame.h"
#include "render/color.h"
#include "render/colortransform.h"

OLIVE_NAMESPACE_ENTER

class ColorManager;

class ColorProcessor;
using ColorProcessorPtr = std::shared_ptr<ColorProcessor>;

class ColorProcessor
{
public:
  enum Direction {
    kNormal,
    kInverse
  };

  ColorProcessor(ColorManager* config, const QString& input, const ColorTransform& dest_space);

  DISABLE_COPY_MOVE(ColorProcessor)

  static ColorProcessorPtr Create(ColorManager* config, const QString& input, const ColorTransform& dest_space);

  OCIO::ConstProcessorRcPtr GetProcessor();

  void ConvertFrame(FramePtr f);
  void ConvertFrame(Frame* f);

  Color ConvertColor(Color in);

  const QString& id() const
  {
    return id_;
  }

  static QString GenerateID(ColorManager* config, const QString& input, const ColorTransform& dest_space);

private:
  OCIO::ConstProcessorRcPtr processor_;

  QString id_;

};

using ColorProcessorChain = QList<ColorProcessorPtr>;

OLIVE_NAMESPACE_EXIT

#endif // COLORPROCESSOR_H
