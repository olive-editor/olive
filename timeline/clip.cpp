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

#include "nodes/node.h"
#include "effects/transition.h"
#include "project/footage.h"
#include "global/config.h"
#include "rendering/cacher.h"
#include "rendering/renderfunctions.h"
#include "panels/project.h"
#include "timeline/sequence.h"
#include "panels/timeline.h"
#include "project/media.h"
#include "undo/undo.h"
#include "global/clipboard.h"
#include "global/debug.h"
#include "global/timing.h"

Clip::Clip(Track *s) :
  track_(s),
  cacher(this),
  enabled_(true),
  clip_in_(0),
  timeline_in_(0),
  timeline_out_(0),
  media_(nullptr),
  reverse_(false),
  autoscale_(olive::config.autoscale_by_default),
  opening_transition(nullptr),
  closing_transition(nullptr),
  undeletable(false),
  replaced(false),
  open_(false),
  texture(0)
{
}

ClipPtr Clip::copy(Track* s) {
  ClipPtr copy = std::make_shared<Clip>(s);

  copy->set_enabled(enabled());
  copy->set_name(name());
  copy->set_clip_in(clip_in());
  copy->set_timeline_in(timeline_in());
  copy->set_timeline_out(timeline_out());
  copy->set_color(color());
  copy->set_media(media(), media_stream_index());
  copy->set_autoscaled(autoscaled());
  copy->set_speed(speed());
  copy->set_reversed(reversed());

  for (int i=0;i<effects.size();i++) {
    copy->effects.append(effects.at(i)->copy(copy.get()));
  }

  copy->set_cached_frame_rate((this->track_ == nullptr) ? cached_frame_rate() : this->track_->sequence()->frame_rate);

  copy->refresh();

  return copy;
}

bool Clip::IsActiveAt(long timecode)
{
  return enabled()
      && timeline_in(true) <= timecode
      && timeline_out(true) > timecode
      && timecode - timeline_in(true) + clip_in(true) < media_length()
      && !track()->IsEffectivelyMuted();
}

bool Clip::IsSelected(bool containing)
{
  if (this->track_ == nullptr) {
    return false;
  }

  return this->track_->IsClipSelected(this, containing);
}

bool Clip::IsTransitionSelected(TransitionType type)
{
  switch (type) {
  case kTransitionOpening:
    return track_->IsTransitionSelected(opening_transition.get());
  case kTransitionClosing:
    return track_->IsTransitionSelected(closing_transition.get());
  default:
    return false;
  }
}

Selection Clip::ToSelection()
{
  return Selection(timeline_in(), timeline_out(), track());
}

