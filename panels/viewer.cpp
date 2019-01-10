#include "viewer.h"

#include "playback/audio.h"
#include "timeline.h"
#include "panels/project.h"
#include "panels/effectcontrols.h"
#include "project/sequence.h"
#include "project/clip.h"
#include "panels/panels.h"
#include "io/config.h"
#include "project/footage.h"
#include "project/media.h"
#include "project/undo.h"
#include "ui/audiomonitor.h"
#include "playback/playback.h"
#include "ui/viewerwidget.h"
#include "ui/viewercontainer.h"
#include "ui/labelslider.h"
#include "ui/timelineheader.h"
#include "ui/resizablescrollbar.h"
#include "debug.h"

#define FRAMES_IN_ONE_MINUTE 1798 // 1800 - 2
#define FRAMES_IN_TEN_MINUTES 17978 // (FRAMES_IN_ONE_MINUTE * 10) - 2

extern "C" {
	#include <libavformat/avformat.h>
	#include <libavcodec/avcodec.h>
}

#include <QtMath>
#include <QAudioOutput>
#include <QPainter>
#include <QStringList>
#include <QTimer>
#include <QHBoxLayout>
#include <QPushButton>

Viewer::Viewer(QWidget *parent) :
	QDockWidget(parent),
	playing(false),
	just_played(false),
	media(NULL),
	seq(NULL),
	created_sequence(false),
	cue_recording_internal(false),
	panel_name("Viewer: "),
	minimum_zoom(1.0)
{
	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

	setup_ui();

	headers->viewer = this;
	headers->snapping = false;
	headers->show_text(false);
	viewer_container->viewer = this;
	viewer_widget = viewer_container->child;
	viewer_widget->viewer = this;
	set_media(NULL);

	currentTimecode->setEnabled(false);
	currentTimecode->set_minimum_value(0);
	currentTimecode->set_default_value(qSNaN());
	currentTimecode->set_value(0, false);
	currentTimecode->set_display_type(LABELSLIDER_FRAMENUMBER);
	connect(currentTimecode, SIGNAL(valueChanged()), this, SLOT(update_playhead()));

	recording_flasher.setInterval(500);

	connect(&playback_updater, SIGNAL(timeout()), this, SLOT(timer_update()));
	connect(&recording_flasher, SIGNAL(timeout()), this, SLOT(recording_flasher_update()));
	connect(horizontal_bar, SIGNAL(valueChanged(int)), headers, SLOT(set_scroll(int)));
	connect(horizontal_bar, SIGNAL(valueChanged(int)), viewer_widget, SLOT(set_waveform_scroll(int)));
	connect(horizontal_bar, SIGNAL(resize_move(double)), this, SLOT(resize_move(double)));

	update_playhead_timecode(0);
	update_end_timecode();
}

Viewer::~Viewer() {}

