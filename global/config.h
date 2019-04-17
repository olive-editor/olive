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

#ifndef CONFIG_H
#define CONFIG_H

#include <QString>

#include "ui/styling.h"

namespace olive {
  /**
   * @brief Version identifier for saved projects
   *
   * This constant is used to identify what version of Olive a project file was saved with. Every project file
   * is saved with the current version number and the version is checked whenever an Olive project is loaded to
   * determine how compatible it'll be with the current version.
   *
   * Sometimes this version identifier is used to invoke backwards compatibility in order to keep older project files
   * able to load, but in this early rapidly developing stage, often backwards compatibility is abandoned. Ideally
   * in the future, a class should be made that's able to "convert" an older project into one that the current
   * loading system understands (so that the loading system doesn't get too bloated with backwards compatibility
   * functions).
   */
  const int kSaveVersion = 190219; // YYMMDD

  /**
   * @brief Minimum project version that this version of Olive can open
   *
   * When loading a project, the project's version number is actually checked whether it is somewhere between
   * kSaveVersion and this value (inclusive). This is used if the current version of Olive contains backwards
   * compatibility functionality for older project versions, and is bumped up if such backwards compatibility is
   * ever removed.
   *
   * As stated in kSaveVersion documentation, ideally in the future, a system would be in place to account for all
   * project version differences and bring them up to date for the current loading algorithm. This means ideally, this
   * constant stays the same forever, but in this early stage it's not strictly necessary.
   */
  const int kMinimumSaveVersion = 190219; // lowest compatible project version

  /**
   * @brief The TimecodeType enum
   *
   * Frame numbers can be displayed in various different ways. The timecode_to_frame() and frame_to_timecode()
   * functions (which should be used for all frame <-> timecode conversions) respond to the timecode display type
   * set in Config::timecode_view corresponding to a value from this enum.
   */
  enum TimecodeType {
    /** Show frame number as a drop-frame timecode */
    kTimecodeDrop,

    /** Show frame number as a non-drop-frame timecode */
    kTimecodeNonDrop,

    /** Show frame number as-is with no modifications */
    kTimecodeFrames,

    /** Show frame number as milliseconds */
    kTimecodeMilliseconds
  };

  /**
   * @brief The RecordingMode enum
   *
   * Olive currently supports recording mono or stereo and gives users the option of which mode to use when
   * recording audio in-app. Audio recording responds to Config::recording_mode to a value from this enum.
   */
  enum RecordingMode {
    /** Record all audio in mono */
    RECORD_MODE_MONO,

    /** Record all audio in stereo */
    RECORD_MODE_STEREO
  };

  /**
   * @brief The AutoScrollMode enum
   *
   * The Timeline in Olive can automatically scroll to follow the playhead when the sequence is playing.
   * The Timeline will respond to Config::autoscroll set to a value from this enum.
   */
  enum AutoScrollMode {
    /** Don't auto-scroll, scrolling will not follow the playhead */
    AUTOSCROLL_NO_SCROLL,

    /** Page auto-scroll (default), if the playhead goes off-screen while playing, the scroll will jump ahead
     * one "page" to follow it */
    AUTOSCROLL_PAGE_SCROLL,

    /** Smooth auto-scroll, Olive will scroll to keep the playhead in the center of the screen at all times while
     * playing */
    AUTOSCROLL_SMOOTH_SCROLL
  };

  /**
   * @brief The ProjectView enum
   *
   * The media in the Project panel can be displayed as a tree hierarchy or as an icon view. The Project panel
   * responds to Config::project_view_type set to a value from this enum.
   */
  enum ProjectView {
    /** Display project media in tree hierarchy */
    PROJECT_VIEW_TREE,

    /** Display project media in icon browser */
    PROJECT_VIEW_ICON,

    /** Display project media in list browser */
    PROJECT_VIEW_LIST
  };

  /**
   * @brief The FrameQueueType enum
   *
   * Olive keeps a "frame queue" in memory to allow smoother playback/seeking. In order to give users control over
   * the amount of memory consumption vs. playback performance, they can control how many frames are cached into
   * memory. For extra fidelity, they can choose this value as a metric of either frames or seconds.
   *
   * The playback engine (playback/playback.h) responds to both Config::previous_queue_type and
   * Config::upcoming_queue_type set to a value from this enum.
   */
  enum FrameQueueType {
    /** Queue size value is in frames */
    FRAME_QUEUE_TYPE_FRAMES,

