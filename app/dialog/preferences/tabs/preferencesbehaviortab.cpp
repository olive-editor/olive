#include "preferencesbehaviortab.h"

#include <QCheckBox>
#include <QVBoxLayout>

PreferencesBehaviorTab::PreferencesBehaviorTab()
{
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setMargin(0);

  behavior_tree_ = new QTreeWidget();
  layout->addWidget(behavior_tree_);

  behavior_tree_->setHeaderLabel(tr("Behavior"));
  behavior_tree_->setRootIsDecorated(false);

  AddItem(tr("Add Default Effects to New Clips"));
  AddItem(tr("Automatically Seek to the Beginning When Playing at the End of a Sequence"));
  AddItem(tr("Selecting Also Seeks"));
  AddItem(tr("Edit Tool Also Seeks"));
  AddItem(tr("Edit Tool Selects Links"));
  AddItem(tr("Seek Also Selects"));
  AddItem(tr("Seek to the End of Pastes"));
  AddItem(tr("Scroll Wheel Zooms"));
  AddItem(tr("Invert Timeline Scroll Axes"));
  AddItem(tr("Enable Drag Files to Timeline"));
  AddItem(tr("Auto-Scale By Default"));
  AddItem(tr("Auto-Seek to Imported Clips"));
  AddItem(tr("Audio Scrubbing"));
  AddItem(tr("Drop Files on Media to Replace"));
  AddItem(tr("Enable Hover Focus"));
  AddItem(tr("Ask For Name When Setting Marker"));

  //scroll_wheel_zooms->setToolTip(tr("Hold CTRL to toggle this setting"));
}

void PreferencesBehaviorTab::Accept()
{

}

void PreferencesBehaviorTab::AddItem(const QString &text, const QString& tooltip)
{
  QTreeWidgetItem* item = new QTreeWidgetItem({text});
  item->setToolTip(0, tooltip);
  item->setCheckState(0, Qt::Unchecked);
  behavior_tree_->addTopLevelItem(item);
}
