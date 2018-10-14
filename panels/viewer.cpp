#include "viewer.h"
#include "ui_viewer.h"

#include "playback/audio.h"
#include "timeline.h"
#include "project/sequence.h"
#include "panels/panels.h"
#include "io/config.h"
#include "io/media.h"
#include "ui/audiomonitor.h"
#include "ui_timeline.h"

#define FRAMES_IN_ONE_MINUTE 1798 // 1800 - 2
#define FRAMES_IN_TEN_MINUTES 17978 // (FRAMES_IN_ONE_MINUTE * 10) - 2

QString panel_name = "Viewer: ";

extern "C" {
	#include <libavcodec/avcodec.h>
}

#include <QtMath>
#include <QAudioOutput>

Viewer::Viewer(QWidget *parent) :
	QDockWidget(parent),
    ui(new Ui::Viewer),
	seq(NULL),
	queue_audio_reset(false),
	playing(false)
{
	ui->setupUi(this);
	ui->headers->viewer = this;
	ui->headers->show_text(false);
	ui->glViewerPane->child = ui->openGLWidget;
    viewer_widget = ui->openGLWidget;
    set_media(MEDIA_TYPE_SEQUENCE, NULL);

    ui->currentTimecode->setEnabled(false);
	ui->currentTimecode->set_default_value(qSNaN());
	ui->currentTimecode->set_value(0, false);
	ui->currentTimecode->set_display_type(LABELSLIDER_FRAMENUMBER);
	connect(ui->currentTimecode, SIGNAL(valueChanged()), this, SLOT(update_playhead()));

	connect(&playback_updater, SIGNAL(timeout()), this, SLOT(timer_update()));

    update_playhead_timecode(0);
    update_end_timecode();
}

Viewer::~Viewer() {
    delete ui;
}

void Viewer::set_main_sequence() {
    set_sequence(true, sequence);
}

void Viewer::reset_all_audio() {
	// reset all clip audio
	if (seq != NULL) {
		audio_ibuffer_frame = seq->playhead;
		audio_ibuffer_timecode = (double) audio_ibuffer_frame / seq->frame_rate;

		for (int i=0;i<seq->clips.size();i++) {
			Clip* c = seq->clips.at(i);
			if (c != NULL) {
				c->reset_audio();
			}
		}
	}
	// panel_timeline->ui->audio_monitor->reset();
	clear_audio_ibuffer();
}

QString frame_to_timecode(long f, int view, double frame_rate) {
    if (view == TIMECODE_FRAMES) {
        return QString::number(f);
    }

    // return timecode
    int hours = 0;
    int mins = 0;
    int secs = 0;
    int frames = 0;
    QString token = ":";

    if (view == TIMECODE_DROP && frame_rate_is_droppable(frame_rate)) {
        //CONVERT A FRAME NUMBER TO DROP FRAME TIMECODE
        //Code by David Heidelberger, adapted from Andrew Duncan, further adapted for Olive by Olive Team
        //Given an int called framenumber and a double called framerate
        //Framerate should be 29.97, 59.94, or 23.976, otherwise the calculations will be off.

        int d;
        int m;

        int dropFrames = round(frame_rate * .066666); //Number of frames to drop on the minute marks is the nearest integer to 6% of the framerate
        int framesPerHour = round(frame_rate*60*60); //Number of frames in an hour
        int framesPer24Hours = framesPerHour*24; //Number of frames in a day - timecode rolls over after 24 hours
        int framesPer10Minutes = round(frame_rate * 60 * 10); //Number of frames per ten minutes
        int framesPerMinute = (round(frame_rate)*60)-  dropFrames; //Number of frames per minute is the round of the framerate * 60 minus the number of dropped frames

        //If framenumber is greater than 24 hrs, next operation will rollover clock
        f = f % framesPer24Hours; //% is the modulus operator, which returns a remainder. a % b = the remainder of a/b

        d = f / framesPer10Minutes; // \ means integer division, which is a/b without a remainder. Some languages you could use floor(a/b)
        m = f % framesPer10Minutes;

        //In the original post, the next line read m>1, which only worked for 29.97. Jean-Baptiste Mardelle correctly pointed out that m should be compared to dropFrames.
        if (m > dropFrames) {
            f = f + (dropFrames*9*d) + dropFrames * ((m - dropFrames) / framesPerMinute);
        } else {
            f = f + dropFrames*9*d;
        }

        int frRound = round(frame_rate);
        frames = f % frRound;
        secs = (f / frRound) % 60;
        mins = ((f / frRound) / 60) % 60;
        hours = (((f / frRound) / 60) / 60);

        token = ";";
    } else {
		// non-drop timecode

        int int_fps = qRound(frame_rate);
        hours = f/ (3600 * int_fps);
        mins = f / (60*int_fps) % 60;
        secs = f/int_fps % 60;
        frames = f%int_fps;
    }
    return QString(QString::number(hours).rightJustified(2, '0') +
                   ":" + QString::number(mins).rightJustified(2, '0') +
                   ":" + QString::number(secs).rightJustified(2, '0') +
                   token + QString::number(frames).rightJustified(2, '0')
                );
}

