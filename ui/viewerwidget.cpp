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

    audio_bytes_written = 0;

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

	handle_media(panel_viewer->sequence, playhead, multithreaded);
	texture_failed = false;

    bool render_audio = (panel_timeline->playing || force_audio);

    // switch_cache
    if (render_audio && audio_bytes_written == audio_cache_size) {
        switch_audio_cache = true;

        reading_audio_cache_A = !reading_audio_cache_A;
        audio_bytes_written = 0;
        clear_cache(!reading_audio_cache_A, reading_audio_cache_A);
	}

//    qDebug() << audio_bytes_written << "/" << audio_cache_size;

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
					int anchor_x = 0;
					int anchor_y = 0;

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
			} else if (c->stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO &&
					   playhead >= c->timeline_in &&
					   render_audio &&
					   !c->reached_end &&
					   (switch_audio_cache || c->reset_audio)) {
                // TODO doesn't affect new clips appearing in the timeline (they'll ALWAYS end up in the other cache with a 1/8 sec delay)

				// cache audio
				if (c->lock.tryLock()) {
					// clip is not caching, start cache
                    c->lock.unlock();

					cache_clip(c, playhead, !reading_audio_cache_A, reading_audio_cache_A, c->reset_audio);
				}
			}
		}
	}

	if (render_audio) {
		switch_audio_cache = false;

		if (audio_cache_A != NULL && audio_bytes_written < audio_cache_size) {
			// send cached/buffered audio to QIODevice/QAudioOutput
			uint8_t* cache = reading_audio_cache_A ? audio_cache_A : audio_cache_B;
            if (panel_timeline->playing) {
                int max_w = audio_cache_size-audio_bytes_written;
                int w = audio_io_device->write((const char*) cache+audio_bytes_written, max_w);
                qDebug() << w << max_w;
                audio_bytes_written += w;
            }
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
