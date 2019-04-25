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

#include "cacher.h"

#ifndef __STDC_FORMAT_MACROS
// For some reason the Windows AppVeyor build fails to find PRIx64 without this definition and including
// <inttypes.h> Maybe something to do with the GCC version being used? Either way, that's why it's here.
#define __STDC_FORMAT_MACROS 1
#endif

#include <inttypes.h>

#include <QOpenGLFramebufferObject>
#include <QtMath>
#include <QAudioOutput>
#include <QStatusBar>
#include <math.h>

#include "project/projectelements.h"
#include "rendering/audio.h"
#include "rendering/renderfunctions.h"
#include "panels/panels.h"
#include "global/config.h"
#include "global/debug.h"
#include "ui/mainwindow.h"

// Enable verbose audio messages - good for debugging reversed audio
//#define AUDIOWARNINGS

const AVPixelFormat kDestPixFmt = AV_PIX_FMT_RGBA;
const AVSampleFormat kDestSampleFmt = AV_SAMPLE_FMT_S16;

double bytes_to_seconds(int nb_bytes, int nb_channels, int sample_rate) {
  return (double(nb_bytes >> 1) / nb_channels / sample_rate);
}

void apply_audio_effects(Clip* clip, double timecode_start, AVFrame* frame, int nb_bytes, QVector<Clip*> nests) {
  // perform all audio effects
  double timecode_end;
  timecode_end = timecode_start + bytes_to_seconds(nb_bytes, frame->channels, frame->sample_rate);

  for (int j=0;j<clip->effects.size();j++) {
    Effect* e = clip->effects.at(j).get();
    if (e->IsEnabled()) {
      e->process_audio(timecode_start, timecode_end, frame->data[0], nb_bytes, 2);
    }
  }
  if (clip->opening_transition != nullptr) {
    if (clip->media() != nullptr && clip->media()->get_type() == MEDIA_TYPE_FOOTAGE) {
      double transition_start = (clip->clip_in(true) / clip->sequence->frame_rate);
      double transition_end = (clip->clip_in(true) + clip->opening_transition->get_length()) / clip->sequence->frame_rate;
      if (timecode_end < transition_end) {
        double adjustment = transition_end - transition_start;
        double adjusted_range_start = (timecode_start - transition_start) / adjustment;
        double adjusted_range_end = (timecode_end - transition_start) / adjustment;
        clip->opening_transition->process_audio(adjusted_range_start, adjusted_range_end, frame->data[0], nb_bytes, kTransitionOpening);
      }
    }
  }
  if (clip->closing_transition != nullptr) {
    if (clip->media() != nullptr && clip->media()->get_type() == MEDIA_TYPE_FOOTAGE) {
      long length_with_transitions = clip->timeline_out(true) - clip->timeline_in(true);
      double transition_start = (clip->clip_in(true) + length_with_transitions - clip->closing_transition->get_length()) / clip->sequence->frame_rate;
      double transition_end = (clip->clip_in(true) + length_with_transitions) / clip->sequence->frame_rate;
      if (timecode_start > transition_start) {
        double adjustment = transition_end - transition_start;
        double adjusted_range_start = (timecode_start - transition_start) / adjustment;
        double adjusted_range_end = (timecode_end - transition_start) / adjustment;
        clip->closing_transition->process_audio(adjusted_range_start, adjusted_range_end, frame->data[0], nb_bytes, kTransitionClosing);
      }
    }
  }

  if (!nests.isEmpty()) {
    Clip* next_nest = nests.last();
    nests.removeLast();
    apply_audio_effects(next_nest,
                        timecode_start + (double(clip->timeline_in(true)-clip->clip_in(true))/clip->sequence->frame_rate),
                        frame,
                        nb_bytes,
                        nests);
  }
}

