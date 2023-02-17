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

#include "serializer220403.h"

#include "config/config.h"
#include "node/factory.h"

namespace olive {

// These wrappers may appear to do nothing, but they seem to address a bug where sometimes
// QXmlStreamWriter would insert nonsense null characters into a project. This is probably some
// real edge case like compiler optimization or some bullshit like that. I don't even know if it
// affects all platforms, but it definitely affected me on Linux.
void WriteStartElement(QXmlStreamWriter *writer, const QString &s)
{
  writer->writeStartElement(s);
}

void WriteEndElement(QXmlStreamWriter *writer)
{
  writer->writeEndElement();
}

ProjectSerializer220403::LoadData ProjectSerializer220403::Load(Project *project, QXmlStreamReader *reader, void *reserved) const
{
  QMap<quintptr, QMap<QString, QString> > properties;
  QMap<quintptr, QMap<quintptr, Node::Position> > positions;
  XMLNodeData xml_node_data;

  LoadData load_data;

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
              // Disable cache while node is being loaded (we'll re-enable it later)
              node->SetCachesEnabled(false);

              LoadNode(node, xml_node_data, reader);

              node->setParent(project);
            }
          }
        } else {
          reader->skipCurrentElement();
        }
      }

    } else if (reader->name() == QStringLiteral("keyframes")) {

      while (XMLReadNextStartElement(reader)) {
        if (reader->name() == QStringLiteral("node")) {
          QString node_id;
          XMLAttributeLoop(reader, attr) {
            if (attr.name() == QStringLiteral("id")) {
              node_id = attr.value().toString();
              break;
            }
          }

          Node *n = nullptr;
          if (!node_id.isEmpty()) {
            n = NodeFactory::CreateFromID(node_id);
          }

          if (!n) {
            reader->skipCurrentElement();
          } else {
            while (XMLReadNextStartElement(reader)) {
              if (reader->name() == QStringLiteral("input")) {
                QString input_id;
                XMLAttributeLoop(reader, attr) {
                  if (attr.name() == QStringLiteral("id")) {
                    input_id = attr.value().toString();
                    break;
                  }
                }

                if (input_id.isEmpty()) {
                  reader->skipCurrentElement();
                } else {
                  while (XMLReadNextStartElement(reader)) {
                    if (reader->name() == QStringLiteral("element")) {
                      QString element_id;
                      XMLAttributeLoop(reader, attr) {
                        if (attr.name() == QStringLiteral("id")) {
                          element_id = attr.value().toString();
                          break;
                        }
                      }

                      if (element_id.isEmpty()) {
                        reader->skipCurrentElement();
                      } else {
                        while (XMLReadNextStartElement(reader)) {
                          if (reader->name() == QStringLiteral("track")) {
                            QString track_id;
                            XMLAttributeLoop(reader, attr) {
                              if (attr.name() == QStringLiteral("id")) {
                                track_id = attr.value().toString();
                                break;
                              }
                            }

                            if (track_id.isEmpty()) {
                              reader->skipCurrentElement();
                            } else {
                              while (XMLReadNextStartElement(reader)) {
                                if (reader->name() == QStringLiteral("key")) {
                                  NodeKeyframe *key = new NodeKeyframe();
                                  key->set_input(input_id);
                                  key->set_element(element_id.toInt());
                                  key->set_track(track_id.toInt());

                                  LoadKeyframe(reader, key, n->GetInputDataType(input_id));

                                  load_data.keyframes[node_id].append(key);
                                } else {
                                  reader->skipCurrentElement();
                                }
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
                }
              } else {
                reader->skipCurrentElement();
              }
            }
          }

          delete n;
        } else {
          reader->skipCurrentElement();
        }
      }

    } else if (reader->name() == QStringLiteral("markers")) {

      while (XMLReadNextStartElement(reader)) {
        if (reader->name() == QStringLiteral("marker")) {
          TimelineMarker *marker = new TimelineMarker();
          LoadMarker(reader, marker);
          load_data.markers.push_back(marker);
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

  // Resolve serialized properties (if any)
  for (auto it=properties.cbegin(); it!=properties.cend(); it++) {
    Node *node = xml_node_data.node_ptrs.value(it.key());
    if (node) {
      load_data.properties.insert(node, it.value());
    }
  }

  // Re-enable caches
  for (Node *n : project->nodes()) {
    n->SetCachesEnabled(true);
  }

  return load_data;
}

void ProjectSerializer220403::Save(QXmlStreamWriter *writer, const SaveData &data, void *reserved) const
{
  if (!data.GetOnlySerializeMarkers().empty()) {

    WriteStartElement(writer, QStringLiteral("markers"));

    for (auto it=data.GetOnlySerializeMarkers().cbegin(); it!=data.GetOnlySerializeMarkers().cend(); it++) {
      TimelineMarker *marker = *it;

      WriteStartElement(writer, QStringLiteral("marker"));

      SaveMarker(writer, marker);

      WriteEndElement(writer); // marker
    }

    WriteEndElement(writer); // markers

  } else if (!data.GetOnlySerializeKeyframes().empty()) {

    WriteStartElement(writer, QStringLiteral("keyframes"));

    // Organize keyframes into node+input
    QHash<QString, QHash<QString, QMap<int, QMap<int, QVector<NodeKeyframe*> > > > > organized;

    for (auto it=data.GetOnlySerializeKeyframes().cbegin(); it!=data.GetOnlySerializeKeyframes().cend(); it++) {
      NodeKeyframe *key = *it;
      organized[key->parent()->id()][key->input()][key->element()][key->track()].append(key);
    }

    for (auto it=organized.cbegin(); it!=organized.cend(); it++) {
      WriteStartElement(writer, QStringLiteral("node"));

      writer->writeAttribute(QStringLiteral("id"), it.key());

      for (auto jt=it.value().cbegin(); jt!=it.value().cend(); jt++) {
        WriteStartElement(writer, QStringLiteral("input"));

        writer->writeAttribute(QStringLiteral("id"), jt.key());

        for (auto kt=jt.value().cbegin(); kt!=jt.value().cend(); kt++) {
          WriteStartElement(writer, QStringLiteral("element"));

          writer->writeAttribute(QStringLiteral("id"), QString::number(kt.key()));

          for (auto lt=kt.value().cbegin(); lt!=kt.value().cend(); lt++) {
            const QVector<NodeKeyframe *> &keys = lt.value();

            WriteStartElement(writer, QStringLiteral("track"));

            writer->writeAttribute(QStringLiteral("id"), QString::number(lt.key()));

            for (NodeKeyframe *key : keys) {
              WriteStartElement(writer, QStringLiteral("key"));
              SaveKeyframe(writer, key, key->parent()->GetInputDataType(key->input()));
              WriteEndElement(writer); // key
            }

            WriteEndElement(writer); // track
          }

          WriteEndElement(writer); // element
        }

        WriteEndElement(writer); // input
      }

      WriteEndElement(writer); // node;
    }

    WriteEndElement(writer); // keyframes

  } else if (Project *project = data.GetProject()) {

    writer->writeTextElement(QStringLiteral("uuid"), project->GetUuid().toString());

    WriteStartElement(writer, QStringLiteral("nodes"));

    const QVector<Node*> &using_node_list = (data.GetOnlySerializeNodes().isEmpty()) ? project->nodes() : data.GetOnlySerializeNodes();

    foreach (Node* node, using_node_list) {
      WriteStartElement(writer, QStringLiteral("node"));

      if (node == project->root()) {
        writer->writeAttribute(QStringLiteral("root"), QStringLiteral("1"));
      } else if (node == project->color_manager()) {
        writer->writeAttribute(QStringLiteral("cm"), QStringLiteral("1"));
      } else if (node == project->settings()) {
        writer->writeAttribute(QStringLiteral("settings"), QStringLiteral("1"));
      }

      writer->writeAttribute(QStringLiteral("id"), node->id());

      SaveNode(node, writer);

      WriteEndElement(writer); // node
    }

    WriteEndElement(writer); // nodes

    WriteStartElement(writer, QStringLiteral("positions"));

    foreach (Node* context, using_node_list) {
      const Node::PositionMap &map = context->GetContextPositions();

      if (!map.isEmpty()) {
        WriteStartElement(writer, QStringLiteral("context"));

        writer->writeAttribute(QStringLiteral("ptr"), QString::number(reinterpret_cast<quintptr>(context)));

        for (auto jt=map.cbegin(); jt!=map.cend(); jt++) {
          if (data.GetOnlySerializeNodes().isEmpty() || data.GetOnlySerializeNodes().contains(jt.key())) {
            WriteStartElement(writer, QStringLiteral("node"));
            SavePosition(writer, jt.key(), jt.value());
            WriteEndElement(writer); // node
          }
        }

        WriteEndElement(writer); // context
      }
    }

    WriteEndElement(writer); // positions

    WriteStartElement(writer, QStringLiteral("properties"));

    for (auto it=data.GetProperties().cbegin(); it!=data.GetProperties().cend(); it++) {
      WriteStartElement(writer, QStringLiteral("node"));

      writer->writeAttribute(QStringLiteral("ptr"), QString::number(reinterpret_cast<quintptr>(it.key())));

      for (auto jt=it.value().cbegin(); jt!=it.value().cend(); jt++) {
        writer->writeTextElement(jt.key(), jt.value());
      }

      WriteEndElement(writer); // node
    }

    WriteEndElement(writer); // properties

    // Save main window project layout
    project->GetLayoutInfo().toXml(writer);

  } else {

    qCritical() << "Cannot save nodes without project object";

  }
}

void ProjectSerializer220403::LoadNode(Node *node, XMLNodeData &xml_node_data, QXmlStreamReader *reader) const
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
    } else if (reader->name() == QStringLiteral("caches")) {
      while (XMLReadNextStartElement(reader)) {
        if (reader->name() == QStringLiteral("audio")) {
          node->audio_playback_cache()->SetUuid(QUuid::fromString(reader->readElementText()));
        } else if (reader->name() == QStringLiteral("video")) {
          node->video_frame_cache()->SetUuid(QUuid::fromString(reader->readElementText()));
        } else if (reader->name() == QStringLiteral("thumb")) {
          node->thumbnail_cache()->SetUuid(QUuid::fromString(reader->readElementText()));
        } else if (reader->name() == QStringLiteral("waveform")) {
          node->waveform_cache()->SetUuid(QUuid::fromString(reader->readElementText()));
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

void ProjectSerializer220403::SaveNode(Node *node, QXmlStreamWriter *writer) const
{
  writer->writeTextElement(QStringLiteral("ptr"), QString::number(reinterpret_cast<quintptr>(node)));

  writer->writeTextElement(QStringLiteral("label"), node->GetLabel());
  writer->writeTextElement(QStringLiteral("color"), QString::number(node->GetOverrideColor()));

  foreach (const QString& input, node->inputs()) {
    WriteStartElement(writer, QStringLiteral("input"));

    SaveInput(node, writer, input);

    WriteEndElement(writer); // input
  }

  WriteStartElement(writer, QStringLiteral("links"));
  foreach (Node* link, node->links()) {
    writer->writeTextElement(QStringLiteral("link"), QString::number(reinterpret_cast<quintptr>(link)));
  }
  WriteEndElement(writer); // links

  WriteStartElement(writer, QStringLiteral("connections"));
  for (auto it=node->input_connections().cbegin(); it!=node->input_connections().cend(); it++) {
    WriteStartElement(writer, QStringLiteral("connection"));

    writer->writeAttribute(QStringLiteral("input"), it->first.input());
    writer->writeAttribute(QStringLiteral("element"), QString::number(it->first.element()));

    writer->writeTextElement(QStringLiteral("output"), QString::number(reinterpret_cast<quintptr>(it->second)));

    WriteEndElement(writer); // connection
  }
  WriteEndElement(writer); // connections

  WriteStartElement(writer, QStringLiteral("hints"));
  for (auto it=node->GetValueHints().cbegin(); it!=node->GetValueHints().cend(); it++) {
    WriteStartElement(writer, QStringLiteral("hint"));

    writer->writeAttribute(QStringLiteral("input"), it.key().input);
    writer->writeAttribute(QStringLiteral("element"), QString::number(it.key().element));

    SaveValueHint(&it.value(), writer);

    WriteEndElement(writer); // hint
  }
  WriteEndElement(writer); // hints

  WriteStartElement(writer, QStringLiteral("caches"));

  writer->writeTextElement(QStringLiteral("audio"), node->audio_playback_cache()->GetUuid().toString());
  writer->writeTextElement(QStringLiteral("video"), node->video_frame_cache()->GetUuid().toString());
  writer->writeTextElement(QStringLiteral("thumb"), node->thumbnail_cache()->GetUuid().toString());
  writer->writeTextElement(QStringLiteral("waveform"), node->waveform_cache()->GetUuid().toString());

  WriteEndElement(writer); // caches

  WriteStartElement(writer, QStringLiteral("custom"));

  SaveNodeCustom(writer, node);

  WriteEndElement(writer); // custom
}

void ProjectSerializer220403::LoadInput(Node *node, QXmlStreamReader *reader, XMLNodeData &xml_node_data) const
{
  if (dynamic_cast<NodeGroup*>(node)) {
    // Ignore input of group
    reader->skipCurrentElement();
    return;
  }

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

void ProjectSerializer220403::SaveInput(Node *node, QXmlStreamWriter *writer, const QString &id) const
{
  writer->writeAttribute(QStringLiteral("id"), id);

  WriteStartElement(writer, QStringLiteral("primary"));

  SaveImmediate(writer, node, id, -1);

  WriteEndElement(writer); // primary

  WriteStartElement(writer, QStringLiteral("subelements"));

  int arr_sz = node->InputArraySize(id);

  writer->writeAttribute(QStringLiteral("count"), QString::number(arr_sz));

  for (int i=0; i<arr_sz; i++) {
    WriteStartElement(writer, QStringLiteral("element"));

    SaveImmediate(writer, node, id, i);

    WriteEndElement(writer); // element
  }

  WriteEndElement(writer); // subelements
}

void ProjectSerializer220403::LoadImmediate(QXmlStreamReader *reader, Node *node, const QString& input, int element, XMLNodeData &xml_node_data) const
{
  Q_UNUSED(xml_node_data)

  NodeValue::Type data_type = node->GetInputDataType(input);

  // HACK: SubtitleParams contain the actual subtitle data, so loading/replacing it will overwrite
  //       the valid subtitles. We hack around it by simply skipping loading subtitles, we'll see
  //       if this ends up being an issue in the future.
  if (data_type == NodeValue::kSubtitleParams) {
    reader->skipCurrentElement();
    return;
  }

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
              NodeKeyframe* key = new NodeKeyframe();
              key->set_input(input);
              key->set_element(element);
              key->set_track(track);

              LoadKeyframe(reader, key, data_type);
              key->setParent(node);
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

void ProjectSerializer220403::SaveImmediate(QXmlStreamWriter *writer, Node *node, const QString& input, int element) const
{
  if (node->IsInputKeyframable(input)) {
    writer->writeTextElement(QStringLiteral("keyframing"), QString::number(node->IsInputKeyframing(input, element)));
  }

  NodeValue::Type data_type = node->GetInputDataType(input);

  // Write standard value
  WriteStartElement(writer, QStringLiteral("standard"));

  foreach (const QVariant& v, node->GetSplitStandardValue(input, element)) {
    WriteStartElement(writer, QStringLiteral("track"));

    if (data_type == NodeValue::kVideoParams) {
      v.value<VideoParams>().Save(writer);
    } else if (data_type == NodeValue::kAudioParams) {
      TypeSerializer::SaveAudioParams(writer, v.value<AudioParams>());
    } else {
      writer->writeCharacters(NodeValue::ValueToString(data_type, v, true));
    }

    WriteEndElement(writer); // track
  }

  WriteEndElement(writer); // standard

  // Write keyframes
  WriteStartElement(writer, QStringLiteral("keyframes"));

  for (const NodeKeyframeTrack& track : node->GetKeyframeTracks(input, element)) {
    WriteStartElement(writer, QStringLiteral("track"));

    for (NodeKeyframe* key : track) {
      WriteStartElement(writer, QStringLiteral("key"));

      SaveKeyframe(writer, key, data_type);

      WriteEndElement(writer); // key
    }

    WriteEndElement(writer); // track
  }

  WriteEndElement(writer); // keyframes

  if (data_type == NodeValue::kColor) {
    // Save color management information
    writer->writeTextElement(QStringLiteral("csinput"), node->GetInputProperty(input, QStringLiteral("col_input")).toString());
    writer->writeTextElement(QStringLiteral("csdisplay"), node->GetInputProperty(input, QStringLiteral("col_display")).toString());
    writer->writeTextElement(QStringLiteral("csview"), node->GetInputProperty(input, QStringLiteral("col_view")).toString());
    writer->writeTextElement(QStringLiteral("cslook"), node->GetInputProperty(input, QStringLiteral("col_look")).toString());
  }
}

void ProjectSerializer220403::LoadKeyframe(QXmlStreamReader *reader, NodeKeyframe *key, NodeValue::Type data_type) const
{
  QString key_input;
  QPointF key_in_handle;
  QPointF key_out_handle;

  XMLAttributeLoop(reader, attr) {
    if (IsCancelled()) {
      return;
    }

    if (attr.name() == QStringLiteral("input")) {
      key_input = attr.value().toString();
    } else if (attr.name() == QStringLiteral("time")) {
      key->set_time(rational::fromString(attr.value().toString().toStdString()));
    } else if (attr.name() == QStringLiteral("type")) {
      key->set_type_no_bezier_adj(static_cast<NodeKeyframe::Type>(attr.value().toInt()));
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

  key->set_value(NodeValue::StringToValue(data_type, reader->readElementText(), true));

  key->set_bezier_control_in(key_in_handle);
  key->set_bezier_control_out(key_out_handle);
}

void ProjectSerializer220403::SaveKeyframe(QXmlStreamWriter *writer, NodeKeyframe *key, NodeValue::Type data_type) const
{
  writer->writeAttribute(QStringLiteral("input"), key->input());
  writer->writeAttribute(QStringLiteral("time"), QString::fromStdString(key->time().toString()));
  writer->writeAttribute(QStringLiteral("type"), QString::number(key->type()));
  writer->writeAttribute(QStringLiteral("inhandlex"), QString::number(key->bezier_control_in().x()));
  writer->writeAttribute(QStringLiteral("inhandley"), QString::number(key->bezier_control_in().y()));
  writer->writeAttribute(QStringLiteral("outhandlex"), QString::number(key->bezier_control_out().x()));
  writer->writeAttribute(QStringLiteral("outhandley"), QString::number(key->bezier_control_out().y()));

  writer->writeCharacters(NodeValue::ValueToString(data_type, key->value(), true));
}

bool ProjectSerializer220403::LoadPosition(QXmlStreamReader *reader, quintptr *node_ptr, Node::Position *pos) const
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

void ProjectSerializer220403::SavePosition(QXmlStreamWriter *writer, Node *node, const Node::Position &pos) const
{
  writer->writeAttribute(QStringLiteral("ptr"), QString::number(reinterpret_cast<quintptr>(node)));

  writer->writeTextElement(QStringLiteral("x"), QString::number(pos.position.x()));
  writer->writeTextElement(QStringLiteral("y"), QString::number(pos.position.y()));
  writer->writeTextElement(QStringLiteral("expanded"), QString::number(pos.expanded));
}

void ProjectSerializer220403::PostConnect(const XMLNodeData &xml_node_data) const
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

      l.group->AddInputPassthrough(resolved, l.passthrough_id);

      l.group->SetInputFlags(l.passthrough_id, resolved.GetFlags() | l.custom_flags);

      if (!l.custom_name.isEmpty()) {
        l.group->SetInputName(l.passthrough_id, l.custom_name);
      }

      l.group->SetInputDataType(l.passthrough_id, l.data_type);

      l.group->SetDefaultValue(l.passthrough_id, l.default_val);

      for (auto it=l.custom_properties.cbegin(); it!=l.custom_properties.cend(); it++) {
        l.group->SetInputProperty(l.passthrough_id, it.key(), it.value());
      }
    }
  }

  for (auto it=xml_node_data.group_output_links.cbegin(); it!=xml_node_data.group_output_links.cend(); it++) {
    if (Node *output_node = xml_node_data.node_ptrs.value(it.value())) {
      it.key()->SetOutputPassthrough(output_node);
    }
  }
}

void ProjectSerializer220403::LoadNodeCustom(QXmlStreamReader *reader, Node *node, XMLNodeData &xml_node_data) const
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
              } else if (reader->name() == QStringLiteral("id")) {
                link.passthrough_id = reader->readElementText();
              } else if (reader->name() == QStringLiteral("name")) {
                link.custom_name = reader->readElementText();
              } else if (reader->name() == QStringLiteral("flags")) {
                link.custom_flags = InputFlags(reader->readElementText().toULongLong());
              } else if (reader->name() == QStringLiteral("type")) {
                link.data_type = NodeValue::GetDataTypeFromName(reader->readElementText());
              } else if (reader->name() == QStringLiteral("default")) {
                link.default_val = NodeValue::StringToValue(link.data_type, reader->readElementText(), false);
              } else if (reader->name() == QStringLiteral("properties")) {
                while (XMLReadNextStartElement(reader)) {
                  if (reader->name() == QStringLiteral("property")) {
                    QString key;
                    QString value;

                    while (XMLReadNextStartElement(reader)) {
                      if (reader->name() == QStringLiteral("key")) {
                        key = reader->readElementText();
                      } else if (reader->name() == QStringLiteral("value")) {
                        value = reader->readElementText();
                      } else {
                        reader->skipCurrentElement();
                      }
                    }

                    if (!key.isEmpty()) {
                      link.custom_properties.insert(key, value);
                    }
                  } else {
                    reader->skipCurrentElement();
                  }
                }
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

void ProjectSerializer220403::SaveNodeCustom(QXmlStreamWriter *writer, Node *node) const
{
  if (ViewerOutput *viewer = dynamic_cast<ViewerOutput*>(node)) {
    // Write TimelinePoints
    WriteStartElement(writer, QStringLiteral("points"));
    SaveTimelinePoints(writer, viewer);
    WriteEndElement(writer); // points

    if (Footage *footage = dynamic_cast<Footage*>(node)) {
      writer->writeTextElement(QStringLiteral("timestamp"), QString::number(footage->timestamp()));
    }
  } else if (Track *track = dynamic_cast<Track*>(node)) {
    writer->writeTextElement(QStringLiteral("height"), QString::number(track->GetTrackHeight()));
  } else if (NodeGroup *group = dynamic_cast<NodeGroup*>(node)) {
    WriteStartElement(writer, QStringLiteral("inputpassthroughs"));

    foreach (const NodeGroup::InputPassthrough &ip, group->GetInputPassthroughs()) {
      WriteStartElement(writer, QStringLiteral("inputpassthrough"));

      // Reference to inner input
      writer->writeTextElement(QStringLiteral("node"), QString::number(reinterpret_cast<quintptr>(ip.second.node())));
      writer->writeTextElement(QStringLiteral("input"), ip.second.input());
      writer->writeTextElement(QStringLiteral("element"), QString::number(ip.second.element()));

      // ID of passthrough
      writer->writeTextElement(QStringLiteral("id"), ip.first);

      // Passthrough-specific details
      NodeInput group_input(group, ip.first, ip.second.element());
      writer->writeTextElement(QStringLiteral("name"), group->Node::GetInputName(ip.first));

      writer->writeTextElement(QStringLiteral("flags"), QString::number((group_input.GetFlags() & ~ip.second.GetFlags()).value()));

      NodeValue::Type data_type = group_input.GetDataType();
      writer->writeTextElement(QStringLiteral("type"), NodeValue::GetDataTypeName(data_type));

      writer->writeTextElement(QStringLiteral("default"), NodeValue::ValueToString(data_type, group_input.GetDefaultValue(), false));

      WriteStartElement(writer, QStringLiteral("properties"));
      auto p = group_input.GetProperties();
      for (auto it=p.cbegin(); it!=p.cend(); it++) {
        WriteStartElement(writer, QStringLiteral("property"));
        writer->writeTextElement(QStringLiteral("key"), it.key());
        writer->writeTextElement(QStringLiteral("value"), it.value().toString());
        WriteEndElement(writer); // property
      }
      WriteEndElement(writer); // properties

      WriteEndElement(writer); // input
    }

    WriteEndElement(writer); // inputpassthroughs

    writer->writeTextElement(QStringLiteral("outputpassthrough"), QString::number(reinterpret_cast<quintptr>(group->GetOutputPassthrough())));
  }
}

void ProjectSerializer220403::LoadTimelinePoints(QXmlStreamReader *reader, ViewerOutput *viewer) const
{
  while (XMLReadNextStartElement(reader)) {
    if (reader->name() == QStringLiteral("markers")) {
      LoadMarkerList(reader, viewer->GetMarkers());
    } else if (reader->name() == QStringLiteral("workarea")) {
      LoadWorkArea(reader, viewer->GetWorkArea());
    } else {
      reader->skipCurrentElement();
    }
  }
}

void ProjectSerializer220403::SaveTimelinePoints(QXmlStreamWriter *writer, ViewerOutput *viewer) const
{
  WriteStartElement(writer, QStringLiteral("workarea"));
  SaveWorkArea(writer, viewer->GetWorkArea());
  WriteEndElement(writer); // workarea

  WriteStartElement(writer, QStringLiteral("markers"));
  SaveMarkerList(writer, viewer->GetMarkers());
  WriteEndElement(writer); // markers
}

void ProjectSerializer220403::LoadMarker(QXmlStreamReader *reader, TimelineMarker *marker) const
{
  rational in, out;

  XMLAttributeLoop(reader, attr) {
    if (attr.name() == QStringLiteral("name")) {
      marker->set_name(attr.value().toString());
    } else if (attr.name() == QStringLiteral("in")) {
      in = rational::fromString(attr.value().toString().toStdString());
    } else if (attr.name() == QStringLiteral("out")) {
      out = rational::fromString(attr.value().toString().toStdString());
    } else if (attr.name() == QStringLiteral("color")) {
      marker->set_color(attr.value().toInt());
    }
  }

  marker->set_time(TimeRange(in, out));

  // This element has no inner text, so just skip it
  reader->skipCurrentElement();
}

void ProjectSerializer220403::SaveMarker(QXmlStreamWriter *writer, TimelineMarker *marker) const
{
  writer->writeAttribute(QStringLiteral("name"), marker->name());
  writer->writeAttribute(QStringLiteral("in"), QString::fromStdString(marker->time().in().toString()));
  writer->writeAttribute(QStringLiteral("out"), QString::fromStdString(marker->time().out().toString()));
  writer->writeAttribute(QStringLiteral("color"), QString::number(marker->color()));
}

void ProjectSerializer220403::LoadWorkArea(QXmlStreamReader *reader, TimelineWorkArea *workarea) const
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

void ProjectSerializer220403::SaveWorkArea(QXmlStreamWriter *writer, TimelineWorkArea *workarea) const
{
  writer->writeAttribute(QStringLiteral("enabled"), QString::number(workarea->enabled()));
  writer->writeAttribute(QStringLiteral("in"), QString::fromStdString(workarea->in().toString()));
  writer->writeAttribute(QStringLiteral("out"), QString::fromStdString(workarea->out().toString()));
}

void ProjectSerializer220403::LoadMarkerList(QXmlStreamReader *reader, TimelineMarkerList *markers) const
{
  while (XMLReadNextStartElement(reader)) {
    if (reader->name() == QStringLiteral("marker")) {
      TimelineMarker *marker = new TimelineMarker(markers);
      LoadMarker(reader, marker);
    } else {
      reader->skipCurrentElement();
    }
  }
}

void ProjectSerializer220403::SaveMarkerList(QXmlStreamWriter *writer, TimelineMarkerList *markers) const
{
  for (auto it=markers->cbegin(); it!=markers->cend(); it++) {
    TimelineMarker* marker = *it;

    WriteStartElement(writer, QStringLiteral("marker"));

    SaveMarker(writer, marker);

    WriteEndElement(writer); // marker
  }
}

void ProjectSerializer220403::LoadValueHint(Node::ValueHint *hint, QXmlStreamReader *reader) const
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

void ProjectSerializer220403::SaveValueHint(const Node::ValueHint *hint, QXmlStreamWriter *writer) const
{
  WriteStartElement(writer, QStringLiteral("types"));

  for (auto it=hint->types().cbegin(); it!=hint->types().cend(); it++) {
    writer->writeTextElement(QStringLiteral("type"), QString::number(*it));
  }

  WriteEndElement(writer); // types

  writer->writeTextElement(QStringLiteral("index"), QString::number(hint->index()));

  writer->writeTextElement(QStringLiteral("tag"), hint->tag());
}

}
