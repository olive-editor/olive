#ifndef VIEWERWINDOW_H
#define VIEWERWINDOW_H

#include <QOpenGLWindow>

class QMutex;

class ViewerWindow : public QOpenGLWindow {
	Q_OBJECT
public:
	ViewerWindow(QOpenGLContext* share);
	void set_texture(GLuint t, double iar, QMutex *imutex);
private:
	void paintGL();
	GLuint texture;
	double ar;
	QMutex* mutex;
};

#endif // VIEWERWINDOW_H
