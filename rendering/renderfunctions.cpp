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

#include <QApplication>
#include <QDesktopWidget>
#include <QDebug>

#ifndef NO_OCIO
#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;
#endif

#include "timeline/clip.h"
#include "timeline/sequence.h"
#include "project/media.h"
#include "effects/effect.h"
#include "project/footage.h"
#include "effects/transition.h"
#include "ui/collapsiblewidget.h"
#include "rendering/audio.h"
#include "global/math.h"
#include "global/config.h"
#include "panels/timeline.h"
#include "panels/viewer.h"
#include "qopenglshaderprogramptr.h"

GLfloat olive::rendering::blit_vertices[] = {
  -1.0f, -1.0f, 0.0f,
  1.0f, -1.0f, 0.0f,
  1.0f, 1.0f, 0.0f,

  -1.0f, -1.0f, 0.0f,
  -1.0f, 1.0f, 0.0f,
  1.0f, 1.0f, 0.0f
};

GLfloat olive::rendering::blit_texcoords[] = {
  0.0, 0.0,
  1.0, 0.0,
  1.0, 1.0,

  0.0, 0.0,
  0.0, 1.0,
  1.0, 1.0
};

void olive::rendering::Blit(QOpenGLShaderProgram* pipeline) {
  QOpenGLFunctions* func = QOpenGLContext::currentContext()->functions();

  pipeline->bind();

  pipeline->setUniformValue("mvp_matrix", QMatrix4x4());
  pipeline->setUniformValue("texture", 0);

  GLuint vertex_location = pipeline->attributeLocation("a_position");
  func->glEnableVertexAttribArray(vertex_location);
  func->glVertexAttribPointer(vertex_location, 3, GL_FLOAT, GL_FALSE, 0, blit_vertices);

  GLuint tex_location = pipeline->attributeLocation("a_texcoord");
  func->glEnableVertexAttribArray(tex_location);
  func->glVertexAttribPointer(tex_location, 2, GL_FLOAT, GL_FALSE, 0, blit_texcoords);

  func->glDrawArrays(GL_TRIANGLES, 0, 6);

  pipeline->release();
}

QOpenGLShaderProgramPtr olive::rendering::GetPipeline()
{
  QOpenGLShaderProgramPtr program = std::make_shared<QOpenGLShaderProgram>();

  program->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/internalshaders/pipeline.vert");
  program->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/internalshaders/pipeline.frag");
  program->link();

  return program;
}

void draw_clip(QOpenGLContext* ctx,
               QOpenGLShaderProgram* pipeline,
               GLuint fbo,
               GLuint texture,
               bool clear) {
  ctx->functions()->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);

  if (clear) {
    ctx->functions()->glClear(GL_COLOR_BUFFER_BIT);
  }

  ctx->functions()->glBindTexture(GL_TEXTURE_2D, texture);

  olive::rendering::Blit(pipeline);

  ctx->functions()->glBindTexture(GL_TEXTURE_2D, 0);

  ctx->functions()->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

GLuint draw_clip(QOpenGLContext* ctx,
                 QOpenGLShaderProgram* pipeline,
                 const FramebufferObject& fbo,
                 GLuint texture,
                 bool clear) {
  fbo.BindBuffer();

  if (clear) {
    ctx->functions()->glClear(GL_COLOR_BUFFER_BIT);
  }

  ctx->functions()->glBindTexture(GL_TEXTURE_2D, texture);

  olive::rendering::Blit(pipeline);

  ctx->functions()->glBindTexture(GL_TEXTURE_2D, 0);

  fbo.ReleaseBuffer();

  return fbo.texture();
}

