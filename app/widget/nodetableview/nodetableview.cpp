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

#include <QHeaderView>

#include "node/param.h"
#include "nodetabletraverser.h"

OLIVE_NAMESPACE_ENTER

NodeTableView::NodeTableView(QWidget* parent) :
  QTreeWidget(parent)
{
  setColumnCount(3);
  setHeaderLabels({tr("Type"), tr("Value"), tr("Source")});
}

void NodeTableView::SetNode(Node *n, const rational &time)
{
  clear();

  NodeTableTraverser traverser;
  NodeValueDatabase db = traverser.GenerateDatabase(n, TimeRange(time, time));

  NodeValueDatabase::const_iterator i;

  for (i=db.begin(); i!=db.end(); i++) {
    const NodeValueTable& table = i.value();

    NodeInput* input = n->GetInputWithID(i.key());
    if (!input) {
      // Filters out table entries that aren't inputs (like "global")
      continue;
    }

    QTreeWidgetItem* top_item = new QTreeWidgetItem();
    top_item->setText(0, input->name());
    top_item->setFirstColumnSpanned(true);
    this->addTopLevelItem(top_item);

    for (int j=table.Count()-1; j>=0; j--) {
      const NodeValue& value = table.at(j);

      QString value_str = NodeInput::ValueToString(value.type(), value.data());

      QString source_name;
      if (value.source()) {
        source_name = value.source()->Name();
      } else {
        source_name = tr("(unknown)");
      }

      QTreeWidgetItem* sub_item = new QTreeWidgetItem();
      sub_item->setText(0, NodeParam::GetPrettyDataTypeName(value.type()));
      sub_item->setText(1, value_str);
      sub_item->setText(2, source_name);
      top_item->addChild(sub_item);

      // Special cases
      if (value.type() == NodeParam::kTexture) {
        // NodeTableTraverser converts footage to VideoParams
        QTreeWidgetItem* red_channel = new QTreeWidgetItem();
        red_channel->setText(0, tr("Red"));
        sub_item->addChild(red_channel);

        QTreeWidgetItem* green_channel = new QTreeWidgetItem();
        green_channel->setText(0, tr("Green"));
        sub_item->addChild(green_channel);

        QTreeWidgetItem* blue_channel = new QTreeWidgetItem();
        blue_channel->setText(0, tr("Blue"));
        sub_item->addChild(blue_channel);

        if (PixelFormat::FormatHasAlphaChannel(value.data().value<VideoParams>().format())) {
          QTreeWidgetItem* alpha_channel = new QTreeWidgetItem();
          alpha_channel->setText(0, tr("Alpha"));
          sub_item->addChild(alpha_channel);
        }
      }
    }
  }
}

void NodeTableView::SetMultipleNodeMessage()
{
  this->clear();

  QTreeWidgetItem* item = new QTreeWidgetItem();
  item->setText(0, tr("Multiple nodes selected"));
  item->setFirstColumnSpanned(true);
  this->addTopLevelItem(item);
}

OLIVE_NAMESPACE_EXIT
