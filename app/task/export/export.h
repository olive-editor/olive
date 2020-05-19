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

#ifndef EXPORTTASK_H
#define EXPORTTASK_H

#include "exportparams.h"
#include "node/output/viewer/viewer.h"
#include "render/colorprocessor.h"
#include "task/render/render.h"
#include "task/task.h"

OLIVE_NAMESPACE_ENTER

class ExportTask : public RenderTask
{
  Q_OBJECT
public:
  ExportTask(ViewerOutput *viewer_node, ColorManager *color_manager, const ExportParams &params);

public slots:
  virtual bool Run() override;

protected:
  virtual QFuture<void> DownloadFrame(FramePtr frame, const QByteArray &hash) override;

  virtual void FrameDownloaded(const QByteArray& hash, const QLinkedList<rational>& times) override;

private:
  QHash<QByteArray, FramePtr> rendered_frame_;

  QHash<rational, FramePtr> time_map_;

  QList< QFuture<bool> > write_frame_futures_;

  ColorManager* color_manager_;

  ExportParams params_;

  Encoder* encoder_;

  ColorProcessorPtr color_processor_;

  int64_t frame_time_;

};

OLIVE_NAMESPACE_EXIT

#endif // EXPORTTASK_H
