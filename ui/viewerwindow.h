#ifndef VIEWERWINDOW_H
#define VIEWERWINDOW_H

#include <QOpenGLWidget>
#include <QTimer>

class QMutex;
class QMenu;
class QShortcut;

class ViewerWindow : public QOpenGLWidget {
	Q_OBJECT
public:
    ViewerWindow(QWidget *parent);
    void set_texture(GLuint t, double iar, QMutex *imutex);
protected:
	virtual void keyPressEvent(QKeyEvent*) override;
	virtual void mousePressEvent(QMouseEvent*) override;
    virtual void mouseMoveEvent(QMouseEvent*) override;

    virtual void paintGL() override;
private:
	GLuint texture;
	double ar;
    QMutex* mutex;

	// exit full screen message
	QTimer fullscreen_msg_timer;
	bool show_fullscreen_msg;
    QRect fullscreen_msg_rect;
private slots:
	void fullscreen_msg_timeout();
};

#endif // VIEWERWINDOW_H
