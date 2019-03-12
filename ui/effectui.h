#ifndef EFFECTUI_H
#define EFFECTUI_H

#include "collapsiblewidget.h"
#include "project/effect.h"

class EffectUI : public CollapsibleWidget {
  Q_OBJECT
public:
  EffectUI(Effect* e);

  Effect* GetEffect();

  int GetRowPos(int row);
signals:
  void CutRequested();
  void CopyRequested();
  void TitleBarContextMenuRequested(const QPoint&);
private:
  Effect* effect_;
  QGridLayout* layout_;
  QVector<QLabel*> labels_;
private slots:
  void show_context_menu(const QPoint&);
};

#endif // EFFECTUI_H
