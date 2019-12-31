#include "preferencesbehaviortab.h"

#include <QCheckBox>
#include <QVBoxLayout>

#include "config/config.h"

PreferencesBehaviorTab::PreferencesBehaviorTab()
{
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setMargin(0);

  behavior_tree_ = new QTreeWidget();
  layout->addWidget(behavior_tree_);

  behavior_tree_->setHeaderLabel(tr("Behavior"));

  QTreeWidgetItem* general_group = AddParent(tr("General"));
  AddItem(tr("Enable hover focus"),
          QStringLiteral("HoverFocus"),
          tr("Panels will be considered focused when the mouse cursor is over them without having to click them."),
          general_group);
  AddItem(tr("Scroll wheel zooms by default instead of scrolling"),
          QStringLiteral("ScrollZooms"),
          tr("Holding CTRL while using Olive toggles this setting"),
          general_group);

  QTreeWidgetItem* audio_group = AddParent(tr("Audio"));
  AddItem(tr("Enable audio scrubbing"),
          QStringLiteral("AudioScrubbing"),
          audio_group);

  QTreeWidgetItem* timeline_group = AddParent(tr("Timeline"));
  AddItem(tr("Auto-Seek to Imported Clips"),
          QStringLiteral("EnableSeekToImport"),
          timeline_group);
  AddItem(tr("Edit Tool Also Seeks"),
          QStringLiteral("EditToolAlsoSeeks"),
          timeline_group);
  AddItem(tr("Edit Tool Selects Links"),
          QStringLiteral("EditToolSelectsLinks"),
          timeline_group);
  AddItem(tr("Enable Drag Files to Timeline"),
          QStringLiteral("EnableDragFilesToTimeline"),
          timeline_group);
  AddItem(tr("Invert Timeline Scroll Axes"),
          QStringLiteral("InvertTimelineScrollAxes"),
          tr("Hold ALT on any UI element to switch scrolling axes"),
          timeline_group);
  AddItem(tr("Seek Also Selects"),
          QStringLiteral("SelectAlsoSeeks"),
          timeline_group);
  AddItem(tr("Seek to the End of Pastes"),
          QStringLiteral("PasteSeeks"),
          timeline_group);
  AddItem(tr("Selecting Also Seeks"),
          QStringLiteral("SelectAlsoSeeks"),
          timeline_group);

  QTreeWidgetItem* playback_group = AddParent(tr("Playback"));
  AddItem(tr("Ask For Name When Setting Marker"),
          QStringLiteral("SetNameWithMarker"),
          playback_group);
  AddItem(tr("Automatically rewind at the end of a sequence"),
          QStringLiteral("AutoSeekToBeginning"),
          playback_group);

  QTreeWidgetItem* project_group = AddParent(tr("Project"));
  AddItem(tr("Drop Files on Media to Replace"),
          QStringLiteral("DropFileOnMediaToReplace"),
          project_group);

  QTreeWidgetItem* node_group = AddParent(tr("Nodes"));
  AddItem(tr("Add Default Effects to New Clips"),
          QStringLiteral("AddDefaultEffectsToClips"),
          node_group);
  AddItem(tr("Auto-Scale By Default"),
          QStringLiteral("AutoscaleByDefault"),
          node_group);
}

void PreferencesBehaviorTab::Accept()
{
  QMap<QTreeWidgetItem*, QString>::const_iterator iterator;

  for (iterator=config_map_.begin();iterator!=config_map_.end();iterator++) {
    Config::Current()[iterator.value()] = (iterator.key()->checkState(0) == Qt::Checked);
  }
}

QTreeWidgetItem* PreferencesBehaviorTab::AddItem(const QString &text, const QString &config_key, const QString& tooltip, QTreeWidgetItem* parent)
{
  QTreeWidgetItem* item = new QTreeWidgetItem({text});
  item->setToolTip(0, tooltip);
  item->setCheckState(0, Config::Current()[config_key].toBool() ? Qt::Checked : Qt::Unchecked);

  config_map_.insert(item, config_key);

  if (parent) {
    parent->addChild(item);
  } else {
    behavior_tree_->addTopLevelItem(item);
  }

  return item;
}

QTreeWidgetItem *PreferencesBehaviorTab::AddItem(const QString &text, const QString &config_key, QTreeWidgetItem *parent)
{
  return AddItem(text, config_key, QString(), parent);
}

QTreeWidgetItem *PreferencesBehaviorTab::AddParent(const QString &text, const QString &tooltip, QTreeWidgetItem *parent)
{
  QTreeWidgetItem* item = new QTreeWidgetItem({text});
  item->setToolTip(0, tooltip);

  if (parent) {
    parent->addChild(item);
  } else {
    behavior_tree_->addTopLevelItem(item);
  }

  return item;
}

QTreeWidgetItem *PreferencesBehaviorTab::AddParent(const QString &text, QTreeWidgetItem *parent)
{
  return AddParent(text, QString(), parent);
}