#define AUDIO_BUFFER_PADDING 2048
void Cacher::CacheAudioWorker() {
  // main thread waits until cacher starts fully, wake it up here
  WakeMainThread();

  bool audio_just_reset = false;

  // for audio clips, something may have triggered an audio reset (common if the user seeked)
  if (audio_reset_) {
    Reset();
    audio_reset_ = false;
    audio_just_reset = true;
  }

  long timeline_in = clip->timeline_in(true);
  long timeline_out = clip->timeline_out(true);
  long target_frame = audio_target_frame;

  bool temp_reverse = (playback_speed_ < 0);
  bool reverse_audio = IsReversed();

  long frame_skip = 0;
  double last_fr = clip->sequence->frame_rate;
  if (!nests_.isEmpty()) {
    for (int i=nests_.size()-1;i>=0;i--) {
      timeline_in = rescale_frame_number(timeline_in, last_fr, nests_.at(i)->sequence->frame_rate) + nests_.at(i)->timeline_in(true) - nests_.at(i)->clip_in(true);
      timeline_out = rescale_frame_number(timeline_out, last_fr, nests_.at(i)->sequence->frame_rate) + nests_.at(i)->timeline_in(true) - nests_.at(i)->clip_in(true);
      target_frame = rescale_frame_number(target_frame, last_fr, nests_.at(i)->sequence->frame_rate) + nests_.at(i)->timeline_in(true) - nests_.at(i)->clip_in(true);

      timeline_out = qMin(timeline_out, nests_.at(i)->timeline_out(true));

      frame_skip = rescale_frame_number(frame_skip, last_fr, nests_.at(i)->sequence->frame_rate);

      long validator = nests_.at(i)->timeline_in(true) - timeline_in;
      if (validator > 0) {
        frame_skip += validator;
        //timeline_in = nests_.at(i)->timeline_in(true);
      }

      last_fr = nests_.at(i)->sequence->frame_rate;
    }
  }

  if (temp_reverse) {
    long seq_end = olive::ActiveSequence->getEndFrame();
    timeline_in = seq_end - timeline_in;
    timeline_out = seq_end - timeline_out;
    target_frame = seq_end - target_frame;

    long temp = timeline_in;
    timeline_in = timeline_out;
    timeline_out = temp;
  }

  while (true) {
    AVFrame* frame;
    int nb_bytes = INT_MAX;

    if (clip->media() == nullptr) {
      frame = frame_;
      nb_bytes = frame->nb_samples * av_get_bytes_per_sample(static_cast<AVSampleFormat>(frame->format)) * frame->channels;
      while ((frame_sample_index_ == -1 || frame_sample_index_ >= nb_bytes) && nb_bytes > 0) {
        // create "new frame"
        memset(frame_->data[0], 0, nb_bytes);
        apply_audio_effects(clip, bytes_to_seconds(frame->pts, frame->channels, frame->sample_rate), frame, nb_bytes, nests_);
        frame_->pts += nb_bytes;
        frame_sample_index_ = 0;
        if (audio_buffer_write == 0) {
          audio_buffer_write = get_buffer_offset_from_frame(last_fr, qMax(timeline_in, target_frame));
        }
        int offset = audio_ibuffer_read - audio_buffer_write;
        if (offset > 0) {
          audio_buffer_write += offset;
          frame_sample_index_ += offset;
        }
      }
    } else if (clip->media()->get_type() == MEDIA_TYPE_FOOTAGE) {
      double timebase = av_q2d(stream->time_base);

      frame = queue_.at(0);

      // retrieve frame
      bool new_frame = false;
      while ((frame_sample_index_ == -1 || frame_sample_index_ >= nb_bytes) && nb_bytes > 0) {

        // no more audio left in frame, get a new one
        if (!reached_end) {
          int loop = 0;

          if (reverse_audio && !audio_just_reset) {
            avcodec_flush_buffers(codecCtx);
            reached_end = false;
            int64_t backtrack_seek = qMax(reverse_target_ - static_cast<int64_t>(av_q2d(av_inv_q(stream->time_base))),
                                          static_cast<int64_t>(0));
            av_seek_frame(formatCtx, stream->index, backtrack_seek, AVSEEK_FLAG_BACKWARD);
#ifdef AUDIOWARNINGS
            if (backtrack_seek == 0) {
              dout << "backtracked to 0";
            }
#endif
          }

          do {
            av_frame_unref(frame);

            int ret;

            while ((ret = av_buffersink_get_frame(buffersink_ctx, frame)) == AVERROR(EAGAIN)) {
              ret = RetrieveFrameFromDecoder(frame_);
              if (ret >= 0) {
                if ((ret = av_buffersrc_add_frame_flags(buffersrc_ctx, frame_, AV_BUFFERSRC_FLAG_KEEP_REF)) < 0) {
                  qCritical() << "Could not feed filtergraph -" << ret;
                  break;
                }
              } else {
                if (ret == AVERROR_EOF) {
#ifdef AUDIOWARNINGS
                  dout << "reached EOF while reading";
#endif
                  // TODO revise usage of reached_end in audio
                  if (!reverse_audio) {
                    reached_end = true;
                  } else {
                  }
                } else {
                  qWarning() << "Raw audio frame data could not be retrieved." << ret;
                  reached_end = true;
                }
                break;
              }
            }

            if (ret < 0) {
              if (ret != AVERROR_EOF) {
                qCritical() << "Could not pull from filtergraph";
                reached_end = true;
                break;
              } else {
#ifdef AUDIOWARNINGS
                dout << "reached EOF while pulling from filtergraph";
#endif
                if (!reverse_audio) break;
              }
            }

            if (reverse_audio) {
              if (loop > 1) {
                AVFrame* rev_frame = queue_.at(1);
                if (ret != AVERROR_EOF) {
                  if (loop == 2) {
#ifdef AUDIOWARNINGS
                    dout << "starting rev_frame";
#endif
                    rev_frame->nb_samples = 0;
                    rev_frame->pts = frame_->pkt_pts;
                  }
                  int offset = rev_frame->nb_samples * av_get_bytes_per_sample(static_cast<AVSampleFormat>(rev_frame->format)) * rev_frame->channels;
#ifdef AUDIOWARNINGS
                  dout << "offset 1:" << offset;
                  dout << "retrieved samples:" << frame->nb_samples << "size:" << (frame->nb_samples * av_get_bytes_per_sample(static_cast<AVSampleFormat>(frame->format)) * frame->channels);
#endif
                  memcpy(
                        rev_frame->data[0]+offset,
                      frame->data[0],
                      (frame->nb_samples * av_get_bytes_per_sample(static_cast<AVSampleFormat>(frame->format)) * frame->channels)
                      );
#ifdef AUDIOWARNINGS
                  dout << "pts:" << frame_->pts << "dur:" << frame_->pkt_duration << "rev_target:" << reverse_target << "offset:" << offset << "limit:" << rev_frame->linesize[0];
#endif
                }

                rev_frame->nb_samples += frame->nb_samples;

                if ((frame_->pts >= reverse_target_) || (ret == AVERROR_EOF)) {
                  /*
#ifdef AUDIOWARNINGS
                  dout << "time for the end of rev cache" << rev_frame->nb_samples << clip->rev_target << frame_->pts << frame_->pkt_duration << frame_->nb_samples;
                  dout << "diff:" << (frame_->pkt_pts + frame_->pkt_duration) - clip->rev_target;
#endif
                  int cutoff = qRound64((((frame_->pkt_pts + frame_->pkt_duration) - reverse_target) * timebase) * audio_output->format().sampleRate());
                  if (cutoff > 0) {
#ifdef AUDIOWARNINGS
                    dout << "cut off" << cutoff << "samples (rate:" << audio_output->format().sampleRate() << ")";
#endif
                    rev_frame->nb_samples -= cutoff;
                  }
*/

#ifdef AUDIOWARNINGS
                  dout << "pre cutoff deets::: rev_frame.pts:" << rev_frame->pts << "rev_frame.nb_samples" << rev_frame->nb_samples << "rev_target:" << reverse_target;
#endif
                  double playback_speed_ = clip->speed().value * clip->media()->to_footage()->speed;
                  rev_frame->nb_samples = qRound64(double(reverse_target_ - rev_frame->pts) * timebase * (current_audio_freq() / playback_speed_));
#ifdef AUDIOWARNINGS
                  dout << "post cutoff deets::" << rev_frame->nb_samples;
#endif

                  int frame_size = rev_frame->nb_samples * rev_frame->channels * av_get_bytes_per_sample(static_cast<AVSampleFormat>(rev_frame->format));
                  int half_frame_size = frame_size >> 1;

                  int sample_size = rev_frame->channels*av_get_bytes_per_sample(static_cast<AVSampleFormat>(rev_frame->format));
                  char* temp_chars = new char[sample_size];
                  for (int i=0;i<half_frame_size;i+=sample_size) {
                    memcpy(temp_chars, &rev_frame->data[0][i], sample_size);

                    memcpy(&rev_frame->data[0][i], &rev_frame->data[0][frame_size-i-sample_size], sample_size);

                    memcpy(&rev_frame->data[0][frame_size-i-sample_size], temp_chars, sample_size);
                  }
                  delete [] temp_chars;

                  reverse_target_ = rev_frame->pts;
                  frame = rev_frame;
                  break;
                }
              }

              loop++;

#ifdef AUDIOWARNINGS
              dout << "loop" << loop;
#endif
            } else {
              frame->pts = frame_->pts;
              break;
            }
          } while (true);
        } else {
          // if there is no more data in the file, we flush the remainder out of swresample
          break;
        }

        new_frame = true;

        if (frame_sample_index_ < 0) {
          frame_sample_index_ = 0;
        } else {
          frame_sample_index_ -= nb_bytes;
        }

        nb_bytes = frame->nb_samples * av_get_bytes_per_sample(static_cast<AVSampleFormat>(frame->format)) * frame->channels;

        if (audio_just_reset) {
          // get precise sample offset for the elected clip_in from this audio frame
          double target_sts = playhead_to_clip_seconds(clip, audio_target_frame);

          int64_t stream_start = qMax(static_cast<int64_t>(0), stream->start_time);
          double frame_sts = ((frame->pts - stream_start) * timebase);

          int nb_samples = qRound64((target_sts - frame_sts)*current_audio_freq());
          frame_sample_index_ = nb_samples * 4;
#ifdef AUDIOWARNINGS
          dout << "fsts:" << frame_sts << "tsts:" << target_sts << "nbs:" << nb_samples << "nbb:" << nb_bytes << "rev_targetToSec:" << (reverse_target * timebase);
          dout << "fsi-calc:" << frame_sample_index;
#endif
          if (reverse_audio) frame_sample_index_ = nb_bytes - frame_sample_index_;
          audio_just_reset = false;
        }

#ifdef AUDIOWARNINGS
        dout << "fsi-post-post:" << frame_sample_index;
#endif
        if (audio_buffer_write == 0) {
          audio_buffer_write = get_buffer_offset_from_frame(last_fr, qMax(timeline_in, target_frame));

          if (frame_skip > 0) {
            int target = get_buffer_offset_from_frame(last_fr, qMax(timeline_in + frame_skip, target_frame));
            frame_sample_index_ += (target - audio_buffer_write);
            audio_buffer_write = target;
          }
        }

        int offset = audio_ibuffer_read - audio_buffer_write;
        if (offset > 0) {
          audio_buffer_write += offset;
          frame_sample_index_ += offset;
        }

        // try to correct negative fsi
        if (frame_sample_index_ < 0) {
          audio_buffer_write -= frame_sample_index_;
          frame_sample_index_ = 0;
        }
      }

      if (reverse_audio) frame = queue_.at(1);

#ifdef AUDIOWARNINGS
      dout << "j" << frame_sample_index << nb_bytes;
#endif

      // apply any audio effects to the data
      if (nb_bytes == INT_MAX) nb_bytes = frame->nb_samples * av_get_bytes_per_sample(static_cast<AVSampleFormat>(frame->format)) * frame->channels;
      if (new_frame) {
        apply_audio_effects(clip, bytes_to_seconds(audio_buffer_write, 2, current_audio_freq()) + audio_ibuffer_timecode + ((double)clip->clip_in(true)/clip->sequence->frame_rate) - ((double)timeline_in/last_fr), frame, nb_bytes, nests_);
      }
    }

    // mix audio into internal buffer
    if (frame->nb_samples == 0) {
      break;
    } else {
      qint64 buffer_timeline_out = get_buffer_offset_from_frame(clip->sequence->frame_rate, timeline_out);

      audio_write_lock.lock();

      int sample_skip = 4*qMax(0, qAbs(playback_speed_)-1);
      int sample_byte_size = av_get_bytes_per_sample(static_cast<AVSampleFormat>(frame->format));

      while (frame_sample_index_ < nb_bytes
             && audio_buffer_write < audio_ibuffer_read+(audio_ibuffer_size>>1)
             && audio_buffer_write < buffer_timeline_out) {
        for (int i=0;i<frame->channels;i++) {
          int upper_byte_index = (audio_buffer_write+1)%audio_ibuffer_size;
          int lower_byte_index = (audio_buffer_write)%audio_ibuffer_size;
          qint16 old_sample = static_cast<qint16>((audio_ibuffer[upper_byte_index] & 0xFF) << 8 | (audio_ibuffer[lower_byte_index] & 0xFF));
          qint16 new_sample = static_cast<qint16>((frame->data[0][frame_sample_index_+1] & 0xFF) << 8 | (frame->data[0][frame_sample_index_] & 0xFF));
          qint16 mixed_sample = mix_audio_sample(old_sample, new_sample);

          audio_ibuffer[upper_byte_index] = quint8((mixed_sample >> 8) & 0xFF);
          audio_ibuffer[lower_byte_index] = quint8(mixed_sample & 0xFF);

          audio_buffer_write+=sample_byte_size;
          frame_sample_index_+=sample_byte_size;
        }

        frame_sample_index_ += sample_skip;

        if (audio_reset_) break;
      }

#ifdef AUDIOWARNINGS
      if (audio_buffer_write >= buffer_timeline_out) dout << "timeline out at fsi" << frame_sample_index << "of frame ts" << frame_->pts;
#endif

      audio_write_lock.unlock();

      if (audio_reset_) return;

      if (scrubbing_) {
        if (audio_thread != nullptr) audio_thread->notifyReceiver();
      }

      if (frame_sample_index_ >= nb_bytes) {
        frame_sample_index_ = -1;
      } else {
        // assume we have no more data to send
        break;
      }

      //			dout << "ended" << frame_sample_index << nb_bytes;
    }
    if (reached_end) {
      frame->nb_samples = 0;
    }
    if (scrubbing_) {
      break;
    }
  }

  // If there's a QObject waiting for audio to be rendered, wake it now
  WakeAudioWakeObject();
}

