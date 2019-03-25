#ifndef SHADERGENERATORS_H
#define SHADERGENERATORS_H

#include "qopenglshaderprogramptr.h"
#include "framebufferobject.h"

#ifndef NO_OCIO
#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE::v1;
#endif

namespace olive {
namespace shader {

QOpenGLShaderProgramPtr GetPipeline(const QString &shader_code = QString());

#ifndef NO_OCIO
enum AlphaAssociateMode {
  NoAssociate,
  Associate,
  DisassociateAndReassociate
};

QOpenGLShaderProgramPtr SetupOCIO(QOpenGLContext *ctx,
                                  GLuint &lut_texture,
                                  OCIO::ConstProcessorRcPtr processor,
                                  AlphaAssociateMode alpha_associate_mode);
#endif

QString GetAlphaDisassociateFunction(const QString& function_name);
QString GetAlphaReassociateFunction(const QString& function_name);
QString GetAlphaAssociateFunction(const QString& function_name);

}
}

#endif // SHADERGENERATORS_H
