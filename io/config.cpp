#include "config.h"

#include <QFile>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "panels/project.h"
#include "panels/panels.h"

#include "debug.h"

Config config;

Config::Config()
	: saved_layout(false),
	  show_track_lines(true),
	  scroll_zooms(false),
	  edit_tool_selects_links(false),
	  edit_tool_also_seeks(false),
	  select_also_seeks(false),
	  paste_seeks(true),
	  img_seq_formats("jpg|jpeg|bmp|tiff|tif|psd|png|tga|jp2|gif"),
	  rectified_waveforms(false),
	  default_transition_length(30),
	  timecode_view(TIMECODE_DROP),
	  show_title_safe_area(false),
	  use_custom_title_safe_ratio(false),
	  custom_title_safe_ratio(1),
	  enable_drag_files_to_timeline(true),
	  autoscale_by_default(false),
	  recording_mode(2),
	  enable_seek_to_import(false),
	  enable_audio_scrubbing(true),
	  drop_on_media_to_replace(true),
	  autoscroll(AUTOSCROLL_PAGE_SCROLL),
	  audio_rate(48000),
	  fast_seeking(false),
	  hover_focus(false),
	  project_view_type(PROJECT_VIEW_TREE),
	  set_name_with_marker(true),
	  show_project_toolbar(false),
	  disable_multithreading_for_images(false),
	  previous_queue_size(3),
	  previous_queue_type(FRAME_QUEUE_TYPE_FRAMES),
	  upcoming_queue_size(0.5),
      upcoming_queue_type(FRAME_QUEUE_TYPE_SECONDS),
      loop(true),
      pause_at_out_point(true)
{}

void Config::load(QString path) {
	QFile f(path);
	if (f.exists() && f.open(QIODevice::ReadOnly)) {
		QXmlStreamReader stream(&f);

		while (!stream.atEnd()) {
			stream.readNext();
			if (stream.isStartElement()) {
				if (stream.name() == "SavedLayout") {
					stream.readNext();
					saved_layout = (stream.text() == "1");
				} else if (stream.name() == "ShowTrackLines") {
					stream.readNext();
					show_track_lines = (stream.text() == "1");
				} else if (stream.name() == "ScrollZooms") {
					stream.readNext();
					scroll_zooms = (stream.text() == "1");
				} else if (stream.name() == "EditToolSelectsLinks") {
					stream.readNext();
					edit_tool_selects_links = (stream.text() == "1");
				} else if (stream.name() == "EditToolAlsoSeeks") {
					stream.readNext();
					edit_tool_also_seeks = (stream.text() == "1");
				} else if (stream.name() == "SelectAlsoSeeks") {
					stream.readNext();
					select_also_seeks = (stream.text() == "1");
				} else if (stream.name() == "PasteSeeks") {
					stream.readNext();
					paste_seeks = (stream.text() == "1");
				} else if (stream.name() == "ImageSequenceFormats") {
					stream.readNext();
					img_seq_formats = stream.text().toString();
				} else if (stream.name() == "RectifiedWaveforms") {
					stream.readNext();
					rectified_waveforms = (stream.text() == "1");
				} else if (stream.name() == "DefaultTransitionLength") {
					stream.readNext();
					default_transition_length = stream.text().toInt();
				} else if (stream.name() == "TimecodeView") {
					stream.readNext();
					timecode_view = stream.text().toInt();
				} else if (stream.name() == "ShowTitleSafeArea") {
					stream.readNext();
					show_title_safe_area = (stream.text() == "1");
				} else if (stream.name() == "UseCustomTitleSafeRatio") {
					stream.readNext();
					use_custom_title_safe_ratio = (stream.text() == "1");
				} else if (stream.name() == "CustomTitleSafeRatio") {
					stream.readNext();
					custom_title_safe_ratio = stream.text().toDouble();
				} else if (stream.name() == "EnableDragFilesToTimeline") {
					stream.readNext();
					enable_drag_files_to_timeline = (stream.text() == "1");;
				} else if (stream.name() == "AutoscaleByDefault") {
					stream.readNext();
					autoscale_by_default = (stream.text() == "1");
				} else if (stream.name() == "RecordingMode") {
					stream.readNext();
					recording_mode = stream.text().toInt();
				} else if (stream.name() == "EnableSeekToImport") {
					stream.readNext();
					enable_seek_to_import = (stream.text() == "1");
				} else if (stream.name() == "AudioScrubbing") {
					stream.readNext();
					enable_audio_scrubbing = (stream.text() == "1");
				} else if (stream.name() == "DropFileOnMediaToReplace") {
					stream.readNext();
					drop_on_media_to_replace = (stream.text() == "1");
				} else if (stream.name() == "Autoscroll") {
					stream.readNext();
					autoscroll = stream.text().toInt();
				} else if (stream.name() == "AudioRate") {
					stream.readNext();
					audio_rate = stream.text().toInt();
				} else if (stream.name() == "FastSeeking") {
					stream.readNext();
					fast_seeking = (stream.text() == "1");
				} else if (stream.name() == "HoverFocus") {
					stream.readNext();
					hover_focus = (stream.text() == "1");
				} else if (stream.name() == "ProjectViewType") {
					stream.readNext();
					project_view_type = stream.text().toInt();
				} else if (stream.name() == "SetNameWithMarker") {
					stream.readNext();
					set_name_with_marker = (stream.text() == "1");
				} else if (stream.name() == "ShowProjectToolbar") {
					stream.readNext();
					show_project_toolbar = (stream.text() == "1");
				} else if (stream.name() == "DisableMultithreadedImages") {
					stream.readNext();
					disable_multithreading_for_images = (stream.text() == "1");
				} else if (stream.name() == "PreviousFrameQueueSize") {
					stream.readNext();
					previous_queue_size = stream.text().toDouble();
				} else if (stream.name() == "PreviousFrameQueueType") {
					stream.readNext();
					previous_queue_type = stream.text().toInt();
				} else if (stream.name() == "UpcomingFrameQueueSize") {
					stream.readNext();
					upcoming_queue_size = stream.text().toDouble();
				} else if (stream.name() == "UpcomingFrameQueueType") {
					stream.readNext();
					upcoming_queue_type = stream.text().toInt();
                } else if (stream.name() == "Loop") {
                    stream.readNext();
                    loop = (stream.text() == "1");
                } else if (stream.name() == "PauseAtOutPoint") {
                    stream.readNext();
                    pause_at_out_point = (stream.text() == "1");
                }
			}
		}
		if (stream.hasError()) {
			dout << "[ERROR] Error parsing config XML." << stream.errorString();
		}

		f.close();
	}
}

