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

  void Close();

  void SetViewerNode(ViewerOutput* viewer_node);

  void SetUpdateWithGraph(bool e)
  {
    update_with_graph_ = e;
  }

  void SetRenderMode(RenderMode::Mode e)
  {
    render_mode_ = e;
  }

  void EnablePreviewGeneration(qint64 job_time)
  {
    preview_job_time_ = job_time;
  }

  void ClearVideoQueue();

  void ProcessUpdateQueue();

  static QByteArray HashNode(const Node* n, const VideoParams& params, const rational& time);

  /**
   * @brief Asynchronously generate a hash at a given time
   */
  QFuture<QVector<QByteArray> > Hash(const QVector<rational> &times);

  /**
   * @brief Asynchronously generate a frame at a given time
   */
  RenderTicketPtr RenderFrame(const rational& time);

  /**
   * @brief Asynchronously generate a chunk of audio
   */
  RenderTicketPtr RenderAudio(const TimeRange& r);

  void SetVideoParams(const VideoParams& params);

  void SetAudioParams(const AudioParams& params);

  void SetVideoDownloadMatrix(const QMatrix4x4& mat);

  static std::list<TimeRange> SplitRangeIntoChunks(const TimeRange& r);

public slots:
  void NodeGraphChanged(NodeInput *source);

protected:
  virtual RenderWorker* CreateNewWorker() = 0;

private:
  void CopyNodeInputValue(NodeInput* input);
  Node *CopyNodeConnections(Node *src_node);
  void CopyNodeMakeConnection(NodeInput *src_input, NodeInput *dst_input);

  void RunNextJob();

  ViewerOutput* viewer_node_;

  // VIDEO MEMBERS
  VideoParams video_params_;
  QMatrix4x4 video_download_matrix_;

  // AUDIO MEMBERS
  AudioParams audio_params_;

  QList<NodeInput*> graph_update_queue_;
  QHash<Node*, Node*> copy_map_;
  ViewerOutput* copied_viewer_node_;

  QThreadPool pool_;

  std::list<RenderTicketPtr> render_queue_;

  struct WorkerData {
    RenderWorker* worker;
    bool busy;
  };

  QVector<WorkerData> workers_;

  bool update_with_graph_;

  qint64 preview_job_time_;

  RenderMode::Mode render_mode_;

private slots:
  void WorkerFinished();

};

OLIVE_NAMESPACE_EXIT

#endif // RENDERBACKEND_H