bool Cacher::IsReversed()
{
  // Here, the Clip reverse and reversed playback speed cancel each other out to produce normal playback
  return (clip->reversed() != playback_speed_ < 0);
}

void Cacher::CacheVideoWorker() {

  // is this media a still image?
  if (clip->media_stream()->infinite_length) {

    // for efficiency, we do slightly different things for a still image

    // if we already queued a frame, we don't actually need to cache anything, so we only retrieve a frame if not
    if (queue_.size() == 0) {

      // retrieve a single frame

      // main thread waits until cacher starts fully, wake it up here
      WakeMainThread();

      AVFrame* still_image_frame;

      if (RetrieveFrameAndProcess(&still_image_frame) >= 0) {

        queue_.lock();
        queue_.append(still_image_frame);
        queue_.unlock();

        SetRetrievedFrame(still_image_frame);
      }

    }

  } else {
    // this media is not a still image and will require more complex caching

    // main thread waits until cacher starts fully, wake it up here
    WakeMainThread();

    // determine if this media is reversed, which will affect how the queue is constructed
    bool reversed = IsReversed();

    // get the timestamp we want in terms of the media's timebase
    int64_t target_pts = seconds_to_timestamp(clip, playhead_to_clip_seconds(clip, playhead_));

    // get the value of one second in terms of the media's timebase
    int64_t second_pts = seconds_to_timestamp(clip, 1); // FIXME: possibly magic number?

    // check which range of frames we have in the queue
    int64_t earliest_pts = INT64_MAX;
    int64_t latest_pts = INT64_MIN;
    int frames_greater_than_target = 0;

    for (int i=0;i<queue_.size();i++) {
      // cache earliest and latest timestamps in the queue
      earliest_pts = qMin(earliest_pts, queue_.at(i)->pts);
      latest_pts = qMax(latest_pts, queue_.at(i)->pts);

      // count upcoming frames
      if (queue_.at(i)->pts > target_pts) {
        frames_greater_than_target++;
      }
    }

    // If we have to seek ahead, we may want to re-use the frame we retrieved later in the pipeline.
    AVFrame* decoded_frame;
    bool have_existing_frame_to_use = false;
    bool seeked_to_zero = false;

    // check if the frame is within this queue or if we'll have to seek elsewhere to get it
    // (we check for one second of time after latest_pts, because if it's within that range it'll likely be faster to
    // play up to that frame than seek to it)
    if (target_pts < earliest_pts || target_pts > latest_pts + second_pts || queue_.size() == 0) {
      // we need to seek to retrieve this frame

      int retrieve_code;
      int64_t seek_ts = target_pts;
      int64_t zero = 0;

      // Some formats don't seek reliably to the last keyframe, as a result we need to seek in a loop to ensure we
      // get a frame prior to the timestamp
      do {

        // if we already allocated a frame here, we'd better free it
        if (have_existing_frame_to_use) {
          av_frame_free(&decoded_frame);
        }

        // If we already seeked to a timestamp of zero, there's no further we can go, so we have to exit the loop if so
        seeked_to_zero = (seek_ts == 0);

        avcodec_flush_buffers(codecCtx);
        av_seek_frame(formatCtx, clip->media_stream_index(), seek_ts, AVSEEK_FLAG_BACKWARD);

        retrieve_code = RetrieveFrameAndProcess(&decoded_frame);

        //qDebug() << "Target:" << target_pts << "Seek:" << seek_ts << "Frame:" << decoded_frame->pts;

        seek_ts = qMax(zero, seek_ts - second_pts);

        have_existing_frame_to_use = true;
      } while (retrieve_code >= 0 && decoded_frame->pts > target_pts && !seeked_to_zero);

      // also we assume none of the frames in the queue are usable
      queue_.lock();
      queue_.clear();
      queue_.unlock();

      // reset upcoming frame count and latest pts for later calculations
      frames_greater_than_target = 0;
      latest_pts = INT64_MIN;
    }

    // get values on old frames to remove from the queue

    // for FRAME_QUEUE_TYPE_SECONDS, this is used to store the maximum timestamp
    // for FRAME_QUEUE_TYPE_FRAMES, this is used to store the maximum number of frames that can be added
    int64_t minimum_ts;

    // check if we can add more frames to this queue or not

    // for FRAME_QUEUE_TYPE_SECONDS, this is used to store the maximum timestamp
    // for FRAME_QUEUE_TYPE_FRAMES, this is used to store the maximum number of frames that can be added
    int64_t maximum_ts;

    // Get queue configuration
    int previous_queue_type, upcoming_queue_type;
    double previous_queue_size, upcoming_queue_size;

    // For reversed playback, we flip the queue stats as "upcoming" frames are going to be played before the "previous"
    // frames now
    if (reversed) {
      previous_queue_type = olive::CurrentConfig.upcoming_queue_type;
      previous_queue_size = olive::CurrentConfig.upcoming_queue_size;
      upcoming_queue_type = olive::CurrentConfig.previous_queue_type;
      upcoming_queue_size = olive::CurrentConfig.previous_queue_size;
    } else {
      previous_queue_type = olive::CurrentConfig.previous_queue_type;
      previous_queue_size = olive::CurrentConfig.previous_queue_size;
      upcoming_queue_type = olive::CurrentConfig.upcoming_queue_type;
      upcoming_queue_size = olive::CurrentConfig.upcoming_queue_size;
    }

    // Determine "previous" queue statistics
    if (previous_queue_type == olive::FRAME_QUEUE_TYPE_FRAMES) {
      // get the maximum number of previous frames that can be in the queue
      minimum_ts = qCeil(previous_queue_size);
    } else {
      // get the minimum frame timestamp that can be added to the queue
      minimum_ts = qRound(target_pts - second_pts * previous_queue_size);
    }

    // Determine "upcoming" queue statistics
    if (upcoming_queue_type == olive::FRAME_QUEUE_TYPE_FRAMES) {
      maximum_ts = qCeil(upcoming_queue_size);
    } else {
      // get the maximum frame timestamp that can be added to the queue
      maximum_ts = qRound(target_pts + second_pts * upcoming_queue_size);
    }

    // if we already have the maximum number of upcoming frames, don't bother running the retrieving any frames at all
    bool start_loop = true;
    if ((upcoming_queue_type == olive::FRAME_QUEUE_TYPE_FRAMES && frames_greater_than_target >= maximum_ts)
        || (upcoming_queue_type == olive::FRAME_QUEUE_TYPE_SECONDS && latest_pts > maximum_ts)) {
      start_loop = false;
    }


    if (start_loop) {

      interrupt_ = false;
      do {

        // retrieve raw RGBA frame from decoder + filter stack
        int retrieve_code = 0;

        // if we retrieved a perfectly good frame earlier by checking the seek, use that here
        if (!have_existing_frame_to_use) {
          retrieve_code = RetrieveFrameAndProcess(&decoded_frame);
        } else {
          have_existing_frame_to_use = false;
        }

        if (retrieve_code < 0 && retrieve_code != AVERROR_EOF) {

          // for some reason we were unable to retrieve a frame, likely a decoder error so we report it
          // again, an EOF isn't an "error" but will how we add frames (see below)

          qCritical() << "Failed to retrieve frame from buffersink." << retrieve_code;
          break;

        } else if (decoded_frame->pts != AV_NOPTS_VALUE) {

          // check if this frame exceeds the minimum timestamp
          if (previous_queue_type == olive::FRAME_QUEUE_TYPE_SECONDS
              && decoded_frame->pts < minimum_ts) {

            // if so, we don't need it
            av_frame_free(&decoded_frame);

          } else {

            if (retrieved_frame == nullptr) {
              if (decoded_frame->pts == target_pts) {

                // We retrieved the exact frame we're looking for

                SetRetrievedFrame(decoded_frame);

              } else if (decoded_frame->pts > target_pts) {

                if (queue_.size() > 0) {

                  SetRetrievedFrame(queue_.last());

                } else if (seeked_to_zero) {

                  // If this flag is set but we still got a frame after the target timestamp, it means this was somehow
                  // the earliest frame we could get
                  SetRetrievedFrame(decoded_frame);
                  seeked_to_zero = false;

                }

              }
            }

            // add the frame to the queue
            queue_.lock();
            queue_.append(decoded_frame);
            queue_.unlock();

            // check the amount of previous frames in the queue by using the current queue size for if we need to
            // remove any old entries (assumes the queue is chronological)
            if (previous_queue_type == olive::FRAME_QUEUE_TYPE_FRAMES) {

              int previous_frame_count = 0;

              if (decoded_frame->pts < target_pts) {
                // if this frame is before the target frame, make sure we don't add too many of them
                previous_frame_count = queue_.size();
              } else {
                // if this frame is after the target frame, clean up any previous frames before it
                // TODO is there a faster way to do this?

                for (int i=0;i<queue_.size();i++) {
                  if (queue_.at(i)->pts > target_pts) {
                    break;
                  } else {
                    previous_frame_count++;
                  }
                }

              }

              // remove frames while the amount of previous frames exceeds the maximum
              while (previous_frame_count > minimum_ts) {
                queue_.lock();
                queue_.removeFirst();
                queue_.unlock();
                previous_frame_count--;
              }

            }

            // check if the queue is full according to olive::CurrentConfig
            if (upcoming_queue_type == olive::FRAME_QUEUE_TYPE_FRAMES) {

              // if this frame is later than the target, it's an "upcoming" frame
              if (decoded_frame->pts > target_pts) {

                // we started a count of upcoming frames above, we can continue it here
                frames_greater_than_target++;

                // compare upcoming frame count with maximum upcoming frames (maximum_ts)
                if (frames_greater_than_target >= maximum_ts) {
                  break;
                }
              }

            } else if (decoded_frame->pts > maximum_ts) { // for `upcoming_queue_type == olive::FRAME_QUEUE_TYPE_SECONDS`
              break;
            }

          }



        } else {

          // if a frame has no timestamp (pts == AV_NOPTS_VALUE), we assume it's an invalid frame and don't use it

          qWarning() << clip->name() << "frame had no PTS value";
          av_frame_free(&decoded_frame);

          if (retrieve_code == AVERROR_EOF && retrieved_frame == nullptr && !queue_.isEmpty()) {
            // if we reached the end of the file, it's not an error but there are no more frames to retrieve
            // some formats EOF before the end of the duration that Olive calculates. In this event, we simply
            // return the last frame we retrieved
            //
            // TODO: Check duration formula

            SetRetrievedFrame(queue_.last());

          } else {

            SetRetrievedFrame(nullptr);

          }

          break;

        }
      } while (!interrupt_);

    }

  }

  // For some reason we couldn't get the frame, we should wake up the RenderThread anyway
  if (retrieved_frame == nullptr) {
    qCritical() << "Couldn't retrieve an appropriate frame. This is an error and may mean this media is corrupt.";
    SetRetrievedFrame(nullptr);
  }
}

