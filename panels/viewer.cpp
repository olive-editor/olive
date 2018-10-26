#include "viewer.h"
#include "ui_viewer.h"

#include "playback/audio.h"
#include "timeline.h"
#include "panels/project.h"
#include "project/sequence.h"
#include "panels/panels.h"
#include "io/config.h"
#include "io/media.h"
#include "project/undo.h"
#include "ui/audiomonitor.h"
#include "ui_timeline.h"

#define FRAMES_IN_ONE_MINUTE 1798 // 1800 - 2
#define FRAMES_IN_TEN_MINUTES 17978 // (FRAMES_IN_ONE_MINUTE * 10) - 2

QString panel_name = "Viewer: ";

extern "C" {
	#include <libavformat/avformat.h>
	#include <libavcodec/avcodec.h>
}

#include <QtMath>
#include <QAudioOutput>
#include <QPainter>
#include <QStringList>
#include <QTimer>

Viewer::Viewer(QWidget *parent) :
	QDockWidget(parent),
    ui(new Ui::Viewer),
	seq(NULL),
	playing(false),
	created_sequence(false),
	cue_recording_internal(false)
{
	ui->setupUi(this);
	ui->headers->viewer = this;
	ui->headers->snapping = false;
	ui->headers->show_text(false);
	ui->glViewerPane->child = ui->openGLWidget;
	ui->openGLWidget->viewer = this;
    viewer_widget = ui->openGLWidget;
    set_media(MEDIA_TYPE_SEQUENCE, NULL);

    ui->currentTimecode->setEnabled(false);
    ui->currentTimecode->set_minimum_value(0);
	ui->currentTimecode->set_default_value(qSNaN());
	ui->currentTimecode->set_value(0, false);
	ui->currentTimecode->set_display_type(LABELSLIDER_FRAMENUMBER);
	connect(ui->currentTimecode, SIGNAL(valueChanged()), this, SLOT(update_playhead()));

	recording_flasher.setInterval(500);

	connect(&playback_updater, SIGNAL(timeout()), this, SLOT(timer_update()));
	connect(&recording_flasher, SIGNAL(timeout()), this, SLOT(recording_flasher_update()));

    update_playhead_timecode(0);
    update_end_timecode();
}

Viewer::~Viewer() {
	delete ui;
}

bool Viewer::is_focused() {
    return ui->headers->hasFocus()
            || ui->openGLWidget->hasFocus()
            || ui->pushButton->hasFocus()
            || ui->pushButton_2->hasFocus()
            || ui->pushButton_3->hasFocus()
            || ui->pushButton_4->hasFocus()
            || ui->pushButton_5->hasFocus();
}

void Viewer::set_main_sequence() {
	clean_created_seq();
    set_sequence(true, sequence);
}

void Viewer::reset_all_audio() {
	// reset all clip audio
	if (seq != NULL) {
		audio_ibuffer_frame = seq->playhead;
		audio_ibuffer_timecode = (double) audio_ibuffer_frame / seq->frame_rate;

		for (int i=0;i<seq->clips.size();i++) {
			Clip* c = seq->clips.at(i);
			if (c != NULL) c->reset_audio();
		}
	}
	clear_audio_ibuffer();
}

long timecode_to_frame(const QString& s, int view, double frame_rate) {
	QList<QString> list = s.split(QRegExp("[:;]"));

	for (int i=0;i<list.size();i++) {
		qDebug() << "list" << i << list.at(i);
	}

	if (view == TIMECODE_FRAMES || list.size() == 1) {
		return s.toLong();
	}

	int frRound = qRound(frame_rate);
	int hours = ((list.size() > 0) ? list.at(0).toInt() : 0) * frRound * 3600;
	int minutes = ((list.size() > 1) ? list.at(1).toInt() : 0) * frRound * 60;
	int seconds = ((list.size() > 2) ? list.at(2).toInt() : 0) * frRound;
	int frames = (list.size() > 3) ? list.at(3).toInt() : 0;

	int f = (frames + seconds + minutes + hours);

	if (view == TIMECODE_DROP && frame_rate_is_droppable(frame_rate)) {
		// return drop
		int d;
		int m;

		int dropFrames = round(frame_rate * .066666); //Number of frames to drop on the minute marks is the nearest integer to 6% of the framerate
		int framesPer10Minutes = round(frame_rate * 60 * 10); //Number of frames per ten minutes
		int framesPerMinute = (round(frame_rate)*60)-  dropFrames; //Number of frames per minute is the round of the framerate * 60 minus the number of dropped frames

		d = f / framesPer10Minutes;
		f -= dropFrames*9*d;

		m = f % framesPer10Minutes;

		if (m > dropFrames) {
			f -= (dropFrames * ((m - dropFrames) / framesPerMinute));
		}
	}

	// return non-drop
	return f;
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
		hours = f / (3600 * int_fps);
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
	return (qFuzzyCompare(rate, 23.976f) || qFuzzyCompare(rate, 29.97f) || qFuzzyCompare(rate, 59.94f));
}

