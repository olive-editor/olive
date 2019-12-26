#include "metareader.h"

#include <QFile>

#include "config/config.h"
#include "node.h"

NodeMetaReader::NodeMetaReader(const QString &xml_meta_filename) :
  xml_filename_(xml_meta_filename),
  iterations_(1),
  iteration_input_(nullptr)
{
  QFile metadata_file(xml_filename_);

  if (metadata_file.open(QFile::ReadOnly)) {
    QXmlStreamReader reader(&metadata_file);

    while (!reader.atEnd()) {
      reader.readNext();

      if (reader.isStartElement() && reader.name() == "effect") {
        XMLReadEffect(&reader);
      }
    }

    metadata_file.close();
  } else {
    qWarning() << "Failed to load node metadata file" << xml_filename_;
  }
}

QString NodeMetaReader::Name() const
{
  return GetStringForCurrentLanguage(&names_);
}

const QString &NodeMetaReader::id() const
{
  return id_;
}

QString NodeMetaReader::Category() const
{
  return GetStringForCurrentLanguage(&categories_);
}

QString NodeMetaReader::Description() const
{
  return GetStringForCurrentLanguage(&descriptions_);
}

const QString &NodeMetaReader::filename() const
{
  return xml_filename_;
}

const QString &NodeMetaReader::frag_code() const
{
  return frag_code_;
}

const QString &NodeMetaReader::vert_code() const
{
  return vert_code_;
}

const int &NodeMetaReader::iterations() const
{
  return iterations_;
}

NodeInput *NodeMetaReader::iteration_input() const
{
  return iteration_input_;
}

const QList<NodeInput *> &NodeMetaReader::inputs() const
{
  return inputs_;
}

void NodeMetaReader::Retranslate()
{
  QMap<QString, QMap<QString, QString> >::const_iterator iterator;

  // Iterate through parameter language tables that we have
  for (iterator=param_names_.begin();iterator!=param_names_.end();iterator++) {
    NodeInput* this_input = GetInputWithID(iterator.key());
    this_input->set_name(GetStringForCurrentLanguage(&iterator.value()));
  }
}

void NodeMetaReader::XMLReadLanguageString(QXmlStreamReader* reader, QMap<QString, QString>* map)
{
  QString lang;

  // Traverse through name attributes for its language
  QXmlStreamAttributes attrs = reader->attributes();
  foreach (const QXmlStreamAttribute& attr, attrs) {
    if (attr.name() == "lang") {
      lang = attr.value().toString();

      // We don't recognize any other "name" attributes at this time
      break;
    }
  }

  // Insert name with language into map
  map->insert(lang, reader->readElementText().trimmed());
}

void NodeMetaReader::XMLReadEffect(QXmlStreamReader* reader)
{
  // Traverse through effect attributes for an ID
  QXmlStreamAttributes attrs = reader->attributes();
  foreach (const QXmlStreamAttribute& attr, attrs) {
    if (attr.name() == "id") {
      id_ = attr.value().toString();

      // We don't recognize any other "effect" attributes at this time
      break;
    }
  }

  if (id_.isEmpty()) {
    qWarning() << "Effect metadata" << xml_filename_ << "has no ID";
    return;
  }

  // Continue reading for other metadata
  while (!reader->atEnd() && !(reader->isEndElement() && reader->name() == "effect")) {
    reader->readNext();

    if (reader->isStartElement()) {
      if (reader->name() == "name") {
        // Pick up name
        XMLReadLanguageString(reader, &names_);
      } else if (reader->name() == "category") {
        // Pick up category
        XMLReadLanguageString(reader, &categories_);
      } else if (reader->name() == "description") {
        // Pick up description
        XMLReadLanguageString(reader, &descriptions_);
      } else if (reader->name() == "iterations") {
        // Pick up iterations
        XMLReadIterations(reader);
      } else if (reader->name() == "fragment") {
        // Pick up fragment shader code
        XMLReadShader(reader, frag_code_);
      } else if (reader->name() == "vertex") {
        // Pick up vertex shader code
        XMLReadShader(reader, vert_code_);
      } else if (reader->name() == "param") {
        // Pick up a parameter
        XMLReadParam(reader);
      }
    }
  }
}