void process_effect(QOpenGLContext* ctx,
                    QOpenGLShaderProgram* pipeline,
                    Clip* c,
                    Effect* e,
                    double timecode,
                    GLTextureCoords& coords,
                    GLuint& composite_texture,
                    bool& fbo_switcher,
                    bool& texture_failed,
                    int data) {
  if (e->IsEnabled()) {
    if (e->Flags() & Effect::CoordsFlag) {
      e->process_coords(timecode, coords, data);
    }
    bool can_process_shaders = ((e->Flags() & Effect::ShaderFlag) && olive::CurrentRuntimeConfig.shaders_are_enabled);
    if (can_process_shaders || (e->Flags() & Effect::SuperimposeFlag)) {
      e->startEffect();
      if (can_process_shaders && e->is_glsl_linked()) {
        for (int i=0;i<e->getIterations();i++) {
          e->process_shader(timecode, coords, i);
          composite_texture = draw_clip(ctx, pipeline, c->fbo[fbo_switcher], composite_texture, true);
          fbo_switcher = !fbo_switcher;
        }
      }
      if (e->Flags() & Effect::SuperimposeFlag) {
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
          if (composite_texture != c->fbo[0].texture() && composite_texture != c->fbo[1].texture()) {
            draw_clip(ctx, pipeline, c->fbo[!fbo_switcher], composite_texture, true);
          }

          composite_texture = draw_clip(ctx, pipeline, c->fbo[!fbo_switcher], superimpose_texture, false);
        }
      }
      e->endEffect();
    }
  }
}