    /** Queue size value is in seconds */
    FRAME_QUEUE_TYPE_SECONDS
  };
}

/**
 * @brief The Config struct
 *
 * This struct handles any configuration that should persist between restarting Olive. It contains several variables
 * as well as functions that load and save all the variables to file.
 */
struct Config {
  /**
   * @brief Config Constructor
   *
   * Sets all configuration variables to their defaults.
   */
  Config();

  /**
   * @brief Show track lines
   *
   * **TRUE** if the Timeline should show lines between tracks.
   */
  bool show_track_lines;

  /**
   * @brief The scroll wheel zooms rather than scrolls
   *
   * **TRUE** if the scroll wheel should zoom in and out rather than scroll up and down.
   * The Control key temporarily toggles this setting.
   */
  bool scroll_zooms;

  /**
   * @brief Edit tool selects links
   *
   * **TRUE** if the edit tool should also select links when the user selects a clip.
   */
  bool edit_tool_selects_links;

  /**
   * @brief Edit tool also seeks
   *
   * **TRUE** if using the edit tool should also seek the sequence's playhead
   */
  bool edit_tool_also_seeks;

  /**
   * @brief Selecting also seeks
   *
   * **TRUE** if the playhead should automatically seek to the start of any clip that gets selected
   */
  bool select_also_seeks;

  /**
   * @brief Paste also seeks
   *
   * **TRUE** if the playhead should seek to the end of clips that are pasted
   */
  bool paste_seeks;

  /**
   * @brief Image sequence formats
   *
   * A '|' separated list of file extensions that Olive will perform an image sequence test heuristic on when importing
   */
  QString img_seq_formats;

  /**
   * @brief Use rectified waveforms
   *
   * **TRUE** if Olive should display waveforms as "rectified". Rectified waveforms start at the bottom rather than
   * from the middle.
   */
  bool rectified_waveforms;

  /**
   * @brief Default transition length
   *
   * The default transition length used when making a transition without the transition tool
   */
  int default_transition_length;

  /**
   * @brief Timecode display mode
   *
   * The display mode with which timecode_to_frame() and frame_to_timecode() will use to convert frame numbers to
   * more human-readable values.
   *
   * Set to a member of enum TimecodeType.
   */
  int timecode_view;

  /**
   * @brief Show title/action safe area
   *
   * **TRUE** if the title/action safe area should be shown on the Viewer.
   */
  bool show_title_safe_area;

  /**
   * @brief Use custom title/action safe area aspect ratio
   *
   * **TRUE** if the title/action save area should use a custom aspect ratio
   */
  bool use_custom_title_safe_ratio;

  /**
   * @brief Custom title/action safe area aspect ratio
   *
   * If Config::use_custom_title_safe_ratio is true, this is the aspect ratio to use.
   *
   * Set to the result of an aspect ratio division (i.e. for 4:3 set to 1.333333 (4.0 / 3.0)
   */
  double custom_title_safe_ratio;

  /**
   * @brief Enable dragging files outside Olive directly into the Timeline
   *
   * **TRUE** if the Timeline should respond to files dropped from outside Olive.
   */
  bool enable_drag_files_to_timeline;

  /**
   * @brief Auto-scale by default
   *
   * **TRUE** if clips imported into the timeline should have Clip::autoscale **TRUE** by default. If a Clip is
   * smaller or larger than the Sequence, auto-scale will automatically resize it to fit to the Sequence boundaries.
   */
  bool autoscale_by_default;

  /**
   * @brief Recording mode/channel layout
   *
   * When recording audio within Olive, use this mode/channel layout (e.g. mono or stereo).
   *
   * Set to a member of enum RecordingMode.
   */
  int recording_mode;

  /**
   * @brief Enable seek to import
   *
   * **TRUE** if the playhead should automatically seek to any newly imported clips
   */
  bool enable_seek_to_import;

  /**
   * @brief Enable audio scrubbing
   *
   * **TRUE** if audio should "scrub" as the user drags the playhead around
   */
  bool enable_audio_scrubbing;

  /**
   * @brief Enable drop on media to replace
   *
   * **TRUE** if dropping a file from outside Olive onto a media item in the Project panel should prompt the user
   * whether the dropped file should replace the media item that the file was dropped on.
   */
  bool drop_on_media_to_replace;

