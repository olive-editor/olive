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
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE::v1;

#include "project/item/footage/footage.h"
#include "project/project.h"
#include "undo/undostack.h"

VideoStreamProperties::VideoStreamProperties(ImageStreamPtr stream) :
  stream_(stream)
{
  QGridLayout* video_layout = new QGridLayout(this);
  video_layout->setMargin(0);

  int row = 0;

  video_layout->addWidget(new QLabel(tr("Color Space:")), row, 0);

  video_color_space_ = new QComboBox();
  OCIO::ConstConfigRcPtr config = stream->footage()->project()->color_manager()->GetConfig();
  int number_of_colorspaces = config->getNumColorSpaces();

  video_color_space_->addItem(tr("Default (%1)").arg(stream->footage()->project()->default_input_colorspace()));

  for (int i=0;i<number_of_colorspaces;i++) {
    QString colorspace = config->getColorSpaceNameByIndex(i);

    video_color_space_->addItem(colorspace);
  }

  video_color_space_->setCurrentText(stream_->colorspace());

  video_layout->addWidget(video_color_space_, row, 1);

  row++;

  video_premultiply_alpha_ = new QCheckBox(tr("Premultiplied Alpha"));
  video_premultiply_alpha_->setChecked(stream_->premultiplied_alpha());
  video_layout->addWidget(video_premultiply_alpha_, row, 0, 1, 2);

  row++;

  if (IsImageSequence(stream.get())) {
    QGroupBox* imgseq_group = new QGroupBox(tr("Image Sequence"));
    QGridLayout* imgseq_layout = new QGridLayout(imgseq_group);

    int imgseq_row = 0;

    VideoStream* video_stream = static_cast<VideoStream*>(stream.get());

    imgseq_layout->addWidget(new QLabel(tr("Start Index:")), imgseq_row, 0);

    imgseq_start_time_ = new IntegerSlider();
    imgseq_start_time_->SetMinimum(0);
    imgseq_start_time_->SetValue(video_stream->start_time());
    imgseq_layout->addWidget(imgseq_start_time_, imgseq_row, 1);

    imgseq_row++;

    imgseq_layout->addWidget(new QLabel(tr("End Index:")), imgseq_row, 0);

    imgseq_end_time_ = new IntegerSlider();
    imgseq_end_time_->SetMinimum(0);
    imgseq_end_time_->SetValue(video_stream->start_time() + video_stream->duration() - 1);
    imgseq_layout->addWidget(imgseq_end_time_, imgseq_row, 1);

    video_layout->addWidget(imgseq_group, row, 0, 1, 2);
  }
}

void VideoStreamProperties::Accept(QUndoCommand *parent)
{
  QString set_colorspace;

  if (video_color_space_->currentIndex() > 0) {
    set_colorspace = video_color_space_->currentText();
  }

  if (video_premultiply_alpha_->isChecked() != stream_->premultiplied_alpha()
      || set_colorspace != stream_->colorspace(false)) {

    new VideoStreamChangeCommand(stream_,
                                 video_premultiply_alpha_->isChecked(),
                                 set_colorspace,
                                 parent);
  }

  if (IsImageSequence(stream_.get())) {
    VideoStreamPtr video_stream = std::static_pointer_cast<VideoStream>(stream_);

    int64_t new_dur = imgseq_end_time_->GetValue() - imgseq_start_time_->GetValue() + 1;

    if (video_stream->start_time() != imgseq_start_time_->GetValue()
        || video_stream->duration() != new_dur) {
      new ImageSequenceChangeCommand(video_stream,
                                     imgseq_start_time_->GetValue(),
                                     new_dur,
                                     parent);
    }
  }
}

bool VideoStreamProperties::SanityCheck()
{
  if (IsImageSequence(stream_.get())) {
    if (imgseq_start_time_->GetValue() >= imgseq_end_time_->GetValue()) {
      QMessageBox::critical(this,
                            tr("Invalid Configuration"),
                            tr("Image sequence end index must be a value higher than the start index."),
                            QMessageBox::Ok);
      return false;
    }
  }

  return true;
}

bool VideoStreamProperties::IsImageSequence(ImageStream *stream)
{
  return (stream->type() == Stream::kVideo && static_cast<VideoStream*>(stream)->is_image_sequence());
}

VideoStreamProperties::VideoStreamChangeCommand::VideoStreamChangeCommand(ImageStreamPtr stream,
                                                                          bool premultiplied,
                                                                          QString colorspace,
                                                                          QUndoCommand *parent) :
  UndoCommand(parent),
  stream_(stream),
  new_premultiplied_(premultiplied),
  new_colorspace_(colorspace)
{
}

void VideoStreamProperties::VideoStreamChangeCommand::redo_internal()
{
  old_premultiplied_ = stream_->premultiplied_alpha();
  old_colorspace_ = stream_->colorspace(false);

  stream_->set_premultiplied_alpha(new_premultiplied_);
  stream_->set_colorspace(new_colorspace_);
}

void VideoStreamProperties::VideoStreamChangeCommand::undo_internal()
{
  stream_->set_premultiplied_alpha(old_premultiplied_);
  stream_->set_colorspace(old_colorspace_);
}

VideoStreamProperties::ImageSequenceChangeCommand::ImageSequenceChangeCommand(VideoStreamPtr video_stream, int64_t start_index, int64_t duration, QUndoCommand *parent) :
  UndoCommand(parent),
  video_stream_(video_stream),
  new_start_index_(start_index),
  new_duration_(duration)
{
}

void VideoStreamProperties::ImageSequenceChangeCommand::redo_internal()
{
  old_start_index_ = video_stream_->start_time();
  video_stream_->set_start_time(new_start_index_);

  old_duration_ = video_stream_->duration();
  video_stream_->set_duration(new_duration_);
}

void VideoStreamProperties::ImageSequenceChangeCommand::undo_internal()
{
  video_stream_->set_start_time(old_start_index_);
  video_stream_->set_duration(old_duration_);
}
