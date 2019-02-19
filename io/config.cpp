/***

    Olive - Non-Linear Video Editor
    Copyright (C) 2019  Olive Team

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "config.h"

#include <QFile>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "panels/project.h"
#include "panels/panels.h"

#include "debug.h"

Config olive::CurrentConfig;
RuntimeConfig olive::CurrentRuntimeConfig;

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
    timecode_view(olive::kTimecodeDrop),
    show_title_safe_area(false),
    use_custom_title_safe_ratio(false),
    custom_title_safe_ratio(1),
    enable_drag_files_to_timeline(true),
    autoscale_by_default(false),
    recording_mode(2),
    enable_seek_to_import(false),
    enable_audio_scrubbing(true),
    drop_on_media_to_replace(true),
    autoscroll(olive::AUTOSCROLL_PAGE_SCROLL),
    audio_rate(48000),
    fast_seeking(false),
    hover_focus(false),
    project_view_type(olive::PROJECT_VIEW_TREE),
    set_name_with_marker(true),
    show_project_toolbar(false),
    previous_queue_size(3),
    previous_queue_type(olive::FRAME_QUEUE_TYPE_FRAMES),
    upcoming_queue_size(0.5),
    upcoming_queue_type(olive::FRAME_QUEUE_TYPE_SECONDS),
    loop(false),
    seek_also_selects(false),
    effect_textbox_lines(3),
    use_software_fallback(false),
    center_timeline_timecodes(true),
    waveform_resolution(64),
    thumbnail_resolution(120),
    add_default_effects_to_clips(true)
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
        } else if (stream.name() == "SeekAlsoSelects") {
          stream.readNext();
          seek_also_selects = (stream.text() == "1");
        } else if (stream.name() == "CSSPath") {
          stream.readNext();
          css_path = stream.text().toString();
        } else if (stream.name() == "EffectTextboxLines") {
          stream.readNext();
          effect_textbox_lines = stream.text().toInt();
        } else if (stream.name() == "UseSoftwareFallback") {
          stream.readNext();
          use_software_fallback = (stream.text() == "1");
        } else if (stream.name() == "CenterTimelineTimecodes") {
          stream.readNext();
          center_timeline_timecodes =  (stream.text() == "1");
        } else if (stream.name() == "PreferredAudioOutput") {
          stream.readNext();
          preferred_audio_output = stream.text().toString();
        } else if (stream.name() == "PreferredAudioInput") {
          stream.readNext();
          preferred_audio_input = stream.text().toString();
        } else if (stream.name() == "LanguageFile") {
          stream.readNext();
          language_file = stream.text().toString();
        } else if (stream.name() == "ThumbnailResolution") {
          stream.readNext();
          thumbnail_resolution = stream.text().toInt();
        } else if (stream.name() == "WaveformResolution") {
          stream.readNext();
          waveform_resolution = stream.text().toInt();
        } else if (stream.name() == "AddDefaultEffectsToClips") {
          stream.readNext();
          add_default_effects_to_clips = (stream.text() == "1");
        }
      }
    }
    if (stream.hasError()) {
      qCritical() << "Error parsing config XML." << stream.errorString();
    }

    f.close();
  }
}

void Config::save(QString path) {
  QFile f(path);
  if (!f.open(QIODevice::WriteOnly)) {
    qCritical() << "Could not save configuration";
    return;
  }

  QXmlStreamWriter stream(&f);
  stream.setAutoFormatting(true);
  stream.writeStartDocument(); // doc
  stream.writeStartElement("Configuration"); // configuration

  stream.writeTextElement("Version", QString::number(olive::kSaveVersion));
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
  stream.writeTextElement("PreviousFrameQueueSize", QString::number(previous_queue_size));
  stream.writeTextElement("PreviousFrameQueueType", QString::number(previous_queue_type));
  stream.writeTextElement("UpcomingFrameQueueSize", QString::number(upcoming_queue_size));
  stream.writeTextElement("UpcomingFrameQueueType", QString::number(upcoming_queue_type));
  stream.writeTextElement("Loop", QString::number(loop));
  stream.writeTextElement("SeekAlsoSelects", QString::number(seek_also_selects));
  stream.writeTextElement("CSSPath", css_path);
  stream.writeTextElement("EffectTextboxLines", QString::number(effect_textbox_lines));
  stream.writeTextElement("UseSoftwareFallback", QString::number(use_software_fallback));
  stream.writeTextElement("CenterTimelineTimecodes", QString::number(center_timeline_timecodes));
  stream.writeTextElement("PreferredAudioOutput", preferred_audio_output);
  stream.writeTextElement("PreferredAudioInput", preferred_audio_input);
  stream.writeTextElement("LanguageFile", language_file);
  stream.writeTextElement("ThumbnailResolution", QString::number(thumbnail_resolution));
  stream.writeTextElement("WaveformResolution", QString::number(waveform_resolution));
  stream.writeTextElement("AddDefaultEffectsToClips", QString::number(add_default_effects_to_clips));

  stream.writeEndElement(); // configuration
  stream.writeEndDocument(); // doc
  f.close();
}

RuntimeConfig::RuntimeConfig() :
  shaders_are_enabled(true),
  disable_blending(false)
{}
