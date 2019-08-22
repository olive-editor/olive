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

#include "renderer.h"

#include <QDebug>

RendererProcessor::RendererProcessor() :
  started_(false)
{
  texture_input_ = new NodeInput("tex_in");
  texture_input_->add_data_input(NodeInput::kTexture);
  AddParameter(texture_input_);

  texture_output_ = new NodeOutput("tex_out");
  texture_output_->set_data_type(NodeInput::kTexture);
  AddParameter(texture_output_);
}

QString RendererProcessor::Name()
{
  return tr("Renderer");
}

QString RendererProcessor::Category()
{
  return tr("Processor");
}

QString RendererProcessor::Description()
{
  return tr("A multi-threaded OpenGL hardware-accelerated node compositor.");
}

QString RendererProcessor::id()
{
  return "org.olivevideoeditor.Olive.renderervenus";
}

void RendererProcessor::Process(const rational &time)
{
  Q_UNUSED(time)

  // FIXME: Test code only
  GLuint tex = texture_input_->get_value(time).value<GLuint>();

  //glReadPixels()

  texture_output_->set_value(tex);
  // End test code

  /*
  // Ensure we have started
  Start();

  if (!started_) {
    qWarning() << tr("An error occurred starting the Renderer node");
    return;
  }

  */
}

void RendererProcessor::Release()
{
  Stop();
}

void RendererProcessor::InvalidateCache(const rational &start_range, const rational &end_range)
{
  Q_UNUSED(start_range)
  Q_UNUSED(end_range)
}

void RendererProcessor::Start()
{
  if (started_) {
    return;
  }

  threads_.resize(QThread::idealThreadCount());

  for (int i=0;i<threads_.size();i++) {
    threads_[i] = std::make_shared<RendererThread>();
    threads_[i]->run();
  }

  started_ = true;
}

void RendererProcessor::Stop()
{
  if (!started_) {
    return;
  }

  started_ = false;

  for (int i=0;i<threads_.size();i++) {
    threads_[i]->Cancel();
  }

  threads_.clear();
}

RendererThread* RendererProcessor::CurrentThread()
{
  return dynamic_cast<RendererThread*>(QThread::currentThread());
}

NodeInput *RendererProcessor::texture_input()
{
  return texture_input_;
}

NodeOutput *RendererProcessor::texture_output()
{
  return texture_output_;
}
