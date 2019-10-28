#ifndef RENDERBACKEND_H
#define RENDERBACKEND_H

#include "node/output/viewer/viewer.h"

class RenderBackend : QObject
{
  Q_OBJECT
public:
  RenderBackend();
  virtual ~RenderBackend();

  virtual bool Init() = 0;

  virtual void GenerateFrame(const rational& time) = 0;

  virtual void GenerateSamples(const rational& time, const rational& length) = 0;

  virtual void Close() = 0;

  const QString& GetError();

  void set_viewer_node(ViewerOutput* viewer_node);

protected:
  virtual void Decompile() = 0;

  void SetError(const QString& error);

  ViewerOutput* viewer_node() const;

private:
  ViewerOutput* viewer_node_;

  QString error_;
};

#endif // RENDERBACKEND_H
