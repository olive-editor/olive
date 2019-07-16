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

RendererProcessor::RendererProcessor() :
  started_(false)
{

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

void RendererProcessor::Start()
{
  if (started_) {
    return;
  }

  threads_.resize(QThread::idealThreadCount());

  for (int i=0;i<threads_.size();i++) {
    threads_[i] = new RendererThread();
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
    delete threads_[i];
  }

  threads_.clear();
}

RendererThread *RendererProcessor::CurrentThread()
{
  return dynamic_cast<RendererThread*>(QThread::currentThread());
}