void Cacher::Reset() {
  // if we seek to a whole other place in the timeline, we'll need to reset the cache with new values
  if (clip->media() == nullptr) {
    if (clip->track() >= 0) {
      // a null-media audio clip is usually an auto-generated sound clip such as Tone or Noise
      reached_end = false;
      audio_target_frame = playhead_;
      frame_sample_index_ = -1;
      frame_->pts = 0;
    }
  } else {

    const FootageStream* ms = clip->media_stream();
    if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
      // flush ffmpeg codecs
      avcodec_flush_buffers(codecCtx);
      reached_end = false;

      // seek (target_frame represents timeline timecode in frames, not clip timecode)

      int64_t timestamp = qRound64(playhead_to_clip_seconds(clip, playhead_) / av_q2d(stream->time_base));

      bool temp_reverse = (playback_speed_ < 0);
      if (clip->reversed() != temp_reverse) {
        reverse_target_ = timestamp;
        timestamp -= av_q2d(av_inv_q(stream->time_base));
#ifdef AUDIOWARNINGS
        dout << "seeking to" << timestamp << "(originally" << reverse_target << ")";
      } else {
        dout << "reset called; seeking to" << timestamp;
#endif
      }
      av_seek_frame(formatCtx, ms->file_index, timestamp, AVSEEK_FLAG_BACKWARD);
      audio_target_frame = playhead_;
      frame_sample_index_ = -1;
    }
  }
}

