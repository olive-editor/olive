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
  QTreeWidgetItem *AddParent(const QString& text, const QString &tooltip, QTreeWidgetItem *parent = nullptr);
  QTreeWidgetItem *AddParent(const QString& text, QTreeWidgetItem *parent = nullptr);

  QTreeWidgetItem *AddItem(const QString& text, const QString& config_key, const QString &tooltip, QTreeWidgetItem *parent );
  QTreeWidgetItem *AddItem(const QString& text, const QString& config_key, QTreeWidgetItem *parent);

  QMap<QTreeWidgetItem*, QString> config_map_;

  QTreeWidget* behavior_tree_;

};

#endif // PREFERENCESBEHAVIORTAB_H
