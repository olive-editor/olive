#ifndef SHADERGENERATORS_H
#define SHADERGENERATORS_H

#include "qopenglshaderprogramptr.h"
#include "framebufferobject.h"
#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE::v1;

namespace olive {
namespace shader {

QOpenGLShaderProgramPtr GetPipeline(const QString &shader_code = QString());

QOpenGLShaderProgramPtr SetupOCIO(QOpenGLContext *ctx,
                                  GLuint &lut_texture,
                                  OCIO::ConstProcessorRcPtr processor,
                                  bool alpha_is_associated);

QString GetAlphaDisassociateFunction(const QString& function_name);
QString GetAlphaReassociateFunction(const QString& function_name);
QString GetAlphaAssociateFunction(const QString& function_name);

}
}

#endif // SHADERGENERATORS_H
