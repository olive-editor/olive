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

#include "clip.h"

#include <QtMath>

#include "project/effect.h"
#include "project/transition.h"
#include "project/footage.h"
#include "io/config.h"
#include "rendering/cacher.h"
#include "rendering/renderfunctions.h"
#include "panels/project.h"
#include "project/sequence.h"
#include "panels/timeline.h"
#include "project/media.h"
#include "io/clipboard.h"
#include "undo.h"
#include "debug.h"

const int kRGBAComponentCount = 4;

Clip::Clip(SequencePtr s) :
  sequence(s),
  cacher(this)
{
  enabled_ = true;
  clip_in_ = 0;
  timeline_in_ = 0;
  timeline_out_ = 0;
  track_ = 0;
  media_ = nullptr;
  speed_.value = 1.0;
  speed_.maintain_audio_pitch = false;
  reverse_ = false;
  autoscale_ = olive::CurrentConfig.autoscale_by_default;
  opening_transition = nullptr;
  closing_transition = nullptr;
  undeletable = false;
  replaced = false;
  fbo = nullptr;
  open_ = false;

  reset();
}

ClipPtr Clip::copy(SequencePtr s) {
  ClipPtr copy = std::make_shared<Clip>(s);

  copy->set_enabled(enabled());
  copy->set_name(name());
  copy->set_clip_in(clip_in());
  copy->set_timeline_in(timeline_in());
  copy->set_timeline_out(timeline_out());
  copy->set_track(track());
  copy->set_color(color());
  copy->set_media(media(), media_stream_index());
  copy->set_autoscaled(autoscaled());
  copy->set_speed(speed());
  copy->set_reversed(reversed());

  for (int i=0;i<effects.size();i++) {
    copy->effects.append(effects.at(i)->copy(copy.get()));
  }

  copy->set_cached_frame_rate((this->sequence == nullptr) ? cached_frame_rate() : this->sequence->frame_rate);

  copy->refresh();

  return copy;
}

bool Clip::IsActiveAt(long timecode)
{
  // these buffers allow clips to be opened and prepared well before they're displayed
  // as well as closed a little after they're not needed anymore
  int open_buffer = qCeil(this->sequence->frame_rate*2);
  int close_buffer = qCeil(this->sequence->frame_rate);


  return enabled()
      && timeline_in(true) < timecode + open_buffer
      && timeline_out(true) > timecode - close_buffer
      && timecode - timeline_in(true) + clip_in(true) < media_length();
}

const QColor &Clip::color()
{
  return color_;
}

void Clip::set_color(int r, int g, int b)
{
  color_.setRed(r);
  color_.setGreen(g);
  color_.setBlue(b);
}

void Clip::set_color(const QColor &c)
{
  color_ = c;
}

Media *Clip::media()
{
  return media_;
}

FootageStream *Clip::media_stream()
{
  if (media() != nullptr
      && media()->get_type() == MEDIA_TYPE_FOOTAGE) {
    return media()->to_footage()->get_stream_from_file_index(track() < 0, media_stream_index());
  }

  return nullptr;
}

int Clip::media_stream_index()
{
  return media_stream_;
}

void Clip::set_media(Media *m, int s)
{
  media_ = m;
  media_stream_ = s;
}

bool Clip::enabled()
{
  return enabled_;
}

void Clip::set_enabled(bool e)
{
  enabled_ = e;
}

void Clip::move(ComboAction* ca, long iin, long iout, long iclip_in, int itrack, bool verify_transitions, bool relative)
{
  ca->append(new MoveClipAction(this, iin, iout, iclip_in, itrack, relative));

  if (verify_transitions) {

    // if this is a shared transition, and the corresponding clip will be moved away somehow
    if (opening_transition != nullptr
        && opening_transition->secondary_clip != nullptr
        && opening_transition->secondary_clip->timeline_out() != iin) {
      // separate transition
      ca->append(new SetPointer(reinterpret_cast<void**>(&opening_transition->secondary_clip), nullptr));
      ca->append(new AddTransitionCommand(nullptr,
                                          opening_transition->secondary_clip,
                                          opening_transition,
                                          nullptr,
                                          0));
    }

    if (closing_transition != nullptr
        && closing_transition->secondary_clip != nullptr
        && closing_transition->parent_clip->timeline_in() != iout) {
      // separate transition
      ca->append(new SetPointer(reinterpret_cast<void**>(&closing_transition->secondary_clip), nullptr));
      ca->append(new AddTransitionCommand(nullptr,
                                          this,
                                          closing_transition,
                                          nullptr,
                                          0));
    }
  }
}

