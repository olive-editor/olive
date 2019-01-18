#include "viewerwindow.h"

#include <QMutex>
#include <QKeyEvent>
#include <QPainter>

ViewerWindow::ViewerWindow(QOpenGLContext *share) :
	QOpenGLWindow(share),
	texture(0),
	mutex(nullptr),
	show_fullscreen_msg(false)
{
	fullscreen_msg_timer.setInterval(2000);
	connect(&fullscreen_msg_timer, SIGNAL(timeout()), this, SLOT(fullscreen_msg_timeout()));
}

void ViewerWindow::set_texture(GLuint t, double iar, QMutex* imutex) {
	texture = t;
	ar = iar;
	mutex = imutex;
	update();
}

void ViewerWindow::keyPressEvent(QKeyEvent *e) {
	if (e->key() == Qt::Key_Escape) {
		hide();
	}
}

void ViewerWindow::mousePressEvent(QMouseEvent *e) {
	if (show_fullscreen_msg && fullscreen_msg_rect.contains(e->pos())) {
		hide();
	}
}

void ViewerWindow::mouseMoveEvent(QMouseEvent *) {
	fullscreen_msg_timer.start();
	if (!show_fullscreen_msg) {
		show_fullscreen_msg = true;
		update();
	}
}

void ViewerWindow::paintGL() {
	if (texture > 0) {
		if (mutex != nullptr) mutex->lock();

		glClearColor(0.0, 0.0, 0.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);

		glEnable(GL_TEXTURE_2D);

		glBindTexture(GL_TEXTURE_2D, texture);

		glLoadIdentity();
		glOrtho(0, 1, 0, 1, -1, 1);

		glBegin(GL_QUADS);

		double top = 0;
		double left = 0;
		double right = 1;
		double bottom = 1;

		double widget_ar = double(width()) / double(height());

		if (widget_ar > ar) {
			double width = 1.0 * ar / widget_ar;
			left = (1.0 - width)*0.5;
			right = left + width;
		} else {
			double height = 1.0 / ar * widget_ar;
			top = (1.0 - height)*0.5;
			bottom = top + height;
		}

		glVertex2d(left, top);
		glTexCoord2d(0, 0);
		glVertex2d(left, bottom);
		glTexCoord2d(1, 0);
		glVertex2d(right, bottom);
		glTexCoord2d(1, 1);
		glVertex2d(right, top);
		glTexCoord2d(0, 1);

		glEnd();

		glBindTexture(GL_TEXTURE_2D, 0);

		glDisable(GL_TEXTURE_2D);

		if (mutex != nullptr) mutex->unlock();
	}

	if (show_fullscreen_msg) {
		QPainter p(this);

		QFont f = p.font();
		f.setPointSize(24);
		p.setFont(f);

		QFontMetrics fm(f);

		QString fs_str = tr("Exit Fullscreen");

		p.setPen(Qt::white);
		p.setBrush(QColor(0, 0, 0, 128));

		int text_width = fm.width(fs_str);
		int text_x = (width()/2)-(text_width/2);
		int text_y = fm.height()+fm.ascent();

		int rect_padding = 8;

		fullscreen_msg_rect = QRect(text_x-rect_padding,
									fm.height()-rect_padding,
									text_width+rect_padding+rect_padding,
									fm.height()+rect_padding+rect_padding);

		p.drawRect(fullscreen_msg_rect);

		p.drawText(text_x, text_y, fs_str);
	}
}

void ViewerWindow::fullscreen_msg_timeout() {
	fullscreen_msg_timer.stop();
	if (show_fullscreen_msg) {
		show_fullscreen_msg = false;
		update();
	}
}