void Cacher::SetRetrievedFrame(AVFrame *f)
{
  if (retrieved_frame == nullptr) {
    retrieve_lock_.lock();
    retrieved_frame = f;
    retrieve_wait_.wakeAll();
    retrieve_lock_.unlock();
  }
}

void Cacher::WakeMainThread()
{
  main_thread_lock_.lock();
  main_thread_wait_.wakeAll();
  main_thread_lock_.unlock();
}

Cacher::Cacher(Clip* c) :
  clip(c),
  frame_(nullptr),
  pkt(nullptr),
  formatCtx(nullptr),
  opts(nullptr),
  filter_graph(nullptr),
  codecCtx(nullptr),
  is_valid_state_(false)
{}

void Cacher::OpenWorker() {
  qint64 time_start = QDateTime::currentMSecsSinceEpoch();

  // set some defaults for the audio cacher
  if (clip->track() >= 0) {
    audio_reset_ = false;
    frame_sample_index_ = -1;
    audio_buffer_write = 0;
  }
  reached_end = false;

  if (clip->media() == nullptr) {
    if (clip->track() >= 0) {
      frame_ = av_frame_alloc();
      frame_->format = kDestSampleFmt;
      frame_->channel_layout = clip->sequence->audio_layout;
      frame_->channels = av_get_channel_layout_nb_channels(frame_->channel_layout);
      frame_->sample_rate = current_audio_freq();
      frame_->nb_samples = 2048;
      av_frame_make_writable(frame_);
      if (av_frame_get_buffer(frame_, 0)) {
        qCritical() << "Could not allocate buffer for tone clip";
      }
      audio_reset_ = true;
    }
  } else if (clip->media()->get_type() == MEDIA_TYPE_FOOTAGE) {
    // opens file resource for FFmpeg and prepares Clip struct for playback
    Footage* m = clip->media()->to_footage();

    // byte array for retriving raw bytes from QString URL
    QByteArray ba;

    // do we have a proxy?
    if (m->proxy
        && !m->proxy_path.isEmpty()
        && QFileInfo::exists(m->proxy_path)) {
      ba = m->proxy_path.toUtf8();
    } else {
      ba = m->url.toUtf8();
    }

    const char* filename = ba.constData();
    const FootageStream* ms = clip->media_stream();

    // for image sequences that don't start at 0, set the index where it does start
    AVDictionary* format_opts = nullptr;
    if (m->start_number > 0) {
      av_dict_set(&format_opts, "start_number", QString::number(m->start_number).toUtf8(), 0);
    }

    formatCtx = nullptr;
    int errCode = avformat_open_input(
          &formatCtx,
          filename,
          nullptr,
          &format_opts
          );
    if (errCode != 0) {
      char err[1024];
      av_strerror(errCode, err, 1024);
      qCritical() << "Could not open" << filename << "-" << err;
      olive::MainWindow->statusBar()->showMessage(tr("Could not open %1 - %2").arg(filename, err));
      return;
    }

    errCode = avformat_find_stream_info(formatCtx, nullptr);
    if (errCode < 0) {
      char err[1024];
      av_strerror(errCode, err, 1024);
      qCritical() << "Could not open" << filename << "-" << err;
      olive::MainWindow->statusBar()->showMessage(tr("Could not open %1 - %2").arg(filename, err));
      return;
    }

    av_dump_format(formatCtx, 0, filename, 0);

    stream = formatCtx->streams[ms->file_index];
    codec = avcodec_find_decoder(stream->codecpar->codec_id);
    codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codecCtx, stream->codecpar);

    opts = nullptr;

    // enable multithreading on decoding
    av_dict_set(&opts, "threads", "auto", 0);

    // enable extra optimization code on h264 (not even sure if they help)
    if (stream->codecpar->codec_id == AV_CODEC_ID_H264) {
      av_dict_set(&opts, "tune", "fastdecode", 0);
      av_dict_set(&opts, "tune", "zerolatency", 0);
    }

    // Open codec
    if (avcodec_open2(codecCtx, codec, &opts) < 0) {
      qCritical() << "Could not open codec";
    }

    // allocate filtergraph
    filter_graph = avfilter_graph_alloc();
    if (filter_graph == nullptr) {
      qCritical() << "Could not create filtergraph";
    }
    char filter_args[512];

    if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
      snprintf(filter_args, sizeof(filter_args), "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
               stream->codecpar->width,
               stream->codecpar->height,
               stream->codecpar->format,
               stream->time_base.num,
               stream->time_base.den,
               stream->codecpar->sample_aspect_ratio.num,
               stream->codecpar->sample_aspect_ratio.den
               );

      avfilter_graph_create_filter(&buffersrc_ctx, avfilter_get_by_name("buffer"), "in", filter_args, nullptr, filter_graph);
      avfilter_graph_create_filter(&buffersink_ctx, avfilter_get_by_name("buffersink"), "out", nullptr, nullptr, filter_graph);

      AVFilterContext* last_filter = buffersrc_ctx;

      char filter_args[100];

      if (ms->video_interlacing != VIDEO_PROGRESSIVE) {
        AVFilterContext* yadif_filter;
        snprintf(filter_args, sizeof(filter_args), "mode=3:parity=%d", ((ms->video_interlacing == VIDEO_TOP_FIELD_FIRST) ? 0 : 1)); // there's a CUDA version if we start using nvdec/nvenc
        avfilter_graph_create_filter(&yadif_filter, avfilter_get_by_name("yadif"), "yadif", filter_args, nullptr, filter_graph);

        avfilter_link(last_filter, 0, yadif_filter, 0);
        last_filter = yadif_filter;
      }

      const char* chosen_format = av_get_pix_fmt_name(kDestPixFmt);
      snprintf(filter_args, sizeof(filter_args), "pix_fmts=%s", chosen_format);

      AVFilterContext* format_conv;
      avfilter_graph_create_filter(&format_conv, avfilter_get_by_name("format"), "fmt", filter_args, nullptr, filter_graph);
      avfilter_link(last_filter, 0, format_conv, 0);

      avfilter_link(format_conv, 0, buffersink_ctx, 0);

      avfilter_graph_config(filter_graph, nullptr);

    } else if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
      if (codecCtx->channel_layout == 0) codecCtx->channel_layout = av_get_default_channel_layout(stream->codecpar->channels);

      // set up cache
      queue_.append(av_frame_alloc());

      if (true) {
        AVFrame* reverse_frame = av_frame_alloc();

        reverse_frame->format = kDestSampleFmt;
        reverse_frame->nb_samples = current_audio_freq()*10;
        reverse_frame->channel_layout = clip->sequence->audio_layout;
        reverse_frame->channels = av_get_channel_layout_nb_channels(clip->sequence->audio_layout);
        av_frame_get_buffer(reverse_frame, 0);

        queue_.append(reverse_frame);
      }

      snprintf(filter_args, sizeof(filter_args), "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%" PRIx64,
               stream->time_base.num,
               stream->time_base.den,
               stream->codecpar->sample_rate,
               av_get_sample_fmt_name(codecCtx->sample_fmt),
               codecCtx->channel_layout
               );

      avfilter_graph_create_filter(&buffersrc_ctx, avfilter_get_by_name("abuffer"), "in", filter_args, nullptr, filter_graph);
      avfilter_graph_create_filter(&buffersink_ctx, avfilter_get_by_name("abuffersink"), "out", nullptr, nullptr, filter_graph);

      enum AVSampleFormat sample_fmts[] = { kDestSampleFmt,  static_cast<AVSampleFormat>(-1) };
      if (av_opt_set_int_list(buffersink_ctx, "sample_fmts", sample_fmts, -1, AV_OPT_SEARCH_CHILDREN) < 0) {
        qCritical() << "Could not set output sample format";
      }

      int64_t channel_layouts[] = { AV_CH_LAYOUT_STEREO, static_cast<AVSampleFormat>(-1) };
      if (av_opt_set_int_list(buffersink_ctx, "channel_layouts", channel_layouts, -1, AV_OPT_SEARCH_CHILDREN) < 0) {
        qCritical() << "Could not set output sample format";
      }

      int target_sample_rate = current_audio_freq();

      double playback_speed_ = clip->speed().value * m->speed;

      if (qFuzzyCompare(playback_speed_, 1.0)) {
        avfilter_link(buffersrc_ctx, 0, buffersink_ctx, 0);
      } else if (clip->speed().maintain_audio_pitch) {
        AVFilterContext* previous_filter = buffersrc_ctx;
        AVFilterContext* last_filter = buffersrc_ctx;

        char speed_param[10];

        double base = (playback_speed_ > 1.0) ? 2.0 : 0.5;

        double speedlog = log(playback_speed_) / log(base);
        int whole2 = qFloor(speedlog);
        speedlog -= whole2;

        if (whole2 > 0) {
          snprintf(speed_param, sizeof(speed_param), "%f", base);
          for (int i=0;i<whole2;i++) {
            AVFilterContext* tempo_filter = nullptr;
            avfilter_graph_create_filter(&tempo_filter, avfilter_get_by_name("atempo"), "atempo", speed_param, nullptr, filter_graph);
            avfilter_link(previous_filter, 0, tempo_filter, 0);
            previous_filter = tempo_filter;
          }
        }

        snprintf(speed_param, sizeof(speed_param), "%f", qPow(base, speedlog));
        last_filter = nullptr;
        avfilter_graph_create_filter(&last_filter, avfilter_get_by_name("atempo"), "atempo", speed_param, nullptr, filter_graph);
        avfilter_link(previous_filter, 0, last_filter, 0);

        avfilter_link(last_filter, 0, buffersink_ctx, 0);
      } else {
        target_sample_rate = qRound64(target_sample_rate / playback_speed_);
        avfilter_link(buffersrc_ctx, 0, buffersink_ctx, 0);
      }

      int sample_rates[] = { target_sample_rate, 0 };
      if (av_opt_set_int_list(buffersink_ctx, "sample_rates", sample_rates, 0, AV_OPT_SEARCH_CHILDREN) < 0) {
        qCritical() << "Could not set output sample rates";
      }

      avfilter_graph_config(filter_graph, nullptr);

      audio_reset_ = true;
    }

    pkt = av_packet_alloc();
    frame_ = av_frame_alloc();
  }

  qInfo() << "Clip opened on track" << clip->track() << "(took" << (QDateTime::currentMSecsSinceEpoch() - time_start) << "ms)";

  is_valid_state_ = true;
}