void Clip::reset() {
  texture = nullptr;
}

void Clip::reset_audio() {
  if (UsesCacher()) {
    cacher.ResetAudio();
  }
  if (media() != nullptr && media()->get_type() == MEDIA_TYPE_SEQUENCE) {
    SequencePtr nested_sequence = media()->to_sequence();
    for (int i=0;i<nested_sequence->clips.size();i++) {
      ClipPtr c = nested_sequence->clips.at(i);
      if (c != nullptr) c->reset_audio();
    }
  }
}

void Clip::refresh() {
  // validates media if it was replaced
  if (replaced && media() != nullptr && media()->get_type() == MEDIA_TYPE_FOOTAGE) {
    FootagePtr m = media()->to_footage();

    if (track() < 0 && m->video_tracks.size() > 0)  {
      set_media(media(), m->video_tracks.at(0).file_index);
    } else if (track() >= 0 && m->audio_tracks.size() > 0) {
      set_media(media(), m->audio_tracks.at(0).file_index);
    }
  }
  replaced = false;

  // reinitializes all effects... just in case
  for (int i=0;i<effects.size();i++) {
    effects.at(i)->refresh();
  }
}

QVector<Marker> &Clip::get_markers() {
  if (media() != nullptr) {
    return media()->get_markers();
  }
  return markers;
}

Clip::~Clip() {
  if (IsOpen()) {
    Close(true);
  }

  effects.clear();
}

long Clip::clip_in(bool with_transition) {
  if (with_transition && opening_transition != nullptr && opening_transition->secondary_clip != nullptr) {
    // we must be the secondary clip, so return (timeline in - length)
    return clip_in_ - opening_transition->get_true_length();
  }
  return clip_in_;
}

void Clip::set_clip_in(long c)
{
  clip_in_ = c;
}

long Clip::timeline_in(bool with_transition) {
  if (with_transition && opening_transition != nullptr && opening_transition->secondary_clip != nullptr) {
    // we must be the secondary clip, so return (timeline in - length)
    return timeline_in_ - opening_transition->get_true_length();
  }
  return timeline_in_;
}

void Clip::set_timeline_in(long t)
{
  timeline_in_ = t;
}

long Clip::timeline_out(bool with_transitions) {
  if (with_transitions && closing_transition != nullptr && closing_transition->secondary_clip != nullptr) {
    // we must be the primary clip, so return (timeline out + length2)
    return timeline_out_ + closing_transition->get_true_length();
  } else {
    return timeline_out_;
  }
}

void Clip::set_timeline_out(long t)
{
  timeline_out_ = t;
}

bool Clip::reversed()
{
  return reverse_;
}

void Clip::set_reversed(bool r)
{
  reverse_ = r;
}

bool Clip::autoscaled()
{
  return autoscale_;
}

void Clip::set_autoscaled(bool b)
{
  autoscale_ = b;
}

double Clip::cached_frame_rate()
{
  return cached_fr_;
}

void Clip::set_cached_frame_rate(double d)
{
  cached_fr_ = d;
}

const QString &Clip::name()
{
  return name_;
}

void Clip::set_name(const QString &s)
{
  name_ = s;
}

const ClipSpeed& Clip::speed()
{
  return speed_;
}

void Clip::set_speed(const ClipSpeed& d)
{
  speed_ = d;
}

AVRational Clip::time_base()
{
  return cacher.media_time_base();
}

int Clip::track()
{
  return track_;
}

void Clip::set_track(int t)
{
  track_ = t;
}

// timeline functions
long Clip::length() {
  return timeline_out_ - timeline_in_;
}

