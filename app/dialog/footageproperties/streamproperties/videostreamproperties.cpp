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
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE::v1;

#include "common/ratiodialog.h"
#include "project/item/footage/footage.h"
#include "project/project.h"
#include "undo/undostack.h"

OLIVE_NAMESPACE_ENTER

VideoStreamProperties::VideoStreamProperties(ImageStreamPtr stream) :
  stream_(stream)
{
  QGridLayout* video_layout = new QGridLayout(this);
  video_layout->setMargin(0);

  int row = 0;

  video_layout->addWidget(new QLabel(tr("Pixel Aspect:")), row, 0);

  pixel_aspect_combo_ = new QComboBox();
  video_layout->addWidget(pixel_aspect_combo_, row, 1);

  AddPixelAspectRatio(tr("Square Pixels"), rational(1));
  AddPixelAspectRatio(tr("NTSC Standard"), rational(8, 9));
  AddPixelAspectRatio(tr("NTSC Widescreen"), rational(32, 27));
  AddPixelAspectRatio(tr("PAL Standard"), rational(16, 15));
  AddPixelAspectRatio(tr("PAL Widescreen"), rational(64, 45));
  AddPixelAspectRatio(tr("HD Anamorphic 1080"), rational(4, 3));

  // Always add custom item last, much of the logic relies on this. Set this to the current AR so
  // that if none of the above are ==, it will eventually select this item
  AddPixelAspectRatio(QString(), rational());
  UpdateCustomItem(stream->pixel_aspect_ratio());

  // Determine which index to select on startup
  for (int i=0; i<pixel_aspect_combo_->count(); i++) {
    if (pixel_aspect_combo_->itemData(i).value<rational>() == stream->pixel_aspect_ratio()) {
      pixel_aspect_combo_->setCurrentIndex(i);
      break;
    }
  }

  // Pick up index signal to query for custom aspect ratio if requested
  connect(pixel_aspect_combo_, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          this, &VideoStreamProperties::PixelAspectComboBoxChanged);

  row++;

  video_layout->addWidget(new QLabel(tr("Interlacing:")), row, 0);

  video_interlace_combo_ = new QComboBox();

  // These must match the Interlacing enum in ImageStream
  video_interlace_combo_->addItem(tr("None (Progressive)"));
  video_interlace_combo_->addItem(tr("Top-Field First"));
  video_interlace_combo_->addItem(tr("Bottom-Field First"));

  video_interlace_combo_->setCurrentIndex(stream->interlacing());

  video_layout->addWidget(video_interlace_combo_, row, 1);

  row++;

  video_layout->addWidget(new QLabel(tr("Color Space:")), row, 0);

  video_color_space_ = new QComboBox();
  OCIO::ConstConfigRcPtr config = stream->footage()->project()->color_manager()->GetConfig();
  int number_of_colorspaces = config->getNumColorSpaces();

  video_color_space_->addItem(tr("Default (%1)").arg(stream->footage()->project()->color_manager()->GetDefaultInputColorSpace()));

  for (int i=0;i<number_of_colorspaces;i++) {
    QString colorspace = config->getColorSpaceNameByIndex(i);

    video_color_space_->addItem(colorspace);
  }

  video_color_space_->setCurrentText(stream_->colorspace(false));

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
      || set_colorspace != stream_->colorspace(false)
      || static_cast<ImageStream::Interlacing>(video_interlace_combo_->currentIndex()) != stream_->interlacing()
      || pixel_aspect_combo_->currentData().value<rational>() != stream_->pixel_aspect_ratio()) {

    new VideoStreamChangeCommand(stream_,
                                 video_premultiply_alpha_->isChecked(),
                                 set_colorspace,
                                 static_cast<ImageStream::Interlacing>(video_interlace_combo_->currentIndex()),
                                 pixel_aspect_combo_->currentData().value<rational>(),
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

void VideoStreamProperties::AddPixelAspectRatio(const QString &name, const rational &ratio)
{
  pixel_aspect_combo_->addItem(GetPixelAspectRatioItemText(name, ratio),
                               QVariant::fromValue(ratio));
}

QString VideoStreamProperties::GetPixelAspectRatioItemText(const QString &name, const rational &ratio)
{
  return tr("%1 (%2)").arg(name, QString::number(ratio.toDouble(), 'f', 4));
}

void VideoStreamProperties::UpdateCustomItem(const rational &ratio)
{
  const int custom_index = pixel_aspect_combo_->count() - 1;

  pixel_aspect_combo_->setItemText(custom_index,
                                   GetPixelAspectRatioItemText(tr("Custom"), ratio));
  pixel_aspect_combo_->setItemData(custom_index,
                                   QVariant::fromValue(ratio));
}

void VideoStreamProperties::PixelAspectComboBoxChanged(int index)
{
  // Detect if custom was selected, in which case query what the new AR should be
  if (index == pixel_aspect_combo_->count() - 1) {
    // Query for custom pixel aspect ratio
    bool ok;

    double custom_ratio = GetFloatRatioFromUser(this, tr("Set Custom Pixel Aspect Ratio"), &ok);

    if (ok) {
      UpdateCustomItem(rational::fromDouble(custom_ratio));
    }
  }
}

VideoStreamProperties::VideoStreamChangeCommand::VideoStreamChangeCommand(ImageStreamPtr stream,
                                                                          bool premultiplied,
                                                                          QString colorspace,
                                                                          ImageStream::Interlacing interlacing,
                                                                          const rational &pixel_ar,
                                                                          QUndoCommand *parent) :
  UndoCommand(parent),
  stream_(stream),
  new_premultiplied_(premultiplied),
  new_colorspace_(colorspace),
  new_interlacing_(interlacing),
  new_pixel_ar_(pixel_ar)
{
}

Project *VideoStreamProperties::VideoStreamChangeCommand::GetRelevantProject() const
{
  return stream_->footage()->project();
}

void VideoStreamProperties::VideoStreamChangeCommand::redo_internal()
{
  old_premultiplied_ = stream_->premultiplied_alpha();
  old_colorspace_ = stream_->colorspace(false);
  old_interlacing_ = stream_->interlacing();
  old_pixel_ar_ = stream_->pixel_aspect_ratio();

  stream_->set_premultiplied_alpha(new_premultiplied_);
  stream_->set_colorspace(new_colorspace_);
  stream_->set_interlacing(new_interlacing_);
  stream_->set_pixel_aspect_ratio(new_pixel_ar_);
}

void VideoStreamProperties::VideoStreamChangeCommand::undo_internal()
{
  stream_->set_premultiplied_alpha(old_premultiplied_);
  stream_->set_colorspace(old_colorspace_);
  stream_->set_interlacing(old_interlacing_);
  stream_->set_pixel_aspect_ratio(old_pixel_ar_);
}

VideoStreamProperties::ImageSequenceChangeCommand::ImageSequenceChangeCommand(VideoStreamPtr video_stream, int64_t start_index, int64_t duration, QUndoCommand *parent) :
  UndoCommand(parent),
  video_stream_(video_stream),
  new_start_index_(start_index),
  new_duration_(duration)
{
}

Project *VideoStreamProperties::ImageSequenceChangeCommand::GetRelevantProject() const
{
  return video_stream_->footage()->project();
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

OLIVE_NAMESPACE_EXIT
