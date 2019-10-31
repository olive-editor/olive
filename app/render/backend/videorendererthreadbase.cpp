/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

#include "videorendererthreadbase.h"

#include <QDebug>
#include <QThread>

#include "common/define.h"
#include "videorenderbackend.h"

VideoRendererThreadBase::VideoRendererThreadBase(VideoRenderBackend* parent, QOpenGLContext *share_ctx, const VideoRenderingParams &params) :
  parent_(parent),
  share_ctx_(share_ctx),
  render_instance_(params)
{
  connect(share_ctx_, SIGNAL(aboutToBeDestroyed()), this, SLOT(Cancel()));
}

RenderInstance *VideoRendererThreadBase::render_instance()
{
  return &render_instance_;
}

void VideoRendererThreadBase::Start()
{
  render_instance_.SetShareContext(share_ctx_);

  // Allocate and create resources
  render_instance_.Start();


  // Set up download functions
  f = render_instance()->context()->functions();
  xf = render_instance()->context()->extraFunctions();

  f->glGenFramebuffers(1, &read_buffer_);

  int buffer_size = PixelService::GetBufferSize(render_instance()->params().format(),
                                                render_instance()->params().width(),
                                                render_instance()->params().height());

  data_buffer_.resize(buffer_size);

  format_info_ = PixelService::GetPixelFormatInfo(render_instance()->params().format());

  // Set up OIIO::ImageSpec for compressing cached images on disk
  spec_ = OIIO::ImageSpec(render_instance()->params().width(), render_instance()->params().height(), kRGBAChannels, format_info_.oiio_desc);
  spec_.attribute("compression", "dwaa:200");
}

void VideoRendererThreadBase::Stop()
{
  f->glDeleteFramebuffers(1, &read_buffer_);

  // Free all resources
  render_instance_.Stop();

  thread()->quit();
}

void VideoRendererThreadBase::Process(const NodeDependency &path, bool sibling)
{
  // Process the Node
  NodeOutput* output_to_process = path.node();
  Node* node_to_process = output_to_process->parent();

  texture_ = nullptr;

  QList<Node*> all_deps;
  bool has_hash = false;
  bool can_cache = true;

  QByteArray hash;

  if (!sibling) {
    node_to_process->Lock();

    all_deps = node_to_process->GetDependencies();
    foreach (Node* dep, all_deps) {
      dep->Lock();
    }

    // Check hash
    QCryptographicHash hasher(QCryptographicHash::Sha1);
    node_to_process->Hash(&hasher, output_to_process, path.in());
    hash = hasher.result();

    has_hash = parent_->HasHash(hash);
    can_cache = false;
  }

  if (!has_hash){

    if ((can_cache = parent_->TryCache(hash))) {

      QList<NodeDependency> deps = node_to_process->RunDependencies(output_to_process, path.in());

      // Ask for other threads to run these deps while we're here
      if (!deps.isEmpty()) {
        for (int i=1;i<deps.size();i++) {
          emit RequestSibling(deps.at(i));
        }
      }

      // Get the requested value
      texture_ = output_to_process->get_value(path.in(), path.in()).value<RenderTexturePtr>();

      render_instance()->context()->functions()->glFinish();
    }
  }

  if (!sibling) {
    foreach (Node* dep, all_deps) {
      dep->Unlock();
    }

    node_to_process->Unlock();
  }

  if (can_cache) {
    // We cached this frame, signal that it will need to be downloaded to disk
    emit CachedFrame(texture_, path.in(), hash);
  } else {
    // This hash already exists, no need to cache, just map it
    emit FrameSkipped(path.in(), hash);
  }
}

void VideoRendererThreadBase::Download(RenderTexturePtr texture, const QString &fn, const QByteArray &hash)
{
  // Download the texture

  f->glBindFramebuffer(GL_READ_FRAMEBUFFER, read_buffer_);

  xf->glFramebufferTexture2D(GL_READ_FRAMEBUFFER,
                             GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_2D,
                             texture->texture(),
                             0);

  f->glReadPixels(0,
                  0,
                  texture->width(),
                  texture->height(),
                  format_info_.pixel_format,
                  format_info_.gl_pixel_type,
                  data_buffer_.data());

  xf->glFramebufferTexture2D(GL_READ_FRAMEBUFFER,
                             GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_2D,
                             0,
                             0);

  f->glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

  std::string working_fn_std = fn.toStdString();

  std::unique_ptr<OIIO::ImageOutput> out = OIIO::ImageOutput::create(working_fn_std);

  if (out) {
    out->open(working_fn_std, spec_);
    out->write_image(format_info_.oiio_desc, data_buffer_.data());
    out->close();

    emit Downloaded(hash);
  } else {
    qWarning() << QStringLiteral("Failed to open output file \"%1\"").arg(fn);
  }
}
