/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#include "serializer211228.h"

#include "config/config.h"
#include "node/factory.h"
#include "node/group/group.h"

namespace olive {

ProjectSerializer211228::LoadData ProjectSerializer211228::Load(Project *project, QXmlStreamReader *reader, LoadType load_type, void *reserved) const
{
  QMap<quintptr, QMap<QString, QString> > properties;
  QMap<quintptr, QMap<quintptr, Node::Position> > positions;
  XMLNodeData xml_node_data;

  while (XMLReadNextStartElement(reader)) {
    if (reader->name() == QStringLiteral("uuid")) {

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
            bool handled_elsewhere = false;

            if (is_root) {
              project->Initialize();
              node = project->root();
            } else if (is_cm) {
              LoadColorManager(reader, project);
              handled_elsewhere = true;
            } else if (is_settings) {
              LoadProjectSettings(reader, project);
              handled_elsewhere = true;
            } else {
              node = NodeFactory::CreateFromID(id);
            }

            if (!handled_elsewhere) {
              if (!node) {
                qWarning() << "Failed to find node with ID" << id;
                reader->skipCurrentElement();
              } else {
                LoadNode(node, xml_node_data, reader);
                node->setParent(project);
              }
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

          if (context_ptr) {
            while (XMLReadNextStartElement(reader)) {
              if (reader->name() == QStringLiteral("node")) {
                quintptr node_ptr;
                Node::Position node_pos;

                if (LoadPosition(reader, &node_ptr, &node_pos)) {
                  if (node_ptr) {
                    positions[context_ptr].insert(node_ptr, node_pos);
                  } else {
                    qWarning() << "Failed to find pointer for node position";
                    reader->skipCurrentElement();
                  }
                }
              } else {
                reader->skipCurrentElement();
              }
            }
          } else {
            qWarning() << "Attempted to load context with no pointer";
            reader->skipCurrentElement();
          }

        } else {

          reader->skipCurrentElement();

        }

      }


    } else if (reader->name() == QStringLiteral("properties")) {

      while (XMLReadNextStartElement(reader)) {
        if (reader->name() == QStringLiteral("node")) {
          quintptr ptr = 0;

          XMLAttributeLoop(reader, attr) {
            if (attr.name() == QStringLiteral("ptr")) {
              ptr = attr.value().toULongLong();

              // Only attribute we're looking for right now
              break;
            }
          }

          if (ptr) {
            QMap<QString, QString> properties_for_node;
            while (XMLReadNextStartElement(reader)) {
              properties_for_node.insert(reader->name().toString(), reader->readElementText());
            }
            properties.insert(ptr, properties_for_node);
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

  // Resolve positions
  for (auto it=positions.cbegin(); it!=positions.cend(); it++) {
    Node *ctx = xml_node_data.node_ptrs.value(it.key());
    if (ctx) {
      for (auto jt=it.value().cbegin(); jt!=it.value().cend(); jt++) {
        Node *n = xml_node_data.node_ptrs.value(jt.key());
        if (n) {
          ctx->SetNodePositionInContext(n, jt.value());
        }
      }
    }
  }

  // Make connections
  PostConnect(xml_node_data);

  LoadData load_data;

  // Resolve serialized properties (if any)
  for (auto it=properties.cbegin(); it!=properties.cend(); it++) {
    Node *node = xml_node_data.node_ptrs.value(it.key());
    if (node) {
      load_data.properties.insert(node, it.value());
    }
  }

  // Resolve tracks
  for (Node *n : project->nodes()) {
    n->SetCachesEnabled(true);

    if (Track *t = dynamic_cast<Track *>(n)) {
      for (int i = 0; i < t->InputArraySize(Track::kBlockInput); i++) {
        Block *b = static_cast<Block*>(t->GetConnectedOutput(Track::kBlockInput, i));
        if (!b->track()) {
          t->AppendBlock(b);
        }
      }
    }
  }

  return load_data;
}

void ProjectSerializer211228::LoadNode(Node *node, XMLNodeData &xml_node_data, QXmlStreamReader *reader) const
{
  while (XMLReadNextStartElement(reader)) {
    if (IsCancelled()) {
      return;
    }

    if (reader->name() == QStringLiteral("input")) {
      LoadInput(node, reader, xml_node_data);
    } else if (reader->name() == QStringLiteral("ptr")) {
      quintptr ptr = reader->readElementText().toULongLong();
      xml_node_data.node_ptrs.insert(ptr, node);
    } else if (reader->name() == QStringLiteral("label")) {
      node->SetLabel(reader->readElementText());
    } else if (reader->name() == QStringLiteral("uuid")) {
      xml_node_data.node_uuids.insert(node, QUuid::fromString(reader->readElementText()));
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

          while (XMLReadNextStartElement(reader)) {
            if (reader->name() == QStringLiteral("output")) {
              output_node_id = reader->readElementText();
            } else {
              reader->skipCurrentElement();
            }
          }

          xml_node_data.desired_connections.append({NodeInput(node, param_id, ele), output_node_id.toULongLong()});
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

  node->LoadFinishedEvent();
}

void ProjectSerializer211228::LoadColorManager(QXmlStreamReader *reader, Project *project) const
{
  while (XMLReadNextStartElement(reader)) {
    if (reader->name() == QStringLiteral("input")) {
      QString id;
      XMLAttributeLoop(reader, attr) {
        if (attr.name() == QStringLiteral("id")) {
          id = attr.value().toString();
        }
      }

      if (id == QStringLiteral("config") || id == QStringLiteral("default_input") || id == QStringLiteral("reference_space")) {
        QString value;

        while (XMLReadNextStartElement(reader)) {
          if (reader->name() == QStringLiteral("primary")) {
            while (XMLReadNextStartElement(reader)) {
              if (reader->name() == QStringLiteral("standard")) {
                while (XMLReadNextStartElement(reader)) {
                  if (reader->name() == QStringLiteral("track")) {
                    value = reader->readElementText();
                  } else {
                    reader->skipCurrentElement();
                  }
                }
              } else {
                reader->skipCurrentElement();
              }
            }
          } else {
            reader->skipCurrentElement();
          }
        }

        if (id == QStringLiteral("default_input")) {
          // Default color space
          // NOTE: Stupidly, we saved these as integers which means we can't add anything to the OCIO
          //       config. So we must convert back to string here.
          static const QStringList list = {
            QStringLiteral("Linear"),
            QStringLiteral("CIE-XYZ D65"),
            QStringLiteral("Filmic Log Encoding"),
            QStringLiteral("sRGB OETF"),
            QStringLiteral("Apple DCI-P3 D65"),
            QStringLiteral("AppleP3 sRGB OETF"),
            QStringLiteral("BT.1886 EOTF"),
            QStringLiteral("AppleP3 Filmic Log Encoding"),
            QStringLiteral("BT.1886 Filmic Log Encoding"),
            QStringLiteral("Fuji F-Log OETF"),
            QStringLiteral("Fuji F-Log F-Gamut"),
            QStringLiteral("Panasonic V-Log V-Gamut"),
            QStringLiteral("Arri Wide Gamut / LogC EI 800"),
            QStringLiteral("Arri Wide Gamut / LogC EI 400"),
            QStringLiteral("Blackmagic Film Wide Gamut (Gen 5)"),
            QStringLiteral("Rec.709 OETF"),
            QStringLiteral("Non-Colour Data")
          };
          int num_value = value.toInt();
          value = list.at(num_value);
          project->SetDefaultInputColorSpace(value);
        } else if (id == QStringLiteral("reference_space")) {
          // Reference space
          if (value == QStringLiteral("1")) {
            value = OCIO::ROLE_COMPOSITING_LOG;
          } else {
            value = OCIO::ROLE_SCENE_LINEAR;
          }
          project->SetColorReferenceSpace(value);
        } else {
          // Config filename
          project->SetColorConfigFilename(value);
        }
      } else {
        reader->skipCurrentElement();
      }
    } else {
      reader->skipCurrentElement();
    }
  }
}

void ProjectSerializer211228::LoadProjectSettings(QXmlStreamReader *reader, Project *project) const
{
  while (XMLReadNextStartElement(reader)) {
    if (reader->name() == QStringLiteral("input")) {
      QString id;
      XMLAttributeLoop(reader, attr) {
        if (attr.name() == QStringLiteral("id")) {
          id = attr.value().toString();
        }
      }

      if (id == QStringLiteral("cache_setting") || id == QStringLiteral("cache_path")) {
        QString value;

        while (XMLReadNextStartElement(reader)) {
          if (reader->name() == QStringLiteral("primary")) {
            while (XMLReadNextStartElement(reader)) {
              if (reader->name() == QStringLiteral("standard")) {
                while (XMLReadNextStartElement(reader)) {
                  if (reader->name() == QStringLiteral("track")) {
                    value = reader->readElementText();
                  } else {
                    reader->skipCurrentElement();
                  }
                }
              } else {
                reader->skipCurrentElement();
              }
            }
          } else {
            reader->skipCurrentElement();
          }
        }

        if (id == QStringLiteral("cache_setting")) {
          project->SetCacheLocationSetting(static_cast<Project::CacheSetting>(value.toInt()));
        } else {
          project->SetCustomCachePath(value);
        }
      } else {
        reader->skipCurrentElement();
      }
    } else {
      reader->skipCurrentElement();
    }
  }
}

void ProjectSerializer211228::LoadInput(Node *node, QXmlStreamReader *reader, XMLNodeData &xml_node_data) const
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

void ProjectSerializer211228::LoadImmediate(QXmlStreamReader *reader, Node *node, const QString& input, int element, XMLNodeData &xml_node_data) const
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
            AudioParams ap = TypeSerializer::LoadAudioParams(reader);
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
                  key_time = rational::fromString(attr.value().toString().toStdString());
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

bool ProjectSerializer211228::LoadPosition(QXmlStreamReader *reader, quintptr *node_ptr, Node::Position *pos) const
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

void ProjectSerializer211228::PostConnect(const XMLNodeData &xml_node_data) const
{
  foreach (const XMLNodeData::SerializedConnection& con, xml_node_data.desired_connections) {
    if (Node *out = xml_node_data.node_ptrs.value(con.output_node)) {
      Node::ConnectEdge(out, con.input);
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

void ProjectSerializer211228::LoadNodeCustom(QXmlStreamReader *reader, Node *node, XMLNodeData &xml_node_data) const
{
  // Viewer-based nodes
  if (ViewerOutput *viewer = dynamic_cast<ViewerOutput*>(node)) {

    Footage *footage = dynamic_cast<Footage*>(node);

    while (XMLReadNextStartElement(reader)) {
      if (reader->name() == QStringLiteral("points")) {
        LoadTimelinePoints(reader, viewer);
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

void ProjectSerializer211228::LoadTimelinePoints(QXmlStreamReader *reader, ViewerOutput *points) const
{
  while (XMLReadNextStartElement(reader)) {
    if (reader->name() == QStringLiteral("markers")) {
      LoadMarkerList(reader, points->GetMarkers());
    } else if (reader->name() == QStringLiteral("workarea")) {
      LoadWorkArea(reader, points->GetWorkArea());
    } else {
      reader->skipCurrentElement();
    }
  }
}

void ProjectSerializer211228::LoadWorkArea(QXmlStreamReader *reader, TimelineWorkArea *workarea) const
{
  rational range_in = workarea->in();
  rational range_out = workarea->out();

  XMLAttributeLoop(reader, attr) {
    if (attr.name() == QStringLiteral("enabled")) {
      workarea->set_enabled(attr.value() != QStringLiteral("0"));
    } else if (attr.name() == QStringLiteral("in")) {
      range_in = rational::fromString(attr.value().toString().toStdString());
    } else if (attr.name() == QStringLiteral("out")) {
      range_out = rational::fromString(attr.value().toString().toStdString());
    }
  }

  TimeRange loaded_workarea(range_in, range_out);

  if (loaded_workarea != workarea->range()) {
    workarea->set_range(loaded_workarea);
  }

  reader->skipCurrentElement();
}

void ProjectSerializer211228::LoadMarkerList(QXmlStreamReader *reader, TimelineMarkerList *markers) const
{
  while (XMLReadNextStartElement(reader)) {
    if (reader->name() == QStringLiteral("marker")) {
      QString name;
      rational in, out;

      XMLAttributeLoop(reader, attr) {
        if (attr.name() == QStringLiteral("name")) {
          name = attr.value().toString();
        } else if (attr.name() == QStringLiteral("in")) {
          in = rational::fromString(attr.value().toString().toStdString());
        } else if (attr.name() == QStringLiteral("out")) {
          out = rational::fromString(attr.value().toString().toStdString());
        }
      }

      new TimelineMarker(OLIVE_CONFIG("MarkerColor").toInt(), TimeRange(in, out), name, markers);
    }

    reader->skipCurrentElement();
  }
}

void ProjectSerializer211228::LoadValueHint(Node::ValueHint *hint, QXmlStreamReader *reader) const
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