void Cacher::CacheWorker() {
  if (clip->track() < 0) {
    // clip is a video track, start caching video
    CacheVideoWorker();
  } else {
    // clip is audio
    CacheAudioWorker();
  }
}

void Cacher::CloseWorker() {
  retrieved_frame = nullptr;
  queue_.lock();
  queue_.clear();
  queue_.unlock();

  if (frame_ != nullptr) {
    av_frame_free(&frame_);
    frame_ = nullptr;
  }

  if (pkt != nullptr) {
    av_packet_free(&pkt);
    pkt = nullptr;
  }

  if (clip->media() != nullptr && clip->media()->get_type() == MEDIA_TYPE_FOOTAGE) {
    if (filter_graph != nullptr) {
      avfilter_graph_free(&filter_graph);
      filter_graph = nullptr;
    }

    if (codecCtx != nullptr) {
      avcodec_close(codecCtx);
      avcodec_free_context(&codecCtx);
      codecCtx = nullptr;
    }

    if (opts != nullptr) {
      av_dict_free(&opts);
    }

    // protection for get_timebase()
    stream = nullptr;

    if (formatCtx != nullptr) {
      avformat_close_input(&formatCtx);
    }
  }

  qInfo() << "Clip closed on track" << clip->track();
}

void Cacher::run() {
  clip->cache_lock.lock();

  OpenWorker();

  clip->state_change_lock.unlock();

  while (caching_) {
    if (!queued_) {
      wait_cond_.wait(&clip->cache_lock);
    }
    queued_ = false;
    if (!caching_) {
      break;
    } else if (is_valid_state_) {
      CacheWorker();
    } else {
      // main thread waits until cacher starts fully, but the cacher can't run, so we just wake it up here
      WakeMainThread();
    }
  }

  is_valid_state_ = false;

  CloseWorker();

  clip->state_change_lock.unlock();

  clip->cache_lock.unlock();
}

