#ifndef SHADERGENERATORS_H
#define SHADERGENERATORS_H

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE::v1;

#include "shaderptr.h"

/**
 *
 * Olive standardizes on OpenGL 3.2 Core which has no fixed pipeline. Instead, the pipeline is provided by the
 * programmer in the form of a shader. This is a collection of OpenGL shader pipeline generators for use throughout
 * Olive.
 *
 */

namespace olive {
namespace gl {

ShaderPtr GetDefaultPipeline(const QString &function_name = QString(), const QString &shader_code = QString());

ShaderPtr GetOCIOPipeline(QOpenGLContext *ctx,
                          GLuint &lut_texture,
                          OCIO::ConstProcessorRcPtr processor,
                          bool alpha_is_associated);

QString GetAlphaDisassociateFunction(const QString& function_name);
QString GetAlphaReassociateFunction(const QString& function_name);
QString GetAlphaAssociateFunction(const QString& function_name);

}
}

#endif // SHADERGENERATORS_H