GLuint compose_sequence(ComposeSequenceParams &params) {
  GLuint final_fbo = params.main_buffer;

  Sequence* s = params.seq;
  long playhead = s->playhead;

  if (!params.nests.isEmpty()) {
    for (int i=0;i<params.nests.size();i++) {
      s = params.nests.at(i)->media()->to_sequence().get();
      playhead += params.nests.at(i)->clip_in(true) - params.nests.at(i)->timeline_in(true);
      playhead = rescale_frame_number(playhead, params.nests.at(i)->sequence->frame_rate, s->frame_rate);
    }

    if (params.video && !params.nests.last()->fbo.isEmpty()) {
      params.nests.last()->fbo[0].BindBuffer();
      glClear(GL_COLOR_BUFFER_BIT);
      final_fbo = params.nests.last()->fbo[0].buffer();
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
          Footage* m = c->media()->to_footage();

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

  QMatrix4x4 projection;

  if (params.video) {
    // set default coordinates based on the sequence, with 0 in the direct center
    //glPushMatrix();
    //glLoadIdentity();

    params.ctx->functions()->glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    int half_width = s->width/2;
    int half_height = s->height/2;
    //glOrtho(-half_width, half_width, -half_height, half_height, -1, 10);
    projection.ortho(-half_width, half_width, -half_height, half_height, -1, 1);
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
        if (c->fbo.isEmpty()) {
          // create 3 fbos for nested sequences, 2 for most clips
          int fbo_count = (c->media() != nullptr && c->media()->get_type() == MEDIA_TYPE_SEQUENCE) ? 3 : 2;

          c->fbo.resize(fbo_count);

          for (int j=0;j<fbo_count;j++) {
            c->fbo[j].Create(params.ctx, video_width, video_height);
          }
        }

        // if clip should actually be shown on screen in this frame
        if (playhead >= c->timeline_in(true)
            && playhead < c->timeline_out(true)) {

          // simple bool for switching between the two framebuffers
          bool fbo_switcher = false;

          params.ctx->functions()->glViewport(0, 0, video_width, video_height);

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
              fbo_switcher = !fbo_switcher;

            } else if (c->media()->get_type() == MEDIA_TYPE_FOOTAGE) {

              if (!c->media()->to_footage()->alpha_is_premultiplied) {

                // alpha is not premultiplied, we'll need to multiply it for the rest of the pipeline
                params.ctx->functions()->glBlendFuncSeparate(GL_SRC_ALPHA, GL_ZERO, GL_ONE, GL_ZERO);

                textureID = draw_clip(params.ctx, params.pipeline, c->fbo[fbo_switcher], textureID, true);

                params.ctx->functions()->glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

                fbo_switcher = !fbo_switcher;

              }

#ifndef NO_OCIO
              // convert to linear colorspace
              if (olive::CurrentConfig.enable_color_management && params.ocio_shader != nullptr)
              {
                params.ocio_shader->bind();

                params.ocio_shader->setUniformValue("tex1", 0);
                params.ocio_shader->setUniformValue("tex2", 2);

                textureID = draw_clip(params.ctx, params.pipeline, c->fbo[fbo_switcher], textureID, true);

                params.ocio_shader->release();

                fbo_switcher = !fbo_switcher;

              }
#endif

            }
          }

          // set up default coordinates for drawing the clip
          GLTextureCoords coords;
          coords.vertex_top_left = QVector3D(-video_width/2, -video_height/2, 0.0f);
          coords.vertex_top_right = QVector3D(video_width/2, -video_height/2, 0.0f);
          coords.vertex_bottom_left = QVector3D(-video_width/2, video_height/2, 0.0f);
          coords.vertex_bottom_right = QVector3D(video_width/2, video_height/2, 0.0f);
          coords.texture_top_left = QVector2D(0.0f, 0.0f);
          coords.texture_top_right = QVector2D(1.0f, 0.0f);
          coords.texture_bottom_left = QVector2D(0.0f, 1.0f);
          coords.texture_bottom_right = QVector2D(1.0f, 1.0f);
          coords.blendmode = -1;
          coords.opacity = 1.0;

          // == EFFECT CODE START ==

          // get current sequence time in seconds (used for effects)
          double timecode = get_timecode(c, playhead);

          // run through all of the clip's effects
          for (int j=0;j<c->effects.size();j++) {

            Effect* e = c->effects.at(j).get();
            process_effect(params.ctx, params.pipeline, c, e, timecode, coords, textureID, fbo_switcher, params.texture_failed, kTransitionNone);


          }

          // if the clip has an opening transition, process that now
          if (c->opening_transition != nullptr) {
            int transition_progress = playhead - c->timeline_in(true);
            if (transition_progress < c->opening_transition->get_length()) {
              process_effect(params.ctx, params.pipeline, c, c->opening_transition.get(), double(transition_progress)/double(c->opening_transition->get_length()), coords, textureID, fbo_switcher, params.texture_failed, kTransitionOpening);
            }
          }

          // if the clip has a closing transition, process that now
          if (c->closing_transition != nullptr) {
            int transition_progress = playhead - (c->timeline_out(true) - c->closing_transition->get_length());
            if (transition_progress >= 0 && transition_progress < c->closing_transition->get_length()) {
              process_effect(params.ctx, params.pipeline, c, c->closing_transition.get(), double(transition_progress)/double(c->closing_transition->get_length()), coords, textureID, fbo_switcher, params.texture_failed, kTransitionClosing);
            }
          }

          // == EFFECT CODE END ==


          // Check whether the parent clip is auto-scaled
          // TODO redo this
          /*
          if (c->autoscaled()
              && (video_width != s->width
                  && video_height != s->height)) {
            float width_multiplier = float(s->width) / float(video_width);
            float height_multiplier = float(s->height) / float(video_height);
            float scale_multiplier = qMin(width_multiplier, height_multiplier);
            glScalef(scale_multiplier, scale_multiplier, 1);
          }
          */

          // Configure effect gizmos if they exist
          if (params.gizmos != nullptr) {
            params.gizmos->gizmo_draw(timecode, coords); // set correct gizmo coords
            params.gizmos->gizmo_world_to_screen(); // convert gizmo coords to screen coords
          }



          if (textureID > 0) {
            // set viewport to sequence size
            params.ctx->functions()->glViewport(0, 0, s->width, s->height);



            // == START RENDER CLIP IN CONTEXT OF SEQUENCE ==



            // use clip textures for nested sequences, otherwise use main frame buffers
            GLuint back_buffer_1;
            GLuint backend_tex_1;
            GLuint backend_tex_2;
            if (params.nests.size() > 0) {
              back_buffer_1 = params.nests.last()->fbo[1].buffer();
              backend_tex_1 = params.nests.last()->fbo[1].texture();
              backend_tex_2 = params.nests.last()->fbo[2].texture();
            } else {
              back_buffer_1 = params.backend_buffer1;
              backend_tex_1 = params.backend_attachment1;
              backend_tex_2 = params.backend_attachment2;
            }

            // render a backbuffer
            params.ctx->functions()->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, back_buffer_1);

            params.ctx->functions()->glClearColor(0.0, 0.0, 0.0, 0.0);
            params.ctx->functions()->glClear(GL_COLOR_BUFFER_BIT);

            // bind final clip texture
            params.ctx->functions()->glBindTexture(GL_TEXTURE_2D, textureID);

            // set texture filter to bilinear
            params.ctx->functions()->glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            params.ctx->functions()->glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            // draw clip on screen according to gl coordinates
            params.pipeline->bind();

            params.pipeline->setUniformValue("mvp_matrix", projection);
            params.pipeline->setUniformValue("texture", 0);

            GLfloat vertices[] = {
              coords.vertex_top_left.x(), coords.vertex_top_left.y(), 0.0f,
              coords.vertex_top_right.x(), coords.vertex_top_right.y(), 0.0f,
              coords.vertex_bottom_right.x(), coords.vertex_bottom_right.y(), 0.0f,

              coords.vertex_top_left.x(), coords.vertex_top_left.y(), 0.0f,
              coords.vertex_bottom_left.x(), coords.vertex_bottom_left.y(), 0.0f,
              coords.vertex_bottom_right.x(), coords.vertex_bottom_right.y(), 0.0f,
            };

            GLfloat texcoords[] = {
              coords.texture_top_left.x(), coords.texture_top_left.y(),
              coords.texture_top_right.x(), coords.texture_top_right.y(),
              coords.texture_bottom_right.x(), coords.texture_bottom_right.y(),

              coords.texture_top_left.x(), coords.texture_top_left.y(),

              coords.texture_bottom_left.x(), coords.texture_bottom_left.y(),
              coords.texture_bottom_right.x(), coords.texture_bottom_right.y(),
            };

            GLuint vertex_location = params.pipeline->attributeLocation("a_position");
            params.ctx->functions()->glEnableVertexAttribArray(vertex_location);
            params.ctx->functions()->glVertexAttribPointer(vertex_location, 3, GL_FLOAT, GL_FALSE, 0, vertices);

            GLuint tex_location = params.pipeline->attributeLocation("a_texcoord");
            params.ctx->functions()->glEnableVertexAttribArray(tex_location);
            params.ctx->functions()->glVertexAttribPointer(tex_location, 2, GL_FLOAT, GL_FALSE, 0, texcoords);

            params.ctx->functions()->glDrawArrays(GL_TRIANGLES, 0, 6);

            params.pipeline->release();

            // release final clip texture
            params.ctx->functions()->glBindTexture(GL_TEXTURE_2D, 0);

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
                draw_clip(params.ctx, params.pipeline, params.nests.last()->fbo[2].buffer(), params.nests.last()->fbo[0].texture(), true);
              } else {
                draw_clip(params.ctx, params.pipeline, params.backend_buffer2, params.main_attachment, true);
              }
            }



            // == START FINAL DRAW ON SEQUENCE BUFFER ==




            // bind front buffer as draw buffer
            params.ctx->functions()->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, final_fbo);

            // Check if we're using a blend mode (< 0 means no blend mode)
            if (coords.blendmode < 0) {

              params.ctx->functions()->glBindTexture(GL_TEXTURE_2D, backend_tex_1);

              olive::rendering::Blit(params.pipeline);

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

              params.ctx->functions()->glClear(GL_COLOR_BUFFER_BIT);

              olive::rendering::Blit(params.pipeline);

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

  if (!params.nests.isEmpty() && !params.nests.last()->fbo.isEmpty()) {
    // returns nested clip's texture
    return params.nests.last()->fbo[0].texture();
  }

  return 0;
}

void compose_audio(Viewer* viewer, Sequence* seq, int playback_speed, bool wait_for_mutexes) {
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

void close_active_clips(Sequence* s) {
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