void Cacher::Open()
{
  wait();

  // set variable defaults for caching
  caching_ = true;
  queued_ = false;

  start((clip->track() < 0) ? QThread::HighPriority : QThread::TimeCriticalPriority);
}

void Cacher::Cache(long playhead, bool scrubbing, QVector<Clip*>& nests, int playback_speed)
{

  if (!is_valid_state_) {
    return;
  }

  if (clip->media_stream() != nullptr
      && queue_.size() > 0
      && clip->media_stream()->infinite_length) {
    retrieved_frame = queue_.at(0);
    return;
  }

  playhead_ = playhead;
  nests_ = nests;
  scrubbing_ = scrubbing;
  playback_speed_ = playback_speed;
  queued_ = true;

  bool wait_for_cacher_to_respond = true;

  if (clip->media() != nullptr) {
    // see if we already have this frame
    retrieve_lock_.lock();
    queue_.lock();
    retrieved_frame = nullptr;
    int64_t target_pts = seconds_to_timestamp(clip, playhead_to_clip_seconds(clip, playhead_));
    for (int i=0;i<queue_.size();i++) {

      if (queue_.at(i)->pts == target_pts) {

        // the queue has a frame with the exact timestamp

        retrieved_frame = queue_.at(i);
        wait_for_cacher_to_respond = false;
        break;
      } else if (i > 0 && queue_.at(i-1)->pts < target_pts && queue_.at(i)->pts > target_pts) {

        // the queue has a frame with a close timestamp that we'll assume is different due to a rounding error

        retrieved_frame = queue_.at(i-1);
        wait_for_cacher_to_respond = false;
        break;
      }
    }
    queue_.unlock();
    retrieve_lock_.unlock();
  }

  if (wait_for_cacher_to_respond) {
    main_thread_lock_.lock();
  }

  // wake up cacher
  wait_cond_.wakeAll();

  // if not, wait for cacher to respond
  if (wait_for_cacher_to_respond) {
    interrupt_ = true;
    main_thread_wait_.wait(&main_thread_lock_, 2000);
  }

  if (wait_for_cacher_to_respond) {
    main_thread_lock_.unlock();
  }
}

