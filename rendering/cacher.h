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

#ifndef CACHER_H
#define CACHER_H

#define __STDC_FORMAT_MACROS 1
#include <inttypes.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
}

#include <memory>
#include <QThread>
#include <QVector>
#include <QWaitCondition>
#include <QMutex>

#include "rendering/clipqueue.h"
#include "rendering/pixelformats.h"

class Clip;

/**
 * @brief The Cacher class
 *
 * For footage clips - usually the majority of clips - decoding can be strenuous on CPU and inconsistent in timing. As
 * a result, we keep a memory cache of upcoming frames that we fill in a background thread so they can be retrieved from
 * a rendering thread later. This class is the background thread filling up a clip's frame cache (also called a "queue"
 * since video files are usually stored with frames in linear chronological order). It involves decoding routines to
 * retrieve raw frames from the file (using libavformat/libavcodec), conversion routines to conform the raw frames to
 * RGBA/S16LE for the rest of the workflow (using libavfilter/libswscale/libswresample), and memory handling routines
 * for keeping the cache within limits defined by the user (see Config::upcoming_queue_type).
 *
 * Generally the Cacher workflow starts by calling Open() which will start the thread, open a file handle, and create a
 * decoding instance. Open() is usually called directly from the parent Clip's Clip::Open() and thus expects the
 * Clip::state_change_lock to be locked. It will unlock it when it's finished opening and is ready to start caching,
 * meaning Clip::state_change_lock can be used to synchronize threads.
 *
 * ---
 *
 * **For video:**
 *
 * After the Cacher has finished opening, request a frame by calling Cache(). Cache() will tell the Cacher
 * information about the current playback state, most importantly the current place in time according to the Sequence's
 * playhead. Cache() determines whether the requested frame is already in the queue, and then signals the Cacher thread
 * to cache ahead if there's room in the queue (and also remove old frames that are no longer necessary). To retrieve
 * the requested frame, call Retrieve().
 *
 * If Cache() found the frame already in the queue, Retrieve() will return immediately with this frame. Otherwise
 * Retrieve() may block while the cacher retrieves it. Therefore it is recommended never to call Retrieve() from
 * the main thread. Retrieve() may also return `nullptr` if there was an issue, e.g. the cacher failed to retrieve
 * the frame.
 *
 * **For audio:**
 *
 * After the Cacher has finished opening, calling Cache() will handle most of the work. It will decode the audio,
 * convert to the correct sample rate and format, reverse or adjust speed if necessary, and send it to the audio
 * buffer ready to be played by the output device. It is important to continually call Cache() as it doesn't get
 * signalled when more samples are available in the audio buffer. Instead, it'll check every time it's called and
 * fill as much of the buffer as it can.
 *
 * If the user seeks, ResetAudio() must be called to signal the Cacher to interrupt the current audio stream and
 * move somewhere else before continuing.
 *
 * ---
 *
 * Finally, when the Cacher/parent Clip are no longer in use, call Close() to free all memory and file handling
 * allocated for the cacher. You can choose whether to wait for Close() and all of its child processes to complete -
 * e.g. if you need to change something with the Clip or attached Footage that changes how it opens and want to be
 * thread-safe - or let the Close thread finish up on its own.
 *
 * Cacher expects to be multithreaded and all of its public functions are thread-safe.
 */
class Cacher : public QThread
{
  Q_OBJECT
public:
  /**
   * @brief Cacher Constructor
   *
   * Create Cacher object. The thread is not started here. To start it, call Open().
   *
   * @param c
   */
  Cacher(Clip* c);

  /**
   * @brief The main QThread loop
   *
   * Once the thread has started, all Cacher functions will be called from here until the Cacher closes at which point
   * it will close and exit gracefully.
   */
  void run();

  /**
   * @brief Open the cacher
   *
   * Starts the thread and all file/decode handlers. Really just sets some default values and starts the thread, which
   * will in turn call OpenWorker() at the start of its functions.
   *
   * Make sure Clip::state_change_lock is LOCKED before calling this function as the opening process will try to unlock
   * it when it's finished (leading to a crash if it's not already locked).
   */
  void Open();

