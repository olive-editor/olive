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

#ifndef RENDERBACKEND_H
#define RENDERBACKEND_H

#include <QtConcurrent/QtConcurrent>

#include "dialog/rendercancel/rendercancel.h"
#include "decodercache.h"
#include "node/graph.h"
#include "node/output/viewer/viewer.h"
#include "render/backend/colorprocessorcache.h"
#include "renderticket.h"
#include "renderworker.h"

OLIVE_NAMESPACE_ENTER

class RenderBackend : public QObject
{
  Q_OBJECT
public:
  RenderBackend(QObject* parent = nullptr);

  virtual ~RenderBackend() override;

  void SetViewerNode(ViewerOutput* viewer_node);

  void SetUpdateWithGraph(bool e);

  void ClearVideoQueue();

  /**
   * @brief Asynchronously generate a hash at a given time
   */
  QFuture<QList<QByteArray> > Hash(const QList<rational>& times);

  /**
   * @brief Asynchronously generate a frame at a given time
   */
  RenderTicketPtr RenderFrame(const rational& time);

  /**
   * @brief Asynchronously generate a chunk of audio
   */
  RenderTicketPtr RenderAudio(const TimeRange& r);

  void SetVideoParams(const VideoRenderingParams& params);

  void SetAudioParams(const AudioRenderingParams& params);

  void SetVideoDownloadMatrix(const QMatrix4x4& mat);

  static QList<TimeRange> SplitRangeIntoChunks(const TimeRange& r);

public slots:
  void NodeGraphChanged(NodeInput *source);

protected:
  virtual RenderWorker* CreateNewWorker() = 0;

  void Close();

private:
  void CopyNodeInputValue(NodeInput* input);
  Node *CopyNodeConnections(Node *src_node);
  void CopyNodeMakeConnection(NodeInput *src_input, NodeInput *dst_input);

  void RunNextJob();

  void ProcessUpdateQueue();

  ViewerOutput* viewer_node_;

  // VIDEO MEMBERS
  VideoRenderingParams video_params_;
  QMatrix4x4 video_download_matrix_;

  // AUDIO MEMBERS
  AudioRenderingParams audio_params_;

  QList<NodeInput*> graph_update_queue_;
  QHash<Node*, Node*> copy_map_;
  ViewerOutput* copied_viewer_node_;

  QThreadPool pool_;

  QLinkedList<RenderTicketPtr> render_queue_;

  struct WorkerData {
    RenderWorker* worker;
    bool busy;
  };

  QVector<WorkerData> workers_;

  bool update_with_graph_;

private slots:
  void WorkerFinished();

};

OLIVE_NAMESPACE_EXIT

#endif // RENDERBACKEND_H
