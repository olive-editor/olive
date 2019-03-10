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

#include "renderfunctions.h"

extern "C" {
#include <libavformat/avformat.h>
}

#include <QOpenGLFramebufferObject>
#include <QApplication>
#include <QDesktopWidget>
#include <QDebug>

#ifdef OLIVE_OCIO
#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;
#endif

#include "project/clip.h"
#include "project/sequence.h"
#include "project/media.h"
#include "project/effect.h"
#include "project/footage.h"
#include "project/transition.h"

#include "ui/collapsiblewidget.h"

#include "rendering/audio.h"

#include "io/math.h"
#include "io/config.h"

#include "panels/timeline.h"
#include "panels/viewer.h"

const int kMaximumRetryCount = 10;

void full_blit() {
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0, 1, 0, 1, -1, 1);

  glBegin(GL_QUADS);
  glTexCoord2f(0, 0); // top left
  glVertex2f(0, 0); // top left
  glTexCoord2f(1, 0); // top right
  glVertex2f(1, 0); // top right
  glTexCoord2f(1, 1); // bottom right
  glVertex2f(1, 1); // bottom right
  glTexCoord2f(0, 1); // bottom left
  glVertex2f(0, 1); // bottom left
  glEnd();

  glPopMatrix();
}

void draw_clip(QOpenGLContext* ctx, GLuint fbo, GLuint texture, bool clear) {
  ctx->functions()->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);

  if (clear) {
    glClear(GL_COLOR_BUFFER_BIT);
  }

  glBindTexture(GL_TEXTURE_2D, texture);

  full_blit();

  glBindTexture(GL_TEXTURE_2D, 0);

  ctx->functions()->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

GLuint draw_clip(QOpenGLFramebufferObject* fbo, GLuint texture, bool clear) {
  fbo->bind();

  if (clear) {
    glClear(GL_COLOR_BUFFER_BIT);
  }

  glBindTexture(GL_TEXTURE_2D, texture);

  full_blit();

  glBindTexture(GL_TEXTURE_2D, 0);

  fbo->release();

  return fbo->texture();
}

void process_effect(Clip* c,
                    Effect* e,
                    double timecode,
                    GLTextureCoords& coords,
                    GLuint& composite_texture,
                    bool& fbo_switcher,
                    bool& texture_failed,
                    int data) {
  if (e->is_enabled()) {
    if (e->enable_coords) {
      e->process_coords(timecode, coords, data);
    }
    bool can_process_shaders = (e->enable_shader && olive::CurrentRuntimeConfig.shaders_are_enabled);
    if (can_process_shaders || e->enable_superimpose) {
      e->startEffect();
      if (can_process_shaders && e->is_glsl_linked()) {
        for (int i=0;i<e->getIterations();i++) {
          e->process_shader(timecode, coords, i);
          composite_texture = draw_clip(c->fbo[fbo_switcher], composite_texture, true);
          fbo_switcher = !fbo_switcher;
        }
      }
      if (e->enable_superimpose) {
        GLuint superimpose_texture = e->process_superimpose(timecode);

        if (superimpose_texture == 0) {
          qWarning() << "Superimpose texture was nullptr, retrying...";
          texture_failed = true;
        } else if (composite_texture == 0) {
          // if there is no previous texture, just return the superimposes texture
          // UNLESS this is a shader-extended superimpose effect in which case,
          // we'll need to draw it below
          composite_texture = superimpose_texture;
        } else {
          // if the source texture is not already a framebuffer texture,
          // we'll need to make it one before drawing a superimpose effect on it
          if (composite_texture != c->fbo[0]->texture() && composite_texture != c->fbo[1]->texture()) {
            draw_clip(c->fbo[!fbo_switcher], composite_texture, true);
          }

          composite_texture = draw_clip(c->fbo[!fbo_switcher], superimpose_texture, false);
        }
      }
      e->endEffect();
    }
  }
}

