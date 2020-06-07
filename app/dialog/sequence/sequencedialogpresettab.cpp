#include "sequencedialogpresettab.h"

#include <QDir>
#include <QGroupBox>
#include <QInputDialog>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QSplitter>
#include <QTreeWidgetItem>
#include <QXmlStreamWriter>

#include "common/filefunctions.h"
#include "node/input.h"
#include "ui/icons/icons.h"
#include "widget/menu/menu.h"

OLIVE_NAMESPACE_ENTER

const int kDataIsPreset = Qt::UserRole;
const int kDataPresetIsCustomRole = Qt::UserRole + 1;
const int kDataPresetDataRole = Qt::UserRole + 2;

SequenceDialogPresetTab::SequenceDialogPresetTab(QWidget* parent) :
  QWidget(parent)
{
  QVBoxLayout* outer_layout = new QVBoxLayout(this);
  outer_layout->setMargin(0);

  preset_tree_ = new QTreeWidget();
  preset_tree_->setColumnCount(1);
  preset_tree_->setHeaderLabel(tr("Preset"));
  preset_tree_->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(preset_tree_, &QTreeWidget::customContextMenuRequested, this, &SequenceDialogPresetTab::ShowContextMenu);
  outer_layout->addWidget(preset_tree_);
  connect(preset_tree_, &QTreeWidget::currentItemChanged, this, &SequenceDialogPresetTab::SelectedItemChanged);
  connect(preset_tree_, &QTreeWidget::itemDoubleClicked, this, &SequenceDialogPresetTab::ItemDoubleClicked);

  // Add "my presets" folder
  my_presets_folder_ = CreateFolder(tr("My Presets"));
  preset_tree_->addTopLevelItem(my_presets_folder_);

  // Add presets
  preset_tree_->addTopLevelItem(CreateHDPresetFolder(tr("4K UHD"), 3840, 2160));
  preset_tree_->addTopLevelItem(CreateHDPresetFolder(tr("1080p"), 1920, 1080));
  preset_tree_->addTopLevelItem(CreateHDPresetFolder(tr("720p"), 1280, 720));

  preset_tree_->addTopLevelItem(CreateSDPresetFolder(tr("NTSC"), 720, 480, rational(30000, 1001)));
  preset_tree_->addTopLevelItem(CreateSDPresetFolder(tr("PAL"), 720, 576, rational(25, 1)));

  // Load custom presets
  QFile preset_file(GetCustomPresetFilename());
  if (preset_file.open(QFile::ReadOnly)) {
    QXmlStreamReader reader(&preset_file);

    while (XMLReadNextStartElement(&reader)) {
      if (reader.name() == QStringLiteral("presets")) {
        while (XMLReadNextStartElement(&reader)) {
          if (reader.name() == QStringLiteral("preset")) {
            SequencePreset p;

            while (XMLReadNextStartElement(&reader)) {
              if (reader.name() == QStringLiteral("name")) {
                p.name = reader.readElementText();
              } else if (reader.name() == QStringLiteral("width")) {
                p.width = reader.readElementText().toInt();
              } else if (reader.name() == QStringLiteral("height")) {
                p.height = reader.readElementText().toInt();
              } else if (reader.name() == QStringLiteral("framerate")) {
                p.frame_rate = rational::fromString(reader.readElementText());
              } else if (reader.name() == QStringLiteral("samplerate")) {
                p.sample_rate = reader.readElementText().toInt();
              } else if (reader.name() == QStringLiteral("chlayout")) {
                p.channel_layout = reader.readElementText().toULongLong();
              } else {
                reader.skipCurrentElement();
              }
            }

            AddItem(my_presets_folder_, p, true);
          } else {
            reader.skipCurrentElement();
          }
        }
      } else {
        reader.skipCurrentElement();
      }
    }

    preset_file.close();
  }
}

