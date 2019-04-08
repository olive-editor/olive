#ifndef EFFECTSPANEL_H
#define EFFECTSPANEL_H

#include "ui/panel.h"
#include "ui/effectui.h"
#include "timeline/clip.h"

class EffectsPanel : public Panel
{
  Q_OBJECT
public:
  explicit EffectsPanel(QWidget *parent = nullptr);
  virtual ~EffectsPanel() override;

  void Reload();
  void SetClips();
  void Clear(bool clear_cache = true);

  virtual bool focused() override;

  bool IsEffectSelected(Effect* e);
  void DeleteSelectedEffects();

public slots:
  void cut();
  void copy(bool del = false);
protected:
  virtual void ClearEvent();
  virtual void LoadEvent();

  QVector<Clip*> selected_clips_;
  QVector<EffectUI*> open_effects_;
private:
  void Load();
  void open_effect(Effect *e);
  void DeleteEffect(ComboAction* ca, Effect* effect_ref);
private slots:
  void deselect_all_effects(QWidget*);
};

#endif // EFFECTSPANEL_H
