#ifndef VIEWERWINDOW_H
#define VIEWERWINDOW_H

#include <QOpenGLWindow>

class ViewerWindow : public QOpenGLWindow {
	Q_OBJECT
public:
	ViewerWindow(QOpenGLContext* share);
	void set_texture(GLuint t, double iar);
private:
	void paintGL();
	GLuint texture;
	double ar;
};

#endif // VIEWERWINDOW_H
