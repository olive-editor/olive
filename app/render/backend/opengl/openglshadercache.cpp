#include "openglshadercache.h"

#include "node/node.h"

OpenGLShaderCache::OpenGLShaderCache()
{

}

QString OpenGLShaderCache::GenerateShaderID(NodeOutput *output)
{
  // Creates a unique identifier for this specific node and this specific output
  return QString("%1:%2").arg(output->parent()->id(), output->id());
}

void OpenGLShaderCache::Clear()
{
  compiled_nodes_.clear();
}

void OpenGLShaderCache::AddShader(NodeOutput *output, OpenGLShaderPtr shader)
{
  compiled_nodes_.insert(GenerateShaderID(output), shader);
}

OpenGLShaderPtr OpenGLShaderCache::GetShader(NodeOutput *output)
{
  return compiled_nodes_.value(GenerateShaderID(output));
}
