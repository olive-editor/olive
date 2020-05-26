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

#ifndef CACHETASK_H
#define CACHETASK_H

#include <QtConcurrent/QtConcurrent>

#include "task/render/render.h"

OLIVE_NAMESPACE_ENTER

class CacheTask : public RenderTask
{
  Q_OBJECT
public:
  CacheTask(ViewerOutput* viewer,
            const VideoRenderingParams &vparams,
            const AudioRenderingParams &aparams,
            bool in_out_only);

public slots:
  virtual bool Run() override;

protected:
  virtual QFuture<void> DownloadFrame(FramePtr frame, const QByteArray &hash) override;

  virtual void FrameDownloaded(const QByteArray& hash, const std::list<rational>& times) override;

  virtual void AudioDownloaded(const TimeRange& range, SampleBufferPtr samples) override;

private:
  bool in_out_only_;

  QThreadPool download_threads_;

};

OLIVE_NAMESPACE_EXIT

#endif // CACHETASK_H
