#ifndef OPENGLSHADERCACHE_H
#define OPENGLSHADERCACHE_H

#include <QString>

#include "openglshader.h"
#include "render/backend/rendercache.h"

using OpenGLShaderCache = RenderCache<QString, OpenGLShaderPtr>;

#endif // OPENGLSHADERCACHE_H
