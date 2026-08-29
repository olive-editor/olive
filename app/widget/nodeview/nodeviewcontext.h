/*
 * Oak Video Editor - Non-Linear Video Editor
 * Copyright (C) 2025 Olive CE Team
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NODEVIEWCONTEXT_H
#define NODEVIEWCONTEXT_H

#include <QGraphicsRectItem>
#include <QGraphicsTextItem>

#include "node/node.h"
#include "node/nodeundo.h"
#include "nodeviewcommon.h"
#include "nodeviewedge.h"

namespace olive
{

class NodeViewContext : public QObject, public QGraphicsRectItem {
	Q_OBJECT
public:
	NodeViewContext(Node *context, QGraphicsItem *item = nullptr);

	virtual ~NodeViewContext() override;

	Node *GetContext() const
	{
		return context_;
	}

	void UpdateRect();

	void SetFlowDirection(NodeViewCommon::FlowDirection dir);

	void SetCurvedEdges(bool e);

	int DeleteSelected(NodeViewDeleteCommand *command);

	void Select(const QVector<Node *> &nodes);

	QVector<NodeViewItem *> GetSelectedItems() const;

	QPointF MapScenePosToNodePosInContext(const QPointF &pos) const;

	NodeViewItem *GetItemFromMap(Node *node) const
	{
		return item_map_.value(node);
	}

	virtual void paint(QPainter *painter,
					   const QStyleOptionGraphicsItem *option,
					   QWidget *widget = nullptr) override;

public slots:
	void AddChild(Node *node);

	void SetChildPosition(Node *node, const QPointF &pos);

	void RemoveChild(Node *node);

	void ChildInputConnected(Node *output, const NodeInput &input);

	bool ChildInputDisconnected(Node *output, const NodeInput &input);

signals:
	void ItemAboutToBeDeleted(NodeViewItem *item);

protected:
	virtual QVariant itemChange(QGraphicsItem::GraphicsItemChange change,
								const QVariant &value) override;

	virtual void mousePressEvent(QGraphicsSceneMouseEvent *event) override;

private:
	void AddNodeInternal(Node *node, NodeViewItem *item);

	void AddEdgeInternal(Node *output, const NodeInput &input,
						 NodeViewItem *from, NodeViewItem *to);

	Node *context_;

	QString lbl_;

	NodeViewCommon::FlowDirection flow_dir_;

	bool curved_edges_;

	int last_titlebar_height_;

	QMap<Node *, NodeViewItem *> item_map_;

	QVector<NodeViewEdge *> edges_;

private slots:
	void GroupAddedNode(Node *node);

	void GroupRemovedNode(Node *node);
};

}

#endif // NODEVIEWCONTEXT_H