  /**
   * @brief Auto-scroll mode
   *
   * The Timeline behavior regarding scrolling to keep the playhead within view while a Sequence is playing.
   *
   * Set to a member of enum AutoScrollMode.
   */
  int autoscroll;

  /**
   * @brief Current audio sample rate
   *
   * The sample rate to set the audio output device to. Also used as the value to resample audio to during playback
   * (but not during rendering).
   */
  int audio_rate;

  /**
   * @brief Enable hover focus
   *
   * Default behavior is panels are focused (and therefore respond to certain keyboard shortcuts)when they are clicked
   * on, but Olive also supports panels being considered "focused" if the mouse is hovered over them.
   *
   * **TRUE** to enable hover focus mode.
   */
  bool hover_focus;

  /**
   * @brief Project view type
   *
   * Whether to show media in the Project panel as a tree hierarchy or as a browser of icons.
   *
   * Set to a member of enum ProjectView.
   */
  int project_view_type;

  /**
   * @brief Ask for a marker name when setting a marker
   *
   * **TRUE** if Olive should ask the user to name a marker when setting one. **FALSE** if markers should just be
   * created without asking.
   */
  bool set_name_with_marker;

  /**
   * @brief Show the project toolbar
   *
   * Olive has an optional toolbar for the Project panel.
   *
   * Set to **TRUE** to show it.
   */
  bool show_project_toolbar;

  /**
   * @brief Previous frame queue size
   *
   * Olive caches frames in memory to improve playback performance (see enum FrameQueueType documentation for more
   * details). This variable states how many frames to keep in memory prior to the playhead (in most cases, frames that
   * have already been played, but are kept in memory in case the user wants to backtrack at any time).
   *
   * This value corresponds to Config::previous_queue_type.
   */
  double previous_queue_size;

  /**
   * @brief Previous frame queue type
   *
   * The metric of which Config::previous_queue_size is using. For example, if Config::previous_queue_size is
   * 3, this variable states whether that is 3 frames or 3 seconds.
   *
   * Set to a member of enum FrameQueueType.
   */
  int previous_queue_type;

  /**
   * @brief Upcoming frame queue size
   *
   * Olive caches frames in memory to improve playback performance (see enum FrameQueueType documentation for more
   * details). This variable states how many upcoming frames are stored in memory. Generally this value will be higher
   * than Config::previous_queue_size since the user will be playing forwards most of the time.
   *
   * This value corresponds to Config::upcoming_queue_type.
   */
  double upcoming_queue_size;

  /**
   * @brief Upcoming frame queue type
   *
   * The metric of which Config::upcoming_queue_size is using. For example, if Config::upcoming_queue_size is
   * 3, this variable states whether that is 3 frames or 3 seconds.
   *
   * Set to a member of enum FrameQueueType.
   */
  int upcoming_queue_type;

  /**
   * @brief Loop
   *
   * If an in/out point are set on the Sequence (Sequence::using_workarea is **TRUE**), set this to **TRUE** if Olive
   * should rewind to the in point and start playing again after it reaches the out point repeatedly until the user
   * pauses.
   */
  bool loop;

  /**
   * @brief Seeking also selects
   *
   * Olive supports automatically selecting clips that the playhead is currently touching for a more efficient workflow.
   *
   * **TRUE** if this mode should be enabled.
   */
  bool seek_also_selects;

  /**
   * @brief Automatically seek to the beginning of a sequence if the user plays beyond the end of it
   *
   * TRUE if this behavior should be enabled.
   */
  bool auto_seek_to_beginning;

  /**
   * @brief CSS Path
   *
   * The URL to a CSS file if the user has loaded a custom stylesheet in. **EMPTY** if the user has not set a
   * stylesheet.
   */
  QString css_path;

  /**
   * @brief Number of lines that an Effect's textbox has
   *
   * The height of a Effect's textbox field in terms of lines.
   *
   * Set to a value >= 1
   */
  int effect_textbox_lines;

  /**
   * @brief Use software fallbacks when possible
   *
   * Olive uses a lot of OpenGL-based hardware acceleration for performance. Some older hardware has difficulty
   * supporting this functionality, so some of it has software-based (not hardware accelerated) fallbacks for these
   * users.
   *
   * **TRUE** if Olive should prefer software fallbacks to hardware acceleration when they're available.
   */
  bool use_software_fallback;

