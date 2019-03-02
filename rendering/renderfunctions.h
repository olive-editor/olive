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

#ifndef RENDERFUNCTIONS_H
#define RENDERFUNCTIONS_H

#include <QOpenGLContext>
#include <QVector>
#include <QOpenGLShaderProgram>

#include "project/sequence.h"
#include "project/effect.h"

#include "panels/viewer.h"

/**
 * @brief The ComposeSequenceParams struct
 *
 * Struct sent to the compose_sequence() function.
 */
struct ComposeSequenceParams {

    /**
     * @brief Reference to the Viewer class that's calling compose_sequence()
     *
     * Primarily used for calling Viewer::play_wake() when appropriate.
     */
    Viewer* viewer;

    /**
     * @brief The OpenGL context to use while rendering.
     *
     * For video rendering, this must be a valid OpenGL context. For audio, this variable is never accessed.
     *
     * \see ComposeSequenceParams::video
     */
    QOpenGLContext* ctx;

    /**
     * @brief The sequence to compose
     *
     * In addition to clips, sequences also contain the playhead position so compose_sequence() knows which frame
     * to render.
     */
    SequencePtr seq;

    /**
     * @brief Array to store the nested sequence hierarchy
     *
     * Should be left empty. This array gets passed around compose_sequence() as it calls itself recursively to
     * handle nested sequences.
     */
    QVector<Clip*> nests;

    /**
     * @brief Set compose mode to video or audio
     *
     * **TRUE** if this function should render video, **FALSE** if this function should render audio.
     */
    bool video;

    /**
     * @brief Set to the Effect whose gizmos were chosen to be drawn on screen
     *
     * The currently active Effect that compose_sequence() will update the gizmos of.
     */
    Effect* gizmos;

    /**
     * @brief A variable that compose_sequence() will set to **TRUE** if any of the clips couldn't be shown.
     *
     * A footage item or shader may not be ready at the time this frame is drawn. If compose_sequence() couldn't draw
     * any of the clips in the scene, this variable is set to **TRUE** indicating that the image rendered is a
     * "best effort", but not the actual image.
     *
     * This variable should be checked after compose_sequence() and a repaint should be triggered if it's **TRUE**.
     *
     * \note This variable is probably bad design and is a relic of an earlier rendering backend. There may be a better
     * way to communicate this information.
     *
     * Additionally, since
     * compose_sequence() for video will now always run in a separate thread anyway, there's no real issue with
     * stalling it to wait for footage to complete opening or whatever may be lagging behind. A possible side effect
     * of this though is that the preview may become less responsive if it's stuck trying to render one frame. With
     * the current system, the preview may show incomplete frames occasionally but at least it will show something.
     * This may be preferable. See ComposeSequenceParams::single_threaded for a similar function that could be
     * removed.
     */
    bool texture_failed;

    /**
     * @brief Run all cachers in the same thread that compose_sequence() is in
     *
     * Standard behavior is that all clips cache frames in their own thread and signals are sent between
     * compose_sequence() and the clip's cacher thread regarding which frames to display and cache without stalling
     * the compose_sequence() thread. Setting this to **TRUE** will run all cachers in the same thread creating a
     * technically more "perfect" connection between them that will also stall the compose_sequence() thread. Used
     * when rendering as timing isn't as important as creating output frames as quickly as possible.
     *
     * \note Exporting should probably be rewritten without this. While running all the cachers in one thread makes
     * it easier to synchronize everything, export performance could probably benefit from keeping them in separate
     * threads and syncing up with them. See ComposeSequenceParams::texture_failed for a similar function that could
     * be removed.
     */
    bool wait_for_mutexes;

    /**
     * @brief Set the current playback speed (adjusted with Shuttle Left/Right)
     *
     * Only used for audio rendering to determine how many samples to skip in order to play audio at the correct speed.
     *
     * \see ComposeSequenceParams::video
     */
    int playback_speed;

    /**
     * @brief Blending mode shader
     *
     * Used only for video rendering. Never accessed with audio rendering.
     *
     * A program containing the current active
     * blending mode shader that can be bound during rendering. Must be compiled and linked beforehand. See
     * RenderThread::blend_mode_program for how this is properly set up.
     *
     * \see ComposeSequenceParams::video
     */
    QOpenGLShaderProgram* blend_mode_program;

    /**
     * @brief Premultiply alpha shader
     *
     * Used only for video rendering. Never accessed with audio rendering.
     *
     * compose_sequence()'s internal composition
     * expects premultipled alpha, but it will pre-emptively multiply any footage that is not set as already
     * premultiplied (see Footage::alpha_is_premultiplied) using this shader. Must be compiled and linked beforehand.
     * See RenderThread::premultiply_program for how this is properly set up.
     */
    QOpenGLShaderProgram* premultiply_program;

