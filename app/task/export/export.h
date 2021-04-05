/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#ifndef EXPORTTASK_H
#define EXPORTTASK_H

#include "exportparams.h"
#include "node/output/viewer/viewer.h"
#include "render/colorprocessor.h"
#include "task/render/render.h"
#include "task/task.h"

namespace olive {

class ExportTask : public RenderTask
{
  Q_OBJECT
public:
  ExportTask(ViewerOutput *viewer_node, ColorManager *color_manager, const ExportParams &params);

protected:
  virtual bool Run() override;

  virtual void FrameDownloaded(FramePtr frame, const QByteArray& hash, const QVector<rational>& times, qint64 job_time) override;

  virtual void AudioDownloaded(const TimeRange& range, SampleBufferPtr samples, qint64 job_time) override;

  virtual bool TwoStepFrameRendering() const override
  {
    return false;
  }

private:
  QHash<rational, FramePtr> time_map_;

  ColorManager* color_manager_;

  ExportParams params_;

  Encoder* encoder_;

  ColorProcessorPtr color_processor_;

  int64_t frame_time_;

  AudioPlaybackCache audio_data_;

};

}

#endif // EXPORTTASK_H
