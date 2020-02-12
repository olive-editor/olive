#ifndef OSLSHADERMAP_H
#define OSLSHADERMAP_H

#include <OSL/oslexec.h>
#include <QString>

#include "../rendercache.h"

using OSLShaderCache = ThreadSafeRenderCache<QString, OSL::ShaderGroupRef>;

#endif // OSLSHADERMAP_H
