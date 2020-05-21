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

#include "export.h"

#include "common/timecodefunctions.h"
#include "render/colormanager.h"

OLIVE_NAMESPACE_ENTER

ExportTask::ExportTask(ViewerOutput* viewer_node,
                       ColorManager* color_manager,
                       const ExportParams& params) :
  RenderTask(viewer_node, params.video_params(), params.audio_params()),
  color_manager_(color_manager),
  params_(params)
{
  SetTitle(tr("Exporting \"%1\"").arg(viewer_node->media_name()));
}

bool ExportTask::Run()
{
  TimeRange range;

  encoder_ = Encoder::CreateFromID(params_.encoder(), params_);
  if (!encoder_) {
    SetError(tr("Failed to create encoder"));
    return false;
  }

  if (!encoder_->Open()) {
    SetError(tr("Failed to open file"));
    encoder_->deleteLater();
    return false;
  }

  if (params_.has_custom_range()) {
    // Render custom range only
    range = params_.custom_range();
  } else {
    // Render entire sequence
    range = TimeRange(0, viewer()->GetLength());
  }

  frame_time_ = Timecode::time_to_timestamp(range.in(), viewer()->video_params().time_base());

  QMatrix4x4 mat;

  if (params_.video_enabled()) {

    // If a transformation matrix is applied to this video, create it here
    if (params_.video_scaling_method() != ExportParams::kStretch) {
      mat = ExportParams::GenerateMatrix(params_.video_scaling_method(),
                                         viewer()->video_params().width(),
                                         viewer()->video_params().height(),
                                         params_.video_params().width(),
                                         params_.video_params().height());
    }

    // Create color processor
    color_processor_ = ColorProcessor::Create(color_manager_,
                                              color_manager_->GetReferenceColorSpace(),
                                              params_.color_transform());
  }

  if (params_.audio_enabled()) {
    audio_data_.SetParameters(audio_params());
  }

  // Start render process
  Render({range}, mat, params_.audio_enabled());

  bool success = true;

  foreach (QFuture<bool> f, write_frame_futures_) {
    f.waitForFinished();

    if (!f.result()) {
      SetError(tr("Failed to write AVFrame"));
      success = false;
    }
  }

  if (params_.audio_enabled()) {
    // Write audio data now
    encoder_->WriteAudio(audio_params(), audio_data_.GetCacheFilename());
  }

  encoder_->Close();

  encoder_->deleteLater();

  return success;
}

void FrameColorConvert(ColorProcessorPtr processor, FramePtr frame)
{
  // OCIO conversion requires a frame in 32F format
  if (frame->format() != PixelFormat::PIX_FMT_RGBA32F) {
    frame = PixelFormat::ConvertPixelFormat(frame, PixelFormat::PIX_FMT_RGBA32F);
  }

  // Color conversion must be done with unassociated alpha, and the pipeline is always associated
  ColorManager::DisassociateAlpha(frame);

  // Convert color space
  processor->ConvertFrame(frame);

  // Re-associate alpha
  ColorManager::ReassociateAlpha(frame);
}

QFuture<void> ExportTask::DownloadFrame(FramePtr frame, const QByteArray &hash)
{
  rendered_frame_.insert(hash, frame);

  return QtConcurrent::run(FrameColorConvert, color_processor_, frame);
}

void ExportTask::FrameDownloaded(const QByteArray &hash, const QLinkedList<rational> &times)
{
  FramePtr f = rendered_frame_.value(hash);

  foreach (const rational& t, times) {
    time_map_.insert(t, f);
  }

  forever {
    rational real_time = Timecode::timestamp_to_time(frame_time_,
                                                     viewer()->video_params().time_base());

    if (!time_map_.contains(real_time)) {
      break;
    }

    // Unfortunately this can't be done in another thread since the frames need to be sent
    // one after the other chronologically.
    encoder_->WriteFrame(time_map_.value(real_time), real_time);

    frame_time_++;

  }
}

void ExportTask::AudioDownloaded(const TimeRange &range, SampleBufferPtr samples)
{
  TimeRange adjusted_range = range;

  if (params_.has_custom_range()) {
    adjusted_range -= params_.custom_range().in();
  }

  qDebug() << "Downloaded audio" << adjusted_range;

  audio_data_.WritePCM(adjusted_range, samples);
}

OLIVE_NAMESPACE_EXIT
