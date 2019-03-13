#ifndef EFFECTUI_H
#define EFFECTUI_H

#include "collapsiblewidget.h"
#include "effects/effect.h"

class EffectUI : public CollapsibleWidget {
  Q_OBJECT
public:
  EffectUI(Effect* e);

  Effect* GetEffect();

  int GetRowY(int row, QWidget *mapToWidget);

  QWidget* Widget(int row, int field);
signals:
  void CutRequested();
  void CopyRequested();
  void TitleBarContextMenuRequested(const QPoint&);
private:
  Effect* effect_;
  QGridLayout* layout_;
  QVector<QLabel*> labels_;
  QVector< QVector<QWidget*> > widgets_;
private slots:
  void show_context_menu(const QPoint&);
};

#endif // EFFECTUI_H
