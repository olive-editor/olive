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
  OpenGLShaderCache() = default;

  void Clear();

  void AddShader(Node* output, OpenGLShaderPtr shader);

  OpenGLShaderPtr GetShader(Node* output);

  bool HasShader(Node* output);

private:
  QString GenerateShaderID(Node *output);

  QMap<QString, OpenGLShaderPtr> compiled_nodes_;

};

#endif // OPENGLSHADERCACHE_H