  /**
   * @brief Request a frame to be cached
   *
   * For video, this function is part 1 of the Cache()/Retrieve() workflow. It signals the thread to start caching and
   * provides a few other details about the playback state. For optimization it'll also check the frame queue if it
   * already contains the requested frame and use it if so, potentially speeding up Retrieve() later on. Otherwise
   * it'll interrupt any currently caching operation and signal it to start again.
   * While Retrieve() will block until the correct frame is retrieved, this function will return fairly quickly (either
   * immediately if the frame was found in the queue, or once the cacher has restarted caching if not). This means
   * Cache() can be called from another thread and then that other thread can do other work while the cacher is
   * retrieving the frame, finally calling Retrieve() once the frame is absolutely necessary.
   *
   * For audio, this function will do all the work of signalling the thread to start caching and sending samples to
   * the output audio buffer. It's used in tandem with ResetAudio() when the Timeline header is changed abruptly.
   *
   * @param playhead
   *
   * The current Timeline played position in frames
   *
   * @param scrubbing
   *
   * **TRUE** if the user is currently scrubbing. **FALSE** if not.
   *
   * @param nests
   *
   * A hierarchy of nested sequences, if the playback traversed any to get to this clip.
   *
   * @param playback_speed
   *
   * The current playback speed (controlled by Shuttle Left/Stop/Right)
   */
  void Cache(long playhead, bool scrubbing, QVector<Clip*>& nests, int playback_speed);

  /**
   * @brief Retrieve frame requested by Cache()
   *
   * Part 2 of the Cache()/Retrieve() workflow, only used for video. Whichever frame was requested by Cache(), this
   * function will try to retrieve it. In most cases, this function will be pretty quick as the frame will be available
   * immediately from Cache()'s optimization or the cacher thread will be close to retrieving the correct frame anyway.
   * However it does block for however long it takes to retrieve the correct frame (if the cacher is running) so it's
   * not recommended to call this from any main/GUI thread.
   *
   * @return
   *
   * The frame requested by Cache(), or `nullptr` if there was an error (e.g. the cacher wasn't running and no frame was
   * available).
   */
  AVFrame* Retrieve();

  /**
   * @brief Close the cacher and free any allocated memory
   *
   * When the Cacher thread is no longer needed, Close() should be called in order to free system resources. This will
   * signal the thread to exit gracefully, but will not delete the thread object since the cacher may need to be
   * re-opened later by Open().
   *
   * @param wait_for_finish
   *
   * **TRUE** if this function should block the calling thread until the Clip has finished closing. Often necessary if
   * the Clip is being closed specifically to make changes to it.
   */
  void Close(bool wait_for_finish);

  /**
   * @brief Interrupt and reset audio state
   *
   * Used in tandem with Cache(), only for audio clips. Cache() will decode and send audio continually as it's
   * repeatedly called. If the audio stream needs to be interrupted and moved somewhere else for any reason
   * (e.g. the user seeked somewhere else), then it's necessary to call ResetAudio() to signal the cacher to
   * seek to the next place indicated by Cache().
   */
  void ResetAudio();

  /**
   * @brief Retrieve current media width
   *
   * In some situations, the actual media we're using may be a different resolution to how we're treating it (e.g.
   * lower resolution proxies). While most functions will happily treat the media as its original resolution, some
   * processes will need the absolute resolution from the file which can be acquired here.
   *
   * Only call after the thread has been opened by Open().
   *
   * @return
   *
   * The true width of the current video file.
   */
  int media_width();

  /**
   * @brief Retrieve current media height
   *
   * See media_width().
   *
   * Only call after the thread has been opened by Open().
   *
   * @return
   *
   * The true height of the current video file.
   */
  int media_height();

  /**
   * @brief Retrieve media time base
   *
   * For some timing operations, it's necessary to use the source media's timebase. Similar to media_width() and
   * media_height(), we need the accurate timebase from the file as a proxy's timebase may or may not be the same
   * as the source file.
   *
   * Only call after the thread has been opened by Open().
   *
   * @return
   *
   * The timebase of the file.
   */
  AVRational media_time_base();

  /**
   * @brief Get cacher queue object
   *
   * @return
   *
   * A pointer to the cacher's internal frame queue
   */
  ClipQueue* queue();

  /**
   * @brief Retrieve OpenGL information about this media's bit depth
   *
   * @return
   *
   * A olive::rendering::PixelFormat value corresponding to a member of olive::rendering::bit_depths.
   */
  const olive::PixelFormat& media_pixel_format();

private:
  /**
   * @brief Reference to the parent clip. Set in the constructor and never changed during this object's lifetime.
   */
  Clip* clip;

  /**
   * @brief Frame queue
   *
   * Valid fames are cached into this, which also does memory handling when necessary.
   */
  ClipQueue queue_;

  /**
   * @brief Main wait condition
   *
   * Used with Clip::cache_lock as the main block while the the Cacher thread isn't running. Wake this condition
   * to start caching.
   */
  QWaitCondition wait_cond_;

  /**
   * @brief Main thread wait condition
   *
   * Used with main_thread_lock_ to block Cache() while waiting for a response from the cacher thread.
   */
  QWaitCondition main_thread_wait_;

