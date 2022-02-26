/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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
#include "node/color/colormanager/colormanager.h"

namespace olive {

ExportTask::ExportTask(ViewerOutput *viewer_node,
                       ColorManager* color_manager,
                       const ExportParams& params) :
  color_manager_(color_manager),
  params_(params)
{
  set_viewer(viewer_node);

  // Adjust video params to have no divider
  VideoParams vp = viewer_node->GetVideoParams();
  vp.set_divider(1);
  vp.set_time_base(params.video_params().time_base());
  vp.set_frame_rate(params.video_params().frame_rate());
  set_video_params(vp);

  set_audio_params(viewer_node->GetAudioParams());

  SetTitle(tr("Exporting \"%1\"").arg(viewer_node->GetLabel()));
  SetNativeProgressSignallingEnabled(false);
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
    SetError(tr("Failed to open file: %1").arg(encoder_->GetError()));
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

  frame_time_ = 0;

  QSize video_force_size;
  QMatrix4x4 video_force_matrix;

  if (params_.video_enabled()) {

    // If a transformation matrix is applied to this video, create it here
    if (video_params().width() != params_.video_params().width()
        || video_params().height() != params_.video_params().height()) {
      video_force_size = QSize(params_.video_params().width(), params_.video_params().height());

      if (params_.video_scaling_method() != ExportParams::kStretch) {
        video_force_matrix = ExportParams::GenerateMatrix(params_.video_scaling_method(),
                                                          video_params().width(),
                                                          video_params().height(),
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

  // Start render process
  TimeRangeList video_range, audio_range;
  TimeRange subtitle_range;

  if (params_.video_enabled()) {
    video_range = {range};
  }

  if (params_.audio_enabled()) {
    audio_range = {range};
  }

  if (params_.subtitles_enabled()) {
    subtitle_range = range;
  }

  Render(color_manager_, video_range, audio_range, subtitle_range, RenderMode::kOnline, nullptr,
         video_force_size, video_force_matrix, encoder_->GetDesiredPixelFormat(),
         color_processor_);

  bool success = true;

  encoder_->Close();

  if (!encoder_->GetError().isEmpty()) {
    SetError(encoder_->GetError());
    success = false;
  }

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

bool ExportTask::FrameDownloaded(FramePtr f, const QByteArray &hash, const QVector<rational> &times)
{
  Q_UNUSED(hash)

  foreach (const rational& t, times) {
    rational actual_time = t;

    if (params_.has_custom_range()) {
      actual_time -= params_.custom_range().in();
    }

    time_map_.insert(actual_time, f);
  }

  while (!IsCancelled()) {
    rational real_time = Timecode::timestamp_to_time(frame_time_,
                                                     video_params().frame_rate_as_time_base());

    if (!time_map_.contains(real_time)) {
      break;
    }

    // Unfortunately this can't be done in another thread since the frames need to be sent
    // one after the other chronologically.
    if (!encoder_->WriteFrame(time_map_.take(real_time), real_time)) {
      SetError(encoder_->GetError());
      return false;
    }

    frame_time_++;
    emit ProgressChanged(double(frame_time_) / double(GetTotalNumberOfFrames()));
  }

  return true;
}

bool ExportTask::AudioDownloaded(const TimeRange &range, SampleBufferPtr samples)
{
  TimeRange adjusted_range = range;

  if (params_.has_custom_range()) {
    adjusted_range -= params_.custom_range().in();
  }

  if (adjusted_range.in() == audio_time_) {
    if (!WriteAudioLoop(adjusted_range, samples)) {
      return false;
    }
  } else {
    audio_map_.insert(adjusted_range, samples);
  }

  return true;
}

bool ExportTask::EncodeSubtitle(const SubtitleBlock *sub)
{
  if (!encoder_->WriteSubtitle(sub)) {
    SetError(encoder_->GetError());
    return false;
  } else {
    return true;
  }
}

bool ExportTask::WriteAudioLoop(const TimeRange& time, SampleBufferPtr samples)
{
  if (!encoder_->WriteAudio(samples)) {
    SetError(encoder_->GetError());
    return false;
  }

  audio_time_ = time.out();

  for (auto it=audio_map_.begin(); it!=audio_map_.end(); it++) {
    TimeRange t = it.key();
    SampleBufferPtr s = it.value();

    if (t.in() == audio_time_) {
      // Erase from audio map since we're just about to write it
      audio_map_.erase(it);

      // Call recursively to write the next sample buffer
      if (!WriteAudioLoop(t, s)) {
        return false;
      }

      // Break out of loop
      break;
    }
  }

  return true;
}

}
