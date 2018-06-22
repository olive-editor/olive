#include "viewerwidget.h"

#include "panels/panels.h"
#include "panels/viewer.h"
#include "panels/timeline.h"
#include "project/sequence.h"
#include "project/effect.h"
#include "playback/playback.h"
#include "playback/audio.h"
#include "io/media.h"

#include <QDebug>

extern "C" {
	#include <libavformat/avformat.h>
}

ViewerWidget::ViewerWidget(QWidget *parent) : QOpenGLWidget(parent)
{	
    multithreaded = true;
    enable_paint = true;
    force_audio = false;

	QSurfaceFormat format;
	format.setDepthBufferSize(24);
	setFormat(format);

	// error handler - retries after 200ms if we couldn't get the entire image
	retry_timer.setInterval(200);
	connect(&retry_timer, SIGNAL(timeout()), this, SLOT(retry()));
}

void ViewerWidget::retry() {
	update();
}

void ViewerWidget::initializeGL() {
    initializeOpenGLFunctions();

    glClearColor(0, 0, 0, 1);
    glMatrixMode(GL_PROJECTION);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

//void ViewerWidget::resizeGL(int w, int h)
//{
//}

void ViewerWidget::paintEvent(QPaintEvent *e) {
    if (enable_paint) QOpenGLWidget::paintEvent(e);
}

void ViewerWidget::paintGL()
{
    if (multithreaded) retry_timer.stop();

    glClear(GL_COLOR_BUFFER_BIT);

    long playhead = panel_timeline->playhead;

    handle_media(sequence, playhead, multithreaded);
    texture_failed = false;

    bool render_audio = (panel_timeline->playing || force_audio);

    cc_lock.lock();

    for (int i=0;i<current_clips.size();i++) {
        Clip* c = current_clips.at(i);

        if (!c->open) {
            qDebug() << "[WARNING] Tried to display clip" << i << "but it's closed";
            texture_failed = true;
        } else if (is_clip_active(c, playhead)) {
            if (c->stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                // start preparing cache
                get_clip_frame(c, playhead);

                if (c->texture == NULL) {
                    qDebug() << "[WARNING] Texture hasn't been created yet";
                    texture_failed = true;
                } else if (playhead >= c->timeline_in) {
                    glLoadIdentity();
                    int half_width = c->sequence->width/2;
                    int half_height = c->sequence->height/2;
                    glOrtho(-half_width, half_width, half_height,- half_height, -1, 1);
                    int anchor_x = c->media_stream->video_width/2;
                    int anchor_y = c->media_stream->video_height/2;

                    // perform all transform effects
                    for (int j=0;j<c->effects.size();j++) {
                        c->effects.at(j)->process_gl(&anchor_x, &anchor_y);
                    }

                    int anchor_right = c->media_stream->video_width - anchor_x;
                    int anchor_bottom = c->media_stream->video_height - anchor_y;

                    c->texture->bind();

                    glBegin(GL_QUADS);
                        glTexCoord2f(0.0, 0.0);
                        glVertex2f(-anchor_x, -anchor_y);
                        glTexCoord2f(1.0, 0.0);
                        glVertex2f(anchor_right, -anchor_y);
                        glTexCoord2f(1.0, 1.0);
                        glVertex2f(anchor_right, anchor_bottom);
                        glTexCoord2f(0.0, 1.0);
                        glVertex2f(-anchor_x, anchor_bottom);
                    glEnd();

                    c->texture->release();
                }
            } else if (render_audio &&
                       c->stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO &&
                       c->lock.tryLock()) {
                // clip is not caching, start caching audio
                c->lock.unlock();
                cache_clip(c, playhead, false, false, c->reset_audio);
            }
        }
    }

    if (panel_timeline->playing) {
        int adjusted_read_index = audio_ibuffer_read%audio_ibuffer_size;
        int max_write = audio_ibuffer_size - adjusted_read_index;
        int actual_write = audio_io_device->write((const char*) audio_ibuffer+adjusted_read_index, max_write);
        memset(audio_ibuffer+adjusted_read_index, 0, actual_write);
        audio_ibuffer_read += actual_write;
        if (actual_write == max_write) {
            // got all the bytes, write again
            actual_write = audio_io_device->write((const char*) audio_ibuffer, audio_ibuffer_size);
            memset(audio_ibuffer, 0, actual_write);
            audio_ibuffer_read += actual_write;
        }
    }

    cc_lock.unlock();

    if (texture_failed) {
        if (multithreaded) {
            retry_timer.start();
        } else {
            paintGL();
        }
    }
}