double Clip::media_frame_rate() {
  Q_ASSERT(track_ < 0);
  if (media_ != nullptr) {
    double rate = media_->get_frame_rate(media_stream_index());
    if (!qIsNaN(rate)) return rate;
  }
  if (sequence != nullptr) return sequence->frame_rate;
  return qSNaN();
}

long Clip::media_length() {
  if (this->sequence != nullptr) {
    double fr = this->sequence->frame_rate;

    fr /= speed_.value;

    if (media_ == nullptr) {
      return LONG_MAX;
    } else {
      switch (media_->get_type()) {
      case MEDIA_TYPE_FOOTAGE:
      {
        FootagePtr m = media_->to_footage();
        const FootageStream* ms = m->get_stream_from_file_index(track_ < 0, media_stream_index());
        if (ms != nullptr && ms->infinite_length) {
          return LONG_MAX;
        } else {
          return m->get_length_in_frames(fr);
        }
      }
      case MEDIA_TYPE_SEQUENCE:
      {
        SequencePtr s = media_->to_sequence();
        return rescale_frame_number(s->getEndFrame(), s->frame_rate, fr);
      }
      }
    }
  }
  return 0;
}

int Clip::media_width() {
  if (media_ == nullptr && sequence != nullptr) return sequence->width;
  switch (media_->get_type()) {
  case MEDIA_TYPE_FOOTAGE:
  {
    const FootageStream* ms = media_stream();
    if (ms != nullptr) return ms->video_width;
    if (sequence != nullptr) return sequence->width;
    break;
  }
  case MEDIA_TYPE_SEQUENCE:
  {
    SequencePtr s = media_->to_sequence();
    return s->width;
  }
  }
  return 0;
}

int Clip::media_height() {
  if (media_ == nullptr && sequence != nullptr) return sequence->height;
  switch (media_->get_type()) {
  case MEDIA_TYPE_FOOTAGE:
  {
    const FootageStream* ms = media_stream();
    if (ms != nullptr) return ms->video_height;
    if (sequence != nullptr) return sequence->height;
  }
    break;
  case MEDIA_TYPE_SEQUENCE:
  {
    SequencePtr s = media_->to_sequence();
    return s->height;
  }
  }
  return 0;
}

void Clip::refactor_frame_rate(ComboAction* ca, double multiplier, bool change_timeline_points) {
  if (change_timeline_points) {
    this->move(ca,
               qRound(double(timeline_in_) * multiplier),
               qRound(double(timeline_out_) * multiplier),
               qRound(double(clip_in_) * multiplier),
               track_);
  }

  // move keyframes
  for (int i=0;i<effects.size();i++) {
    EffectPtr e = effects.at(i);
    for (int j=0;j<e->row_count();j++) {
      EffectRow* r = e->row(j);
      for (int l=0;l<r->fieldCount();l++) {
        EffectField* f = r->field(l);
        for (int k=0;k<f->keyframes.size();k++) {
          ca->append(new SetLong(&f->keyframes[k].time, f->keyframes[k].time, qRound(f->keyframes[k].time * multiplier)));
        }
      }
    }
  }
}

void Clip::Open() {
  if (!open_ && state_change_lock.tryLock()) {
    open_ = true;

    for (int i=0;i<effects.size();i++) {
      effects.at(i)->open();
    }

    // reset variable used to optimize uploading frame data
    texture_frame = -1;

    if (UsesCacher()) {
      // cacher will unlock open_lock
      cacher.Open();
    } else {
      // this media doesn't use a cacher, so we unlock here
      state_change_lock.unlock();
    }
  }
}

void Clip::Close(bool wait) {
  // thread safety, prevents Close() running from two separate threads simultaneously
  if (open_ && state_change_lock.tryLock()) {
    open_ = false;

    if (media() != nullptr && media()->get_type() == MEDIA_TYPE_SEQUENCE) {
      close_active_clips(media()->to_sequence());
    }

    // destroy opengl texture in main thread
    delete texture;
    texture = nullptr;

    // close all effects
    for (int i=0;i<effects.size();i++) {
      if (effects.at(i)->is_open()) {
        effects.at(i)->close();
      }
    }

    // delete framebuffers
    if (fbo != nullptr) {
      // delete 3 fbos for nested sequences, 2 for most clips
      int fbo_count = (media() != nullptr && media()->get_type() == MEDIA_TYPE_SEQUENCE) ? 3 : 2;

      for (int j=0;j<fbo_count;j++) {
        delete fbo[j];
      }

      delete [] fbo;

      fbo = nullptr;
    }

    if (UsesCacher()) {
      cacher.Close(wait);
    } else {
      state_change_lock.unlock();
    }
  }
}

