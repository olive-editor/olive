#ifndef EFFECTUI_H
#define EFFECTUI_H

#include "collapsiblewidget.h"
#include "effects/effect.h"

class EffectUI : public CollapsibleWidget {
  Q_OBJECT
public:
  EffectUI(Effect* e);

  void AddAdditionalEffect(Effect* e);

  Effect* GetEffect();

  int GetRowY(int row, QWidget *mapToWidget);

  void UpdateFromEffect();

  bool IsAttachedToClip(Clip* c);

signals:
  void CutRequested();
  void CopyRequested();
  void TitleBarContextMenuRequested(const QPoint&);
private:
  QWidget* Widget(int row, int field);

  Effect* effect_;
  QVector<Effect*> additional_effects_;
  QGridLayout* layout_;
  QVector<QLabel*> labels_;
  QVector< QVector<QWidget*> > widgets_;
  QVector<KeyframeNavigator*> keyframe_navigators_;

  bool multiple_;

  void AttachKeyframeNavigationToRow(EffectRow* row, KeyframeNavigator* nav);
private slots:
  void show_context_menu(const QPoint&);
};

#endif // EFFECTUI_H
