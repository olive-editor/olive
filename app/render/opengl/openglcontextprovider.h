#ifndef OPENGLCONTEXTPROVIDER_H
#define OPENGLCONTEXTPROVIDER_H

class QOpenGLContext;

namespace olive
{

class OpenGLContextProvider {
public:
	virtual ~OpenGLContextProvider() = default;
	virtual QOpenGLContext *OpenGLContext() const = 0;
};

}

#endif // OPENGLCONTEXTPROVIDER_H