void Viewer::seek(long p) {
	pause();
	seq->playhead = p;
	update_parents();
}

void Viewer::go_to_start() {
	if (seq != NULL) {
		if (seq->using_workarea && seq->playhead != seq->workarea_in) {
			seek(seq->workarea_in);
		} else {
			seek(0);
		}
	}
}

void Viewer::previous_frame() {
	if (seq != NULL && seq->playhead > 0) seek(seq->playhead-1);
}

void Viewer::next_frame() {
	if (seq != NULL) seek(seq->playhead+1);
}

void Viewer::go_to_end() {
	if (seq != NULL) {
		if (seq->using_workarea && seq->playhead != seq->workarea_out) {
			seek(seq->workarea_out);
		} else {
			seek(seq->getEndFrame());
		}
	}
}

void Viewer::cue_recording(long start, long end, int track) {
	recording_start = start;
	recording_end = end;
	recording_track = track;
	panel_sequence_viewer->seek(recording_start);
	cue_recording_internal = true;
	recording_flasher.start();
}

void Viewer::uncue_recording() {
	cue_recording_internal = false;
	recording_flasher.stop();
	ui->pushButton_3->setStyleSheet(QString());
}

bool Viewer::is_recording_cued() {
	return cue_recording_internal;
}

void Viewer::toggle_play() {
	if (playing) {
		pause();
	} else {
		play();
	}
}

void Viewer::play() {
	if (panel_sequence_viewer->playing) panel_sequence_viewer->pause();
	if (panel_footage_viewer->playing) panel_footage_viewer->pause();

	if (seq != NULL) {
		reset_all_audio();
		if (audio_output->format().sampleRate() != seq->audio_frequency
				|| audio_output->format().channelCount() != av_get_channel_layout_nb_channels(seq->audio_layout)) {
			init_audio(seq);
		}
		if (is_recording_cued() && !start_recording()) {
			qDebug() << "[ERROR] Failed to record audio";
			return;
		}
		playhead_start = seq->playhead;
		start_msecs = QDateTime::currentMSecsSinceEpoch();
		playing = true;
		set_playpause_icon(false);
		playback_updater.start();
		timer_update();
		audio_thread->notifyReceiver();
	}
}

void Viewer::pause() {
	playing = false;
	set_playpause_icon(true);
	playback_updater.stop();

	if (is_recording_cued()) {
		uncue_recording();

		if (recording) {
			stop_recording();

			// import audio
			panel_project->process_file_list(false, QStringList(get_recorded_audio_filename()), NULL, NULL);

			// add it to the sequence
			Clip* c = new Clip(seq);
			Media* m = panel_project->last_imported_media.at(0);
			c->media = m; // latest media
			c->media_type = MEDIA_TYPE_FOOTAGE;
			c->media_stream = 0;
			c->timeline_in = recording_start;
			c->timeline_out = seq->playhead;
			c->clip_in = 0;
			c->track = recording_track;
			c->color_r = 128;
			c->color_g = 192;
			c->color_b = 128;
			c->name = m->name;

			QVector<Clip*> add_clips;
			add_clips.append(c);
			undo_stack.push(new AddClipCommand(seq, add_clips)); // add clip
		}
	}
}

void Viewer::update_playhead_timecode(long p) {
	ui->currentTimecode->set_value(p, false);
}

void Viewer::update_end_timecode() {
	ui->endTimecode->setText((seq == NULL) ? frame_to_timecode(0, config.timecode_view, 30) : frame_to_timecode(seq->getEndFrame(), config.timecode_view, seq->frame_rate));
}

void Viewer::update_header_zoom() {
	if (seq != NULL) {
		long sequenceEndFrame = seq->getEndFrame();
		ui->headers->update_zoom((sequenceEndFrame > 0) ? ((double) ui->headers->width() / (double) sequenceEndFrame) : 1);
	}
}

