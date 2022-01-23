/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#include "serializer210528.h"

#include "node/factory.h"

namespace olive {

ProjectSerializer210528::LoadData ProjectSerializer210528::Load(Project *project, QXmlStreamReader *reader, void *reserved) const
{
  XMLNodeData xml_node_data;

  while (XMLReadNextStartElement(reader)) {
    if (reader->name() == QStringLiteral("url")) {
      project->SetSavedURL(reader->readElementText());
    } else if (reader->name() == QStringLiteral("project")) {
      while (XMLReadNextStartElement(reader)) {
        if (reader->name() == QStringLiteral("layout")) {

          // Since the main window's functions have to occur in the GUI thread (and we're likely
          // loading in a secondary thread), we load all necessary data into a separate struct so we
          // can continue loading and queue it with the main window so it can handle the data
          // appropriately in its own thread.

          project->SetLayoutInfo(MainWindowLayoutInfo::fromXml(reader, xml_node_data.node_ptrs));

        } else if (reader->name() == QStringLiteral("uuid")) {

          project->SetUuid(QUuid::fromString(reader->readElementText()));

        } else if (reader->name() == QStringLiteral("nodes")) {

          while (XMLReadNextStartElement(reader)) {
            if (reader->name() == QStringLiteral("node")) {
              bool is_root = false;
              bool is_cm = false;
              bool is_settings = false;
              QString id;

              {
                XMLAttributeLoop(reader, attr) {
                  if (attr.name() == QStringLiteral("id")) {
                    id = attr.value().toString();
                  } else if (attr.name() == QStringLiteral("root") && attr.value() == QStringLiteral("1")) {
                    is_root = true;
                  } else if (attr.name() == QStringLiteral("cm") && attr.value() == QStringLiteral("1")) {
                    is_cm = true;
                  } else if (attr.name() == QStringLiteral("settings") && attr.value() == QStringLiteral("1")) {
                    is_settings = true;
                  }
                }
              }

              if (id.isEmpty()) {
                qWarning() << "Failed to load node with empty ID";
                reader->skipCurrentElement();
              } else {
                Node* node;

                if (is_root) {
                  node = project->root();
                } else if (is_cm) {
                  node = project->color_manager();
                } else if (is_settings) {
                  node = project->settings();
                } else {
                  node = NodeFactory::CreateFromID(id);
                }

                if (!node) {
                  qWarning() << "Failed to find node with ID" << id;
                  reader->skipCurrentElement();
                } else {
                  LoadNode(node, xml_node_data, reader);
                  node->setParent(project);
                }
              }
            } else {
              reader->skipCurrentElement();
            }
          }

        } else if (reader->name() == QStringLiteral("positions")) {

          while (XMLReadNextStartElement(reader)) {

            if (reader->name() == QStringLiteral("context")) {

              quintptr context_ptr = 0;
              XMLAttributeLoop(reader, attr) {
                if (attr.name() == QStringLiteral("ptr")) {
                  context_ptr = attr.value().toULongLong();
                  break;
                }
              }

              Node *context = xml_node_data.node_ptrs.value(context_ptr);

              if (!context) {
                qWarning() << "Failed to find pointer for context";
                reader->skipCurrentElement();
              } else {
                while (XMLReadNextStartElement(reader)) {
                  if (reader->name() == QStringLiteral("node")) {
                    quintptr node_ptr;
                    Node::Position node_pos;

                    if (LoadPosition(reader, &node_ptr, &node_pos)) {
                      Node *node = xml_node_data.node_ptrs.value(node_ptr);

                      if (node) {
                        context->SetNodePositionInContext(node, node_pos);
                      } else {
                        qWarning() << "Failed to find pointer for node position";
                        reader->skipCurrentElement();
                      }
                    }
                  } else {
                    reader->skipCurrentElement();
                  }
                }
              }

            } else {

              reader->skipCurrentElement();

            }

          }

        } else {

          // Skip this
          reader->skipCurrentElement();

        }
      }
    } else {
      reader->skipCurrentElement();
    }
  }

  // Make connections
  PostConnect(xml_node_data);

  return LoadData();
}

void ProjectSerializer210528::LoadNode(Node *node, XMLNodeData &xml_node_data, QXmlStreamReader *reader) const
{
  while (XMLReadNextStartElement(reader)) {
    if (IsCancelled()) {
      return;
    }

    if (reader->name() == QStringLiteral("input")) {
      LoadInput(node, reader, xml_node_data);
    } else if (reader->name() == QStringLiteral("ptr")) {
      xml_node_data.node_ptrs.insert(reader->readElementText().toULongLong(), node);
    } else if (reader->name() == QStringLiteral("label")) {
      node->SetLabel(reader->readElementText());
    } else if (reader->name() == QStringLiteral("uuid")) {
      node->SetUUID(QUuid::fromString(reader->readElementText()));
    } else if (reader->name() == QStringLiteral("color")) {
      node->SetOverrideColor(reader->readElementText().toInt());
    } else if (reader->name() == QStringLiteral("links")) {
      while (XMLReadNextStartElement(reader)) {
        if (reader->name() == QStringLiteral("link")) {
          xml_node_data.block_links.append({node, reader->readElementText().toULongLong()});
        } else {
          reader->skipCurrentElement();
        }
      }
    } else if (reader->name() == QStringLiteral("custom")) {

      LoadNodeCustom(reader, node, xml_node_data);

    } else if (reader->name() == QStringLiteral("connections")) {
      // Load connections
      while (XMLReadNextStartElement(reader)) {
        if (reader->name() == QStringLiteral("connection")) {
          QString param_id;
          int ele = -1;

          XMLAttributeLoop(reader, attr) {
            if (attr.name() == QStringLiteral("element")) {
              ele = attr.value().toInt();
            } else if (attr.name() == QStringLiteral("input")) {
              param_id = attr.value().toString();
            }
          }

          QString output_node_id;
          QString output_param_id;

          while (XMLReadNextStartElement(reader)) {
            if (reader->name() == QStringLiteral("node")) {
              output_node_id = reader->readElementText();
            } else if (reader->name() == QStringLiteral("output")) {
              output_param_id = reader->readElementText();
            } else {
              reader->skipCurrentElement();
            }
          }

          xml_node_data.desired_connections.append({NodeInput(node, param_id, ele), output_node_id.toULongLong(), output_param_id});
        } else {
          reader->skipCurrentElement();
        }
      }
    } else if (reader->name() == QStringLiteral("hints")) {
      while (XMLReadNextStartElement(reader)) {
        if (reader->name() == QStringLiteral("hint")) {
          QString input;
          int element = -1;

          XMLAttributeLoop(reader, attr) {
            if (attr.name() == QStringLiteral("input")) {
              input = attr.value().toString();
            } else if (attr.name() == QStringLiteral("element")) {
              element = attr.value().toInt();
            }
          }

          Node::ValueHint vh;
          LoadValueHint(&vh, reader);
          node->SetValueHintForInput(input, vh, element);
        } else {
          reader->skipCurrentElement();
        }
      }
    } else {
      reader->skipCurrentElement();
    }
  }
}

void ProjectSerializer210528::LoadInput(Node *node, QXmlStreamReader *reader, XMLNodeData &xml_node_data) const
{
  QString param_id;

  XMLAttributeLoop(reader, attr) {
    if (attr.name() == QStringLiteral("id")) {
      param_id = attr.value().toString();

      break;
    }
  }

  if (param_id.isEmpty()) {
    qWarning() << "Failed to load parameter with missing ID";
    reader->skipCurrentElement();
    return;
  }

  if (!node->HasInputWithID(param_id)) {
    qWarning() << "Failed to load parameter that didn't exist:" << param_id;
    reader->skipCurrentElement();
    return;
  }

  while (XMLReadNextStartElement(reader)) {
    if (IsCancelled()) {
      return;
    }

    if (reader->name() == QStringLiteral("primary")) {
      // Load primary immediate
      LoadImmediate(reader, node, param_id, -1, xml_node_data);
    } else if (reader->name() == QStringLiteral("subelements")) {
      // Load subelements
      XMLAttributeLoop(reader, attr) {
        if (attr.name() == QStringLiteral("count")) {
          node->InputArrayResize(param_id, attr.value().toInt());
        }
      }

      int element_counter = 0;

      while (XMLReadNextStartElement(reader)) {
        if (reader->name() == QStringLiteral("element")) {
          LoadImmediate(reader, node, param_id, element_counter, xml_node_data);

          element_counter++;
        } else {
          reader->skipCurrentElement();
        }
      }
    } else {
      reader->skipCurrentElement();
    }
  }
}

void ProjectSerializer210528::LoadImmediate(QXmlStreamReader *reader, Node *node, const QString& input, int element, XMLNodeData &xml_node_data) const
{
  Q_UNUSED(xml_node_data)

  NodeValue::Type data_type = node->GetInputDataType(input);

  while (XMLReadNextStartElement(reader)) {
    if (reader->name() == QStringLiteral("standard")) {
      // Load standard value
      int val_index = 0;

      while (XMLReadNextStartElement(reader)) {
        if (IsCancelled()) {
          return;
        }

        if (reader->name() == QStringLiteral("track")) {
          QVariant value_on_track;

          if (data_type == NodeValue::kVideoParams) {
            VideoParams vp;
            vp.Load(reader);
            value_on_track = QVariant::fromValue(vp);
          } else if (data_type == NodeValue::kAudioParams) {
            AudioParams ap;
            ap.Load(reader);
            value_on_track = QVariant::fromValue(ap);
          } else {
            QString value_text = reader->readElementText();

            if (!value_text.isEmpty()) {
              value_on_track = NodeValue::StringToValue(data_type, value_text, true);
            }
          }

          node->SetSplitStandardValueOnTrack(input, val_index, value_on_track, element);

          val_index++;
        } else {
          reader->skipCurrentElement();
        }
      }
    } else if (reader->name() == QStringLiteral("keyframing") && node->IsInputKeyframable(input)) {
      node->SetInputIsKeyframing(input, reader->readElementText().toInt(), element);
    } else if (reader->name() == QStringLiteral("keyframes")) {
      int track = 0;

      while (XMLReadNextStartElement(reader)) {
        if (IsCancelled()) {
          return;
        }

        if (reader->name() == QStringLiteral("track")) {
          while (XMLReadNextStartElement(reader)) {
            if (IsCancelled()) {
              return;
            }

            if (reader->name() == QStringLiteral("key")) {
              QString key_input;
              rational key_time;
              NodeKeyframe::Type key_type = NodeKeyframe::kLinear;
              QVariant key_value;
              QPointF key_in_handle;
              QPointF key_out_handle;

              XMLAttributeLoop(reader, attr) {
                if (IsCancelled()) {
                  return;
                }

                if (attr.name() == QStringLiteral("input")) {
                  key_input = attr.value().toString();
                } else if (attr.name() == QStringLiteral("time")) {
                  key_time = rational::fromString(attr.value().toString());
                } else if (attr.name() == QStringLiteral("type")) {
                  key_type = static_cast<NodeKeyframe::Type>(attr.value().toInt());
                } else if (attr.name() == QStringLiteral("inhandlex")) {
                  key_in_handle.setX(attr.value().toDouble());
                } else if (attr.name() == QStringLiteral("inhandley")) {
                  key_in_handle.setY(attr.value().toDouble());
                } else if (attr.name() == QStringLiteral("outhandlex")) {
                  key_out_handle.setX(attr.value().toDouble());
                } else if (attr.name() == QStringLiteral("outhandley")) {
                  key_out_handle.setY(attr.value().toDouble());
                }
              }

              key_value = NodeValue::StringToValue(data_type, reader->readElementText(), true);

              NodeKeyframe* key = new NodeKeyframe(key_time, key_value, key_type, track, element, key_input, node);
              key->set_bezier_control_in(key_in_handle);
              key->set_bezier_control_out(key_out_handle);
            } else {
              reader->skipCurrentElement();
            }
          }

          track++;
        } else {
          reader->skipCurrentElement();
        }
      }
    } else if (reader->name() == QStringLiteral("csinput")) {
      node->SetInputProperty(input, QStringLiteral("col_input"), reader->readElementText());
    } else if (reader->name() == QStringLiteral("csdisplay")) {
      node->SetInputProperty(input, QStringLiteral("col_display"), reader->readElementText());
    } else if (reader->name() == QStringLiteral("csview")) {
      node->SetInputProperty(input, QStringLiteral("col_view"), reader->readElementText());
    } else if (reader->name() == QStringLiteral("cslook")) {
      node->SetInputProperty(input, QStringLiteral("col_look"), reader->readElementText());
    } else {
      reader->skipCurrentElement();
    }
  }
}

bool ProjectSerializer210528::LoadPosition(QXmlStreamReader *reader, quintptr *node_ptr, Node::Position *pos) const
{
  bool got_node_ptr = false;
  bool got_pos_x = false;
  bool got_pos_y = false;

  XMLAttributeLoop(reader, attr) {
    if (attr.name() == QStringLiteral("ptr")) {
      *node_ptr = attr.value().toULongLong();
      got_node_ptr = true;
      break;
    }
  }

  while (XMLReadNextStartElement(reader)) {
    if (reader->name() == QStringLiteral("x")) {
      pos->position.setX(reader->readElementText().toDouble());
      got_pos_x = true;
    } else if (reader->name() == QStringLiteral("y")) {
      pos->position.setY(reader->readElementText().toDouble());
      got_pos_y = true;
    } else if (reader->name() == QStringLiteral("expanded")) {
      pos->expanded = reader->readElementText().toInt();
    } else {
      reader->skipCurrentElement();
    }
  }

  return got_node_ptr && got_pos_x && got_pos_y;
}

void ProjectSerializer210528::PostConnect(const XMLNodeData &xml_node_data) const
{
  foreach (const XMLNodeData::SerializedConnection& con, xml_node_data.desired_connections) {
    if (Node *out = xml_node_data.node_ptrs.value(con.output_node)) {
      // Use output param as hint tag since we grandfathered those in
      Node::ValueHint hint(con.output_param);

      Node::ConnectEdge(out, con.input);

      con.input.node()->SetValueHintForInput(con.input.input(), hint, con.input.element());
    }
  }

  foreach (const XMLNodeData::BlockLink& l, xml_node_data.block_links) {
    Node *a = l.block;
    Node *b = xml_node_data.node_ptrs.value(l.link);

    Node::Link(a, b);
  }

  foreach (const XMLNodeData::GroupLink &l, xml_node_data.group_input_links) {
    if (Node *input_node = xml_node_data.node_ptrs.value(l.input_node)) {
      NodeInput resolved(input_node, l.input_id, l.input_element);

      l.group->AddInputPassthrough(resolved);
    }
  }

  for (auto it=xml_node_data.group_output_links.cbegin(); it!=xml_node_data.group_output_links.cend(); it++) {
    if (Node *output_node = xml_node_data.node_ptrs.value(it.value())) {
      it.key()->SetOutputPassthrough(output_node);
    }
  }
}

void ProjectSerializer210528::LoadNodeCustom(QXmlStreamReader *reader, Node *node, XMLNodeData &xml_node_data) const
{
  // Viewer-based nodes
  if (ViewerOutput *viewer = dynamic_cast<ViewerOutput*>(node)) {

    Footage *footage = dynamic_cast<Footage*>(node);

    while (XMLReadNextStartElement(reader)) {
      if (reader->name() == QStringLiteral("points")) {
        LoadTimelinePoints(reader, viewer->GetTimelinePoints());
      } else if (reader->name() == QStringLiteral("timestamp") && footage) {
        footage->set_timestamp(reader->readElementText().toLongLong());
      } else {
        reader->skipCurrentElement();
      }
    }

  } else if (Track *track = dynamic_cast<Track*>(node)) {

    while (XMLReadNextStartElement(reader)) {
      if (reader->name() == QStringLiteral("height")) {
        track->SetTrackHeight(reader->readElementText().toDouble());
      } else {
        reader->skipCurrentElement();
      }
    }

  } else if (NodeGroup *group = dynamic_cast<NodeGroup*>(node)) {

    while (XMLReadNextStartElement(reader)) {
      if (reader->name() == QStringLiteral("inputpassthroughs")) {
        while (XMLReadNextStartElement(reader)) {
          if (reader->name() == QStringLiteral("inputpassthrough")) {
            XMLNodeData::GroupLink link;

            link.group = group;

            while (XMLReadNextStartElement(reader)) {
              if (reader->name() == QStringLiteral("node")) {
                link.input_node = reader->readElementText().toULongLong();
              } else if (reader->name() == QStringLiteral("input")) {
                link.input_id = reader->readElementText();
              } else if (reader->name() == QStringLiteral("element")) {
                link.input_element = reader->readElementText().toInt();
              } else {
                reader->skipCurrentElement();
              }
            }

            xml_node_data.group_input_links.append(link);
          } else {
            reader->skipCurrentElement();
          }
        }
      } else if (reader->name() == QStringLiteral("outputpassthrough")) {
        xml_node_data.group_output_links.insert(group, reader->readElementText().toULongLong());
      } else {
        reader->skipCurrentElement();
      }
    }

  } else {
    reader->skipCurrentElement();
  }
}

void ProjectSerializer210528::LoadTimelinePoints(QXmlStreamReader *reader, TimelinePoints *points) const
{
  while (XMLReadNextStartElement(reader)) {
    if (reader->name() == QStringLiteral("markers")) {
      LoadMarkerList(reader, points->markers());
    } else if (reader->name() == QStringLiteral("workarea")) {
      LoadWorkArea(reader, points->workarea());
    } else {
      reader->skipCurrentElement();
    }
  }
}

void ProjectSerializer210528::LoadWorkArea(QXmlStreamReader *reader, TimelineWorkArea *workarea) const
{
  rational range_in = workarea->in();
  rational range_out = workarea->out();

  XMLAttributeLoop(reader, attr) {
    if (attr.name() == QStringLiteral("enabled")) {
      workarea->set_enabled(attr.value() != QStringLiteral("0"));
    } else if (attr.name() == QStringLiteral("in")) {
      range_in = rational::fromString(attr.value().toString());
    } else if (attr.name() == QStringLiteral("out")) {
      range_out = rational::fromString(attr.value().toString());
    }
  }

  TimeRange loaded_workarea(range_in, range_out);

  if (loaded_workarea != workarea->range()) {
    workarea->set_range(loaded_workarea);
  }

  reader->skipCurrentElement();
}

void ProjectSerializer210528::LoadMarkerList(QXmlStreamReader *reader, TimelineMarkerList *markers) const
{
  while (XMLReadNextStartElement(reader)) {
    if (reader->name() == QStringLiteral("marker")) {
      QString name;
      rational in, out;

      XMLAttributeLoop(reader, attr) {
        if (attr.name() == QStringLiteral("name")) {
          name = attr.value().toString();
        } else if (attr.name() == QStringLiteral("in")) {
          in = rational::fromString(attr.value().toString());
        } else if (attr.name() == QStringLiteral("out")) {
          out = rational::fromString(attr.value().toString());
        }
      }

      markers->AddMarker(TimeRange(in, out), name);
    }

    reader->skipCurrentElement();
  }
}

void ProjectSerializer210528::LoadValueHint(Node::ValueHint *hint, QXmlStreamReader *reader) const
{
  QVector<NodeValue::Type> types;

  while (XMLReadNextStartElement(reader)) {
    if (reader->name() == QStringLiteral("types")) {
      while (XMLReadNextStartElement(reader)) {
        if (reader->name() == QStringLiteral("type")) {
          types.append(static_cast<NodeValue::Type>(reader->readElementText().toInt()));
        } else {
          reader->skipCurrentElement();
        }
      }
    } else if (reader->name() == QStringLiteral("index")) {
      hint->set_index(reader->readElementText().toInt());
    } else if (reader->name() == QStringLiteral("tag")) {
      hint->set_tag(reader->readElementText());
    } else {
      reader->skipCurrentElement();
    }
  }

  hint->set_type(types);
}

}