olive::TrackType Clip::type()
{
  return track()->type();
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
    return media()->to_footage()->get_stream_from_file_index(type() == olive::kTypeVideo, media_stream_index());
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

void Clip::Move(ComboAction *ca, long iin, long iout, long iclip_in, Track *itrack, bool verify_transitions, bool relative)
{
  track()->sequence()->MoveClip(this, ca, iin, iout, iclip_in, itrack, verify_transitions, relative);
}

bool Clip::enabled()
{
  return enabled_;
}

void Clip::set_enabled(bool e)
{
  enabled_ = e;
}

void Clip::reset_audio() {
  if (UsesCacher()) {
    cacher.ResetAudio();
  }
  if (media() != nullptr && media()->get_type() == MEDIA_TYPE_SEQUENCE) {

    QVector<Clip*> nested_sequence_clips = media()->to_sequence()->GetAllClips();

    for (int i=0;i<nested_sequence_clips.size();i++) {
      nested_sequence_clips.at(i)->reset_audio();
    }

  }
}

void Clip::refresh() {
  // validates media if it was replaced
  if (replaced && media() != nullptr && media()->get_type() == MEDIA_TYPE_FOOTAGE) {
    Footage* m = media()->to_footage();

    if (type() == olive::kTypeVideo && m->video_tracks.size() > 0)  {
      set_media(media(), m->video_tracks.at(0).file_index);
    } else if (type() == olive::kTypeAudio && m->audio_tracks.size() > 0) {
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

int Clip::IndexOfEffect(Node *e)
{
  for (int i=0;i<effects.size();i++) {
    if (effects.at(i).get() == e) {
      return i;
    }
  }
  return -1;
}

Clip::~Clip() {
  if (IsOpen()) {
    Close(true);
  }
}

void Clip::Save(QXmlStreamWriter &stream)
{
  stream.writeStartElement("clip");
  stream.writeAttribute("id", QString::number(load_id));

  stream.writeAttribute("enabled", QString::number(enabled()));
  stream.writeAttribute("name", name());
  stream.writeAttribute("clipin", QString::number(clip_in()));
  stream.writeAttribute("in", QString::number(timeline_in()));
  stream.writeAttribute("out", QString::number(timeline_out()));

  stream.writeAttribute("r", QString::number(color().red()));
  stream.writeAttribute("g", QString::number(color().green()));
  stream.writeAttribute("b", QString::number(color().blue()));

  stream.writeAttribute("autoscale", QString::number(autoscaled()));
  stream.writeAttribute("speed", QString::number(speed().value, 'f', 10));
  stream.writeAttribute("maintainpitch", QString::number(speed().maintain_audio_pitch));
  stream.writeAttribute("reverse", QString::number(reversed()));

  if (media() != nullptr) {
    stream.writeAttribute("type", QString::number(media()->get_type()));
    switch (media()->get_type()) {
    case MEDIA_TYPE_FOOTAGE:
      stream.writeAttribute("media", QString::number(media()->to_footage()->save_id));
      stream.writeAttribute("stream", QString::number(media_stream_index()));
      break;
    case MEDIA_TYPE_SEQUENCE:
      stream.writeAttribute("sequence", QString::number(media()->to_sequence()->save_id));
      break;
    }
  }

  // save markers
  // only necessary for null media clips, since media has its own markers
  if (media() == nullptr) {
    for (int k=0;k<get_markers().size();k++) {
      get_markers().at(k).Save(stream);
    }
  }

  // save clip links
  stream.writeStartElement("linked"); // linked
  for (int k=0;k<linked.size();k++) {
    stream.writeStartElement("link"); // link
    stream.writeAttribute("id", QString::number(linked.at(k)->load_id));
    stream.writeEndElement(); // link
  }
  stream.writeEndElement(); // linked

  // save opening and closing transitions
  for (int t=kTransitionOpening;t<=kTransitionClosing;t++) {
    TransitionPtr transition = (t == kTransitionOpening) ? opening_transition : closing_transition;

    if (transition != nullptr) {
      stream.writeStartElement((t == kTransitionOpening) ? "opening" : "closing");

      // check if this is a shared transition
      if (this == transition->secondary_clip) {
        // if so, just save a reference to the other clip
        stream.writeAttribute("shared",
                              QString::number(transition->parent_clip->load_id));
      } else {
        // otherwise save the whole transition
        transition->save(stream);
      }

      stream.writeEndElement(); // opening/closing
    }
  }

  for (int k=0;k<effects.size();k++) {
    stream.writeStartElement("effect"); // effect
    effects.at(k)->save(stream);
    stream.writeEndElement(); // effect
  }

  stream.writeEndElement(); // clip
}

long Clip::clip_in(bool with_transition) {
  if (with_transition && opening_transition != nullptr && opening_transition->secondary_clip != nullptr) {
    // we must be the secondary clip, so return (clip in - length)
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
    // we must be the primary clip, so return (timeline out + length)
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

Track *Clip::track()
{
  return track_;
}

void Clip::set_track(Track *t)
{
  track_ = t;
}

// timeline functions
long Clip::length() {
  return timeline_out_ - timeline_in_;
}

double Clip::media_frame_rate() {
  Q_ASSERT(type() == olive::kTypeVideo);
  if (media_ != nullptr) {
    double rate = media_->get_frame_rate(media_stream_index());
    if (!qIsNaN(rate)) return rate;
  }
  if (track() != nullptr) return track()->sequence()->frame_rate;
  return qSNaN();
}

long Clip::media_length() {
  if (this->track() != nullptr) {
    double fr = this->track()->sequence()->frame_rate;

    fr /= speed_.value;

    if (media_ == nullptr) {
      return LONG_MAX;
    } else {
      switch (media_->get_type()) {
      case MEDIA_TYPE_FOOTAGE:
      {
        Footage* m = media_->to_footage();
        const FootageStream* ms = m->get_stream_from_file_index(type() == olive::kTypeVideo, media_stream_index());
        if (ms != nullptr && ms->infinite_length) {
          return LONG_MAX;
        } else {
          return m->get_length_in_frames(fr);
        }
      }
      case MEDIA_TYPE_SEQUENCE:
      {
        Sequence* s = media_->to_sequence().get();
        return rescale_frame_number(s->GetEndFrame(), s->frame_rate, fr);
      }
      }
    }
  }
  return 0;
}

int Clip::media_width() {
  if (media_ == nullptr && track() != nullptr) return track()->sequence()->width;
  switch (media_->get_type()) {
  case MEDIA_TYPE_FOOTAGE:
  {
    const FootageStream* ms = media_stream();
    if (ms != nullptr) return ms->video_width;
    if (track() != nullptr) return track()->sequence()->width;
    break;
  }
  case MEDIA_TYPE_SEQUENCE:
  {
    Sequence* s = media_->to_sequence().get();
    return s->width;
  }
  }
  return 0;
}

int Clip::media_height() {
  if (media_ == nullptr && track() != nullptr) return track()->sequence()->height;
  switch (media_->get_type()) {
  case MEDIA_TYPE_FOOTAGE:
  {
    const FootageStream* ms = media_stream();
    if (ms != nullptr) return ms->video_height;
    if (track() != nullptr) return track()->sequence()->height;
  }
    break;
  case MEDIA_TYPE_SEQUENCE:
  {
    Sequence* s = media_->to_sequence().get();
    return s->height;
  }
  }
  return 0;
}

void Clip::refactor_frame_rate(ComboAction* ca, double multiplier, bool change_timeline_points) {
  if (change_timeline_points) {
    track()->sequence()->MoveClip(this,
                                  ca,
                                  qRound(double(timeline_in_) * multiplier),
                                  qRound(double(timeline_out_) * multiplier),
                                  qRound(double(clip_in_) * multiplier),
                                  track_);
  }

  // move keyframes
  for (int i=0;i<effects.size();i++) {
    NodePtr e = effects.at(i);
    for (int j=0;j<e->row_count();j++) {
      EffectRow* r = e->row(j);
      for (int l=0;l<r->FieldCount();l++) {
        EffectField* f = r->Field(l);
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
    texture_timestamp = -1;

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
      media()->to_sequence()->Close();
    }

    // destroy opengl texture in main thread
    if (texture > 0) {
      QOpenGLContext::currentContext()->functions()->glDeleteTextures(1, &texture);
      texture = 0;
    }

    // close all effects
    for (int i=0;i<effects.size();i++) {
      if (effects.at(i)->is_open()) {
        effects.at(i)->close();
      }
    }

    // delete framebuffers
    fbo.clear();

    // delete OCIO shader
    ocio_shader = nullptr;

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

      //if (frame->pts != texture_timestamp) {

      bool allocate_data = false;

      QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();

      // check if the opengl texture exists yet, create it if not
      if (texture == 0) {

        // create texture object
        f->glGenTextures(1, &texture);

        f->glBindTexture(GL_TEXTURE_2D, texture);

        // set texture filtering to bilinear
        f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // set texture wrapping to clamp
        f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // queue an allocation ahead
        allocate_data = true;

      } else {

        f->glBindTexture(GL_TEXTURE_2D, texture);

      }

      int video_width = cacher.media_width();
      int video_height = cacher.media_height();

      const olive::PixelFormatInfo& pix_fmt_info = olive::pixel_formats.at(cacher.media_pixel_format());

      f->glPixelStorei(GL_UNPACK_ROW_LENGTH, frame->linesize[0]/pix_fmt_info.bytes_per_pixel);

      if (allocate_data) {

        // the raw frame size may differ from the one we're using (e.g. a lower resolution proxy), so we make sure
        // the texture is using the correct dimensions, but then treat it as if it's the original resolution in the
        // composition
        f->glTexImage2D(
              GL_TEXTURE_2D,
              0,
              pix_fmt_info.internal_format,
              video_width,
              video_height,
              0,
              pix_fmt_info.pixel_format,
              pix_fmt_info.pixel_type,
              frame->data[0]
            );

      } else {

        f->glTexSubImage2D(GL_TEXTURE_2D,
                           0,
                           0,
                           0,
                           video_width,
                           video_height,
                           pix_fmt_info.pixel_format,
                           pix_fmt_info.pixel_type,
                           frame->data[0]
            );

      }

      f->glBindTexture(GL_TEXTURE_2D, 0);

      f->glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

      texture_timestamp = frame->pts;

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
  return type() == olive::kTypeAudio || (media() != nullptr && media()->get_type() == MEDIA_TYPE_FOOTAGE);
}

ClipSpeed::ClipSpeed() :
  value(1.0),
  maintain_audio_pitch(false)
{
}