GLuint compose_sequence(ComposeSequenceParams &params) {
//  qint64 time = QDateTime::currentMSecsSinceEpoch();

  GLuint final_fbo = params.main_buffer;

  SequencePtr s = params.seq;
  long playhead = s->playhead;

  if (!params.nests.isEmpty()) {
    for (int i=0;i<params.nests.size();i++) {
      s = params.nests.at(i)->media()->to_sequence();
      playhead += params.nests.at(i)->clip_in(true) - params.nests.at(i)->timeline_in(true);
      playhead = rescale_frame_number(playhead, params.nests.at(i)->sequence->frame_rate, s->frame_rate);
    }

    if (params.video && params.nests.last()->fbo != nullptr) {
      params.nests.last()->fbo[0]->bind();
      glClear(GL_COLOR_BUFFER_BIT);
      final_fbo = params.nests.last()->fbo[0]->handle();
    }
  }

  int audio_track_count = 0;

  QVector<Clip*> current_clips;

  // loop through clips, find currently active, and sort by track
  for (int i=0;i<s->clips.size();i++) {

    Clip* c = s->clips.at(i).get();

    if (c != nullptr) {

      // if clip is video and we're processing video
      if ((c->track() < 0) == params.video) {

        bool clip_is_active = false;

        // is the clip a "footage" clip?
        if (c->media() != nullptr && c->media()->get_type() == MEDIA_TYPE_FOOTAGE) {
          FootagePtr m = c->media()->to_footage();

          // does the clip have a valid media source?
          if (!m->invalid && !(c->track() >= 0 && !is_audio_device_set())) {

            // is the media process and ready?
            if (m->ready) {
              const FootageStream* ms = c->media_stream();

              // does the media have a valid media stream source and is it active?
              if (ms != nullptr && c->IsActiveAt(playhead)) {

                // open if not open
                if (!c->IsOpen()) {
                  c->Open();
                }

                clip_is_active = true;

                // increment audio track count
                if (c->track() >= 0) audio_track_count++;

              } else if (c->IsOpen()) {

                // close the clip if it isn't active anymore
                c->Close(false);

              }
            } else {

              // media wasn't ready, schedule a redraw
              params.texture_failed = true;

            }
          }
        } else {
          // if the clip is a nested sequence or null clip, just open it

          if (c->IsActiveAt(playhead)) {
            if (!c->IsOpen()) {
              c->Open();
            }
            clip_is_active = true;
          } else if (c->IsOpen()) {
            c->Close(false);
          }
        }

        // if the clip is active, added it to "current_clips", sorted by track
        if (clip_is_active) {
          bool added = false;

          // track sorting is only necessary for video clips
          // audio clips are mixed equally, so we skip sorting for those
          if (params.video) {

            // insertion sort by track
            for (int j=0;j<current_clips.size();j++) {
              if (current_clips.at(j)->track() < c->track()) {
                current_clips.insert(j, c);
                added = true;
                break;
              }
            }

          }

          if (!added) {
            current_clips.append(c);
          }
        }
      }
    }
  }

  if (params.video) {
    // set default coordinates based on the sequence, with 0 in the direct center
    glPushMatrix();
    glLoadIdentity();

    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    int half_width = s->width/2;
    int half_height = s->height/2;
    glOrtho(-half_width, half_width, -half_height, half_height, -1, 10);
  }

  // loop through current clips

  for (int i=0;i<current_clips.size();i++) {
    Clip* c = current_clips.at(i);

    bool got_mutex = true;

    if (params.wait_for_mutexes) {
      // wait for clip to finish opening
      c->state_change_lock.lock();
    } else {
      got_mutex = c->state_change_lock.tryLock();
    }

    if (got_mutex && c->IsOpen()) {
      // if clip is a video clip
      if (c->track() < 0) {

        // reset OpenGL to full color
        glColor4f(1.0, 1.0, 1.0, 1.0);

        // textureID variable contains texture to be drawn on screen at the end
        GLuint textureID = 0;

        // store video source dimensions
        int video_width = c->media_width();
        int video_height = c->media_height();

        // if media is footage
        if (c->media() != nullptr && c->media()->get_type() == MEDIA_TYPE_FOOTAGE) {

          // retrieve video frame from cache and store it in c->texture
          c->Cache(qMax(playhead, c->timeline_in()), false, params.nests, params.playback_speed);
          if (!c->Retrieve()) {
            params.texture_failed = true;
          } else {
            // retrieve ID from c->texture
            textureID = c->texture->textureId();
          }

          if (textureID == 0) {
            qWarning() << "Failed to create texture";
          }
        }

        // prepare framebuffers for backend drawing operations
        if (c->fbo == nullptr) {
          // create 3 fbos for nested sequences, 2 for most clips
          int fbo_count = (c->media() != nullptr && c->media()->get_type() == MEDIA_TYPE_SEQUENCE) ? 3 : 2;

          c->fbo = new QOpenGLFramebufferObject* [size_t(fbo_count)];

          for (int j=0;j<fbo_count;j++) {
            c->fbo[j] = new QOpenGLFramebufferObject(video_width, video_height);
          }
        }

        // if clip should actually be shown on screen in this frame
        if (playhead >= c->timeline_in(true)
            && playhead < c->timeline_out(true)) {
          glPushMatrix();

          // simple bool for switching between the two framebuffers
          bool fbo_switcher = false;

          glViewport(0, 0, video_width, video_height);

          if (c->media() != nullptr) {
            if (c->media()->get_type() == MEDIA_TYPE_SEQUENCE) {
              // for a nested sequence, run this function again on that sequence and retrieve the texture

              // add nested sequence to nest list
              params.nests.append(c);

              // compose sequence
              textureID = compose_sequence(params);

              // remove sequence from nest list
              params.nests.removeLast();

              // compose_sequence() would have written to this clip's fbo[0], so we switch to fbo[1]
              fbo_switcher = true;
            } else if (c->media()->get_type() == MEDIA_TYPE_FOOTAGE) {

              if (!c->media()->to_footage()->alpha_is_premultiplied) {
                // alpha is not premultiplied, we'll need to multiply it for the rest of the pipeline
                params.ctx->functions()->glBlendFuncSeparate(GL_SRC_ALPHA, GL_ZERO, GL_ONE, GL_ZERO);

                textureID = draw_clip(c->fbo[fbo_switcher], textureID, true);

                glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

                fbo_switcher = !fbo_switcher;
              }

#ifdef OLIVE_OCIO
              // convert to linear colorspace
              if (olive::CurrentConfig.enable_color_management)
              {
                params.ocio_shader->bind();

                params.ocio_shader->setUniformValue("tex1", 0);
                params.ocio_shader->setUniformValue("tex2", 2);

                textureID = draw_clip(c->fbo[fbo_switcher], textureID, true);

                params.ocio_shader->release();

                fbo_switcher = !fbo_switcher;
              }
#endif

            }
          }

          // set up default coordinates for drawing the clip
          GLTextureCoords coords;
          coords.grid_size = 1;
          coords.vertexTopLeftX = coords.vertexBottomLeftX = -video_width/2;
          coords.vertexTopLeftY = coords.vertexTopRightY = -video_height/2;
          coords.vertexTopRightX = coords.vertexBottomRightX = video_width/2;
          coords.vertexBottomLeftY = coords.vertexBottomRightY = video_height/2;
          coords.vertexBottomLeftZ = coords.vertexBottomRightZ = coords.vertexTopLeftZ = coords.vertexTopRightZ = 1;
          coords.textureTopLeftY = coords.textureTopRightY = coords.textureTopLeftX = coords.textureBottomLeftX = 0.0;
          coords.textureBottomLeftY = coords.textureBottomRightY = coords.textureTopRightX = coords.textureBottomRightX = 1.0;
          coords.textureTopLeftQ = coords.textureTopRightQ = coords.textureTopLeftQ = coords.textureBottomLeftQ = 1;
          coords.blendmode = -1;
          coords.opacity = 1.0;

          // if auto-scale is enabled, auto-scale the clip
          if (c->autoscaled() && (video_width != s->width && video_height != s->height)) {
            float width_multiplier = float(s->width) / float(video_width);
            float height_multiplier = float(s->height) / float(video_height);
            float scale_multiplier = qMin(width_multiplier, height_multiplier);
            glScalef(scale_multiplier, scale_multiplier, 1);
          }

          // == EFFECT CODE START ==

          // get current sequence time in seconds (used for effects)
          double timecode = get_timecode(c, playhead);

          // run through all of the clip's effects
          for (int j=0;j<c->effects.size();j++) {
            Effect* e = c->effects.at(j).get();
            process_effect(c, e, timecode, coords, textureID, fbo_switcher, params.texture_failed, kTransitionNone);

            if (e == params.gizmos) {
              e->gizmo_draw(timecode, coords); // set correct gizmo coords
              e->gizmo_world_to_screen(); // convert gizmo coords to screen coords
            }
          }

          // if the clip has an opening transition, process that now
          if (c->opening_transition != nullptr) {
            int transition_progress = playhead - c->timeline_in(true);
            if (transition_progress < c->opening_transition->get_length()) {
              process_effect(c, c->opening_transition.get(), double(transition_progress)/double(c->opening_transition->get_length()), coords, textureID, fbo_switcher, params.texture_failed, kTransitionOpening);
            }
          }

          // if the clip has a closing transition, process that now
          if (c->closing_transition != nullptr) {
            int transition_progress = playhead - (c->timeline_out(true) - c->closing_transition->get_length());
            if (transition_progress >= 0 && transition_progress < c->closing_transition->get_length()) {
              process_effect(c, c->closing_transition.get(), double(transition_progress)/double(c->closing_transition->get_length()), coords, textureID, fbo_switcher, params.texture_failed, kTransitionClosing);
            }
          }

          // == EFFECT CODE END ==

          if (textureID > 0) {
            // set viewport to sequence size
            params.ctx->functions()->glViewport(0, 0, s->width, s->height);



            // == START RENDER CLIP IN CONTEXT OF SEQUENCE ==



            // use clip textures for nested sequences, otherwise use main frame buffers
            GLuint back_buffer_1;
            GLuint backend_tex_1;
            GLuint backend_tex_2;
            if (params.nests.size() > 0) {
              back_buffer_1 = params.nests.last()->fbo[1]->handle();
              backend_tex_1 = params.nests.last()->fbo[1]->texture();
              backend_tex_2 = params.nests.last()->fbo[2]->texture();
            } else {
              back_buffer_1 = params.backend_buffer1;
              backend_tex_1 = params.backend_attachment1;
              backend_tex_2 = params.backend_attachment2;
            }

            // render a backbuffer
            params.ctx->functions()->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, back_buffer_1);

            glClearColor(0.0, 0.0, 0.0, 0.0);
            glClear(GL_COLOR_BUFFER_BIT);

            // bind final clip texture
            glBindTexture(GL_TEXTURE_2D, textureID);

            // set texture filter to bilinear
            params.ctx->functions()->glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            params.ctx->functions()->glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            // draw clip on screen according to gl coordinates
            glBegin(GL_QUADS);

            glTexCoord2f(coords.textureTopLeftX, coords.textureTopLeftY); // top left
            glVertex2f(coords.vertexTopLeftX, coords.vertexTopLeftY); // top left
            glTexCoord2f(coords.textureTopRightX, coords.textureTopRightY); // top right
            glVertex2f(coords.vertexTopRightX, coords.vertexTopRightY); // top right
            glTexCoord2f(coords.textureBottomRightX, coords.textureBottomRightY); // bottom right
            glVertex2f(coords.vertexBottomRightX, coords.vertexBottomRightY); // bottom right
            glTexCoord2f(coords.textureBottomLeftX, coords.textureBottomLeftY); // bottom left
            glVertex2f(coords.vertexBottomLeftX, coords.vertexBottomLeftY); // bottom left

            glEnd();

            // release final clip texture
            glBindTexture(GL_TEXTURE_2D, 0);

            params.ctx->functions()->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);



            // == END RENDER CLIP IN CONTEXT OF SEQUENCE ==



            //
            //
            // PROCESS POST-SHADERS
            //
            //



            // copy front buffer to back buffer (only if we're using a blending mode)
            if (coords.blendmode >= 0) {
              if (params.nests.size() > 0) {
                draw_clip(params.ctx, params.nests.last()->fbo[2]->handle(), params.nests.last()->fbo[0]->texture(), true);
              } else {
                draw_clip(params.ctx, params.backend_buffer2, params.main_attachment, true);
              }
            }



            // == START FINAL DRAW ON SEQUENCE BUFFER ==




            // bind front buffer as draw buffer
            params.ctx->functions()->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, final_fbo);

            // Check if we're using a blend mode (< 0 means no blend mode)
            if (coords.blendmode < 0) {

              params.ctx->functions()->glBindTexture(GL_TEXTURE_2D, backend_tex_1);

              glColor4f(coords.opacity, coords.opacity, coords.opacity, coords.opacity);

              full_blit();

              params.ctx->functions()->glBindTexture(GL_TEXTURE_2D, 0);

            } else {

              // load background texture into texture unit 0
              params.ctx->functions()->glActiveTexture(GL_TEXTURE0 + 0); // Texture unit 0
              params.ctx->functions()->glBindTexture(GL_TEXTURE_2D, backend_tex_2);

              // load foreground texture into texture unit 1
              params.ctx->functions()->glActiveTexture(GL_TEXTURE0 + 1); // Texture unit 1
              params.ctx->functions()->glBindTexture(GL_TEXTURE_2D, backend_tex_1);

              // bind and configure blending mode shader
              params.blend_mode_program->bind();
              params.blend_mode_program->setUniformValue("blendmode", coords.blendmode);
              params.blend_mode_program->setUniformValue("opacity", coords.opacity);
              params.blend_mode_program->setUniformValue("background", 0);
              params.blend_mode_program->setUniformValue("foreground", 1);

              glClear(GL_COLOR_BUFFER_BIT);

              full_blit();

              // release blend mode shader
              params.blend_mode_program->release();

              // unbind texture from texture unit 1
              params.ctx->functions()->glBindTexture(GL_TEXTURE_2D, 0);

              // unbind texture from texture unit 0
              params.ctx->functions()->glActiveTexture(GL_TEXTURE0 + 0); // Texture unit 0
              params.ctx->functions()->glBindTexture(GL_TEXTURE_2D, 0);

            }

            // unbind framebuffer
            params.ctx->functions()->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);




            // == END FINAL DRAW ON SEQUENCE BUFFER ==
          }

          // prepare gizmos
          /*
          if ((*params.gizmos) != nullptr
              && params.nests.isEmpty()
              && ((*params.gizmos) == first_gizmo_effect
                  || (*params.gizmos) == selected_effect)) {
            (*params.gizmos)->gizmo_draw(timecode, coords); // set correct gizmo coords
            (*params.gizmos)->gizmo_world_to_screen(); // convert gizmo coords to screen coords
          }
          */

          glPopMatrix();
        }
      } else {
        if (c->media() != nullptr && c->media()->get_type() == MEDIA_TYPE_SEQUENCE) {
          params.nests.append(c);
          compose_sequence(params);
          params.nests.removeLast();
        } else {
          // Check whether cacher is currently active, if not activate it now
          if (c->cache_lock.tryLock()) {

            c->cache_lock.unlock();

            c->Cache(playhead,
                     (params.viewer != nullptr && !params.viewer->playing),
                     params.nests,
                     params.playback_speed);

          }
        }

        // visually update all the keyframe values
        if (c->sequence == params.seq) { // only if you can currently see them
          double ts = (playhead - c->timeline_in(true) + c->clip_in(true))/s->frame_rate;
          for (int i=0;i<c->effects.size();i++) {
            EffectPtr e = c->effects.at(i);
            for (int j=0;j<e->row_count();j++) {
              EffectRow* r = e->row(j);
              for (int k=0;k<r->fieldCount();k++) {
                r->field(k)->validate_keyframe_data(ts);
              }
            }
          }
        }
      }
    } else {
      params.texture_failed = true;
    }

    if (got_mutex) {
      c->state_change_lock.unlock();
    }
  }

  if (audio_track_count == 0 && params.viewer != nullptr) {
    params.viewer->play_wake();
  }

  if (params.video) {
    glPopMatrix();
  }