  /**
   * @brief Main thread mutex
   *
   * Used with main_thread_wait_ to block Cache() while waiting for a response from the cacher thread.
   */
  QMutex main_thread_lock_;

  /**
   * @brief Retrieve() wait condition
   *
   * Used with retrieve_lock_ to block Retrieve() if the cacher hasn't retrieved the correct frame yet.
   */
  QWaitCondition retrieve_wait_;

  /**
   * @brief Retrieve() mutex
   *
   * Used with retrieve_wait_ to block Retrieve() if the cacher hasn't retrieved the correct frame yet.
   */
  QMutex retrieve_lock_;

  /**
   * @brief Set and used by CacheAudioWorker if the decoder receives an EOF.
   *
   * Deprecated. CacheAudioWorker() is functional but probably should be rewritten.
   */
  bool reached_end;

  /**
   * @brief Current Sequence playhead set by Cache()
   */
  long playhead_;

  /**
   * @brief Current Sequence scrubbing state set by Cache()
   */
  bool scrubbing_;

  /**
   * @brief Current Sequence playback speed set by Cache()
   */
  int playback_speed_;

  /**
   * @brief Current nested Sequence hierarchy set by Cache()
   */
  QVector<Clip*> nests_;

  /**
   * @brief Signal cache to continue operation after one cycle rather than wait for another signal
   *
   * Each cycle of the cacher thread (see run()) will set this to false in the beginning. Each call of Cache() will set
   * this to **TRUE**. If this variable is **TRUE**, the cacher won't wait for another signal before starting the next
   * cache cycle, and will instead just start it.
   *
   * Used if Cache() is called and interrupts the cacher while it's already running so that the cacher will restart
   * itself automatically rather than wait for the next cache signal.
   */
  bool queued_;

  /**
   * @brief Interrupt the current cache cycle
   *
   * A cache cycle will cache several frames at a time. Since decoding can be strenuous and time consuming, the
   * cycle can be interrupted if it needs to abruptly start caching somewhere else. Best used in tandem with
   * queued_ to automatically start the next cache cycle.
   */
  bool interrupt_;

  // ffmpeg media handling
  /**
   * @brief FFmpeg format/file context - used for media decoding
   */
  AVFormatContext* formatCtx;

  /**
   * @brief FFmpeg decoder context - used for media decoding
   */
  AVCodecContext* codecCtx;

  /**
   * @brief FFmpeg stream - used for media decoding
   */
  AVStream* stream;

  /**
   * @brief FFmpeg packet - used for media decoding
   */
  AVPacket* pkt;

  /**
   * @brief FFmpeg frame - used for media decoding
   *
   * This is usually used as a raw decoded frame before the RGBA conversion/AVFilter stack. Converted/filtered frames go
   * into Cacher::queue.
   */
  AVFrame* frame_;

  /**
   * @brief Retrieved frame reference for Retrieve()
   *
   * If a frame was found by either Cache() or CacheVideoWorker(), it's set here. If no frame is ready yet, this is set
   * to `nullptr`.
   */
  AVFrame* retrieved_frame = nullptr;

  // converters/filters
  /**
   * @brief FFmpeg filter stack
   *
   * Used for conversion from the media's pixel format to RGBA for OpenGL. Also any other FFmpeg filters are implemented
   * here if necessary (e.g. yadif for deinterlacing). GLSL effects are preferred when available since FFmpeg filters
   * aren't always fast enough for realtime playback.
   */
  AVFilterGraph* filter_graph;

  /**
   * @brief FFmpeg buffer source
   *
   * Raw decoded frames are added to this for conversion/filtering
   */
  AVFilterContext* buffersrc_ctx;

  /**
   * @brief FFmpeg buffer sink
   *
   * Converted/filtered frames are retrieved from here and sent to Cacher::queue.
   */
  AVFilterContext* buffersink_ctx;

  /**
   * @brief FFmpeg codec reference
   */
  AVCodec* codec;

  /**
   * @brief Options set by the cacher for FFmpeg's decoders (settings like multithreading or other optimizations)
   */
  AVDictionary* opts;

  // audio playback variables
  /**
   * @brief Internal audio reset variable
   *
   * Set by AudioReset() and read by CacheAudioWorker() when the audio state needs to be interrupted and reset.
   */
  bool audio_reset_;

  /**
   * @brief Internal reverse target variable
   *
   * Used by CacheAudioWorker() to stitch audio frames together when reversing. Stores the current frame's timestamp
   * so it knows how much to decode up to when it backtracks and decodes the next samples.
   */
  int64_t reverse_target_;

  /**
   * @brief Internal frame sample index variable
   *
   * Used by CacheAudioWorker() to mark which part of the audio frame to read from
   */
  int frame_sample_index_;