SequenceDialogPresetTab::~SequenceDialogPresetTab()
{
  // Save custom presets to disk
  QFile preset_file(GetCustomPresetFilename());
  if (preset_file.open(QFile::WriteOnly)) {
    QXmlStreamWriter writer(&preset_file);
    writer.setAutoFormatting(true);

    writer.writeStartDocument();

    writer.writeStartElement(QStringLiteral("presets"));

    foreach (const SequencePreset& p, custom_preset_data_) {
      writer.writeStartElement(QStringLiteral("preset"));

      writer.writeTextElement(QStringLiteral("name"), p.name);
      writer.writeTextElement(QStringLiteral("width"), QString::number(p.width));
      writer.writeTextElement(QStringLiteral("height"), QString::number(p.height));
      writer.writeTextElement(QStringLiteral("framerate"), p.frame_rate.toString());
      writer.writeTextElement(QStringLiteral("samplerate"), QString::number(p.sample_rate));
      writer.writeTextElement(QStringLiteral("chlayout"), QString::number(p.channel_layout));

      writer.writeEndElement(); // preset
    }

    writer.writeEndElement(); // presets

    writer.writeEndDocument();

    preset_file.close();
  }
}

void SequenceDialogPresetTab::SaveParametersAsPreset(SequencePreset preset)
{
  QString preset_name;
  int existing_preset;

  forever {
    preset_name = GetPresetName(preset_name);

    if (preset_name.isEmpty()) {
      // Dialog cancelled - leave function entirely
      return;
    }

    existing_preset = -1;
    for (int i=0; i<custom_preset_data_.size(); i++) {
      if (custom_preset_data_.at(i).name == preset_name) {
        existing_preset = i;
        break;
      }
    }

    if (existing_preset == -1
        || QMessageBox::question(this,
                                 tr("Preset exists"),
                                 tr("A preset with this name already exists. "
                                    "Would you like to replace it?")) == QMessageBox::Yes) {
      break;
    }
  }

  preset.name = preset_name;

  if (existing_preset >= 0) {
    custom_preset_data_.replace(existing_preset, preset);
  } else {
    AddItem(my_presets_folder_, preset, true);
  }
}

QTreeWidgetItem* SequenceDialogPresetTab::CreateFolder(const QString &name)
{
  QTreeWidgetItem* folder = new QTreeWidgetItem();
  folder->setText(0, name);
  folder->setIcon(0, icon::Folder);
  return folder;
}

QTreeWidgetItem *SequenceDialogPresetTab::CreateHDPresetFolder(const QString &name, int width, int height)
{
  QTreeWidgetItem* parent = CreateFolder(name);
  AddItem(parent, {tr("%1 23.976 FPS").arg(name),
                   width,
                   height,
                   rational(24000, 1001),
                   48000,
                   AV_CH_LAYOUT_STEREO});
  AddItem(parent, {tr("%1 25 FPS").arg(name),
                   width,
                   height,
                   rational(25, 1),
                   48000,
                   AV_CH_LAYOUT_STEREO});
  AddItem(parent, {tr("%1 29.97 FPS").arg(name),
                   width,
                   height,
                   rational(30000, 1001),
                   48000,
                   AV_CH_LAYOUT_STEREO});
  AddItem(parent, {tr("%1 50 FPS").arg(name),
                   width,
                   height,
                   rational(50, 1),
                   48000,
                   AV_CH_LAYOUT_STEREO});
  AddItem(parent, {tr("%1 59.94 FPS").arg(name),
                   width,
                   height,
                   rational(60000, 1001),
                   48000,
                   AV_CH_LAYOUT_STEREO});
  return parent;
}

QTreeWidgetItem *SequenceDialogPresetTab::CreateSDPresetFolder(const QString &name, int width, int height, const rational& frame_rate)
{
  QTreeWidgetItem* parent = CreateFolder(name);
  preset_tree_->addTopLevelItem(parent);
  AddItem(parent, {tr("%1 Standard").arg(name),
                   width,
                   height,
                   frame_rate,
                   48000,
                   AV_CH_LAYOUT_STEREO});
  AddItem(parent, {tr("%1 Widescreen").arg(name),
                   width,
                   height,
                   frame_rate,
                   48000,
                   AV_CH_LAYOUT_STEREO});
  return parent;
}

