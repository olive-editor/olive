#include "nodeeditor.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QGraphicsProxyWidget>
#include <QSplitter>
#include <QDebug>

#include "global/global.h"

NodeEditor::NodeEditor(QWidget *parent) :
  EffectsPanel(parent),
  view_(&scene_)
{ 
  setWindowTitle(tr("Node Editor"));
  resize(720, 480);

  QWidget* central_widget = new QWidget();
  setWidget(central_widget);

  QSplitter* splitter = new QSplitter();
  splitter->setOrientation(Qt::Horizontal);
  splitter->setChildrenCollapsible(false);

  splitter->addWidget(&view_);

  QSizePolicy view_policy;
  view_policy.setHorizontalStretch(1);
  view_.setSizePolicy(view_policy);
  view_.setInteractive(true);
  view_.setDragMode(QGraphicsView::RubberBandDrag);

  prop_layout_ = new QVBoxLayout(&props_);
  prop_layout_->setMargin(0);
  prop_layout_->setSpacing(0);
  prop_layout_->addStretch();
  splitter->addWidget(&props_);

  QVBoxLayout* layout = new QVBoxLayout(central_widget);
  layout->setSpacing(0);
  layout->setMargin(0);

  layout->addWidget(splitter);

  connect(&scene_, SIGNAL(changed(const QList<QRectF>&)), this, SLOT(ItemsChanged()));
  connect(&scene_, SIGNAL(selectionChanged()), this, SLOT(UpdateNodeProperties()));
  connect(&view_, SIGNAL(RequestContextMenu()), this, SLOT(ContextMenu()));
}

NodeEditor::~NodeEditor()
{
  Clear(true);
}

void NodeEditor::Retranslate()
{
}

void NodeEditor::LoadEvent()
{
  if (!open_effects_.isEmpty()) {
    QVector<NodeEdge*> added_edges;

    Clip* first_clip = open_effects_.first()->GetEffect()->parent_clip;

    for (int i=0;i<open_effects_.size();i++) {
      EffectUI* effect_ui = open_effects_.at(i);
      Node* node = effect_ui->GetEffect();

      if (node->parent_clip == first_clip) {
        NodeUI* node_ui = new NodeUI();

        effect_ui->SetSelectable(false);

        node_ui->SetNode(node);
        node_ui->AddToScene(&scene_);
        node_ui->setPos(effect_ui->GetEffect()->pos());

        nodes_.append(node_ui);

        effect_ui->setVisible(false);
        prop_layout_->insertWidget(prop_layout_->count()-1, effect_ui);
      }
    }
  }

  LoadEdges();
}

void NodeEditor::AboutToClearEvent()
{
  foreach (NodeUI* node, nodes_) {
    delete node;
  }

  nodes_.clear();

  ClearEdges();
}

void NodeEditor::ClearEdges()
{
  foreach (NodeEdgeUI* edge, edges_) {
    scene_.removeItem(edge);
    delete edge;
  }

  edges_.clear();

  DisconnectAllRows();
}

void NodeEditor::LoadEdges()
{
  if (!open_effects_.isEmpty()) {
    QVector<NodeEdge*> added_edges;

    Clip* first_clip = open_effects_.first()->GetEffect()->parent_clip;

    for (int i=0;i<open_effects_.size();i++) {

      Node* n = open_effects_.at(i)->GetEffect();

      if (n->parent_clip == first_clip) {

        // Connect all rows to this
        for (int j=0;j<n->row_count();j++) {
          ConnectRow(n->row(j));
        }

        // Get node edges
        QVector<NodeEdgePtr> edges = n->GetAllEdges();
        for (int j=0;j<edges.size();j++) {

          NodeEdge* edge = edges.at(j).get();

          if (!added_edges.contains(edge)) {
            NodeEdgeUI* edge_ui = new NodeEdgeUI(edge);
            scene_.addItem(edge_ui);
            edges_.append(edge_ui);

            added_edges.append(edge);
          }
        }
      }
    }

    ItemsChanged();
  }
}

void NodeEditor::ConnectRow(EffectRow *row)
{
  if (!connected_rows_.contains(row)) {
    connect(row, SIGNAL(EdgesChanged()), this, SLOT(ReloadEdges()));
    connected_rows_.append(row);
  }
}

void NodeEditor::DisconnectAllRows()
{
  for (int i=0;i<connected_rows_.size();i++) {
    disconnect(connected_rows_.at(i), SIGNAL(EdgesChanged()), this, SLOT(ReloadEdges()));
  }
  connected_rows_.clear();
}

void NodeEditor::ItemsChanged()
{
  foreach (NodeEdgeUI* edge, edges_) {
    edge->adjust();
  }

  foreach (NodeUI* node, nodes_) {
    node->GetNode()->SetPos(node->pos());
  }
}

void NodeEditor::ReloadEdges()
{
  ClearEdges();
  LoadEdges();
}

void NodeEditor::ContextMenu()
{
  if (!open_effects_.isEmpty()) {
    Clip* c = open_effects_.first()->GetEffect()->parent_clip;
    olive::Global->ShowEffectMenu(EFFECT_TYPE_EFFECT, c->type(), {c});
  }
}

void NodeEditor::UpdateNodeProperties()
{
  // Find matching widget
  for (int j=0;j<open_effects_.size();j++) {
    open_effects_.at(j)->setVisible(false);
  }

  QList<QGraphicsItem*> sel = scene_.selectedItems();
  for (int i=0;i<sel.size();i++) {
    NodeUI* node_ui = dynamic_cast<NodeUI*>(sel.at(i));

    if (node_ui != nullptr) {
      for (int j=0;j<open_effects_.size();j++) {
        if (open_effects_.at(j)->GetEffect() == node_ui->GetNode()) {
          open_effects_.at(j)->setVisible(true);
          break;
        }
      }
    }
  }
}