bool Viewer::is_focused() {
	return headers->hasFocus()
			|| viewer_widget->hasFocus()
			|| btnSkipToStart->hasFocus()
			|| btnRewind->hasFocus()
			|| btnPlay->hasFocus()
			|| btnFastForward->hasFocus()
			|| btnSkipToEnd->hasFocus();
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

	if (view == TIMECODE_FRAMES || (list.size() == 1 && view != TIMECODE_MILLISECONDS)) {
		return s.toLong();
	}

	int frRound = qRound(frame_rate);
	int hours, minutes, seconds, frames;

	if (view == TIMECODE_MILLISECONDS) {
		long milliseconds = s.toLong();

		hours = milliseconds/3600000;
		milliseconds -= (hours*3600000);
		minutes = milliseconds/60000;
		milliseconds -= (minutes*60000);
		seconds = milliseconds/1000;
		milliseconds -= (seconds*1000);
		frames = qRound64((milliseconds*0.001)*frame_rate);

		seconds = qRound64(seconds * frame_rate);
		minutes = qRound64(minutes * frame_rate * 60);
		hours = qRound64(hours * frame_rate * 3600);
	} else {
		hours = ((list.size() > 0) ? list.at(0).toInt() : 0) * frRound * 3600;
		minutes = ((list.size() > 1) ? list.at(1).toInt() : 0) * frRound * 60;
		seconds = ((list.size() > 2) ? list.at(2).toInt() : 0) * frRound;
		frames = (list.size() > 3) ? list.at(3).toInt() : 0;
	}

	int f = (frames + seconds + minutes + hours);

	if ((view == TIMECODE_DROP || view == TIMECODE_MILLISECONDS) && frame_rate_is_droppable(frame_rate)) {
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

	if ((view == TIMECODE_DROP || view == TIMECODE_MILLISECONDS) && frame_rate_is_droppable(frame_rate)) {
		//CONVERT A FRAME NUMBER TO DROP FRAME TIMECODE
		//Code by David Heidelberger, adapted from Andrew Duncan, further adapted for Olive by Olive Team
		//Given an int called framenumber and a double called framerate
		//Framerate should be 29.97, 59.94, or 23.976, otherwise the calculations will be off.

		int d;
		int m;

		int dropFrames = round(frame_rate * .066666); //Number of frames to drop on the minute marks is the nearest integer to 6% of the framerate
		int framesPerHour = round(frame_rate*60*60); //Number of frqRound64ames in an hour
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
	if (view == TIMECODE_MILLISECONDS) {
		return QString::number((hours*3600000)+(mins*60000)+(secs*1000)+qCeil(frames*1000/frame_rate));
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
	if (main_sequence) {
		panel_timeline->scroll_to_frame(p);
		panel_effect_controls->scroll_to_frame(p);
	}
	update_parents();
	reset_all_audio();
	audio_scrub = true;
}

void Viewer::go_to_start() {
	if (seq != NULL) seek(0);
}

void Viewer::go_to_end() {
	if (seq != NULL) seek(seq->getEndFrame());
}

void Viewer::go_to_in() {
	if (seq != NULL) {
        if (seq->using_workarea && seq->enable_workarea) {
			seek(seq->workarea_in);
		} else {
			go_to_start();
		}
	}
}

void Viewer::previous_frame() {
	if (seq != NULL && seq->playhead > 0) seek(seq->playhead-1);
}

void Viewer::next_frame() {
	if (seq != NULL) seek(seq->playhead+1);
}

void Viewer::go_to_out() {
	if (seq != NULL) {
        if (seq->using_workarea && seq->enable_workarea) {
			seek(seq->workarea_out);
		} else {
			go_to_end();
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
	btnPlay->setStyleSheet(QString());
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
		if (!is_recording_cued()
				&& seq->playhead >= get_seq_out()
				&& (config.loop || !main_sequence)) {
			seek(get_seq_in());
		}

		reset_all_audio();
		if (is_recording_cued() && !start_recording()) {
			dout << "[ERROR] Failed to record audio";
			return;
		}
		playhead_start = seq->playhead;
		playing = true;
		just_played = true;
		set_playpause_icon(false);
		start_msecs = QDateTime::currentMSecsSinceEpoch();
		timer_update();
	}
}

void Viewer::play_wake() {
	if (just_played) {
		start_msecs = QDateTime::currentMSecsSinceEpoch();
		playback_updater.start();
		if (audio_thread != NULL) audio_thread->notifyReceiver();
		just_played = false;
	}
}

void Viewer::pause() {
	playing = false;
	just_played = false;
	set_playpause_icon(true);
	playback_updater.stop();

	if (is_recording_cued()) {
		uncue_recording();

		if (recording) {
			stop_recording();

			// import audio
			QStringList file_list;
			file_list.append(get_recorded_audio_filename());
			panel_project->process_file_list(file_list);

			// add it to the sequence
			Clip* c = new Clip(seq);
			Media* m = panel_project->last_imported_media.at(0);
			Footage* f = m->to_footage();

			f->ready_lock.lock();

			c->media = m; // latest media
			c->media_stream = 0;
			c->timeline_in = recording_start;
			c->timeline_out = recording_start + f->get_length_in_frames(seq->frame_rate);
			c->clip_in = 0;
			c->track = recording_track;
			c->color_r = 128;
			c->color_g = 192;
			c->color_b = 128;
			c->name = m->get_name();

			f->ready_lock.unlock();

			QVector<Clip*> add_clips;
			add_clips.append(c);
			undo_stack.push(new AddClipCommand(seq, add_clips)); // add clip
		}
	}
}

void Viewer::update_playhead_timecode(long p) {
	currentTimecode->set_value(p, false);
}

void Viewer::update_end_timecode() {
	endTimecode->setText((seq == NULL) ? frame_to_timecode(0, config.timecode_view, 30) : frame_to_timecode(seq->getEndFrame(), config.timecode_view, seq->frame_rate));
}

void Viewer::update_header_zoom() {
	if (seq != NULL) {
		long sequenceEndFrame = seq->getEndFrame();
		if (cached_end_frame != sequenceEndFrame) {
			minimum_zoom = (sequenceEndFrame > 0) ? ((double) headers->width() / (double) sequenceEndFrame) : 1;
			headers->update_zoom(qMax(headers->get_zoom(), minimum_zoom));
			set_sb_max();
			viewer_widget->waveform_zoom = headers->get_zoom();
		} else {
			headers->update();
		}
	}
}

void Viewer::update_parents() {
	if (main_sequence) {
		update_ui(false);
	} else {
		update_viewer();
	}
}

void Viewer::resizeEvent(QResizeEvent *event) {
	if (seq != NULL) {
		set_sb_max();
	}
}

void Viewer::update_viewer() {
	update_header_zoom();
	viewer_widget->update();
	if (seq != NULL) update_playhead_timecode(seq->playhead);
	update_end_timecode();
}

void Viewer::clear_in() {
    if (seq->using_workarea) {
        undo_stack.push(new SetTimelineInOutCommand(seq, true, 0, seq->workarea_out));
        update_parents();
    }
}

void Viewer::clear_out() {
    if (seq->using_workarea) {
        undo_stack.push(new SetTimelineInOutCommand(seq, true, seq->workarea_in, seq->getEndFrame()));
        update_parents();
    }
}

void Viewer::clear_inout_point() {
	if (seq->using_workarea) {
		undo_stack.push(new SetTimelineInOutCommand(seq, false, 0, 0));
		update_parents();
    }
}

void Viewer::toggle_enable_inout() {
    if (seq != NULL && seq->using_workarea) {
        undo_stack.push(new SetBool(&seq->enable_workarea, !seq->enable_workarea));
        update_parents();
    }
}

void Viewer::set_in_point() {
	headers->set_in_point(seq->playhead);
}

void Viewer::set_out_point() {
	headers->set_out_point(seq->playhead);
}

void Viewer::set_zoom(bool in) {
	if (seq != NULL) {
		set_zoom_value(in ? headers->get_zoom()*2 : qMax(minimum_zoom, headers->get_zoom()*0.5));
	}
}

void Viewer::set_zoom_value(double d) {
	headers->update_zoom(d);
	if (viewer_widget->waveform) {
		viewer_widget->waveform_zoom = d;
		viewer_widget->update();
	}
	if (seq != NULL) {
		set_sb_max();
		if (!horizontal_bar->is_resizing())
			center_scroll_to_playhead(horizontal_bar, headers->get_zoom(), seq->playhead);
	}
}

void Viewer::set_sb_max() {
	headers->set_scrollbar_max(horizontal_bar, seq->getEndFrame(), headers->width());
}

long Viewer::get_seq_in() {
    return (seq->using_workarea && seq->enable_workarea)
			? seq->workarea_in
			: 0;
}

long Viewer::get_seq_out() {
    return (seq->using_workarea && seq->enable_workarea && previous_playhead < seq->workarea_out)
			? seq->workarea_out
			: seq->getEndFrame();
}

void Viewer::setup_ui() {
	QWidget* contents = new QWidget();

	QVBoxLayout* layout = new QVBoxLayout(contents);
	layout->setSpacing(0);
	layout->setMargin(0);

	viewer_container = new ViewerContainer(contents);
	viewer_container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	layout->addWidget(viewer_container);

	headers = new TimelineHeader(contents);
	layout->addWidget(headers);

	horizontal_bar = new ResizableScrollBar(contents);
	horizontal_bar->setSingleStep(20);
	horizontal_bar->setOrientation(Qt::Horizontal);
	layout->addWidget(horizontal_bar);

	QWidget* lower_controls = new QWidget(contents);

	QHBoxLayout* lower_control_layout = new QHBoxLayout(lower_controls);
	lower_control_layout->setMargin(0);

	// current time code
	QWidget* current_timecode_container = new QWidget(lower_controls);
	QHBoxLayout* current_timecode_container_layout = new QHBoxLayout(current_timecode_container);
	current_timecode_container_layout->setSpacing(0);
	current_timecode_container_layout->setMargin(0);
	currentTimecode = new LabelSlider(current_timecode_container);
	current_timecode_container_layout->addWidget(currentTimecode);
	lower_control_layout->addWidget(current_timecode_container);

	QWidget* playback_controls = new QWidget(lower_controls);

	QHBoxLayout* playback_control_layout = new QHBoxLayout(playback_controls);
	playback_control_layout->setSpacing(0);
	playback_control_layout->setMargin(0);

	btnSkipToStart = new QPushButton(playback_controls);
	QIcon goToStartIcon;
	goToStartIcon.addFile(QStringLiteral(":/icons/prev.png"), QSize(), QIcon::Normal, QIcon::Off);
	goToStartIcon.addFile(QStringLiteral(":/icons/prev-disabled.png"), QSize(), QIcon::Disabled, QIcon::Off);
	btnSkipToStart->setIcon(goToStartIcon);
	connect(btnSkipToStart, SIGNAL(clicked(bool)), this, SLOT(go_to_in()));
	playback_control_layout->addWidget(btnSkipToStart);

	btnRewind = new QPushButton(playback_controls);
	QIcon rewindIcon;
	rewindIcon.addFile(QStringLiteral(":/icons/rew.png"), QSize(), QIcon::Normal, QIcon::Off);
	rewindIcon.addFile(QStringLiteral(":/icons/rew-disabled.png"), QSize(), QIcon::Disabled, QIcon::Off);
	btnRewind->setIcon(rewindIcon);
	connect(btnRewind, SIGNAL(clicked(bool)), this, SLOT(previous_frame()));
	playback_control_layout->addWidget(btnRewind);

	btnPlay = new QPushButton(playback_controls);
	playIcon.addFile(QStringLiteral(":/icons/play.png"), QSize(), QIcon::Normal, QIcon::On);
	playIcon.addFile(QStringLiteral(":/icons/play-disabled.png"), QSize(), QIcon::Disabled, QIcon::On);
	btnPlay->setIcon(playIcon);
	connect(btnPlay, SIGNAL(clicked(bool)), this, SLOT(toggle_play()));
	playback_control_layout->addWidget(btnPlay);

	btnFastForward = new QPushButton(playback_controls);
	QIcon ffIcon;
	ffIcon.addFile(QStringLiteral(":/icons/ff.png"), QSize(), QIcon::Normal, QIcon::On);
	ffIcon.addFile(QStringLiteral(":/icons/ff-disabled.png"), QSize(), QIcon::Disabled, QIcon::Off);
	btnFastForward->setIcon(ffIcon);
	connect(btnFastForward, SIGNAL(clicked(bool)), this, SLOT(next_frame()));
	playback_control_layout->addWidget(btnFastForward);

	btnSkipToEnd = new QPushButton(playback_controls);
	QIcon nextIcon;
	nextIcon.addFile(QStringLiteral(":/icons/next.png"), QSize(), QIcon::Normal, QIcon::Off);
	nextIcon.addFile(QStringLiteral(":/icons/next-disabled.png"), QSize(), QIcon::Disabled, QIcon::Off);
	btnSkipToEnd->setIcon(nextIcon);
	connect(btnSkipToEnd, SIGNAL(clicked(bool)), this, SLOT(go_to_out()));
	playback_control_layout->addWidget(btnSkipToEnd);

	lower_control_layout->addWidget(playback_controls);

	QWidget* end_timecode_container = new QWidget(lower_controls);

	QHBoxLayout* end_timecode_layout = new QHBoxLayout(end_timecode_container);
	end_timecode_layout->setSpacing(0);
	end_timecode_layout->setMargin(0);

	endTimecode = new QLabel(end_timecode_container);
	endTimecode->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

	end_timecode_layout->addWidget(endTimecode);

	lower_control_layout->addWidget(end_timecode_container);

	layout->addWidget(lower_controls);

	setWidget(contents);
}

void Viewer::set_media(Media* m) {
	main_sequence = false;
	media = m;
	clean_created_seq();
	if (media != NULL) {
		switch (media->get_type()) {
		case MEDIA_TYPE_FOOTAGE:
		{
			Footage* footage = media->to_footage();

			seq = new Sequence();
			created_sequence = true;
			seq->wrapper_sequence = true;
			seq->name = footage->name;

			seq->using_workarea = footage->using_inout;
			if (footage->using_inout) {
				seq->workarea_in = footage->in;
				seq->workarea_out = footage->out;
			}

			seq->frame_rate = 30;

			if (footage->video_tracks.size() > 0) {
				const FootageStream& video_stream = footage->video_tracks.at(0);
				seq->width = video_stream.video_width;
				seq->height = video_stream.video_height;
				if (video_stream.video_frame_rate > 0 && !video_stream.infinite_length) seq->frame_rate = video_stream.video_frame_rate * footage->speed;

				Clip* c = new Clip(seq);
				c->media = media;
				c->media_stream = video_stream.file_index;
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
				const FootageStream& audio_stream = footage->audio_tracks.at(0);
				seq->audio_frequency = audio_stream.audio_frequency;

				Clip* c = new Clip(seq);
				c->media = media;
				c->media_stream = audio_stream.file_index;
				c->timeline_in = 0;
				c->timeline_out = footage->get_length_in_frames(seq->frame_rate);
				c->track = 0;
				c->clip_in = 0;
				c->recalculateMaxLength();
				seq->clips.append(c);

				if (footage->video_tracks.size() == 0) {
					viewer_widget->waveform = true;
					viewer_widget->waveform_clip = c;
					viewer_widget->waveform_ms = &audio_stream;
					viewer_widget->update();
				}
			} else {
				seq->audio_frequency = 48000;
			}

			seq->audio_layout = AV_CH_LAYOUT_STEREO;
		}
			break;
		case MEDIA_TYPE_SEQUENCE:
			seq = media->to_sequence();
			break;
		}
	}
	set_sequence(false, seq);
}

void Viewer::update_playhead() {
	seek(currentTimecode->value());
}

void Viewer::timer_update() {
	previous_playhead = seq->playhead;

	seq->playhead = qRound(playhead_start + ((QDateTime::currentMSecsSinceEpoch()-start_msecs) * 0.001 * seq->frame_rate));
	update_parents();

	long end_frame = get_seq_out();
	if (!recording
			&& playing
			&& seq->playhead >= end_frame
			&& previous_playhead < end_frame) {
		if (!config.pause_at_out_point && config.loop) {
			seek(get_seq_in());
			play();
		} else if (config.pause_at_out_point || !main_sequence) {
			pause();
		}
	} else if (recording && recording_start != recording_end && seq->playhead >= recording_end) {
		pause();
	}
}

void Viewer::recording_flasher_update() {
	if (btnPlay->styleSheet().isEmpty()) {
		btnPlay->setStyleSheet("background: red;");
	} else {
		btnPlay->setStyleSheet(QString());
	}
}

void Viewer::resize_move(double d) {
	set_zoom_value(headers->get_zoom()*d);
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
	pause();

	reset_all_audio();

	if (seq != NULL) {
		closeActiveClips(seq);
	}

	main_sequence = main;
	seq = (main) ? sequence : s;

	bool null_sequence = (seq == NULL);

	headers->setEnabled(!null_sequence);
	currentTimecode->setEnabled(!null_sequence);
	viewer_widget->setEnabled(!null_sequence);
	viewer_widget->setVisible(!null_sequence);
	btnSkipToStart->setEnabled(!null_sequence);
	btnRewind->setEnabled(!null_sequence);
	btnPlay->setEnabled(!null_sequence);
	btnFastForward->setEnabled(!null_sequence);
	btnSkipToEnd->setEnabled(!null_sequence);

	if (!null_sequence) {
		currentTimecode->set_frame_rate(seq->frame_rate);

		playback_updater.setInterval(qFloor(1000 / seq->frame_rate));

		update_playhead_timecode(seq->playhead);
		update_end_timecode();

		viewer_container->adjust();

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
	btnPlay->setIcon(play ? playIcon : QIcon(":/icons/pause.png"));
}