void NodeMetaReader::XMLReadIterations(QXmlStreamReader* reader)
{
  int iteration_pickup = reader->readElementText().toInt();

  if (iterations_ > 0) {
    iterations_ = iteration_pickup;
  } else {
    // If the iteration value is invalid, don't set it, print an error insteead
    qWarning() << "Invalid iteration number in" << xml_filename_ << "- setting to default (1)";
  }
}

void NodeMetaReader::XMLReadParam(QXmlStreamReader *reader)
{
  QString param_id;
  NodeParam::DataType param_type = NodeParam::kAny;
  bool is_iterative = false;

  // Traverse through parameter attributes for an ID
  QXmlStreamAttributes attrs = reader->attributes();
  foreach (const QXmlStreamAttribute& attr, attrs) {
    if (attr.name() == "id") {
      param_id = attr.value().toString();
    } else if (attr.name() == "type") {
      param_type = NodeParam::StringToDataType(attr.value().toString());
    } else if (attr.name() == "iterative_input") {
      is_iterative = true;
    }
  }

  if (param_id.isEmpty()) {
    qWarning() << "Effect metadata" << xml_filename_ << "contains a parameter with no ID - parameter was not added";
    return;
  }

  QVariant default_val;
  QVariant min_val;
  bool has_min = false;
  QVariant max_val;
  bool has_max = false;

  // Traverse through param contents for more data
  while (!reader->atEnd() && !(reader->isEndElement() && reader->name() == "param")) {
    reader->readNext();

    if (reader->isStartElement()) {
      // NOTE: readElementText() returns a string, but for number types (which min and max apply to), QVariant will
      // convert them automatically
      if (reader->name() == "name") {

        // See if we've already made a table for this param before
        if (!param_names_.contains(param_id)) {
          param_names_.insert(param_id, QMap<QString, QString>());
        }

        // Insert language into table
        XMLReadLanguageString(reader, &param_names_[param_id]);

      } else if (reader->name() == "default") {
        default_val = reader->readElementText();
      } else if (reader->name() == "min") {
        has_min = true;
        min_val = reader->readElementText();
      } else if (reader->name() == "max") {
        has_max = true;
        max_val = reader->readElementText();
      }
    }
  }

  NodeInput* input = new NodeInput(param_id, param_type, default_val);

  if (has_min) {
    input->set_minimum(min_val);
  }

  if (has_max) {
    input->set_maximum(max_val);
  }

  if (is_iterative) {
    iteration_input_ = input;
  }

  inputs_.append(input);
}

void NodeMetaReader::XMLReadShader(QXmlStreamReader *reader, QString &destination)
{
  QString code_url;

  // Traverse through parameter attributes for an ID
  QXmlStreamAttributes attrs = reader->attributes();
  foreach (const QXmlStreamAttribute& attr, attrs) {
    if (attr.name() == "url") {
      code_url = attr.value().toString();

      // We don't recognize any other "shader" attributes at this time
      break;
    }
  }

  // Add code in file from URL
  if (!code_url.isEmpty()) {
    destination.append(Node::ReadFileAsString(code_url));
  }

  // Add any code that's inline in the XML
  QString element_text = reader->readElementText().trimmed();
  if (!element_text.isEmpty()) {
    destination.append(element_text);
  }
}

QString NodeMetaReader::GetStringForCurrentLanguage(const QMap<QString, QString> *language_map)
{
  if (language_map->isEmpty()) {
    // There are no entries for this map, this must be an empty string
    return QString();
  }

  // Get current language config
  QString language = Config::Current()["Language"].toString();

  // See if our map has an exact language match
  QString str_for_lang = language_map->value(language);
  if (!str_for_lang.isEmpty()) {
    return str_for_lang;
  }

  // If not, try to find a match with the same language but not the same derivation
  QString base_lang = language.split('_').first();
  QList<QString> available_languages = language_map->keys();
  foreach (const QString& l, available_languages) {
    if (l.startsWith(base_lang)) {
      // This is the same language, so we can return this
      return language_map->value(l);
    }
  }

  // We couldn't find an exact or close match, just return the first in the list
  // (assume a string in the wrong language is better than no string at all)
  return language_map->first();
}

NodeInput *NodeMetaReader::GetInputWithID(const QString &id) const
{
  foreach (NodeInput* input, inputs_) {
    if (input->id() == id) {
      return input;
    }
  }
  return nullptr;
}
