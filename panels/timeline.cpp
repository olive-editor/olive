#include "timeline.h"
#include "ui_timeline.h"

#include "panels/panels.h"
#include "panels/effectcontrols.h"
#include "ui/timelinewidget.h"
#include "project/sequence.h"
#include "project/clip.h"
#include "ui/viewerwidget.h"
#include "playback/audio.h"
#include "panels/viewer.h"
#include "playback/cacher.h"
#include "playback/playback.h"

#include <QTime>

Timeline::Timeline(QWidget *parent) :
	QDockWidget(parent),
	ui(new Ui::Timeline)
{
    selecting = moving_init = moving_proc = splitting = importing = playing = trim_in = snapped = false;
    snapping = true;
    last_frame = playhead = snap_point = 0;
	trim_target = -1;

	ui->setupUi(this);

	ui->video_area->bottom_align = true;

	tool_buttons.append(ui->toolArrowButton);
	tool_buttons.append(ui->toolEditButton);
	tool_buttons.append(ui->toolRippleButton);
	tool_buttons.append(ui->toolRazorButton);
	tool_buttons.append(ui->toolSlipButton);
	tool_buttons.append(ui->toolRollingButton);

	ui->toolArrowButton->click();

	zoom = 1.0f;

	sequence = NULL;
	set_sequence(NULL);

	connect(&playback_updater, SIGNAL(timeout()), this, SLOT(repaint_timeline()));
}

Timeline::~Timeline()
{
	delete ui;
}

void Timeline::go_to_start() {
	seek(0);
}

void Timeline::previous_frame() {
	seek(playhead-1);
}

void Timeline::next_frame() {
	seek(playhead+1);
}

void Timeline::seek(long p) {
	pause();

	// reset all clip audio
	for (int i=0;i<sequence->clip_count();i++) {
		Clip& c = sequence->get_clip(i);
		c.reset_audio = true;
		c.frame_sample_index = 0;
	}
	clear_cache(true, true);
    audio_bytes_written = 0;
	switch_audio_cache = true;
	reading_audio_cache_A = false;

	playhead = p;
}

bool Timeline::toggle_play() {
	if (playing) {
		pause();
	} else {
		play();
	}
	return playing;
}

void Timeline::play() {
	playhead_start = playhead;
	playback_timer.start();
	playback_updater.start();
	playing = true;

	if (switch_audio_cache) {
		// add something to pre-cache audio
	}
}

void Timeline::pause() {
	playing = false;
}

void Timeline::go_to_end() {
	seek(sequence->getEndFrame());
}

void Timeline::set_sequence(Sequence *s) {
	if (sequence != NULL) {
		// clean up - close all open clips
		for (int i=0;i<sequence->clip_count();i++) {
			Clip& c = sequence->get_clip(i);
			if (c.open) {
				close_clip(&c);
			}
		}
	}

	sequence = s;
	bool null_sequence = (s == NULL);

	for (int i=0;i<tool_buttons.count();i++) {
		tool_buttons[i]->setEnabled(!null_sequence);
	}
	ui->pushButton_4->setEnabled(!null_sequence);
	ui->pushButton_5->setEnabled(!null_sequence);
	ui->video_area->setEnabled(!null_sequence);
	ui->audio_area->setEnabled(!null_sequence);

	if (null_sequence) {
		setWindowTitle("Timeline: <none>");
	} else {
		setWindowTitle("Timeline: " + sequence->name);
		redraw_all_clips();
		playback_updater.setInterval(floor(1000 / sequence->frame_rate));
	}
}

bool Timeline::focused() {
	return (ui->video_area->hasFocus() || ui->audio_area->hasFocus());
}

void Timeline::repaint_timeline() {
	if (playing) {
		playhead = playhead_start + (playback_timer.elapsed() * 0.001 * sequence->frame_rate);
	}
	ui->video_area->update();
	ui->audio_area->update();
    if (last_frame != playhead) {
		panel_viewer->viewer_widget->update();
		last_frame = playhead;
	}
}

void Timeline::redraw_all_clips() {
	ui->video_area->redraw_clips();
	ui->audio_area->redraw_clips();
}

void Timeline::select_all() {
	selections.clear();
	for (int i=0;i<sequence->clip_count();i++) {
		Clip& c = sequence->get_clip(i);
		selections.append({c.timeline_in, c.timeline_out, c.track});
	}
}

