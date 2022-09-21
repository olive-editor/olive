#ifndef MULTICAMWIDGET_H
#define MULTICAMWIDGET_H

#include "node/input/multicam/multicamnode.h"
#include "render/previewautocacher.h"
#include "widget/manageddisplay/manageddisplay.h"

namespace olive {

class MulticamWidget : public ManagedDisplayWidget
{
  Q_OBJECT
public:
  explicit MulticamWidget(QWidget *parent = nullptr);

  MANAGEDDISPLAYWIDGET_DEFAULT_DESTRUCTOR(MulticamWidget)

  void SetNode(MultiCamNode *n);

  void ConnectCacher(PreviewAutoCacher *c)
  {
    cacher_ = c;
  }

public slots:
  void SetTime(const rational &r);

protected slots:
  virtual void OnInit() override;

  virtual void OnPaint() override;

  virtual void OnDestroy() override;

signals:


private:
  MultiCamNode *node_;

  QVariant pipeline_;

  PreviewAutoCacher *cacher_;

  rational time_;

  TexturePtr tex_;

  QVector<RenderTicketWatcher*> watchers_;
  QVector<TexturePtr> textures_;

private slots:
  void RenderedFrame();

};

}

#endif // MULTICAMWIDGET_H
