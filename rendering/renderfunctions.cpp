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
#include <QOpenGLExtraFunctions>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QDebug>

#include "timeline/clip.h"
#include "timeline/sequence.h"
#include "project/media.h"
#include "effects/effect.h"
#include "project/footage.h"
#include "effects/transition.h"
#include "ui/collapsiblewidget.h"
#include "rendering/audio.h"
#include "global/math.h"
#include "global/timing.h"
#include "global/config.h"
#include "panels/timeline.h"
#include "qopenglshaderprogramptr.h"
#include "shadergenerators.h"

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

GLfloat olive::rendering::flipped_blit_texcoords[] = {
  0.0, 1.0,
  1.0, 1.0,
  1.0, 0.0,

  0.0, 1.0,
  0.0, 0.0,
  1.0, 0.0
};

void PrepareToDraw(QOpenGLFunctions* f) {
  f->glGenerateMipmap(GL_TEXTURE_2D);
  f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
}

void olive::rendering::Blit(QOpenGLShaderProgram* pipeline, bool flipped, QMatrix4x4 matrix) {

  QOpenGLFunctions* func = QOpenGLContext::currentContext()->functions();
  PrepareToDraw(func);

  QOpenGLVertexArrayObject m_vao;
  m_vao.create();
  m_vao.bind();

  QOpenGLBuffer m_vbo;
  m_vbo.create();
  m_vbo.bind();
  m_vbo.allocate(blit_vertices, 18 * sizeof(GLfloat));
  m_vbo.release();

  QOpenGLBuffer m_vbo2;
  m_vbo2.create();
  m_vbo2.bind();
  m_vbo2.allocate(flipped ? flipped_blit_texcoords : blit_texcoords, 12 * sizeof(GLfloat));
  m_vbo2.release();

  pipeline->bind();

  pipeline->setUniformValue("mvp_matrix", matrix);
  pipeline->setUniformValue("texture", 0);


  GLuint vertex_location = pipeline->attributeLocation("a_position");
  m_vbo.bind();
  func->glEnableVertexAttribArray(vertex_location);
  func->glVertexAttribPointer(vertex_location, 3, GL_FLOAT, GL_FALSE, 0, 0);
  m_vbo.release();

  GLuint tex_location = pipeline->attributeLocation("a_texcoord");
  m_vbo2.bind();
  func->glEnableVertexAttribArray(tex_location);
  func->glVertexAttribPointer(tex_location, 2, GL_FLOAT, GL_FALSE, 0, 0);
  m_vbo2.release();

  func->glDrawArrays(GL_TRIANGLES, 0, 6);

  pipeline->release();

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

      if (!e->is_open()) {
        e->open();
      }

      if (can_process_shaders && e->is_shader_linked()) {
        for (int i=0;i<e->getIterations();i++) {
          e->process_shader(timecode, coords, i);
          composite_texture = draw_clip(ctx, e->GetShaderPipeline(), c->fbo.at(fbo_switcher), composite_texture, true);
          fbo_switcher = !fbo_switcher;
        }
      }
      if (e->Flags() & Effect::SuperimposeFlag) {
        GLuint superimpose_texture = e->process_superimpose(ctx, timecode);

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
          if (composite_texture != c->fbo.at(0).texture() && composite_texture != c->fbo.at(1).texture()) {
            draw_clip(ctx, pipeline, c->fbo.at(!fbo_switcher), composite_texture, true);
          }

          composite_texture = draw_clip(ctx, pipeline, c->fbo.at(!fbo_switcher), superimpose_texture, false);
        }
      }
    }
  }
}

