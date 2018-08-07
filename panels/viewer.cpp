#include "viewer.h"
#include "ui_viewer.h"

#include "playback/audio.h"
#include "timeline.h"
#include "project/sequence.h"
#include "panels/panels.h"

#define FRAMES_IN_ONE_MINUTE 1798 // 1800 - 2
#define FRAMES_IN_TEN_MINUTES 17978 // (FRAMES_IN_ONE_MINUTE * 10) - 2

extern "C" {
	#include <libavcodec/avcodec.h>
}

#include <QAudioOutput>

Viewer::Viewer(QWidget *parent) :
	QDockWidget(parent),
	ui(new Ui::Viewer)
{
	ui->setupUi(this);
	ui->glViewerPane->child = ui->openGLWidget;
    viewer_widget = ui->openGLWidget;
    timecode_view = TIMECODE_DROP;
    update_sequence();

    update_playhead_timecode(0);
    update_end_timecode();
}

Viewer::~Viewer()
{
    init_audio();
	delete ui;
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

void Viewer::update_playhead_timecode(long p) {
    ui->currentTimecode->setText(frame_to_timecode(p, timecode_view, (sequence != NULL) ? sequence->frame_rate : 30));
}

void Viewer::update_end_timecode() {
    if (sequence == NULL) {
        ui->endTimecode->setText(frame_to_timecode(0, timecode_view, 30));
    } else {
        ui->endTimecode->setText(frame_to_timecode(sequence->getEndFrame(), timecode_view, sequence->frame_rate));
    }
}

void Viewer::update_sequence() {
    bool null_sequence = (sequence == NULL);

    ui->openGLWidget->setEnabled(!null_sequence);
    ui->openGLWidget->setVisible(!null_sequence);
    ui->pushButton->setEnabled(!null_sequence);
    ui->pushButton_2->setEnabled(!null_sequence);
    ui->pushButton_3->setEnabled(!null_sequence);
    ui->pushButton_4->setEnabled(!null_sequence);
    ui->pushButton_5->setEnabled(!null_sequence);

    init_audio();

    if (!null_sequence) {
        if (frame_rate_is_droppable(sequence->frame_rate)) {
            timecode_view = TIMECODE_DROP;
        } else {
            timecode_view = TIMECODE_NONDROP;
        }

        update_playhead_timecode(panel_timeline->playhead);
        update_end_timecode();

        ui->glViewerPane->aspect_ratio = (float) sequence->width / (float) sequence->height;
        ui->glViewerPane->adjust();
    } else {
        update_playhead_timecode(0);
        update_end_timecode();
    }

	viewer_widget->repaint();

    update();
}

void Viewer::on_pushButton_clicked()
{
	panel_timeline->go_to_start();
}

void Viewer::on_pushButton_5_clicked()
{
	panel_timeline->go_to_end();
}

void Viewer::on_pushButton_2_clicked()
{
	panel_timeline->previous_frame();
}

void Viewer::on_pushButton_4_clicked()
{
	panel_timeline->next_frame();
}

void Viewer::on_pushButton_3_clicked()
{
    panel_timeline->toggle_play();
}

void Viewer::set_playpause_icon(bool play) {
    if (play) {
        ui->pushButton_3->setIcon(QIcon(":/icons/play.png"));
    } else {
        ui->pushButton_3->setIcon(QIcon(":/icons/pause.png"));
    }
}