void Config::save(QString path) {
	QFile f(path);
	if (!f.open(QIODevice::WriteOnly)) {
		dout << "[ERROR] Could not save configuration";
		return;
	}

	QXmlStreamWriter stream(&f);
	stream.setAutoFormatting(true);
	stream.writeStartDocument(); // doc
	stream.writeStartElement("Configuration"); // configuration

	stream.writeTextElement("Version", QString::number(SAVE_VERSION));
	stream.writeTextElement("SavedLayout", QString::number(saved_layout));
	stream.writeTextElement("ShowTrackLines", QString::number(show_track_lines));
	stream.writeTextElement("ScrollZooms", QString::number(scroll_zooms));
	stream.writeTextElement("EditToolSelectsLinks", QString::number(edit_tool_selects_links));
	stream.writeTextElement("EditToolAlsoSeeks", QString::number(edit_tool_also_seeks));
	stream.writeTextElement("SelectAlsoSeeks", QString::number(select_also_seeks));
	stream.writeTextElement("PasteSeeks", QString::number(paste_seeks));
	stream.writeTextElement("ImageSequenceFormats", img_seq_formats);
	stream.writeTextElement("RectifiedWaveforms", QString::number(rectified_waveforms));
	stream.writeTextElement("DefaultTransitionLength", QString::number(default_transition_length));
	stream.writeTextElement("TimecodeView", QString::number(timecode_view));
	stream.writeTextElement("ShowTitleSafeArea", QString::number(show_title_safe_area));
	stream.writeTextElement("UseCustomTitleSafeRatio", QString::number(use_custom_title_safe_ratio));
	stream.writeTextElement("CustomTitleSafeRatio", QString::number(custom_title_safe_ratio));
	stream.writeTextElement("EnableDragFilesToTimeline", QString::number(enable_drag_files_to_timeline));
	stream.writeTextElement("AutoscaleByDefault", QString::number(autoscale_by_default));
	stream.writeTextElement("RecordingMode", QString::number(recording_mode));
	stream.writeTextElement("EnableSeekToImport", QString::number(enable_seek_to_import));
	stream.writeTextElement("AudioScrubbing", QString::number(enable_audio_scrubbing));
	stream.writeTextElement("DropFileOnMediaToReplace", QString::number(drop_on_media_to_replace));
	stream.writeTextElement("Autoscroll", QString::number(autoscroll));
	stream.writeTextElement("AudioRate", QString::number(audio_rate));
	stream.writeTextElement("FastSeeking", QString::number(fast_seeking));
	stream.writeTextElement("HoverFocus", QString::number(hover_focus));
	stream.writeTextElement("ProjectViewType", QString::number(project_view_type));
	stream.writeTextElement("SetNameWithMarker", QString::number(set_name_with_marker));
	stream.writeTextElement("ShowProjectToolbar", QString::number(panel_project->toolbar_widget->isVisible()));
	stream.writeTextElement("DisableMultithreadedImages", QString::number(disable_multithreading_for_images));
	stream.writeTextElement("PreviousFrameQueueSize", QString::number(previous_queue_size));
	stream.writeTextElement("PreviousFrameQueueType", QString::number(previous_queue_type));
	stream.writeTextElement("UpcomingFrameQueueSize", QString::number(upcoming_queue_size));
	stream.writeTextElement("UpcomingFrameQueueType", QString::number(upcoming_queue_type));
    stream.writeTextElement("Loop", QString::number(loop));
    stream.writeTextElement("PauseAtOutPoint", QString::number(pause_at_out_point));

	stream.writeEndElement(); // configuration
	stream.writeEndDocument(); // doc
	f.close();
}