void Timeline::delete_selection(bool ripple_delete) {
	if (selections.size() > 0) {
		panel_effect_controls->set_clip(NULL);

		long ripple_point = selections.at(0).in;
		long ripple_length = selections.at(0).out - selections.at(0).in;

		for (int i=0;i<selections.size();i++) {
			const Selection& s = selections.at(i);
			sequence->delete_area(s.in, s.out, s.track);
			if (ripple_delete) {
				if (ripple_point > s.in) ripple_point = s.in;
				if (ripple_length > s.out - s.in) ripple_length = s.out - s.in;
			}
		}
		selections.clear();

		if (ripple_delete) {
			long validator;
			for (int i=0;i<sequence->clip_count();i++) {
				// check every clip after and see if it'll collide
				// NOTE, we could probably re-use the validation code for the ripple tool here for optimization (since it's technically better code I think)
				Clip& c = sequence->get_clip(i);
				if (c.timeline_in >= ripple_point) {
					for (int j=0;j<sequence->clip_count();j++) {
						Clip& cc = sequence->get_clip(j);
						if (cc.timeline_in < ripple_point) {
							validator = c.timeline_in - ripple_length - cc.timeline_out;
							if (validator < 0) ripple_length += validator;

							if (ripple_length <= 0) {
								// we've seen all we need to see here (can't ripple so stop looping)
								i = j = sequence->clip_count();
							}
						}
					}
				}
			}
			if (ripple_length > 0) ripple(ripple_point, -ripple_length);
		}

		redraw_all_clips();
	}
}

void Timeline::zoom_in() {
	zoom *= 2;
	redraw_all_clips();
}

void Timeline::zoom_out() {
	zoom /= 2;
	redraw_all_clips();
}

void Timeline::ripple(long ripple_point, long ripple_length) {
	for (int i=0;i<sequence->clip_count();i++) {
		Clip& c = sequence->get_clip(i);
		if (c.timeline_in >= ripple_point) {
			c.timeline_in += ripple_length;
			c.timeline_out += ripple_length;
		}
	}
	for (int i=0;i<selections.size();i++) {
		Selection& s = selections[i];
		if (s.in >= ripple_point) {
			s.in += ripple_length;
			s.out += ripple_length;
		}
	}
}

void Timeline::decheck_tool_buttons(QObject* sender) {
	for (int i=0;i<tool_buttons.count();i++) {
		if (tool_buttons[i] != sender) {
			tool_buttons[i]->setChecked(false);
		}
	}
}

void Timeline::on_toolEditButton_toggled(bool checked)
{
	if (checked) {
		decheck_tool_buttons(sender());
		ui->timeline_area->setCursor(Qt::IBeamCursor);
		tool = TIMELINE_TOOL_EDIT;
	}
}

void Timeline::on_toolArrowButton_toggled(bool checked)
{
	if (checked) {
		decheck_tool_buttons(sender());
		ui->timeline_area->setCursor(Qt::ArrowCursor);
		tool = TIMELINE_TOOL_POINTER;
	}
}

void Timeline::on_toolRazorButton_toggled(bool checked)
{
	if (checked) {
		decheck_tool_buttons(sender());
		ui->timeline_area->setCursor(Qt::IBeamCursor);
		tool = TIMELINE_TOOL_RAZOR;
	}
}

void Timeline::on_pushButton_4_clicked()
{
	zoom_in();
}

void Timeline::on_pushButton_5_clicked()
{
	zoom_out();
}

bool Timeline::is_clip_selected(int clip_index) {
	Clip& clip = sequence->get_clip(clip_index);
	for (int i=0;i<selections.size();i++) {
		const Selection& s = selections.at(i);
		if (clip.track == s.track && clip.timeline_in >= s.in && clip.timeline_out <= s.out) {
			return true;
		}
	}
	return false;
}

void Timeline::on_toolRippleButton_toggled(bool checked)
{
	if (checked) {
		decheck_tool_buttons(sender());
		ui->timeline_area->setCursor(Qt::ArrowCursor);
		tool = TIMELINE_TOOL_RIPPLE;
	}
}

void Timeline::on_toolRollingButton_toggled(bool checked)
{
	if (checked) {
		decheck_tool_buttons(sender());
		ui->timeline_area->setCursor(Qt::ArrowCursor);
		tool = TIMELINE_TOOL_ROLLING;
	}
}

void Timeline::on_toolSlipButton_toggled(bool checked)
{
	if (checked) {
		decheck_tool_buttons(sender());
		ui->timeline_area->setCursor(Qt::ArrowCursor);
		tool = TIMELINE_TOOL_SLIP;
	}
}

void Timeline::on_snappingButton_toggled(bool checked)
{
    snapping = checked;

    qDebug() << "clips vector size:" << sizeof(Clip) * sequence->clip_count();
}
