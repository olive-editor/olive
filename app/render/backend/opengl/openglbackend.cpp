#include "openglbackend.h"

#include <QEventLoop>
#include <QThread>

#include "functions.h"

OpenGLBackend::OpenGLBackend(QObject *parent) :
  VideoRenderBackend(parent),
  push_time_(-1)
{
}

OpenGLBackend::~OpenGLBackend()
{
  Close();
}

bool OpenGLBackend::Init()
{
  if (!VideoRenderBackend::Init()) {
    return false;
  }

  QOpenGLContext* share_ctx = QOpenGLContext::currentContext();

  if (share_ctx == nullptr) {
    qCritical() << "No active OpenGL context to connect to";
    return false;
  }

  // Initiate one thread per CPU core
  for (int i=0;i<threads().size();i++) {
    QThread* thread = threads().at(i);

    // Create one processor object for each thread
    OpenGLWorker* processor = new OpenGLWorker(share_ctx, &shader_cache_, &decoder_cache_);
    processor->SetParameters(params());

    // Finally, we can move it to its own thread
    processor->moveToThread(thread);

    // Add processor to list
    processors_.append(processor);

    // This function blocks the main thread intentionally. See the documentation for this function to see why.
    processor->Init();
  }

  // Create master texture (the one sent to the viewer)
  master_texture_ = std::make_shared<OpenGLTexture>();
  master_texture_->Create(share_ctx, params().effective_width(), params().effective_height(), params().format());

  // Create internal FBO for copying textures
  copy_buffer_.Create(share_ctx);
  copy_buffer_.Attach(master_texture_);
  copy_pipeline_ = OpenGLShader::CreateDefault();

  return true;
}

void OpenGLBackend::GenerateFrame(const rational &time)
{
  Q_UNUSED(time)

  /*threads().first()->Queue(NodeDependency(viewer_node()->texture_input()->get_connected_output(), time, time),
                           true,
                           false);*/
}

void OpenGLBackend::Close()
{
  if (!IsStarted()) {
    return;
  }

  Decompile();

  copy_buffer_.Destroy();
  master_texture_ = nullptr;
  copy_pipeline_ = nullptr;

  VideoRenderBackend::Close();
}

OpenGLTexturePtr OpenGLBackend::GetCachedFrameAsTexture(const rational &time)
{
  last_time_requested_ = time;

  if (push_time_ >= 0) {
    rational temp_push_time = push_time_;
    push_time_ = -1;

    if (time == temp_push_time) {
      return master_texture_;
    }
  }

  const char* cached_frame = GetCachedFrame(time);

  if (cached_frame != nullptr) {
    master_texture_->Upload(cached_frame);

    return master_texture_;
  }

  return nullptr;
}

bool OpenGLBackend::Compile()
{
  Decompile();

  if (viewer_node() == nullptr || !viewer_node()->texture_input()->IsConnected()) {
    // Nothing to be done, nothing to compile
    return true;
  }

  // Traverse node graph compiling where necessary
  bool ret = TraverseCompiling(viewer_node());

  if (ret) {
    qDebug() << "Compiled successfully!";
  } else {
    qDebug() << "Compile failed:" << GetError();
  }

  return ret;
}

void OpenGLBackend::Decompile()
{
  shader_cache_.Clear();
}

bool OpenGLBackend::TraverseCompiling(Node *)
{
  /*foreach (NodeParam* param, n->parameters()) {
    if (param->type() == NodeParam::kInput && param->IsConnected()) {
      NodeOutput* connected_output = static_cast<NodeInput*>(param)->get_connected_output();

      // Generate the ID we'd use for this shader
      QString output_id = GenerateShaderID(connected_output);

      // Check if we have a shader or not
      if (GetShaderFromID(output_id) == nullptr) {
        // Since we don't have a shader, compile one now
        QString node_code = connected_output->parent()->Code(connected_output);

        // If the node has no code, it mustn't be GPU accelerated
        if (!node_code.isEmpty()) {
          // Since we have shader code, compile it now
          CompiledNode compiled_info;
          compiled_info.id = output_id;

          if (!(compiled_info.program = std::make_shared<OpenGLShader>())) {
            SetError("Failed to create OpenGL shader object");
            return false;
          }

          if (!compiled_info.program->create()) {
            SetError("Failed to create OpenGL shader on device");
            return false;
          }

          if (!compiled_info.program->addShaderFromSourceCode(QOpenGLShader::Fragment, node_code)) {
            SetError("Failed to add OpenGL shader code");
            return false;
          }

          if (compiled_info.program->link()) {
            SetError("Failed to compile OpenGL shader");
            return false;
          }

          compiled_nodes_.append(compiled_info);

          qDebug() << "Compiled" << compiled_info.id;
        }
      }

      if (!TraverseCompiling(connected_output->parent())) {
        return false;
      }
    }
  }*/

  return true;
}

void OpenGLBackend::ThreadCallback(OpenGLTexturePtr texture, const rational& time, const QByteArray& hash)
{
  // Threads are all done now, time to proceed
  caching_ = false;

  DeferMap(time, hash);

  if (texture != nullptr) {
    // We received a texture, time to start downloading it
    QString fn = CachePathName(hash);

    /*
    download_threads_[last_download_thread_%download_threads_.size()]->Queue(texture,
                                                                             fn,
                                                                             hash);

    last_download_thread_++;
    */
  } else {
    // There was no texture here, we must update the viewer
    DownloadThreadComplete(hash);
  }

  // If the connected output is using this time, signal it to update
  if (last_time_requested_ == time) {

    copy_buffer_.Bind();

    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();

    if (texture == nullptr) {

      // No texture, clear the master and push it
      f->glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
      f->glClear(GL_COLOR_BUFFER_BIT);

    } else {
      texture->Bind();

      f->glViewport(0, 0, master_texture_->width(), master_texture_->height());

      olive::gl::Blit(copy_pipeline_);

      texture->Release();
    }

    copy_buffer_.Release();

    push_time_ = time;

    emit CachedFrameReady(time);
  }

  CacheNext();
}

void OpenGLBackend::ThreadRequestSibling(NodeDependency dep)
{
  Q_UNUSED(dep)

  // Try to queue another thread to run this dep in advance
  for (int i=1;i<threads().size();i++) {
    /*if (threads().at(i)->Queue(dep, false, true)) {
      return;
    }*/
  }
}

void OpenGLBackend::ThreadSkippedFrame(const rational& time, const QByteArray& hash)
{
  caching_ = false;

  DeferMap(time, hash);

  if (!IsCaching(hash)) {
    DownloadThreadComplete(hash);

    // Signal output to update value
    emit CachedFrameReady(time);
  }

  CacheNext();
}

void OpenGLBackend::DownloadThreadComplete(const QByteArray &hash)
{
  cache_hash_list_mutex_.lock();
  cache_hash_list_.removeAll(hash);
  cache_hash_list_mutex_.unlock();

  for (int i=0;i<deferred_maps_.size();i++) {
    const HashTimeMapping& deferred = deferred_maps_.at(i);

    if (deferred_maps_.at(i).hash == hash) {
      // Insert into hash map
      time_hash_map_.insert(deferred.time, deferred.hash);

      deferred_maps_.removeAt(i);
      i--;
    }
  }
}