QString SequenceDialogPresetTab::GetPresetName(QString start)
{
  bool ok;

  forever {
    start = QInputDialog::getText(this,
                                        tr("Save Preset"),
                                        tr("Set preset name:"),
                                        QLineEdit::Normal,
                                        start,
                                        &ok);

    if (!ok) {
      // Dialog cancelled - leave function entirely
      return QString();
    }

    if (start.isEmpty()) {
      // No preset name entered, start loop over
      QMessageBox::critical(this, tr("Invalid preset name"),
                            tr("You must enter a preset name"), QMessageBox::Ok);
    } else {
      break;
    }
  }

  return start;
}

QTreeWidgetItem *SequenceDialogPresetTab::GetSelectedItem()
{
  QList<QTreeWidgetItem*> selected_items = preset_tree_->selectedItems();

  if (selected_items.isEmpty()) {
    return nullptr;
  } else {
    return selected_items.first();
  }
}

QTreeWidgetItem *SequenceDialogPresetTab::GetSelectedCustomPreset()
{
  QTreeWidgetItem* sel = GetSelectedItem();

  if (sel
      && sel->data(0, kDataIsPreset).toBool()
      && sel->data(0, kDataPresetIsCustomRole).toBool()) {
    return sel;
  }

  return nullptr;
}

QString SequenceDialogPresetTab::GetCustomPresetFilename()
{
  return QDir(FileFunctions::GetConfigurationLocation()).filePath(QStringLiteral("presets"));
}

void SequenceDialogPresetTab::AddItem(QTreeWidgetItem *folder, const SequencePreset& preset, bool is_custom, const QString &description)
{
  QTreeWidgetItem* item = new QTreeWidgetItem();
  item->setText(0, preset.name);
  item->setIcon(0, icon::Video);
  item->setToolTip(0, description);
  item->setData(0, kDataIsPreset, true);
  item->setData(0, kDataPresetIsCustomRole, is_custom);

  QList<SequencePreset>& list = is_custom ? custom_preset_data_ : default_preset_data_;
  item->setData(0, kDataPresetDataRole, list.size());
  list.append(preset);

  folder->addChild(item);
}

void SequenceDialogPresetTab::SelectedItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous)
{
  Q_UNUSED(previous)

  if (current->data(0, kDataIsPreset).toBool()) {
    int preset_index = current->data(0, kDataPresetDataRole).toInt();

    const SequencePreset& preset_data = (current->data(0, kDataPresetIsCustomRole).toBool())
        ? custom_preset_data_.at(preset_index)
        : default_preset_data_.at(preset_index);

    emit PresetChanged(preset_data);
  }
}

void SequenceDialogPresetTab::ItemDoubleClicked(QTreeWidgetItem *item, int column)
{
  Q_UNUSED(column)

  if (item->data(0, kDataIsPreset).toBool()) {
    emit PresetAccepted();
  }
}

void SequenceDialogPresetTab::ShowContextMenu()
{
  QTreeWidgetItem* sel = GetSelectedCustomPreset();

  if (sel) {
    Menu m(this);

    QAction* delete_action = m.addAction(tr("Delete Preset"));
    connect(delete_action, &QAction::triggered, this, &SequenceDialogPresetTab::DeleteSelectedPreset);

    m.exec(QCursor::pos());
  }
}

void SequenceDialogPresetTab::DeleteSelectedPreset()
{
  QTreeWidgetItem* sel = GetSelectedCustomPreset();

  if (sel) {
    int preset_index = sel->data(0, kDataPresetDataRole).toInt();

    // Shift all items whose index was after this preset forward
    for (int i=0; i<my_presets_folder_->childCount(); i++) {
      QTreeWidgetItem* custom_item = my_presets_folder_->child(i);
      int this_item_index = custom_item->data(0, kDataPresetDataRole).toInt();

      if (this_item_index > preset_index) {
        custom_item->setData(0, kDataPresetDataRole, this_item_index - 1);
      }
    }

    // Remove the preset
    custom_preset_data_.removeAt(preset_index);

    // Delete the item
    delete sel;
  }
}

OLIVE_NAMESPACE_EXIT
