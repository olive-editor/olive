#include "multicamwidget.h"

namespace olive {

#define super ManagedDisplayWidget

MulticamWidget::MulticamWidget(QWidget *parent) :
  super{parent},
  node_(nullptr),
  cacher_(nullptr),
  tex_(nullptr)
{

}

void MulticamWidget::SetNode(MultiCamNode *n)
{
  if (node_ == n) {
    return;
  }

  if (node_) {
    // Disconnect
  }

  node_ = n;

  if (node_) {
    // Connect
  }

  SetTime(time_);
}

void MulticamWidget::SetTime(const rational &r)
{
  time_ = r;

  if (node_) {
    if (cacher_) {
      int sources = node_->InputArraySize(node_->kSourcesInput);
      watchers_.resize(sources);
      textures_.resize(sources);

      watchers_.fill(nullptr);
      textures_.fill(nullptr);

      for (int i=0; i<sources; i++) {
        if (Node *n = node_->GetConnectedOutput(node_->kSourcesInput, 0)) {
          auto w = new RenderTicketWatcher(this);
          connect(w, &RenderTicketWatcher::Finished, this, &MulticamWidget::RenderedFrame);
          w->SetTicket(cacher_->GetSingleFrame(n, time_, false));
          watchers_[i] = w;
        }
      }
    }
  }
}

void MulticamWidget::OnInit()
{
  super::OnInit();

  pipeline_ = renderer()->CreateNativeShader(ShaderCode(FileFunctions::ReadFileAsString(QStringLiteral(":/shaders/multicam.frag"))));
}

void MulticamWidget::OnPaint()
{
  // Clear display surface
  renderer()->ClearDestination();

  if (tex_) {
    ColorTransformJob job;
    job.SetColorProcessor(color_service());
    job.SetInputTexture(tex_);
    job.SetInputAlphaAssociation(kAlphaNone);
    renderer()->BlitColorManaged(job, GetViewportParams());
  }
}

void MulticamWidget::OnDestroy()
{
  pipeline_.clear();
  tex_ = nullptr;

  super::OnDestroy();
}

void MulticamWidget::RenderedFrame()
{
  auto watcher = static_cast<RenderTicketWatcher*>(sender());
  if (watcher->HasResult()) {
    int index = watchers_.indexOf(watcher);

    if (index != -1) {
      if ((textures_[index] = watcher->Get().value<TexturePtr>())) {
        update();
      }
    }
  }
  delete watcher;
}

}