GLuint olive::rendering::compose_sequence(ComposeSequenceParams &params) {
  GLuint final_fbo = params.video ? params.main_buffer->buffer() : 0;

  Sequence* s = params.seq;
  long playhead = s->playhead;

  if (!params.nests.isEmpty()) {

    for (int i=0;i<params.nests.size();i++) {
      s = params.nests.at(i)->media()->to_sequence().get();
      playhead += params.nests.at(i)->clip_in(true) - params.nests.at(i)->timeline_in(true);
      playhead = rescale_frame_number(playhead, params.nests.at(i)->sequence->frame_rate, s->frame_rate);
    }

    if (params.video && !params.nests.last()->fbo.isEmpty()) {
      params.nests.last()->fbo.at(0).BindBuffer();
      params.ctx->functions()->glClear(GL_COLOR_BUFFER_BIT);
      final_fbo = params.nests.last()->fbo.at(0).buffer();
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

    params.ctx->functions()->glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    int half_width = s->width/2;
    int half_height = s->height/2;
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

        // prepare framebuffers for backend drawing operations
        if (c->fbo.isEmpty()) {
          // create 3 fbos for nested sequences, 2 for most clips
          int fbo_count = (c->media() != nullptr && c->media()->get_type() == MEDIA_TYPE_SEQUENCE) ? 3 : 2;

          c->fbo.resize(fbo_count);

          for (int j=0;j<fbo_count;j++) {
            c->fbo[j].Create(params.ctx, video_width, video_height);
          }
        }

        bool convert_frame_to_internal = false;

        // if media is footage
        if (c->media() != nullptr && c->media()->get_type() == MEDIA_TYPE_FOOTAGE) {

          // retrieve video frame from cache and store it in c->texture
          c->Cache(qMax(playhead, c->timeline_in()), false, params.nests, params.playback_speed);
          if (!c->Retrieve()) {
            params.texture_failed = true;
          } else {
            // retrieve ID from c->texture
            textureID = c->texture;
          }

          if (textureID == 0) {

            qWarning() << "Failed to create texture";

          } else {

            convert_frame_to_internal = true;

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

              // Convert frame from source to linear colorspace
              if (olive::CurrentConfig.enable_color_management)
              {

                // Convert texture to sequence's internal format
                if (textureID != c->fbo.at(0).texture() && textureID != c->fbo.at(1).texture()) {
                  textureID = draw_clip(params.ctx, params.pipeline, c->fbo.at(fbo_switcher), textureID, true);
                  fbo_switcher = !fbo_switcher;
                }

                // Check if this clip has an OCIO shader set up or not
                if (c->ocio_shader == nullptr) {


                  // Set default input colorspace
                  QString input_cs = OCIO::ROLE_SCENE_LINEAR;

                  if (c->media() != nullptr && c->media()->get_type() == MEDIA_TYPE_FOOTAGE) {
                    input_cs = c->media()->to_footage()->Colorspace();
                  }

                  // Try to get a shader based on the input color space to scene linear
                  try {
                    OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
                    OCIO::ConstProcessorRcPtr processor = config->getProcessor(input_cs.toUtf8(),
                                                                               OCIO::ROLE_SCENE_LINEAR);

                    c->ocio_shader = olive::shader::SetupOCIO(params.ctx,
                                                              c->ocio_lut_texture,
                                                              processor,
                                                              c->media()->to_footage()->alpha_is_associated);
                  } catch (OCIO::Exception& e) {
                    qWarning() << e.what();
                  }
                }

                // Ensure we got a shader, and if so, blit with it
                if (c->ocio_shader != nullptr) {
                  textureID = olive::rendering::OCIOBlit(c->ocio_shader.get(),
                                                         c->ocio_lut_texture,
                                                         c->fbo.at(fbo_switcher),
                                                         textureID);

                  fbo_switcher = !fbo_switcher;
                }

              }
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
          if (c->autoscaled()
              && (video_width != s->width
                  && video_height != s->height)) {
            float width_multiplier = float(s->width) / float(video_width);
            float height_multiplier = float(s->height) / float(video_height);
            float scale_multiplier = qMin(width_multiplier, height_multiplier);

            coords.matrix.scale(scale_multiplier, scale_multiplier);
          }

          // Configure effect gizmos if they exist
          if (params.gizmos != nullptr) {
            // set correct gizmo coords at this matrix
            params.gizmos->gizmo_draw(timecode, coords);

            // convert gizmo coords to screen coords
            params.gizmos->gizmo_world_to_screen(coords.matrix, projection);
          }



          if (textureID > 0) {

            // set viewport to sequence size
            params.ctx->functions()->glViewport(0, 0, s->width, s->height);



            // == START RENDER CLIP IN CONTEXT OF SEQUENCE ==



            // use clip textures for nested sequences, otherwise use main frame buffers
            GLuint back_buffer_1;
            GLuint back_buffer_2;
            GLuint backend_tex_1;
            GLuint backend_tex_2;
            GLuint comp_texture;
            if (params.nests.size() > 0) {
              back_buffer_1 = params.nests.last()->fbo[1].buffer();
              back_buffer_2 = params.nests.last()->fbo[2].buffer();
              backend_tex_1 = params.nests.last()->fbo[1].texture();
              backend_tex_2 = params.nests.last()->fbo[2].texture();
              comp_texture = params.nests.last()->fbo[0].texture();
            } else {
              back_buffer_1 = params.backend_buffer1->buffer();
              back_buffer_2 = params.backend_buffer2->buffer();
              backend_tex_1 = params.backend_buffer1->texture();
              backend_tex_2 = params.backend_buffer2->texture();
              comp_texture = params.main_buffer->texture();
            }

            // render a backbuffer
            params.ctx->functions()->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, back_buffer_1);

            params.ctx->functions()->glClearColor(0.0, 0.0, 0.0, 0.0);
            params.ctx->functions()->glClear(GL_COLOR_BUFFER_BIT);

            // bind final clip texture
            params.ctx->functions()->glBindTexture(GL_TEXTURE_2D, textureID);

            // set texture filter to bilinear
            PrepareToDraw(params.ctx->functions());

            // draw clip on screen according to gl coordinates
            params.pipeline->bind();

            params.pipeline->setUniformValue("mvp_matrix", projection * coords.matrix);
            params.pipeline->setUniformValue("texture", 0);
            params.pipeline->setUniformValue("opacity", coords.opacity);

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

            QOpenGLVertexArrayObject vao;
            vao.create();
            vao.bind();

            QOpenGLBuffer vertex_buffer;
            vertex_buffer.create();
            vertex_buffer.bind();
            vertex_buffer.allocate(vertices, 18 * sizeof(GLfloat));
            vertex_buffer.release();

            QOpenGLBuffer texcoord_buffer;
            texcoord_buffer.create();
            texcoord_buffer.bind();
            texcoord_buffer.allocate(texcoords, 12 * sizeof(GLfloat));
            texcoord_buffer.release();

            GLuint vertex_location = params.pipeline->attributeLocation("a_position");
            vertex_buffer.bind();
            params.ctx->functions()->glEnableVertexAttribArray(vertex_location);
            params.ctx->functions()->glVertexAttribPointer(vertex_location, 3, GL_FLOAT, GL_FALSE, 0, 0);
            vertex_buffer.release();

            GLuint tex_location = params.pipeline->attributeLocation("a_texcoord");
            texcoord_buffer.bind();
            params.ctx->functions()->glEnableVertexAttribArray(tex_location);
            params.ctx->functions()->glVertexAttribPointer(tex_location, 2, GL_FLOAT, GL_FALSE, 0, 0);
            texcoord_buffer.release();

            params.ctx->functions()->glDrawArrays(GL_TRIANGLES, 0, 6);

            params.pipeline->setUniformValue("opacity", 1.0f);

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
              draw_clip(params.ctx, params.pipeline, back_buffer_2, comp_texture, true);
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

  if (audio_track_count == 0) {
    WakeAudioWakeObject();
  }

  if (!params.nests.isEmpty() && !params.nests.last()->fbo.isEmpty()) {
    // returns nested clip's texture
    return params.nests.last()->fbo[0].texture();
  }

  return 0;
}

void olive::rendering::compose_audio(Viewer* viewer, Sequence* seq, int playback_speed, bool wait_for_mutexes) {
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

GLuint olive::rendering::OCIOBlit(QOpenGLShaderProgram *pipeline,
                                  GLuint lut,
                                  const FramebufferObject& fbo,
                                  GLuint texture)
{
  if (pipeline == nullptr) {
    return 0;
  }

  QOpenGLContext* ctx = QOpenGLContext::currentContext();
  QOpenGLExtraFunctions* xf = ctx->extraFunctions();

  xf->glActiveTexture(GL_TEXTURE2);
  xf->glBindTexture(GL_TEXTURE_3D, lut);
  xf->glActiveTexture(GL_TEXTURE0);

  pipeline->bind();

  pipeline->setUniformValue("tex2", 2);

  //textureID = draw_clip(params.ctx, pipeline, c->fbo.at(fbo_switcher), textureID, true);
  GLuint textureID = draw_clip(ctx, pipeline, fbo, texture, true);

  pipeline->release();

  xf->glActiveTexture(GL_TEXTURE2);
  xf->glBindTexture(GL_TEXTURE_3D, 0);
  xf->glActiveTexture(GL_TEXTURE0);

  return textureID;
}
