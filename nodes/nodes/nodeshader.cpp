#include "nodeshader.h"

NodeShader::NodeShader(Clip* c,
                       const QString &name,
                       const QString &id,
                       const QString &category,
                       const QString &filename) :
  Node(c),
  name_(name),
  id_(id),
  category_(category),
  filename_(filename)
{
  if (!filename_.isEmpty()) {
    QFile effect_file(filename_);
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
            qCritical() << "Couldn't load field from" << filename_ << "- ID, type, and name cannot be empty.";
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
        } else if (reader.name() == "description" && reader.isStartElement()) {
          reader.readNext();
          description_ = reader.text().toString();
        }
        reader.readNext();
      }

      effect_file.close();
    } else {
      qCritical() << "Failed to open effect file" << filename;
    }
  }
}

QString NodeShader::name()
{
  return name_;
}

QString NodeShader::id()
{
  return id_;
}

QString NodeShader::category()
{
  return category_;
}

QString NodeShader::description()
{
  return description_;
}

EffectType NodeShader::type()
{
  return EFFECT_TYPE_EFFECT;
}

olive::TrackType NodeShader::subtype()
{
  return olive::kTypeVideo;
}

bool NodeShader::IsCreatable()
{
  return !filename_.isEmpty();
}

NodePtr NodeShader::Create(Clip *)
{
  Q_ASSERT(false);
}
