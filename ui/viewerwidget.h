#ifndef VIEWERWIDGET_H
#define VIEWERWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QMatrix4x4>
#include <QOpenGLTexture>
#include <QTimer>
struct Clip;

class Viewer;

class ViewerWidget : public QOpenGLWidget, public QOpenGLFunctions
{
	Q_OBJECT
public:
	ViewerWidget(QWidget *parent = 0);

    bool multithreaded;
    bool force_audio;
    bool enable_paint;
protected:
    void paintEvent(QPaintEvent *e);
    void initializeGL();
//    void resizeGL(int w, int h);
    void paintGL();
private:
	QTimer retry_timer;
    QVector<Clip*> current_clips;
private slots:
	void retry();
};

#endif // VIEWERWIDGET_H