  /**
   * @brief Center Timeline timecodes
   *
   * By default, Olive shows timecodes in the TimelineHeader centered to the corresponding frame point. This may not
   * always be desirable as, for example, this forces the initial 00:00:00;00 timecode's left half to be cut off.
   * Olive supports aligning the timecode to the right of the frame rather than the center to address this.
   */
  bool center_timeline_timecodes;

  /**
   * @brief Preferred audio output device
   *
   * Sets the audio device Olive should use to output audio to.
   *
   * Set to the name of the audio device or **EMPTY** to try using the default.
   */
  QString preferred_audio_output;

  /**
   * @brief Preferred audio input device
   *
   * Sets the audio device Olive should use to input audio from.
   *
   * Set to the name of the audio device or **EMPTY** to try using the default.
   */
  QString preferred_audio_input;

  /**
   * @brief Language/translation file
   *
   * Sets the translation file to load to display Olive in a different language.
   *
   * Set to the URL of the language file to load, or **EMPTY** to use default en-US language.
   */
  QString language_file;

  /**
   * @brief Waveform resolution
   *
   * Sets how detailed the waveforms should be in the Timeline. Higher value = more detail.
   *
   * Specifically sets how many samples per second should be "cached" for preview. If the waveforms are too blocky,
   * set this higher. If Timeline performance is slow, set this lower.
   */
  int waveform_resolution;

  /**
   * @brief Thumbnail resolution
   *
   * The vertical pixel height to use for generating thumbnails.
   */
  int thumbnail_resolution;

  /**
   * @brief Add default effects to clips
   *
   * **TRUE** if new clips imported into the Timeline should have a set of default effects (TransformEffect,
   * VolumeEffect, and PanEffect) added to them by default.
   */
  bool add_default_effects_to_clips;

  /**
   * @brief Invert Timeline scroll axes
   *
   * **TRUE** if scrolling vertically on the Timeline should scroll it horizontally
   */
  bool invert_timeline_scroll_axes;

  /**
   * @brief Style to use when theming Olive.
   *
   * Set to a member of olive::styling::Style.
   */
  olive::styling::Style style;

  /**
   * @brief Use native menu styling
   *
   * Use native styling on menus rather than cross-platform Fusion.
   */
  bool use_native_menu_styling;

  /**
   * @brief Default Sequence video width
   */
  int default_sequence_width;

  /**
   * @brief Default Sequence video height
   */
  int default_sequence_height;

  /**
   * @brief Default Sequence video frame rate
   */
  double default_sequence_framerate;

  /**
   * @brief Default Sequence audio frequency
   */
  int default_sequence_audio_frequency;

  /**
   * @brief Default Sequence audio channel layout
   */
  int default_sequence_audio_channel_layout;

  /**
   * @brief Sets whether panels should load locked or not
   */
  bool locked_panels;

  /**
   * @brief Load config from file
   *
   * Load configuration parameters from file
   *
   * @param path
   *
   * URL to the configuration file to load
   */
  void load(QString path);

  /**
   * @brief Save config to file
   *
   * Save current configuration parameters to file
   *
   * @param path
   *
   * URL to save the configuration file to.
   */
  void save(QString path);
};

/**
 * @brief The RuntimeConfig struct
 *
 * This struct handles any configuration that's set as a command-line argument to Olive, and shouldn't be persistent
 * between restarts of Olive.
 */
struct RuntimeConfig {
  /**
   * @brief RuntimeConfig Constructor
   *
   * Sets default runtime configuration
   */
  RuntimeConfig();

  /**
   * @brief Enable shaders
   *
   * Debugging tool. Set to **FALSE** to bypass OpenGL shaders.
   */
  bool shaders_are_enabled;

  /**
   * @brief Disable blending modes
   *
   * Some users had difficulty utilizing blending modes (provided by shaders). Set this to **TRUE** to bypass
   * shader-based blending modes and utilize standard (less versatile) OpenGL blending instead.
   */
  bool disable_blending;

  /**
   * @brief Load an external translation file
   *
   * Overrides Config::language_file and sets the path to a language file to use.
   */
  QString external_translation_file;
};

namespace olive {
extern Config CurrentConfig;
extern RuntimeConfig CurrentRuntimeConfig;
}

#endif // CONFIG_H