//  qDebug() << "compose sequence took" << QDateTime::currentMSecsSinceEpoch() - time;

  if (!params.nests.isEmpty() && params.nests.last()->fbo != nullptr) {
    // returns nested clip's texture
    return params.nests.last()->fbo[0]->texture();
  }

  return 0;
}

void compose_audio(Viewer* viewer, SequencePtr seq, int playback_speed, bool wait_for_mutexes) {
  ComposeSequenceParams params;
  params.viewer = viewer;
  params.ctx = nullptr;
  params.seq = seq;
  params.video = false;
  params.gizmos = nullptr;
  params.wait_for_mutexes = wait_for_mutexes;
  params.playback_speed = playback_speed;
  params.blend_mode_program = nullptr;
  compose_sequence(params);
}

long rescale_frame_number(long framenumber, double source_frame_rate, double target_frame_rate) {
  return qRound((double(framenumber)/source_frame_rate)*target_frame_rate);
}

double get_timecode(Clip* c, long playhead) {
  return double(playhead_to_clip_frame(c, playhead))/c->sequence->frame_rate;
}

long playhead_to_clip_frame(Clip* c, long playhead) {
  return (qMax(0L, playhead - c->timeline_in(true)) + c->clip_in(true));
}

double playhead_to_clip_seconds(Clip* c, long playhead) {
  // returns time in seconds
  long clip_frame = playhead_to_clip_frame(c, playhead);

  if (c->reversed()) {
    clip_frame = c->media_length() - clip_frame - 1;
  }

  double secs = (double(clip_frame)/c->sequence->frame_rate)*c->speed().value;
  if (c->media() != nullptr && c->media()->get_type() == MEDIA_TYPE_FOOTAGE) {
    secs *= c->media()->to_footage()->speed;
  }

  return secs;
}

int64_t seconds_to_timestamp(Clip *c, double seconds) {
  return qRound64(seconds * av_q2d(av_inv_q(c->time_base())));
}

int64_t playhead_to_timestamp(Clip* c, long playhead) {
  return seconds_to_timestamp(c, playhead_to_clip_seconds(c, playhead));
}

void close_active_clips(SequencePtr s) {
  if (s != nullptr) {
    for (int i=0;i<s->clips.size();i++) {
      Clip* c = s->clips.at(i).get();
      if (c != nullptr) {
        c->Close(true);
      }
    }
  }
}

void UpdateOCIOGLState(const ComposeSequenceParams& params)
{
}
