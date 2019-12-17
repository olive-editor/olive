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

#include "videostreamproperties.h"

#include <QGridLayout>
#include <QLabel>
#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE::v1;

#include "project/item/footage/footage.h"
#include "project/project.h"
#include "undo/undostack.h"

VideoStreamProperties::VideoStreamProperties(VideoStreamPtr stream) :
  stream_(stream)
{
  QGridLayout* video_layout = new QGridLayout(this);
  video_layout->setMargin(0);

  video_layout->addWidget(new QLabel(tr("Color Space:")), 0, 0);

  video_color_space_ = new QComboBox();
  OCIO::ConstConfigRcPtr config = stream->footage()->project()->color_manager()->GetConfig();
  int number_of_colorspaces = config->getNumColorSpaces();

  for (int i=0;i<number_of_colorspaces;i++) {
    QString colorspace = config->getColorSpaceNameByIndex(i);

    video_color_space_->addItem(colorspace);
  }

  video_color_space_->setCurrentText(stream_->colorspace());

  video_layout->addWidget(video_color_space_, 0, 1);

  video_premultiply_alpha_ = new QCheckBox(tr("Premultiplied Alpha"));
  video_premultiply_alpha_->setChecked(stream_->premultiplied_alpha());
  video_layout->addWidget(video_premultiply_alpha_, 1, 0, 1, 2);
}

void VideoStreamProperties::Accept(QUndoCommand *parent)
{
  if (video_premultiply_alpha_->isChecked() != stream_->premultiplied_alpha()
      || video_color_space_->currentText() != stream_->colorspace()) {
    new VideoStreamChangeCommand(stream_,
                                 video_premultiply_alpha_->isChecked(),
                                 video_color_space_->currentText(),
                                 parent);
  }
}

VideoStreamProperties::VideoStreamChangeCommand::VideoStreamChangeCommand(VideoStreamPtr stream,
                                                                          bool premultiplied,
                                                                          QString colorspace,
                                                                          QUndoCommand *parent) :
  QUndoCommand(parent),
  stream_(stream),
  new_premultiplied_(premultiplied),
  new_colorspace_(colorspace)
{
}

void VideoStreamProperties::VideoStreamChangeCommand::redo()
{
  old_premultiplied_ = stream_->premultiplied_alpha();
  old_colorspace_ = stream_->colorspace();

  stream_->set_premultiplied_alpha(new_premultiplied_);
  stream_->set_colorspace(new_colorspace_);
}

void VideoStreamProperties::VideoStreamChangeCommand::undo()
{
  stream_->set_premultiplied_alpha(old_premultiplied_);
  stream_->set_colorspace(old_colorspace_);
}
