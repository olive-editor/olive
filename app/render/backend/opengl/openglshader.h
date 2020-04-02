#ifndef OPENGLSHADER_H
#define OPENGLSHADER_H

#include <memory>
#include <QOpenGLShaderProgram>

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE::v1;

class OpenGLShader;
using OpenGLShaderPtr = std::shared_ptr<OpenGLShader>;

/**
 * @brief A simple QOpenGLShaderProgram derivative with static functions for creating
 */
class OpenGLShader : public QOpenGLShaderProgram {
public:
  OpenGLShader();

  static OpenGLShaderPtr Create();

  static OpenGLShaderPtr CreateDefault(const QString &function_name = QString(),
                                       const QString &shader_code = QString());

  static OpenGLShaderPtr CreateOCIO(QOpenGLContext* ctx,
                                    GLuint& lut_texture,
                                    OCIO::ConstProcessorRcPtr processor,
                                    bool alpha_is_associated);

  static QString CodeDefaultFragment(const QString &function_name = QString(),
                                     const QString &shader_code = QString());
  static QString CodeDefaultVertex();
  static QString CodeAlphaDisassociate(const QString& function_name);
  static QString CodeAlphaReassociate(const QString& function_name);
  static QString CodeAlphaAssociate(const QString& function_name);

};

#endif // OPENGLSHADER_H
