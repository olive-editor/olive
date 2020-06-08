#ifndef SEQUENCEDIALOGPRESETTAB_H
#define SEQUENCEDIALOGPRESETTAB_H

#include <QLabel>
#include <QTreeWidget>
#include <QWidget>

#include "sequencepreset.h"

OLIVE_NAMESPACE_ENTER

class SequenceDialogPresetTab : public QWidget
{
  Q_OBJECT
public:
  SequenceDialogPresetTab(QWidget* parent = nullptr);

  virtual ~SequenceDialogPresetTab() override;

public slots:
  void SaveParametersAsPreset(SequencePreset preset);

signals:
  void PresetChanged(const SequencePreset& preset);

  void PresetAccepted();

private:
  QTreeWidgetItem *CreateFolder(const QString& name);

  QTreeWidgetItem *CreateHDPresetFolder(const QString& name, int width, int height, int divider);

  QTreeWidgetItem *CreateSDPresetFolder(const QString& name, int width, int height, const rational &frame_rate, int divider);

  QString GetPresetName(QString start);

  QTreeWidgetItem* GetSelectedItem();
  QTreeWidgetItem* GetSelectedCustomPreset();

  static QString GetCustomPresetFilename();

  void AddItem(QTreeWidgetItem* folder,
               const SequencePreset& preset,
               bool is_custom = false,
               const QString& description = QString());

  QTreeWidget* preset_tree_;

  QTreeWidgetItem* my_presets_folder_;

  QList<SequencePreset> default_preset_data_;
  QList<SequencePreset> custom_preset_data_;

private slots:
  void SelectedItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);

  void ItemDoubleClicked(QTreeWidgetItem *item, int column);

  void ShowContextMenu();

  void DeleteSelectedPreset();

};

OLIVE_NAMESPACE_EXIT

#endif // SEQUENCEDIALOGPRESETTAB_H