AVFrame *Cacher::Retrieve()
{
  if (!caching_) {
    return nullptr;
  }

  // for thread-safety, we lock a mutex to ensure this thread is never woken by anything out of sync

  retrieve_lock_.lock();

  // check if there's a frame ready to be shown by the cacher

  if (retrieved_frame == nullptr) {
    // wait for cacher to finish caching


    if (clip->cache_lock.tryLock()) {

      // If the queue could lock, the cacher isn't running which means no frame is coming. This is an error.
      qCritical() << "Cacher frame was null while the cacher wasn't running on clip" << clip->name();
      clip->cache_lock.unlock();

    } else {

      // cacher is running, wait for it to give a frame
      retrieve_wait_.wait(&retrieve_lock_);

    }

  }

  retrieve_lock_.unlock();

  return retrieved_frame;
}

void Cacher::Close(bool wait_for_finish)
{
  caching_ = false;
  wait_cond_.wakeAll();

  if (wait_for_finish) {
    wait();
  }
}

void Cacher::ResetAudio()
{
  // using audio_write_lock seems like a good idea, but hasn't been tested yet. If there are audio issues when seeking,
  // try uncommenting them

//  audio_write_lock.lock();
  audio_reset_ = true;
  frame_sample_index_ = -1;
  audio_buffer_write = 0;
//  audio_write_lock.unlock();
}

int Cacher::media_width()
{
  return stream->codecpar->width;
}

int Cacher::media_height()
{
  return stream->codecpar->height;
}

AVRational Cacher::media_time_base()
{
  return stream->time_base;
}

ClipQueue *Cacher::queue()
{
  return &queue_;
}

int Cacher::RetrieveFrameFromDecoder(AVFrame* f) {
  int result = 0;
  int receive_ret;

  // do we need to retrieve a new packet for a new frame?
  av_frame_unref(f);
  while ((receive_ret = avcodec_receive_frame(codecCtx, f)) == AVERROR(EAGAIN)) {
    int read_ret = 0;
    do {
      if (pkt->buf != nullptr) {
        av_packet_unref(pkt);
      }
      read_ret = av_read_frame(formatCtx, pkt);
    } while (read_ret >= 0 && pkt->stream_index != clip->media_stream_index());

    if (read_ret >= 0) {
      int send_ret = avcodec_send_packet(codecCtx, pkt);
      if (send_ret < 0) {
        qCritical() << "Failed to send packet to decoder." << send_ret;
        return send_ret;
      }
    } else {
      if (read_ret == AVERROR_EOF) {
        int send_ret = avcodec_send_packet(codecCtx, nullptr);
        if (send_ret < 0) {
          qCritical() << "Failed to send packet to decoder." << send_ret;
          return send_ret;
        }
      } else {
        qCritical() << "Could not read frame." << read_ret;
        return read_ret; // skips trying to find a frame at all
      }
    }
  }
  if (receive_ret < 0) {
    if (receive_ret != AVERROR_EOF) qCritical() << "Failed to receive packet from decoder." << receive_ret;
    result = receive_ret;
  }

  return result;
}

int Cacher::RetrieveFrameAndProcess(AVFrame **f)
{
  // error codes from FFmpeg
  int retrieve_code, read_code, send_code;

  // frame for FFmpeg to decode into
  *f = av_frame_alloc();

  // loop to pull frames from the AVFilter stack
  while ((retrieve_code = av_buffersink_get_frame(buffersink_ctx, *f)) == AVERROR(EAGAIN)) {

    // retrieve frame from decoder
    read_code = RetrieveFrameFromDecoder(frame_);

    if (read_code >= 0) {

      // we retrieved a decoded video frame, which we will send to the AVFilter stack to convert to RGBA (with other
      // adjustments if necessary)

      if ((send_code = av_buffersrc_add_frame_flags(buffersrc_ctx, frame_, AV_BUFFERSRC_FLAG_KEEP_REF)) < 0) {
        qCritical() << "Failed to add frame to buffer source." << send_code;
        break;
      }

      // we don't need the original frame to we free it here
      av_frame_unref(frame_);

    } else {

      // AVERROR_EOF means we've reached the end of the file, not technically an error, but it's useful to know that
      // there are no more frames in this file
      if (read_code != AVERROR_EOF) {
        qCritical() << "Failed to read frame." << read_code;
      }
      break;
    }
  }

  if (read_code == AVERROR_EOF) {
    return AVERROR_EOF;
  }
  return retrieve_code;
}
