#include "openglshadercache.h"

#include "node/node.h"

void OpenGLShaderCache::Clear()
{
  compiled_nodes_.clear();
}

void OpenGLShaderCache::AddShader(Node *output, OpenGLShaderPtr shader)
{
  compiled_nodes_.insert(GenerateShaderID(output), shader);
}

OpenGLShaderPtr OpenGLShaderCache::GetShader(Node *output)
{
  return compiled_nodes_.value(GenerateShaderID(output));
}

QString OpenGLShaderCache::GenerateShaderID(Node *output)
{
  return output->id();
}

bool OpenGLShaderCache::HasShader(Node *output)
{
  return compiled_nodes_.contains(GenerateShaderID(output));
}
