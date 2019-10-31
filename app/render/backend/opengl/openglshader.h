#ifndef OPENGLSHADER_H
#define OPENGLSHADER_H

#include <memory>
#include <QOpenGLShaderProgram>

class OpenGLShader;
using OpenGLShaderPtr = std::shared_ptr<OpenGLShader>;

class OpenGLShader : public QOpenGLShaderProgram {
public:
  OpenGLShader();

  static OpenGLShaderPtr CreateDefault(const QString &function_name = QString(),
                                       const QString &shader_code = QString());

  static QString CodeDefaultFragment(const QString &function_name = QString(),
                                     const QString &shader_code = QString());
  static QString CodeDefaultVertex();
  static QString CodeAlphaDisassociate(const QString& function_name);
  static QString CodeAlphaReassociate(const QString& function_name);
  static QString CodeAlphaAssociate(const QString& function_name);

private:

};

#endif // OPENGLSHADER_H