bool frame_rate_is_droppable(float rate) {
    return (rate == 23.976f || rate == 29.97f || rate == 59.94f);
}

void Viewer::seek(long p) {
	pause();
	seq->playhead = p;
	queue_audio_reset = true;
	panel_timeline->repaint_timeline(false);
}

void Viewer::go_to_start() {
	seek(0);
}

void Viewer::previous_frame() {
	if (seq->playhead > 0) seek(seq->playhead-1);
}

void Viewer::next_frame() {
	seek(seq->playhead+1);
}

void Viewer::toggle_play() {
	if (playing) {
		pause();
	} else {
		play();
	}
}

void Viewer::play() {
	if (queue_audio_reset) {
		reset_all_audio();
		queue_audio_reset = false;
	}
	playhead_start = seq->playhead;
	start_msecs = QDateTime::currentMSecsSinceEpoch();
	playback_updater.start();
	playing = true;
	panel_sequence_viewer->set_playpause_icon(false);
	audio_thread->notifyReceiver();
}

void Viewer::pause() {
	playing = false;
	panel_sequence_viewer->set_playpause_icon(true);
	playback_updater.stop();
}

void Viewer::go_to_end() {
	seek(seq->getEndFrame());
}

void Viewer::update_playhead_timecode(long p) {
	ui->currentTimecode->set_value(p, false);
}

void Viewer::update_end_timecode() {
    ui->endTimecode->setText((seq == NULL) ? frame_to_timecode(0, config.timecode_view, 30) : frame_to_timecode(seq->getEndFrame(), config.timecode_view, seq->frame_rate));
}

void Viewer::set_media(int type, void* media) {
	switch (type) {
	case MEDIA_TYPE_FOOTAGE:
	{

	}
		break;
	case MEDIA_TYPE_SEQUENCE:
        set_sequence(false, static_cast<Sequence*>(media));
		break;
	}
}

void Viewer::on_pushButton_clicked() {
	go_to_start();
}

void Viewer::on_pushButton_5_clicked() {
	go_to_end();
}

void Viewer::on_pushButton_2_clicked() {
	previous_frame();
}

void Viewer::on_pushButton_4_clicked() {
	next_frame();
}

void Viewer::on_pushButton_3_clicked() {
	toggle_play();
}

void Viewer::update_playhead() {
	seek(ui->currentTimecode->value());
}

void Viewer::timer_update() {
	seq->playhead = round(playhead_start + ((QDateTime::currentMSecsSinceEpoch()-start_msecs) * 0.001 * seq->frame_rate));
	if (main_sequence) panel_timeline->repaint_timeline(false);
}

void Viewer::set_sequence(bool main, Sequence *s) {
	reset_all_audio();

    main_sequence = main;
    seq = (main) ? sequence : s;
    viewer_widget->display_sequence = seq;

    bool null_sequence = (seq == NULL);

    ui->headers->setEnabled(!null_sequence);
    ui->currentTimecode->setEnabled(!null_sequence);
    ui->openGLWidget->setEnabled(!null_sequence);
    ui->openGLWidget->setVisible(!null_sequence);
    ui->pushButton->setEnabled(!null_sequence);
    ui->pushButton_2->setEnabled(!null_sequence);
    ui->pushButton_3->setEnabled(!null_sequence);
    ui->pushButton_4->setEnabled(!null_sequence);
    ui->pushButton_5->setEnabled(!null_sequence);

    init_audio();

    if (!null_sequence) {
		playback_updater.setInterval(qFloor(1000 / seq->frame_rate));

        config.timecode_view = (frame_rate_is_droppable(seq->frame_rate)) ? TIMECODE_DROP : TIMECODE_NONDROP;

        update_playhead_timecode(seq->playhead);
        update_end_timecode();

        ui->glViewerPane->aspect_ratio = (float) seq->width / (float) seq->height;
        ui->glViewerPane->adjust();

        setWindowTitle(panel_name + seq->name);
    } else {
        update_playhead_timecode(0);
        update_end_timecode();

        setWindowTitle(panel_name + "(none)");
    }

    viewer_widget->update();

    update();
}

void Viewer::set_playpause_icon(bool play) {
	ui->pushButton_3->setIcon(QIcon((play) ? ":/icons/play.png" : ":/icons/pause.png"));
}