void Viewer::update_parents() {
	if (main_sequence) {
		update_ui(false);
	} else {
		update_viewer();
	}
}

void Viewer::update_viewer() {
	viewer_widget->update();
	update_header_zoom();
	if (seq != NULL) update_playhead_timecode(seq->playhead);
    update_end_timecode();
}

void Viewer::clear_inout_point() {
    if (seq->using_workarea) {
        undo_stack.push(new SetTimelineInOutCommand(seq, false, 0, 0));
        update_parents();
    }
}

void Viewer::set_in_point() {
	ui->headers->set_in_point(seq->playhead);
}

void Viewer::set_out_point() {
	ui->headers->set_out_point(seq->playhead);
}

void Viewer::set_media(int type, void* media) {
	main_sequence = false;
	clean_created_seq();
	switch (type) {
	case MEDIA_TYPE_FOOTAGE:
	{
		Media* footage = static_cast<Media*>(media);

		seq = new Sequence();
		created_sequence = true;
        seq->wrapper_sequence = true;
		seq->name = footage->name;

        seq->using_workarea = footage->using_inout;
        seq->workarea_in = footage->in;
        seq->workarea_out = footage->out;

		seq->frame_rate = 30;

		if (footage->video_tracks.size() > 0) {
			MediaStream* video_stream = footage->video_tracks.at(0);
			seq->width = video_stream->video_width;
			seq->height = video_stream->video_height;
			if (video_stream->video_frame_rate > 0 && !video_stream->infinite_length) seq->frame_rate = video_stream->video_frame_rate;

			Clip* c = new Clip(seq);
			c->media = footage;
			c->media_type = type;
			c->media_stream = video_stream->file_index;
			c->timeline_in = 0;
			c->timeline_out = footage->get_length_in_frames(seq->frame_rate);
			if (c->timeline_out <= 0) c->timeline_out = 150;
			c->track = -1;
			c->clip_in = 0;
			c->recalculateMaxLength();
			seq->clips.append(c);
		} else {
			seq->width = 1920;
			seq->height = 1080;
		}

		if (footage->audio_tracks.size() > 0) {
			MediaStream* audio_stream = footage->audio_tracks.at(0);
			seq->audio_frequency = audio_stream->audio_frequency;

			Clip* c = new Clip(seq);
			c->media = footage;
			c->media_type = type;
			c->media_stream = audio_stream->file_index;
			c->timeline_in = 0;
			c->timeline_out = footage->get_length_in_frames(seq->frame_rate);
			c->track = 0;
			c->clip_in = 0;
			c->recalculateMaxLength();
			seq->clips.append(c);

			if (footage->video_tracks.size() == 0) {
				viewer_widget->waveform = true;
				viewer_widget->waveform_clip = c;
				viewer_widget->waveform_ms = audio_stream;
				viewer_widget->update();
			}
		} else {
			seq->audio_frequency = 48000;
		}

		seq->audio_layout = AV_CH_LAYOUT_STEREO;

		set_sequence(false, seq);
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
	update_parents();
}

void Viewer::recording_flasher_update() {
	if (ui->pushButton_3->styleSheet().isEmpty()) {
		ui->pushButton_3->setStyleSheet("background: red;");
	} else {
		ui->pushButton_3->setStyleSheet(QString());
	}
}

void Viewer::clean_created_seq() {
	viewer_widget->waveform = false;

	if (created_sequence) {
        // TODO delete undo commands referencing this sequence to avoid crashes
        /*for (int i=0;i<undo_stack.count();i++) {
            undo_stack.command(i)
        }*/

		delete seq;
		seq = NULL;
		created_sequence = false;
	}
}

void Viewer::set_sequence(bool main, Sequence *s) {	
	reset_all_audio();

    main_sequence = main;
	seq = (main) ? sequence : s;

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

	init_audio(seq);

    if (!null_sequence) {
		ui->currentTimecode->set_frame_rate(seq->frame_rate);

		playback_updater.setInterval(qFloor(1000 / seq->frame_rate));

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

	update_header_zoom();

    viewer_widget->update();

    update();
}

void Viewer::set_playpause_icon(bool play) {
	ui->pushButton_3->setIcon(QIcon((play) ? ":/icons/play.png" : ":/icons/pause.png"));
}
