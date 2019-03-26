#ifndef TIMING_H
#define TIMING_H

#include <QString>
#include <QtMath>

class Clip;

/**
  * @brief Get timecode
  *
  * Get the current clip/media time from the Timeline playhead in seconds. For instance if the playhead was at the start
  * of a clip (whose in point wasn't trimmed), this would be 0.0 as it's the start of the clip/media;
  *
  * @param c
  *
  * Clip to get the timecode of
  *
  * @param playhead
  *
  * Sequence playhead to convert to a clip/media timecode
  *
  * @return
  *
  * Timecode in seconds
  */
double get_timecode(Clip  *c, long playhead);

/**
  * @brief Rescale a frame number between two frame rates
  *
  * Converts a frame number from one frame rate to its equivalent in another frame rate
  *
  * @param framenumber
  *
  * The frame number to convert
  *
  * @param source_frame_rate
  *
  * Frame rate that the frame number is currently in
  *
  * @param target_frame_rate
  *
  * Frame rate to convert to
  *
  * @return
  *
  * Rescaled frame number
  */
long rescale_frame_number(long framenumber, double source_frame_rate, double target_frame_rate);

/**
  * @brief Convert playhead frame number to a clip frame number
  *
  * Converts a Timeline playhead to a the current clip's frame. Equivalent to
  * `PLAYHEAD - CLIP_TIMELINE_IN + CLIP_MEDIA_IN`. All keyframes are in clip frames.
  *
  * @param c
  *
  * The clip to get the current frame number of
  *
  * @param playhead
  *
  * The current Timeline frame number
  *
  * @return
  *
  * The curren frame number of the clip at `playhead`
  */
long playhead_to_clip_frame(Clip* c, long playhead);

/**
  * @brief Converts the playhead to clip seconds
  *
  * Get the current timecode at the playhead in terms of clip seconds.
  *
  * FIXME: Possible duplicate of get_timecode()? Will need to research this more.
  *
  * @param c
  *
  * Clip to return clip seconds of.
  *
  * @param playhead
  *
  * Current Timeline playhead to convert to clip seconds
  *
  * @return
  *
  * Clip time in seconds
  */
double playhead_to_clip_seconds(Clip  *c, long playhead);

/**
  * @brief Convert seconds to FFmpeg timestamp
  *
  * Used for interaction with FFmpeg, converts seconds in a floating-point value to a timestamp in AVStream->time_base
  * units.
  *
  * @param c
  *
  * Clip to get timestamp of
  *
  * @param seconds
  *
  * Clip time in seconds
  *
  * @return
  *
  * An FFmpeg-compatible timestamp in AVStream->time_base units.
  */
int64_t seconds_to_timestamp(Clip* c, double seconds);

/**
  * @brief Convert Timeline playhead to FFmpeg timestamp
  *
  * Used for interaction with FFmpeg, converts the Timeline playhead to a timestamp in AVStream->time_base
  * units.
  *
  * @param c
  *
  * Clip to get timestamp of
  *
  * @param playhead
  *
  * Timeline playhead to convert to a timestamp
  *
  * @return
  *
  * An FFmpeg-compatible timestamp in AVStream->time_base units.
  */
int64_t playhead_to_timestamp(Clip  *c, long playhead);

bool frame_rate_is_droppable(double rate);
long timecode_to_frame(const QString& s, int view, double frame_rate);
QString frame_to_timecode(long f, int view, double frame_rate);

#endif // TIMING_H
