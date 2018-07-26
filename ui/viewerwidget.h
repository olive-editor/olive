#ifndef VIEWERWIDGET_H
#define VIEWERWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QMatrix4x4>
#include <QOpenGLTexture>
#include <QTimer>

struct Clip;
struct Sequence;

class Viewer;

class ViewerWidget : public QOpenGLWidget, public QOpenGLFunctions
{
	Q_OBJECT
public:
    ViewerWidget(QWidget *parent = 0);

    bool multithreaded;
    bool force_audio;
    bool enable_paint;
    bool flip;
    void paintGL();
    void initializeGL();
protected:
    void paintEvent(QPaintEvent *e);
//    void resizeGL(int w, int h);
private:
	QTimer retry_timer;
    QVector<qint16> samples;
private slots:
	void retry();
    void deleteFunction();
    void compose_sequence(Sequence* s, bool render_audio);
    int send_audio_to_output(int offset, int max);
};

#endif // VIEWERWIDGET_H
