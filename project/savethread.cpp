#include "savethread.h"

#include <QXmlStreamWriter>
#include <QFile>
#include <QDir>
#include <QDebug>

#include "global/global.h"
#include "global/config.h"
#include "projectmodel.h"

void RecursiveSave() {

}

void Project::save_folder(QXmlStreamWriter& stream, int type, bool set_ids_only, const QModelIndex& parent) {
  for (int i=0;i<olive::project_model.rowCount(parent);i++) {
    const QModelIndex& item = olive::project_model.index(i, 0, parent);
    Media* m = olive::project_model.getItem(item);

    if (type == m->get_type()) {
      if (m->get_type() == MEDIA_TYPE_FOLDER) {
        if (set_ids_only) {
          m->temp_id = folder_id; // saves a temporary ID for matching in the project file
          folder_id++;
        } else {
          // if we're saving folders, save the folder
          stream.writeStartElement("folder");
          stream.writeAttribute("name", m->get_name());
          stream.writeAttribute("id", QString::number(m->temp_id));
          if (!item.parent().isValid()) {
            stream.writeAttribute("parent", "0");
          } else {
            stream.writeAttribute("parent", QString::number(olive::project_model.getItem(item.parent())->temp_id));
          }
          stream.writeEndElement();
        }
        // save_folder(stream, item, type, set_ids_only);
      } else {
        int folder = m->parentItem()->temp_id;
        if (type == MEDIA_TYPE_FOOTAGE) {

        } else if (type == MEDIA_TYPE_SEQUENCE) {
          Sequence* s = m->to_sequence().get();
          if (set_ids_only) {
            s->save_id = sequence_id;
            sequence_id++;
          } else {
            s->Save(stream);
          }
        }
      }
    }

    if (m->get_type() == MEDIA_TYPE_FOLDER) {
      save_folder(stream, type, set_ids_only, item);
    }
  }
}

void olive::Save(bool autorecovery)
{
  QFile file(autorecovery ? olive::Global->get_autorecovery_filename() : olive::ActiveProjectFilename);
  if (!file.open(QIODevice::WriteOnly)) {
    qCritical() << "Could not open file";
    return;
  }

  QXmlStreamWriter stream(&file);
  stream.setAutoFormatting(true);
  stream.writeStartDocument(); // doc

  stream.writeStartElement("project"); // project

  stream.writeTextElement("version", QString::number(olive::kSaveVersion));

  stream.writeTextElement("url", olive::ActiveProjectFilename);

  // Prepare project for saving and retrieve element count
  int element_count = olive::project_model.PrepareToSave();

  // Write element count to file - used by the loading thread to determine its loading progress
  stream.writeTextElement("elements", QString::number(element_count));

  olive::project_model.Save(stream);

  stream.writeEndElement(); // project

  stream.writeEndDocument(); // doc

  file.close();

  if (!autorecovery) {
    add_recent_project(olive::ActiveProjectFilename);
    olive::Global->set_modified(false);
  }
}
