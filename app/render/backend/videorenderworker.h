#ifndef VIDEORENDERWORKER_H
#define VIDEORENDERWORKER_H

#include <QCryptographicHash>
#include <QObject>

#include "decodercache.h"
#include "node/dependency.h"
#include "render/videoparams.h"

class VideoRenderWorker : public QObject {
  Q_OBJECT
public:
  VideoRenderWorker(DecoderCache* decoder_cache, QObject* parent = nullptr);

  Q_DISABLE_COPY_MOVE(VideoRenderWorker)

  bool IsStarted();

  void SetParameters(const VideoRenderingParams& video_params);

  virtual bool Init();

  bool IsAvailable();

public slots:
  void Close();

  void Render(NodeDependency path);

  void RenderAsSibling(NodeDependency dep);

  void Download(NodeDependency dep, QVariant texture, QString filename);

signals:
  void RequestSibling(NodeDependency path);

  void CompletedFrame(NodeDependency path);

  void CompletedDownload(NodeDependency path);

protected:
  virtual bool InitInternal() = 0;

  virtual void CloseInternal() = 0;

  virtual QVariant FrameToTexture(FramePtr frame) = 0;

  const VideoRenderingParams& video_params();

  DecoderCache* decoder_cache();

  Node* ValidateBlock(Node* n, const rational& time);

  virtual void ParametersChangedEvent(){}

  virtual bool OutputIsShader(NodeOutput *output) = 0;

  virtual QVariant RunNodeAsShader(NodeOutput *output) = 0;

  virtual void TextureToBuffer(const QVariant& texture, QByteArray& buffer) = 0;

private:
  void ProcessNode();

  StreamPtr ResolveStreamFromInput(NodeInput* input);
  DecoderPtr ResolveDecoderFromInput(NodeInput* input);

  QList<NodeInput*> ProcessNodeInputsForTime(Node* n, const TimeRange& time);

  void HashNodeRecursively(QCryptographicHash* hash, Node *n, const rational &time);

  VideoRenderingParams video_params_;

  DecoderCache* decoder_cache_;

  QAtomicInt working_;

  QByteArray download_buffer_;

  bool started_;

private slots:

};

#endif // VIDEORENDERWORKER_H
