#include "nodeshader.h"

NodeShader::NodeShader(Clip* c, const EffectMeta *em) :
  Effect(c, em)
{
  if (em != nullptr) {
    // set up UI from effect file
    name = em->name;

    if (!em->filename.isEmpty() && em->internal == -1) {
      QFile effect_file(em->filename);
      if (effect_file.open(QFile::ReadOnly)) {
        QXmlStreamReader reader(&effect_file);

        while (!reader.atEnd()) {
          if (reader.name() == "field" && reader.isStartElement()) {
            int type = olive::nodes::kInvalid;
            QString id;
            QString name;

            // get field type
            const QXmlStreamAttributes& attributes = reader.attributes();
            for (int i=0;i<attributes.size();i++) {
              const QXmlStreamAttribute& attr = attributes.at(i);
              if (attr.name() == "type") {
                QString comp = attr.value().toString().toUpper();
                type = olive::nodes::StringToDataType(comp);
              } else if (attr.name() == "id") {
                id = attr.value().toString();
              } else if (attr.name() == "name") {
                name = attr.value().toString();
              }
            }

            if (id.isEmpty() || name.isEmpty() || type == olive::nodes::kInvalid) {
              qCritical() << "Couldn't load field from" << em->filename << "- ID, type, and name cannot be empty.";
            } else {
              EffectRow* field = nullptr;

              switch (type) {
              case olive::nodes::kFloat:
              {

                DoubleInput* double_field = new DoubleInput(this, id, name);

                for (int i=0;i<attributes.size();i++) {
                  const QXmlStreamAttribute& attr = attributes.at(i);
                  if (attr.name() == "default") {
                    double_field->SetDefault(attr.value().toDouble());
                  } else if (attr.name() == "min") {
                    double_field->SetMinimum(attr.value().toDouble());
                  } else if (attr.name() == "max") {
                    double_field->SetMaximum(attr.value().toDouble());
                  }
                }

                field = double_field;
              }
                break;
              case olive::nodes::kColor:
              {
                QColor color;

                field = new ColorInput(this, id, name);

                for (int i=0;i<attributes.size();i++) {
                  const QXmlStreamAttribute& attr = attributes.at(i);
                  if (attr.name() == "r") {
                    color.setRed(attr.value().toInt());
                  } else if (attr.name() == "g") {
                    color.setGreen(attr.value().toInt());
                  } else if (attr.name() == "b") {
                    color.setBlue(attr.value().toInt());
                  } else if (attr.name() == "rf") {
                    color.setRedF(attr.value().toDouble());
                  } else if (attr.name() == "gf") {
                    color.setGreenF(attr.value().toDouble());
                  } else if (attr.name() == "bf") {
                    color.setBlueF(attr.value().toDouble());
                  } else if (attr.name() == "hex") {
                    color.setNamedColor(attr.value().toString());
                  }
                }

                field->SetValueAt(0, color);
              }
                break;
              case olive::nodes::kString:
                field = new StringInput(this, id, name);
                for (int i=0;i<attributes.size();i++) {
                  const QXmlStreamAttribute& attr = attributes.at(i);
                  if (attr.name() == "default") {
                    field->SetValueAt(0, attr.value().toString());
                  }
                }
                break;
              case olive::nodes::kBoolean:
                field = new BoolInput(this, id, name);
                for (int i=0;i<attributes.size();i++) {
                  const QXmlStreamAttribute& attr = attributes.at(i);
                  if (attr.name() == "default") {
                    field->SetValueAt(0, attr.value() == "1");
                  }
                }
                break;
              case olive::nodes::kCombo:
              {
                ComboInput* combo_field = new ComboInput(this, id, name);
                int combo_default_index = 0;
                for (int i=0;i<attributes.size();i++) {
                  const QXmlStreamAttribute& attr = attributes.at(i);
                  if (attr.name() == "default") {
                    combo_default_index = attr.value().toInt();
                    break;
                  }
                }
                int combo_item_count = 0;
                while (!reader.atEnd() && !(reader.name() == "field" && reader.isEndElement())) {
                  reader.readNext();
                  if (reader.name() == "option" && reader.isStartElement()) {
                    reader.readNext();
                    combo_field->AddItem(reader.text().toString(), combo_item_count);
                    combo_item_count++;
                  }
                }
                combo_field->SetValueAt(0, combo_default_index);
                field = combo_field;
              }
                break;
              case olive::nodes::kFont:
                field = new FontInput(this, id, name);
                for (int i=0;i<attributes.size();i++) {
                  const QXmlStreamAttribute& attr = attributes.at(i);
                  if (attr.name() == "default") {
                    field->SetValueAt(0, attr.value().toString());
                  }
                }
                break;
              case olive::nodes::kFile:
                field = new FileInput(this, id, name);
                for (int i=0;i<attributes.size();i++) {
                  const QXmlStreamAttribute& attr = attributes.at(i);
                  if (attr.name() == "filename") {
                    field->SetValueAt(0, attr.value().toString());
                  }
                }
                break;
              }
            }
          } else if (reader.name() == "shader" && reader.isStartElement()) {
            SetFlags(Flags() | ShaderFlag);
            const QXmlStreamAttributes& attributes = reader.attributes();
            for (int i=0;i<attributes.size();i++) {
              const QXmlStreamAttribute& attr = attributes.at(i);
              if (attr.name() == "vert") {
                shader_vert_path_ = attr.value().toString();
              } else if (attr.name() == "frag") {
                shader_frag_path_ = attr.value().toString();
              } else if (attr.name() == "iterations") {
                setIterations(attr.value().toInt());
              } else if (attr.name() == "function") {
                shader_function_name_ = attr.value().toString();
              }
            }
          }/* else if (reader.name() == "superimpose" && reader.isStartElement()) {
            enable_superimpose = true;
            const QXmlStreamAttributes& attributes = reader.attributes();
            for (int i=0;i<attributes.size();i++) {
              const QXmlStreamAttribute& attr = attributes.at(i);
              if (attr.name() == "script") {
                QFile script_file(get_effects_dir() + "/" + attr.value().toString());
                if (script_file.open(QFile::ReadOnly)) {
                  script = script_file.readAll();
                } else {
                  qCritical() << "Failed to open superimpose script file for" << em->filename;
                  enable_superimpose = false;
                }
                break;
              }
            }
          }*/
          reader.readNext();
        }

        effect_file.close();
      } else {
        qCritical() << "Failed to open effect file" << em->filename;
      }
    }
  }
}
