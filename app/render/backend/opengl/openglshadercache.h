#ifndef OPENGLSHADERCACHE_H
#define OPENGLSHADERCACHE_H

#include <QMutex>

#include "node/output.h"
#include "openglshader.h"

/**
 * @brief Thread-safe cache of OpenGL shaders
 */
class OpenGLShaderCache
{
public:
  OpenGLShaderCache();

  void Clear();

  void AddShader(NodeOutput* output, OpenGLShaderPtr shader);

  OpenGLShaderPtr GetShader(NodeOutput* output);

private:
  QString GenerateShaderID(NodeOutput* output);

  QMap<QString, OpenGLShaderPtr> compiled_nodes_;

};

#endif // OPENGLSHADERCACHE_H
