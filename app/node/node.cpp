/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#include "node.h"

#include <QApplication>
#include <QGuiApplication>
#include <QDebug>
#include <QFile>

#include "common/timecodefunctions.h"
#include "common/xmlutils.h"
#include "config/config.h"
#include "project/project.h"
#include "project/item/footage/footage.h"
#include "project/item/footage/videostream.h"
#include "ui/colorcoding.h"
#include "widget/nodeview/nodeviewundo.h"

namespace olive {

#define super NodeConnectable

Node::Node() :
  can_be_deleted_(true),
  override_color_(-1)
{
}

Node::~Node()
{
  DisconnectAll();

  setParent(nullptr);
}

NodeGraph *Node::parent() const
{
  return static_cast<NodeGraph*>(QObject::parent());
}

void Node::Load(QXmlStreamReader *reader, XMLNodeData& xml_node_data, const QAtomicInt* cancelled)
{
  while (XMLReadNextStartElement(reader)) {
    if (cancelled && *cancelled) {
      return;
    }

    if (reader->name() == QStringLiteral("input")) {
      QString param_id;

      XMLAttributeLoop(reader, attr) {
        if (attr.name() == QStringLiteral("id")) {
          param_id = attr.value().toString();

          break;
        }
      }

      if (param_id.isEmpty()) {
        qDebug() << "Found parameter with no ID";
        continue;
      }

      NodeInput* param = GetInputWithID(param_id);

      if (!param) {
        qDebug() << "No parameter in" << id() << "with parameter" << param_id;
        reader->skipCurrentElement();
        continue;
      }

      param->Load(reader, xml_node_data, cancelled);
    } else if (reader->name() == QStringLiteral("ptr")) {
      xml_node_data.node_ptrs.insert(reader->readElementText().toULongLong(), this);
    } else if (reader->name() == QStringLiteral("pos")) {
      QPointF p;

      while (XMLReadNextStartElement(reader)) {
        if (reader->name() == QStringLiteral("x")) {
          p.setX(reader->readElementText().toDouble());
        } else if (reader->name() == QStringLiteral("y")) {
          p.setY(reader->readElementText().toDouble());
        } else {
          reader->skipCurrentElement();
        }
      }

      SetPosition(p);
    } else if (reader->name() == QStringLiteral("label")) {
      SetLabel(reader->readElementText());
    } else if (reader->name() == QStringLiteral("color")) {
      override_color_ = reader->readElementText().toInt();
    } else if (reader->name() == QStringLiteral("custom")) {
      LoadInternal(reader, xml_node_data);
    } else {
      reader->skipCurrentElement();
    }
  }
}

void Node::Save(QXmlStreamWriter *writer) const
{
  writer->writeTextElement(QStringLiteral("ptr"), QString::number(reinterpret_cast<quintptr>(this)));

  writer->writeStartElement(QStringLiteral("pos"));
  writer->writeTextElement(QStringLiteral("x"), QString::number(GetPosition().x()));
  writer->writeTextElement(QStringLiteral("y"), QString::number(GetPosition().y()));
  writer->writeEndElement(); // pos

  writer->writeTextElement(QStringLiteral("label"), GetLabel());
  writer->writeTextElement(QStringLiteral("color"), QString::number(override_color_));

  foreach (NodeInput* input, inputs_) {
    writer->writeStartElement(QStringLiteral("input"));

    input->Save(writer);

    writer->writeEndElement(); // input
  }

  writer->writeStartElement(QStringLiteral("custom"));
  SaveInternal(writer);
  writer->writeEndElement(); // custom
}

QString Node::ShortName() const
{
  return Name();
}

QString Node::Description() const
{
  // Return an empty string by default
  return QString();
}

void Node::Retranslate()
{
}

Color Node::color() const
{
  int c;

  if (override_color_ >= 0) {
    c = override_color_;
  } else {
    c = Config::Current()[QStringLiteral("CatColor%1").arg(this->Category().first())].toInt();
  }

  return ColorCoding::GetColor(c);
}

QLinearGradient Node::gradient_color(qreal top, qreal bottom) const
{
  QLinearGradient grad;

  grad.setStart(0, top);
  grad.setFinalStop(0, bottom);

  QColor c = color().toQColor();

  grad.setColorAt(0.0, c.lighter());
  grad.setColorAt(1.0, c);

  return grad;
}

QBrush Node::brush(qreal top, qreal bottom) const
{
  if (Config::Current()[QStringLiteral("UseGradients")].toBool()) {
    return gradient_color(top, bottom);
  } else {
    return color().toQColor();
  }
}

void Node::RemoveNodesAndExclusiveDependencies(Node *node, QUndoCommand *command)
{
  // Remove main node
  RemoveNodeAndDisconnect(node, command);

  // Remove exclusive dependencies
  QVector<Node*> deps = node->GetExclusiveDependencies();
  foreach (Node* d, deps) {
    RemoveNodeAndDisconnect(d, command);
  }
}

void Node::RemoveNodeAndDisconnect(Node *node, QUndoCommand *command)
{
  // Disconnect everything
  foreach (const InputConnection& conn, node->output_connections()) {
    new NodeEdgeRemoveCommand(node, conn.input, conn.element, command);
  }

  foreach (NodeInput* input, node->inputs_) {
    for (auto it=input->edges().cbegin(); it!=input->edges().cend(); it++) {
      new NodeEdgeRemoveCommand(it->second, input, it->first, command);
    }
  }

  // Remove node
  new NodeRemoveCommand(node, command);
}

NodeValueTable Node::Value(NodeValueDatabase &value) const
{
  return value.Merge();
}

void Node::InvalidateCache(const TimeRange &range, const InputConnection &from)
{
  Q_UNUSED(from)

  SendInvalidateCache(range);
}

void Node::BeginOperation()
{
  // Ripple through graph
  foreach (const InputConnection& conn, output_connections()) {
    conn.input->parent()->BeginOperation();
  }
}

void Node::EndOperation()
{
  // Ripple through graph
  foreach (const InputConnection& conn, output_connections()) {
    conn.input->parent()->EndOperation();
  }
}

TimeRange Node::InputTimeAdjustment(NodeInput *, int, const TimeRange &input_time) const
{
  // Default behavior is no time adjustment at all
  return input_time;
}

TimeRange Node::OutputTimeAdjustment(NodeInput *, int, const TimeRange &input_time) const
{
  // Default behavior is no time adjustment at all
  return input_time;
}

QVector<Node *> Node::CopyDependencyGraph(const QVector<Node *> &nodes, QUndoCommand* command)
{
  int nb_nodes = nodes.size();

  QVector<Node*> copies(nb_nodes);

  for (int i=0; i<nb_nodes; i++) {
    // Create another of the same node
    Node* c = nodes.at(i)->copy();;

    // Copy the values, but NOT the connections, since we'll be connecting to our own clones later
    Node::CopyInputs(nodes.at(i), c, false);

    // Add to graph
    NodeGraph* graph = static_cast<NodeGraph*>(nodes.at(i)->parent());
    if (command) {
      new NodeAddCommand(graph, c, command);
    } else {
      c->setParent(graph);
    }

    // Store in array at the same index as source
    copies[i] = c;
  }

  CopyDependencyGraph(nodes, copies, command);

  return copies;
}

void Node::CopyDependencyGraph(const QVector<Node *> &src, const QVector<Node *> &dst, QUndoCommand *command)
{
  for (int i=0; i<src.size(); i++) {
    foreach (NodeInput* input, src.at(i)->inputs()) {
      for (auto it=input->edges().cbegin(); it!=input->edges().cend(); it++) {
        int connection_index = src.indexOf(it->second);

        if (connection_index > -1) {
          // Found a connection
          Node* dst_output = dst.at(connection_index);
          NodeInput* dst_input = dst.at(i)->GetInputWithID(input->id());

          if (command) {
            new NodeEdgeAddCommand(dst_output, dst_input, it->first, command);
          } else {
            ConnectEdge(dst_output, dst_input, it->first);
          }
        }
      }
    }
  }
}

void Node::SendInvalidateCache(const TimeRange &range)
{
  foreach (const InputConnection& conn, output_connections()) {
    // Send clear cache signal to the Node
    conn.input->parent()->InvalidateCache(range, conn);
  }
}

void Node::InvalidateAll(NodeInput* input, int element)
{
  InvalidateCache(TimeRange(RATIONAL_MIN, RATIONAL_MAX), {input, element});
}

void Node::IgnoreInvalidationsFrom(NodeInput *input)
{
  ignore_connections_.append(input);
}

void Node::LoadInternal(QXmlStreamReader *reader, XMLNodeData &)
{
  reader->skipCurrentElement();
}

void Node::SaveInternal(QXmlStreamWriter *) const
{
}

QVector<NodeInput *> Node::GetInputsToHash() const
{
  return inputs_;
}

bool Node::HasGizmos() const
{
  return false;
}

void Node::DrawGizmos(NodeValueDatabase &, QPainter *)
{
}

bool Node::GizmoPress(NodeValueDatabase &, const QPointF &)
{
  return false;
}

void Node::GizmoMove(const QPointF &, const rational&)
{
}

void Node::GizmoRelease()
{
}

const QString &Node::GetLabel() const
{
  return label_;
}

void Node::SetLabel(const QString &s)
{
  if (label_ != s) {
    label_ = s;

    emit LabelChanged(label_);
  }
}

void Node::Hash(QCryptographicHash &hash, const rational& time) const
{
  // Add this Node's ID
  hash.addData(id().toUtf8());

  QVector<NodeInput*> inputs = GetInputsToHash();

  foreach (NodeInput* input, inputs) {
    // For each input, try to hash its value
    HashInputElement(hash, input, -1, time);
    for (int i=0; i<input->ArraySize(); i++) {
      HashInputElement(hash, input, i, time);
    }
  }
}

void Node::CopyInputs(Node *source, Node *destination, bool include_connections)
{
  Q_ASSERT(source->id() == destination->id());

  const QVector<NodeInput*>& src_param = source->inputs_;
  const QVector<NodeInput*>& dst_param = destination->inputs_;

  for (int i=0;i<src_param.size();i++) {
    NodeInput* src = static_cast<NodeInput*>(src_param.at(i));
    NodeInput* dst = static_cast<NodeInput*>(dst_param.at(i));

    NodeInput::CopyValues(src, dst, include_connections);
  }

  destination->SetPosition(source->GetPosition());
  destination->SetLabel(source->GetLabel());
}

bool Node::CanBeDeleted() const
{
  return can_be_deleted_;
}

void Node::SetCanBeDeleted(bool s)
{
  can_be_deleted_ = s;
}

/**
 * @brief Recursively collects dependencies of Node `n` and appends them to QList `list`
 *
 * @param traverse
 *
 * TRUE to recursively traverse each node for a complete dependency graph. FALSE to return only the immediate
 * dependencies.
 */
QVector<Node *> Node::GetDependenciesInternal(bool traverse, bool exclusive_only) const
{
  QVector<Node*> list;

  foreach (NodeInput* i, inputs_) {
    i->GetDependencies(list, traverse, exclusive_only);
  }

  return list;
}

void Node::HashInputElement(QCryptographicHash &hash, NodeInput *input, int element, const rational &time) const
{
  // Get time adjustment
  // For a single frame, we only care about one of the times
  rational input_time = InputTimeAdjustment(input, element, TimeRange(time, time)).in();

  if (input->IsConnected(element)) {
    // Traverse down this edge
    input->GetConnectedNode(element)->Hash(hash, input_time);
  } else {
    // Grab the value at this time
    QVariant value = input->GetValueAtTime(input_time, element);
    hash.addData(NodeValue::ValueToBytes(input->GetDataType(), value));
  }

  // We have one exception for FOOTAGE types, since we resolve the footage into a frame in the renderer
  if (input->GetDataType() == NodeValue::kFootage) {
    Stream* stream = Node::ValueToPtr<Stream>(input->GetStandardValue(element));

    if (stream) {
      // Add footage details to hash

      // Footage filename
      hash.addData(stream->footage()->filename().toUtf8());

      // Footage last modified date
      hash.addData(QString::number(stream->footage()->timestamp()).toUtf8());

      // Footage stream
      hash.addData(QString::number(stream->index()).toUtf8());

      if (stream->type() == Stream::kVideo) {
        VideoStream* image_stream = static_cast<VideoStream*>(stream);

        // Current color config and space
        hash.addData(image_stream->footage()->project()->color_manager()->GetConfigFilename().toUtf8());
        hash.addData(image_stream->colorspace().toUtf8());

        // Alpha associated setting
        hash.addData(QString::number(image_stream->premultiplied_alpha()).toUtf8());

        // Pixel aspect ratio
        hash.addData(reinterpret_cast<const char*>(&image_stream->pixel_aspect_ratio()), sizeof(rational));
      }

      // Footage timestamp
      if (stream->type() == Stream::kVideo) {
        VideoStream* video_stream = static_cast<VideoStream*>(stream);

        int64_t video_ts = Timecode::time_to_timestamp(input_time, video_stream->timebase());

        // Add timestamp in units of the video stream's timebase
        hash.addData(reinterpret_cast<const char*>(&video_ts), sizeof(int64_t));

        // Add start time - used for both image sequences and video streams
        hash.addData(QString::number(video_stream->start_time()).toUtf8());
      }
    }
  }
}

void Node::ParameterConnected(Node *source, int element)
{
  NodeInput* input = static_cast<NodeInput*>(sender());

  emit InputConnected(source, input, element);

  if (ignore_connections_.contains(input)) {
    return;
  }

  InvalidateAll(input, element);
}

void Node::ParameterDisconnected(Node *source, int element)
{
  NodeInput* input = static_cast<NodeInput*>(sender());

  emit InputDisconnected(source, input, element);

  if (ignore_connections_.contains(input)) {
    return;
  }

  InvalidateAll(input, element);
}

QVector<Node *> Node::GetDependencies() const
{
  return GetDependenciesInternal(true, false);
}

QVector<Node *> Node::GetExclusiveDependencies() const
{
  return GetDependenciesInternal(true, true);
}

QVector<Node *> Node::GetImmediateDependencies() const
{
  return GetDependenciesInternal(false, false);
}

ShaderCode Node::GetShaderCode(const QString &shader_id) const
{
  Q_UNUSED(shader_id)

  return ShaderCode(QString(), QString());
}

void Node::ProcessSamples(NodeValueDatabase &, const SampleBufferPtr, SampleBufferPtr, int) const
{
}

void Node::GenerateFrame(FramePtr frame, const GenerateJob &job) const
{
  Q_UNUSED(frame)
  Q_UNUSED(job)
}

NodeInput *Node::GetInputWithID(const QString &id) const
{
  foreach (NodeInput* i, inputs_) {
    if (i->id() == id) {
      return i;
    }
  }

  return nullptr;
}

bool Node::OutputsTo(Node *n, bool recursively) const
{
  foreach (const InputConnection& conn, output_connections()) {
    Node* connected = conn.input->parent();

    if (connected == n) {
      return true;
    } else if (recursively && connected->OutputsTo(n, recursively)) {
      return true;
    }
  }

  return false;
}

bool Node::OutputsTo(const QString &id, bool recursively) const
{
  foreach (const InputConnection& conn, output_connections()) {
    Node* connected = conn.input->parent();

    if (connected->id() == id) {
      return true;
    } else if (recursively && connected->OutputsTo(id, recursively)) {
      return true;
    }
  }

  return false;
}

bool Node::OutputsTo(NodeInput *input, bool recursively) const
{
  foreach (const InputConnection& conn, output_connections()) {
    NodeInput* connected = conn.input;

    if (connected == input) {
      return true;
    } else if (recursively && connected->parent()->OutputsTo(input, recursively)) {
      return true;
    }
  }

  return false;
}

bool Node::InputsFrom(Node *n, bool recursively) const
{
  foreach (NodeInput* input, inputs_) {
    for (auto it=input->edges().cbegin(); it!=input->edges().cend(); it++) {
      Node* connected = it->second;

      if (connected == n) {
        return true;
      } else if (recursively && connected->InputsFrom(n, recursively)) {
        return true;
      }
    }
  }

  return false;
}

bool Node::InputsFrom(const QString &id, bool recursively) const
{
  foreach (NodeInput* input, inputs_) {
    for (auto it=input->edges().cbegin(); it!=input->edges().cend(); it++) {
      Node* connected = it->second;

      if (connected->id() == id) {
        return true;
      } else if (recursively && connected->InputsFrom(id, recursively)) {
        return true;
      }
    }
  }

  return false;
}

int Node::GetRoutesTo(Node *n) const
{
  bool outputs_directly = false;
  int routes = 0;

  foreach (const InputConnection& conn, edges()) {
    Node* connected_node = conn.input->parent();

    if (connected_node == n) {
      outputs_directly = true;
    } else {
      routes += connected_node->GetRoutesTo(n);
    }
  }

  if (outputs_directly) {
    routes++;
  }

  return routes;
}

void Node::DisconnectAll()
{
  // Disconnect outputs (inputs will be disconnected in their respective destructors)
  while (!edges().empty()) {
    DisconnectEdge(this, edges().front().input, edges().front().element);
  }
}

QString Node::GetCategoryName(const CategoryID &c)
{
  switch (c) {
  case kCategoryInput:
    return tr("Input");
  case kCategoryOutput:
    return tr("Output");
  case kCategoryGeneral:
    return tr("General");
  case kCategoryDistort:
    return tr("Distort");
  case kCategoryMath:
    return tr("Math");
  case kCategoryColor:
    return tr("Color");
  case kCategoryFilter:
    return tr("Filter");
  case kCategoryTimeline:
    return tr("Timeline");
  case kCategoryGenerator:
    return tr("Generator");
  case kCategoryChannels:
    return tr("Channel");
  case kCategoryTransition:
    return tr("Transition");
  case kCategoryUnknown:
  case kCategoryCount:
    break;
  }

  return tr("Uncategorized");
}

QVector<TimeRange> Node::TransformTimeTo(const TimeRange &time, Node *target, bool input_dir)
{
  QVector<TimeRange> paths_found;

  if (input_dir) {
    // If this input is connected, traverse it to see if we stumble across the specified `node`
    foreach (NodeInput* input, inputs_) {
      for (auto it=input->edges().cbegin(); it!=input->edges().cend(); it++) {
        TimeRange input_adjustment = InputTimeAdjustment(input, it->first, time);
        Node* connected = it->second;

        if (connected == target) {
          // We found the target, no need to keep traversing
          if (!paths_found.contains(input_adjustment)) {
            paths_found.append(input_adjustment);
          }
        } else {
          // We did NOT find the target, traverse this
          paths_found.append(connected->TransformTimeTo(input_adjustment, target, input_dir));
        }
      }
    }
  } else {
    // If this input is connected, traverse it to see if we stumble across the specified `node`
    foreach (const InputConnection& conn, edges()) {
      Node* input_node = conn.input->parent();

      TimeRange output_adjustment = input_node->OutputTimeAdjustment(conn.input, conn.element, time);

      if (input_node == target) {
        paths_found.append(output_adjustment);
      } else {
        paths_found.append(input_node->TransformTimeTo(output_adjustment, target, input_dir));
      }
    }
  }

  return paths_found;
}

QVariant Node::PtrToValue(void *ptr)
{
  return reinterpret_cast<quintptr>(ptr);
}

bool Node::HasParamWithID(const QString &id) const
{
  foreach (NodeInput* i, inputs_) {
    if (i->id() == id) {
      return true;
    }
  }

  return false;
}

const QPointF &Node::GetPosition() const
{
  return position_;
}

void Node::SetPosition(const QPointF &pos)
{
  position_ = pos;

  emit PositionChanged(position_);
}

void Node::ParameterValueChanged(const TimeRange& range, int element)
{
  NodeInput* input = static_cast<NodeInput*>(sender());

  emit ValueChanged(input, element);

  if (ignore_connections_.contains(input)) {
    return;
  }

  InvalidateCache(range, InputConnection(static_cast<NodeInput*>(sender()), element));
}

QRectF Node::CreateGizmoHandleRect(const QPointF &pt, int radius)
{
  return QRectF(pt.x() - radius,
                pt.y() - radius,
                2*radius,
                2*radius);
}

double Node::GetGizmoHandleRadius(const QTransform &transform)
{
  double raw_value = QFontMetrics(qApp->font()).height() * 0.25;

  raw_value /= transform.m11();

  return raw_value;
}

void Node::DrawAndExpandGizmoHandles(QPainter *p, int handle_radius, QRectF *rects, int count)
{
  for (int i=0; i<count; i++) {
    QRectF& r = rects[i];

    // Draw rect on screen
    p->drawRect(r);

    // Extend rect so it's easier to drag with handle
    r.adjust(-handle_radius, -handle_radius, handle_radius, handle_radius);
  }
}

void Node::childEvent(QChildEvent *event)
{
  super::childEvent(event);

  NodeInput* input = dynamic_cast<NodeInput*>(event->child());

  if (input) {
    if (event->type() == QEvent::ChildAdded) {
      // Ensure no other param with this ID has been added to this Node (since that defeats the purpose)
      Q_ASSERT(!HasParamWithID(input->id()));

      // Keep main output as the last parameter, assume if there are no parameters that this is the output parameter
      inputs_.append(input);

      connect(input, &NodeInput::ValueChanged, this, &Node::ParameterValueChanged);
      connect(input, &NodeInput::InputConnected, this, &Node::ParameterConnected);
      connect(input, &NodeInput::InputDisconnected, this, &Node::ParameterDisconnected);
    } else if (event->type() == QEvent::ChildRemoved) {
      disconnect(input, &NodeInput::ValueChanged, this, &Node::ParameterValueChanged);
      disconnect(input, &NodeInput::InputConnected, this, &Node::ParameterConnected);
      disconnect(input, &NodeInput::InputDisconnected, this, &Node::ParameterDisconnected);

      inputs_.removeOne(input);
    }
  }
}

}
