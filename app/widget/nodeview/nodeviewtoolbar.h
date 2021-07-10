#ifndef NODEVIEWTOOLBAR_H
#define NODEVIEWTOOLBAR_H

#include <QPushButton>
#include <QWidget>

namespace olive {

class NodeViewToolBar : public QWidget
{
  Q_OBJECT
public:
  NodeViewToolBar(QWidget *parent = nullptr);

public slots:
  void SetMiniMapEnabled(bool e)
  {
    minimap_btn_->setChecked(e);
  }

signals:
  void MiniMapEnabledToggled(bool e);

protected:
  virtual void changeEvent(QEvent *e) override;

private:
  void Retranslate();

  void UpdateIcons();

  QPushButton *minimap_btn_;

};

}

#endif // NODEVIEWTOOLBAR_H
