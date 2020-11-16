/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "nodetableview.h"

#include <QCheckBox>
#include <QHeaderView>

#include "node/param.h"
#include "nodetabletraverser.h"

OLIVE_NAMESPACE_ENTER

NodeTableView::NodeTableView(QWidget* parent) :
  QTreeWidget(parent)
{
  setColumnCount(3);
  setHeaderLabels({tr("Type"),
                   tr("Source"),
                   tr("R/X"),
                   tr("G/Y"),
                   tr("B/Z"),
                   tr("A/W")});
}

void NodeTableView::SelectNodes(const QVector<Node *> &nodes)
{
  foreach (Node* n, nodes) {
    QTreeWidgetItem* top_item = new QTreeWidgetItem();
    top_item->setText(0, n->Name());
    top_item->setFirstColumnSpanned(true);
    this->addTopLevelItem(top_item);
    top_level_item_map_.insert(n, top_item);
  }

  SetTime(last_time_);
}

void NodeTableView::DeselectNodes(const QVector<Node *> &nodes)
{
  foreach (Node* n, nodes) {
    delete top_level_item_map_.take(n);
  }
}

void NodeTableView::SetTime(const rational &time)
{
  last_time_ = time;

  NodeTableTraverser traverser;

  QMap<Node*, QTreeWidgetItem*>::const_iterator i;
  for (i=top_level_item_map_.constBegin(); i!=top_level_item_map_.constEnd(); i++) {
    Node* node = i.key();
    QTreeWidgetItem* item = i.value();

    // Generate a value database for this node at this time
    NodeValueDatabase db = traverser.GenerateDatabase(node, TimeRange(time, time));

    // Delete any children of this item that aren't in this database
    for (int j=0; j<item->childCount(); j++) {
      if (!db.contains(item->child(j)->data(0, Qt::UserRole).toString())) {
        delete item->takeChild(j);
        j--;
      }
    }

    // Update all inputs
    NodeValueDatabase::const_iterator l;

    for (l=db.begin(); l!=db.end(); l++) {
      const NodeValueTable& table = l.value();

      NodeInput* input = node->GetInputWithID(l.key());
      if (!input) {
        // Filters out table entries that aren't inputs (like "global")
        continue;
      }

      QTreeWidgetItem* input_item = nullptr;

      for (int j=0; j<item->childCount(); j++) {
        QTreeWidgetItem* compare = item->child(j);

        if (compare->data(0, Qt::UserRole).toString() == input->id()) {
          input_item = compare;
          break;
        }
      }

      if (!input_item) {
        input_item = new QTreeWidgetItem();
        input_item->setText(0, input->name());
        input_item->setData(0, Qt::UserRole, input->id());
        input_item->setFirstColumnSpanned(true);
        item->addChild(input_item);
      }

      // Create children if necessary
      while (input_item->childCount() < table.Count())  {
        input_item->addChild(new QTreeWidgetItem());
      }

      // Remove children if necessary
      while (input_item->childCount() > table.Count()) {
        delete input_item->takeChild(input_item->childCount() - 1);
      }

      for (int j=0;j<table.Count();j++) {
        const NodeValue& value = table.at(table.Count() - 1 - j);

        // Create item
        QTreeWidgetItem* sub_item = input_item->child(j);

        // Set data type name
        sub_item->setText(0, NodeParam::GetPrettyDataTypeName(value.type()));

        // Determine source
        QString source_name;
        if (value.source()) {
          source_name = value.source()->Name();
        } else {
          source_name = tr("(unknown)");
        }
        sub_item->setText(1, source_name);

        switch (value.type()) {
        case NodeParam::kTexture:
        {
          // NodeTableTraverser puts video params in here
          for (int k=0;k<VideoParams::kRGBAChannelCount;k++) {
            this->setItemWidget(sub_item, 2 + k, new QCheckBox());
          }
          break;
        }
        default:
        {
          QVector<QVariant> split_values = input->split_normal_value_into_track_values(value.data());
          for (int k=0;k<split_values.size();k++) {
            sub_item->setText(2 + k, NodeInput::ValueToString(value.type(), split_values.at(k), true));
          }
        }
        }
      }
    }
  }
}

/*
void NodeTableView::SetNode(Node *n, const rational &time)
{
  NodeTableTraverser traverser;
  NodeValueDatabase db = traverser.GenerateDatabase(n, TimeRange(time, time));

  // Remove top items if necessary
  for (int i=0;i<this->topLevelItemCount();i++) {
    if (!db.contains(this->topLevelItem(i)->data(0, Qt::UserRole).toString())) {
      delete this->takeTopLevelItem(i);
      i--;
    }
  }

  NodeValueDatabase::const_iterator i;

  for (i=db.begin(); i!=db.end(); i++) {
    const NodeValueTable& table = i.value();

    NodeInput* input = n->GetInputWithID(i.key());
    if (!input) {
      // Filters out table entries that aren't inputs (like "global")
      continue;
    }

    QTreeWidgetItem* top_item = nullptr;

    for (int j=0;j<this->topLevelItemCount();j++) {
      QTreeWidgetItem* compare = this->topLevelItem(j);

      if (compare->data(0, Qt::UserRole).toString() == input->id()) {
        top_item = compare;
        break;
      }
    }

    if (!top_item) {
      top_item = new QTreeWidgetItem();
      top_item->setText(0, input->name());
      top_item->setData(0, Qt::UserRole, input->id());
      top_item->setFirstColumnSpanned(true);
      this->addTopLevelItem(top_item);
    }

    // Create children if necessary
    while (top_item->childCount() < table.Count())  {
      top_item->addChild(new QTreeWidgetItem());
    }

    // Remove children if necessary
    while (top_item->childCount() > table.Count()) {
      delete top_item->takeChild(top_item->childCount() - 1);
    }

    for (int j=0;j<table.Count();j++) {
      const NodeValue& value = table.at(table.Count() - 1 - j);

      // Create item
      QTreeWidgetItem* sub_item = top_item->child(j);

      // Set data type name
      sub_item->setText(0, NodeParam::GetPrettyDataTypeName(value.type()));

      // Determine source
      QString source_name;
      if (value.source()) {
        source_name = value.source()->Name();
      } else {
        source_name = tr("(unknown)");
      }
      sub_item->setText(1, source_name);

      switch (value.type()) {
      case NodeParam::kTexture:
      {
        // NodeTableTraverser puts video params in here
        VideoParams p = value.data().value<VideoParams>();
        int channel_count = PixelFormat::ChannelCount(p.format());

        for (int k=0;k<channel_count;k++) {
          this->setItemWidget(sub_item, 2 + k, new QCheckBox());
        }
        break;
      }
      default:
      {
        QVector<QVariant> split_values = input->split_normal_value_into_track_values(value.data());
        for (int k=0;k<split_values.size();k++) {
          sub_item->setText(2 + k, NodeInput::ValueToString(value.type(), split_values.at(k)));
        }
      }
      }
    }
  }
}
*/

OLIVE_NAMESPACE_EXIT
