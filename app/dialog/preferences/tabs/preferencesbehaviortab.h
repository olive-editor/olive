#ifndef PREFERENCESBEHAVIORTAB_H
#define PREFERENCESBEHAVIORTAB_H

#include <QTreeWidget>

#include "preferencestab.h"

class PreferencesBehaviorTab : public PreferencesTab
{
  Q_OBJECT
public:
  PreferencesBehaviorTab();

  virtual void Accept() override;

private:
  void AddItem(const QString& text, const QString &tooltip = QString());

  QTreeWidget* behavior_tree_;
};

#endif // PREFERENCESBEHAVIORTAB_H
