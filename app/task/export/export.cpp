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

  // For safety, if we're overwriting, we save to a temporary filename and then only overwrite it
  // at the end
  QString real_filename = params_.filename();
  if (QFileInfo::exists(params_.filename())) {
    // Generate a filename that definitely doesn't exist
    params_.SetFilename(FileFunctions::GetSafeTemporaryFilename(real_filename));
  }

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

  QSize video_force_size;
  QMatrix4x4 video_force_matrix;

  if (params_.video_enabled()) {

    // If a transformation matrix is applied to this video, create it here
    if (viewer()->video_params().width() != params_.video_params().width()
        || params_.video_params().height() != params_.video_params().height()) {
      video_force_size = QSize(params_.video_params().width(), params_.video_params().height());

      if (params_.video_scaling_method() != ExportParams::kStretch) {
        video_force_matrix = ExportParams::GenerateMatrix(params_.video_scaling_method(),
                                                          viewer()->video_params().width(),
                                                          viewer()->video_params().height(),
                                                          params_.video_params().width(),
                                                          params_.video_params().height());
      }
    } else {
      // Disables forcing size in the renderer
      video_force_size = QSize(0, 0);
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
  TimeRangeList video_range, audio_range;

  if (params_.video_enabled()) {
    video_range = {range};
  }

  if (params_.audio_enabled()) {
    audio_range = {range};
    audio_data_.SetLength(range.length());
  }

  Render(color_manager_, video_range, audio_range, RenderMode::kOnline, nullptr,
         video_force_size, video_force_matrix, encoder_->GetDesiredPixelFormat(),
         color_processor_);

  bool success = true;

  if (params_.audio_enabled()) {
    // Write audio data now
    encoder_->WriteAudio(audio_params(), audio_data_.CreatePlaybackDevice(encoder_));
  }

  encoder_->Close();

  delete encoder_;

  // If cancelled, delete the file we made, which is always a file we created since we write to a
  // temp file during the actual encoding process
  if (IsCancelled()) {
    QFile::remove(params_.filename());
  } else if (params_.filename() != real_filename) {
    // If we were writing to a temp file, overwrite now
    if (!FileFunctions::RenameFileAllowOverwrite(params_.filename(), real_filename)) {
      SetError(tr("Failed to overwrite \"%1\". Export has been saved as \"%2\" instead.")
               .arg(real_filename, params_.filename()));
      success = false;
    }
  }

  return success;
}

void ExportTask::FrameDownloaded(FramePtr f, const QByteArray &hash, const QVector<rational> &times, qint64 job_time)
{
  Q_UNUSED(job_time)
  Q_UNUSED(hash)

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
    encoder_->WriteFrame(time_map_.take(real_time), real_time);

    frame_time_++;
  }
}

void ExportTask::AudioDownloaded(const TimeRange &range, SampleBufferPtr samples, qint64 job_time)
{
  Q_UNUSED(job_time)

  TimeRange adjusted_range = range;

  if (params_.has_custom_range()) {
    adjusted_range -= params_.custom_range().in();
  }

  audio_data_.WritePCM(adjusted_range, samples, QDateTime::currentMSecsSinceEpoch());
}

OLIVE_NAMESPACE_EXIT