  /**
   * @brief Internal audio buffer write variable
   *
   * Used by CacheAudioWorker() to mark which part of the audio buffer to write to
   */
  qint64 audio_buffer_write;

  /**
   * @brief Internal variable that holds the playhead the last time the audio state was reset
   */
  long audio_target_frame;

  /**
   * @brief Main while loop condition to determine whether thread should continue looping
   *
   * Open() sets this to **TRUE**, Close() sets this to **FALSE**. If it's false, the main loop in run() will exit and
   * the thread will exit cleanly. It's not recommended to set this variable directly, use Open() and Close() instead.
   */
  bool caching_;

  /**
   * @brief Internal variable for whether the current Cacher state is valid or not
   *
   * If there was an error opening the Cacher for any reason, this will be false.
   */
  bool is_valid_state_;

  /**
   * @brief Internal function for opening the file handles and decoder
   *
   * After the thread has started, it'll call this function to start all resources necessary for caching. Any
   * FFmpeg decoding variables and filters are set up here.
   *
   * This is
   * fundamentally different from Open(), this is only meant to be called within the cacher thread and never from
   * outside and doesn't start the thread like Open() does.
   */
  void OpenWorker();

  /**
   * @brief Internal function for starting a cache cycle
   *
   * This used to have more function, but now just differentiates between CacheVideoWorker() for video clips and
   * CacheAudioWorker() for audio clips.
   */
  void CacheWorker();

  /**
   * @brief Internal function for closing cacher
   *
   * Called if the main thread loop in run() exits by setting caching_ to **FALSE**. Free's up handles and memory
   * allocated by OpenWorker().
   */
  void CloseWorker();

  /**
   * @brief Internal function for resetting audio state
   *
   * This used to be a common function, but is now simply a legacy function for CacheAudioWorker(). Resets and
   * flushes decoders and seeks to the correct timestamp.
   */
  void Reset();

  /**
   * @brief Internal function for setting retrieved_frame and waking up any threads waiting for it
   *
   * @param f
   *
   * The frame to set as the retrieved frame.
   */
  void SetRetrievedFrame(AVFrame* f);

  /**
   * @brief Internal function to wake an external calling thread
   *
   * In some situations, Cache() may wait for the cacher to respond before returning. This is to assist in thread
   * synchronization, making sure the cacher has started working and has locked any resources it needs before any
   * other threads can access them (e.g. with a function like Retrieve() ). This must be called at the start of
   * any CacheVideoWorker() or CacheAudioWorker() control paths to ensure the render thread doesn't get stuck.
   */
  void WakeMainThread();

  /**
   * @brief Retrieve frame from decoder
   *
   * Retrieves the next decoded frame from the decoder. Depending on the source media, this frame may or may not be
   * suitable for usage later in the pipeline as it may or may not be the correct pixel/sample format. For a suitable
   * frame for the pipeline, use RetrieveFrameAndProcess() instead (which in turn uses this function anyway).
   *
   * @param f
   *
   * Frame buffer to decode frame into
   *
   * @return
   *
   * FFmpeg error code (>= 0 on success, a negative error code on failure)
   */
  int RetrieveFrameFromDecoder(AVFrame* f);

  /**
   * @brief Retrieve frame from decoder and run it through filter stack
   *
   * Retrieves the next decoded frame and runs it through the AVFilter stack to create an RGBA frame compatible with
   * the rest of the pipeline and OpenGL. Use this function if you need a ready-made frame.
   *
   * @param f
   *
   * A pointer to an AVFrame object. It does not need to be allocated, as this function allocates an AVFrame itself.
   * You'll also need to free it later with av_frame_free() (though ClipQueue will do this automatically if the frame is
   * added to it).
   *
   * @return
   *
   * FFmpeg error code (>= 0 on success, a negative error code on failure)
   */
  int RetrieveFrameAndProcess(AVFrame **f);

  /**
   * @brief Internal video caching function
   *
   * Performs one video cache cycle. Seeks the media and cleans old frames from the queue if necessary. Decodes frames
   * and adds them to the queue (after calculating whether they're necessary).
   */
  void CacheVideoWorker();

  /**
   * @brief Internal audio caching function
   *
   * Perform one audio cache cycle. Retrieves audio from decoder, reverses and changes speed if necessary, and sends
   * audio to the audio buffer which will later be sent to the audio output device.
   */
  void CacheAudioWorker();

  /**
   * @brief Internal function using the Cacher's known information to determine whether this media is playing in reverse
   */
  bool IsReversed();

  /**
   * @brief Internal struct holding bit depth information for the current media
   */
  olive::PixelFormat media_pixel_format_;
};

#endif // CACHER_H