    /**
     * @brief The OpenGL framebuffer object that the final texture to be shown is rendered to.
     *
     * Used only for video rendering. Never accessed with audio rendering.
     *
     * When compose_sequence() is rendering the final image, this framebuffer will be bound.
     */
    GLuint main_buffer;

    /**
     * @brief The attachment to the framebuffer in main_buffer
     *
     * Used only for video rendering. Never accessed with audio rendering.
     *
     * The OpenGL texture attached to the framebuffer referenced by main_buffer.
     */
    GLuint main_attachment;

    /**
     * @brief Backend OpenGL framebuffer 1 used for further processing before rendering to main_buffer
     *
     * In some situations, compose_sequence() will do some processing through shaders that requires "ping-ponging"
     * between framebuffers. backend_buffer1 and backend_buffer2 are used for this purpose.
     */
    GLuint backend_buffer1;

    /**
     * @brief Backend OpenGL framebuffer 1's texture attachment
     *
     * The texture that ComposeSequenceParams::backend_buffer1 renders to. Bound and drawn to
     * ComposeSequenceParams::backend_buffer2 to "ping-pong" between them and various shaders.
     */
    GLuint backend_attachment1;

    /**
     * @brief Backend OpenGL framebuffer 2 used for further processing before rendering to main_buffer
     *
     * In some situations, compose_sequence() will do some processing through shaders that requires "ping-ponging"
     * between framebuffers. backend_buffer1 and backend_buffer2 are used for this purpose.
     */
    GLuint backend_buffer2;

    /**
     * @brief Backend OpenGL framebuffer 2's texture attachment
     *
     * The texture that ComposeSequenceParams::backend_buffer2 renders to. Bound and drawn to
     * ComposeSequenceParams::backend_buffer1 to "ping-pong" between them and various shaders.
     */
    GLuint backend_attachment2;

    /**
     * @brief OpenGL shader containing OpenColorIO shader information
     */
    QOpenGLShaderProgram* ocio_shader;

    /**
     * @brief OpenGL texture containing LUT obtained form OpenColorIO
     */
    GLuint ocio_lut_texture;
};

/**
 * @brief Compose a frame of a given sequence
 *
 * For any given Sequence, this function will render the current frame indicated by Sequence::playhead. Will
 * automatically open and close clips (memory allocation and file handles) as necessary, communicate with the
 * Clip::cacher objects to retrieve upcoming frames and store them in memory, run Effect processing functions, and
 * finally composite all the currently active clips together into a final texture.
 *
 * Will sometimes render a frame incomplete or inaccurately, e.g. if a video file hadn't finished opening by the time
 * of the render or a clip's cacher didn't have the requested frame available at the time of the render. If so,
 * the `texture_failed` variable of `params` will be set to **TRUE**. Check this after calling compose_sequence() and
 * if it is **TRUE**, compose_sequence() should be called again later to attempt another render (unless the Sequence
 * is being played, in which case just play the next frame rather than redrawing an old frame).
 *
 * @param params
 *
 * A struct of parameters to use while rendering.
 *
 * @return A reference to the OpenGL texture resulting from the render. Will usually be equal to
 * ComposeSequenceParams::main_attachment unless it's rendering a nested sequence, in which case it'll be a reference
 * to one of the textures referenced by Clip::fbo. Can be used directly to draw the rendered frame.
 */
GLuint compose_sequence(ComposeSequenceParams &params);

/**
 * @brief Convenience wrapper function for compose_sequence() to render audio
 *
 * Much of the functionality provided (and parameters required) by compose_sequence() is only useful/necessary for
 * video rendering. For audio rendering, this function is easier to handle and will correctly set up
 * compose_sequence() to render audio without the cumbersome effort of setting up a ComposeSequenceParams object.
 *
 * @param viewer
 *
 * The Viewer object calling this function
 *
 * @param seq
 *
 * The Sequence whose audio to render.
 *
 * @param playback_speed
 *
 * The current playback speed (controlled by Shuttle Left/Right)
 *
 * @param
 *
 * Whether to wait for media to open or simply fail if the media is not yet open. This should usually be **FALSE**.
 */
void compose_audio(Viewer* viewer, SequencePtr seq, int playback_speed, bool wait_for_mutexes);

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
double get_timecode(Clip *c, long playhead);

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
double playhead_to_clip_seconds(Clip *c, long playhead);

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
int64_t playhead_to_timestamp(Clip *c, long playhead);

/**
 * @brief Close all open clips in a Sequence
 *
 * Closes any currently open clips on a Sequence and waits for them to close before returning. This may be slow as a
 * result on large Sequence objects. If a Clip is a nested Sequence, this function calls itself recursively on that
 * Sequence too.
 *
 * @param s
 *
 * The Sequence to close all clips on.
 */
void close_active_clips(SequencePtr s);

#endif // RENDERFUNCTIONS_H