bool Clip::IsOpen()
{
  return open_;
}

void Clip::Cache(long playhead, bool scrubbing, QVector<Clip*>& nests, int playback_speed) {
  cacher.Cache(playhead, scrubbing, nests, playback_speed);
  cacher_frame = playhead;
}

bool Clip::Retrieve()
{
  bool ret = false;

  if (UsesCacher()) {

    // Retrieve the frame from the cacher that we requested in Cache().
    AVFrame* frame = cacher.Retrieve();

    // Wait for exclusive control of the queue to avoid any threading collisions
    cacher.queue()->lock();

    // Check if we retrieved a frame (nullptr) and if the queue stil contains this frame.
    //
    // `nullptr` is returned if the cacher failed to get any sort of frame and is uncommon, but we do need
    // to handle it.
    //
    // We check the queue because in some situations (e.g. intensive scrubbing), in the time it took to gain
    // exclusive control of the queue, the cacher may have deleted the frame.
    // Therefore we check to ensure the queue still contains the frame now that we have exclusive control,
    // to avoid any attempt to utilize now-freed memory.

    if (frame != nullptr && cacher.queue()->contains(frame)) {

      // check if the opengl texture exists yet, create it if not
      if (texture == nullptr) {
        texture = new QOpenGLTexture(QOpenGLTexture::Target2D);

        // the raw frame size may differ from the one we're using (e.g. a lower resolution proxy), so we make sure
        // the texture is using the correct dimensions, but then treat it as if it's the original resolution in the
        // composition
        texture->setSize(cacher.media_width(), cacher.media_height());

        texture->setFormat(QOpenGLTexture::RGBA8_UNorm);
        texture->setMipLevels(texture->maximumMipLevels());
        texture->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
        texture->allocateStorage(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8);
      }

      glPixelStorei(GL_UNPACK_ROW_LENGTH, frame->linesize[0]/kRGBAComponentCount);

      // 2 data buffers to ping-pong between
      bool using_db_1 = true;
      uint8_t* data_buffer_1 = frame->data[0];
      uint8_t* data_buffer_2 = nullptr;

      int frame_size = frame->linesize[0]*frame->height;

      for (int i=0;i<effects.size();i++) {
        EffectPtr e = effects.at(i);
        if (e->enable_image && e->is_enabled()) {
          if (data_buffer_1 == frame->data[0]) {
            data_buffer_1 = new uint8_t[frame_size];
            data_buffer_2 = new uint8_t[frame_size];

            memcpy(data_buffer_1, frame->data[0], frame_size);
          }

          e->process_image(get_timecode(this, cacher_frame),
                           using_db_1 ? data_buffer_1 : data_buffer_2,
                           using_db_1 ? data_buffer_2 : data_buffer_1,
                           frame_size
                           );

          using_db_1 = !using_db_1;
        }
      }

      texture->setData(QOpenGLTexture::RGBA,
                          QOpenGLTexture::UInt8,
                          const_cast<const uint8_t*>(using_db_1 ? data_buffer_1 : data_buffer_2));

      if (data_buffer_1 != frame->data[0]) {
        qDebug() << data_buffer_1 << frame->data[0];
        delete [] data_buffer_1;
        delete [] data_buffer_2;
      }

      glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

      ret = true;
    } else {
      qCritical() << "Failed to retrieve frame for clip" << name();
    }

    cacher.queue()->unlock();
  }

  return ret;
}

bool Clip::UsesCacher()
{
  return track() >= 0 || (media() != nullptr && media()->get_type() == MEDIA_TYPE_FOOTAGE);
}
