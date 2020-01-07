#ifndef OPENGLEXPORTER_H
#define OPENGLEXPORTER_H

#include "render/backend/exporter.h"
#include "render/backend/opengl/openglbackend.h"
#include "render/backend/audio/audiobackend.h"
#include "render/backend/opengl/openglframebuffer.h"
#include "render/backend/opengl/opengltexture.h"

class OpenGLExporter : public Exporter
{
public:
  OpenGLExporter(ViewerOutput* viewer,
                 Encoder* encoder,
                 QObject* parent = nullptr);

  virtual ~OpenGLExporter() override;

protected:
  virtual bool Initialize() override;

  virtual void Cleanup() override;

  virtual FramePtr TextureToFrame(const QVariant &texture) override;

private:
  OpenGLFramebuffer buffer_;

  OpenGLTexturePtr texture_;

  OpenGLShaderPtr pipeline_;

};

#endif // OPENGLEXPORTER_H
