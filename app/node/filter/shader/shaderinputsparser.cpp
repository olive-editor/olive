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

#include "shaderinputsparser.h"

#include <QString>
#include <QRegularExpression>
#include <QMap>
#include <limits>
#include <QVector2D>



namespace olive {

namespace {

// This kind of comments are parsed for input parameters
const QString OLIVE_MARKED_COMMENT = QString::fromLatin1("//OVE");
// definition of a uniform variable
const QString UNIFORM_TAG = QString::fromLatin1("uniform");

// per Shader metadata

// title of the shader. This will become part of node name
QRegularExpression SHADER_NAME_REGEX("^\\s*//OVE\\s*shader_name:\\s*(?<name>.*)\\s*$");
// description. So far not used by application
QRegularExpression SHADER_DESCRIPTION_REGEX("^\\s*//OVE\\s*shader_description:\\s*(?<description>.*)\\s*$");
// Version string. So far not used by application
QRegularExpression SHADER_VERSION_REGEX("^\\s*//OVE\\s*shader_version:\\s*(?<version>.*)\\s*$");
// human name for default input
QRegularExpression SHADER_MAIN_INPUT_NAME_REGEX("^\\s*//OVE\\s*main_input_name:\\s*(?<name>.*)\\s*$");

// number of times the fragment shader is invoked
QRegularExpression NUM_OF_ITERATIONS_REGEX("^\\s*//OVE\\s*number_of_iterations:\\s*(?<num>.*)\\s*$");

// per Input metadata

// human name of the input
QRegularExpression INPUT_NAME_REGEX("^\\s*//OVE\\s*name:\\s*(?<name>.*)\\s*$");
// name of uniform variable
QRegularExpression INPUT_UNIFORM_REGEX("^\\s*uniform\\s+(?<type>[A-Za-z0-9_]+)\\s+(?<name>[A-Za-z0-9_]+)\\s*;");
// kind of input
QRegularExpression INPUT_TYPE_REGEX("^\\s*//OVE\\s*type:\\s*(?<type>.*)\\s*$");
// input flags
QRegularExpression INPUT_FLAG_REGEX("^\\s*//OVE\\s*flag:\\s*(?<flags>.*)\\s*$");
// minimum value
QRegularExpression INPUT_MIN_REGEX("^\\s*//OVE\\s*min:\\s*(?<min>.*)\\s*$");
// maximum value
QRegularExpression INPUT_MAX_REGEX("^\\s*//OVE\\s*max:\\s*(?<max>.*)\\s*$");
// default value
QRegularExpression INPUT_DEFAULT_REGEX("^\\s*//OVE\\s*default:\\s*(?<default>.*)\\s*$");
// list of values for a selection combo
QRegularExpression INPUT_VALUES_REGEX("^\\s*//OVE\\s*values:\\s*(?<values>.*)\\s*$");
// shape used to draw a point
QRegularExpression INPUT_SHAPE_REGEX("^\\s*//OVE\\s*shape:\\s*(?<shape>.*)\\s*$");
// color used to draw gizmo
QRegularExpression INPUT_COLOR_REGEX("^\\s*//OVE\\s*color:\\s*(?<color>.*)\\s*$");
// description. So far not used by application
QRegularExpression INPUT_DESCRIPTION_REGEX("^\\s*//OVE\\s*description:\\s*(?<description>.*)\\s*$");

// when this is found, parsing is stopped
QRegularExpression STOP_PARSE_REGEX("^\\s*//OVE\\s*end\\s*$");

// lookup table for kind of input
const QMap<QString, NodeValue::Type> INPUT_TYPE_TABLE{{"TEXTURE", NodeValue::kTexture},
                                                      {"COLOR", NodeValue::kColor},
                                                      {"FLOAT", NodeValue::kFloat},
                                                      {"INTEGER", NodeValue::kInt},
                                                      {"BOOLEAN", NodeValue::kBoolean},
                                                      {"SELECTION", NodeValue::kCombo},
                                                      {"POINT", NodeValue::kVec2}
                                                     };

// table with default minimum and maximum values for a node type.
// This is needed to avoid that 0 is used as minimum and maximum if user does not specify one
const QMap<NodeValue::Type, QVariant> MINIMUM_TABLE{{NodeValue::kFloat, QVariant( std::numeric_limits<float>::lowest())},
                                                    {NodeValue::kInt, QVariant( std::numeric_limits<int>::lowest())}
                                                   };

const QMap<NodeValue::Type, QVariant> MAXIMUM_TABLE{{NodeValue::kFloat, QVariant( std::numeric_limits<float>::max())},
                                                    {NodeValue::kInt, QVariant( std::numeric_limits<int>::max())}
                                                   };

} // namespace


ShaderInputsParser::ShaderInputsParser( const QString & shader_code) :
  shader_code_(shader_code),
  line_number_(1),
  number_of_iterations_(1)
{
  clearCurrentInput();

  // build the table that matches an OVE markup comment with its parse function
  INPUT_PARAM_PARSE_TABLE.insert( & SHADER_NAME_REGEX, & ShaderInputsParser::parseShaderName);
  INPUT_PARAM_PARSE_TABLE.insert( & SHADER_DESCRIPTION_REGEX, & ShaderInputsParser::parseShaderDescription);
  INPUT_PARAM_PARSE_TABLE.insert( & SHADER_VERSION_REGEX, & ShaderInputsParser::parseShaderVersion);
  INPUT_PARAM_PARSE_TABLE.insert( & SHADER_MAIN_INPUT_NAME_REGEX, & ShaderInputsParser::parseMainInputName);
  INPUT_PARAM_PARSE_TABLE.insert( & NUM_OF_ITERATIONS_REGEX, & ShaderInputsParser::parseNumberOfIterations);

  INPUT_PARAM_PARSE_TABLE.insert( & INPUT_NAME_REGEX, & ShaderInputsParser::parseInputName);
  INPUT_PARAM_PARSE_TABLE.insert( & INPUT_UNIFORM_REGEX, & ShaderInputsParser::parseInputUniform);
  INPUT_PARAM_PARSE_TABLE.insert( & INPUT_TYPE_REGEX, & ShaderInputsParser::parseInputType);
  INPUT_PARAM_PARSE_TABLE.insert( & INPUT_FLAG_REGEX, & ShaderInputsParser::parseInputFlags);
  INPUT_PARAM_PARSE_TABLE.insert( & INPUT_VALUES_REGEX, & ShaderInputsParser::parseInputValueList);
  INPUT_PARAM_PARSE_TABLE.insert( & INPUT_SHAPE_REGEX, & ShaderInputsParser::parseInputPointShape);
  INPUT_PARAM_PARSE_TABLE.insert( & INPUT_COLOR_REGEX, & ShaderInputsParser::parseInputGizmoColor);
  INPUT_PARAM_PARSE_TABLE.insert( & INPUT_MIN_REGEX, & ShaderInputsParser::parseInputMin);
  INPUT_PARAM_PARSE_TABLE.insert( & INPUT_MAX_REGEX, & ShaderInputsParser::parseInputMax);
  INPUT_PARAM_PARSE_TABLE.insert( & INPUT_DEFAULT_REGEX, & ShaderInputsParser::parseInputDefault);
  INPUT_PARAM_PARSE_TABLE.insert( & INPUT_DESCRIPTION_REGEX, & ShaderInputsParser::parseInputDescription);

  INPUT_PARAM_PARSE_TABLE.insert( & STOP_PARSE_REGEX, & ShaderInputsParser::stopParse);
}

void ShaderInputsParser::Parse()
{
  int fragment_size = shader_code_.length();
  int cursor = 0;
  QString line;
  InputParseState state = PARSING;

  static const QRegularExpression LINE_BREAK = QRegularExpression("[\\n\\r]+");

  input_list_.clear();
  error_list_.clear();
  line_number_ = 1;

  shader_name_ = QString();
  shader_description_ = QString();
  shader_version_ = QString();
  number_of_iterations_ = 1;

  // parse line by line
  while (state != SHADER_COMPLETE) {

    int line_end_index = shader_code_.indexOf( LINE_BREAK, cursor);

    if (line_end_index == -1) {
      // last line of file
      line = QString( shader_code_).mid( cursor, fragment_size - cursor);
      cursor = fragment_size;
    }
    else {
      line = QString( shader_code_).mid( cursor, line_end_index - cursor);
      cursor = line_end_index;
    }

    line = line.trimmed();

    // check if this line must be parsed
    if ( (line.startsWith(QString(OLIVE_MARKED_COMMENT))) ||
         (line.startsWith(QString(UNIFORM_TAG))) ) {

      state = parseSingleLine( line);

      if (state == INPUT_COMPLETE) {
        input_list_.append( currentInput_);
        clearCurrentInput();
      }
    }

    if (state != SHADER_COMPLETE) {

      // go to the begin of next line.
      // Different platforms have different end of line.
      while ((cursor < fragment_size) &&
             (shader_code_.at(cursor).isSpace()) ) {

        if (shader_code_.at(cursor) == '\n') {
          line_number_++;
        }
        ++cursor;
      }

      // check for end of fragment
      if (cursor >= fragment_size) {
        state = SHADER_COMPLETE;
      }
    }
  }
}


void ShaderInputsParser::clearCurrentInput()
{
  currentInput_.human_name.clear();
  currentInput_.uniform_name.clear();
  currentInput_.description.clear();
  currentInput_.type_string.clear();
  currentInput_.type = NodeValue::kNone;
  currentInput_.flags = InputFlags(kInputFlagNormal);
  currentInput_.values.clear();
  currentInput_.pointShape = PointGizmo::kSquare;
  currentInput_.gizmoColor = Qt::white;
  currentInput_.min = QVariant();
  currentInput_.max = QVariant();
  currentInput_.default_value = QVariant();
}


ShaderInputsParser::InputParseState ShaderInputsParser::parseSingleLine( const QString & line)
{
  QRegularExpressionMatch match;
  InputParseState new_state = PARSING;

  QMap<const QRegularExpression *, LineParseFunction>::iterator it;

  // iterate over all regular expressions for lines
  for (it = INPUT_PARAM_PARSE_TABLE.begin(); it != INPUT_PARAM_PARSE_TABLE.end(); ++it) {

    match = it.key()->match( line);

    if (match.hasMatch()) {
      LineParseFunction parseFunction = it.value();
      new_state = (this->*parseFunction)( match);

      // stop at the first match
      break;
    }
  }

  return new_state;
}

ShaderInputsParser::InputParseState
ShaderInputsParser::parseShaderName(const QRegularExpressionMatch & match)
{
  shader_name_ = match.captured("name");
  return PARSING;
}

ShaderInputsParser::InputParseState
ShaderInputsParser::parseShaderDescription(const QRegularExpressionMatch & match)
{
  if (shader_description_.size() > 0) {
    shader_description_.append(QStringLiteral("\n"));
  }
  shader_description_ += match.captured("description");
  return PARSING;
}

ShaderInputsParser::InputParseState
ShaderInputsParser::parseShaderVersion(const QRegularExpressionMatch & match)
{
  shader_version_ = match.captured("version");
  return PARSING;
}

ShaderInputsParser::InputParseState
ShaderInputsParser::parseMainInputName(const QRegularExpressionMatch & match)
{
  main_input_name_ = match.captured("name");
  return PARSING;
}

ShaderInputsParser::InputParseState ShaderInputsParser::parseNumberOfIterations(const QRegularExpressionMatch & match)
{
  bool ok = false;
  int num_iter = match.captured("num").toInt( & ok);

  if (ok && (num_iter >= 1)) {
    number_of_iterations_ = num_iter;
  } else {
    number_of_iterations_ = 1;
    reportError(QObject::tr("Number of iterations must be a number greater or equal to 1"));
  }

  return PARSING;
}

ShaderInputsParser::InputParseState
ShaderInputsParser::parseInputName(const QRegularExpressionMatch & match)
{
  currentInput_.human_name = match.captured("name");
  return PARSING;
}

ShaderInputsParser::InputParseState
ShaderInputsParser::parseInputUniform(const QRegularExpressionMatch & match)
{
  ShaderInputsParser::InputParseState state = PARSING;

  // not all uniforms are exposed as inputs
  if (currentInput_.type != NodeValue::kNone) {
    currentInput_.uniform_name = match.captured("name");
    checkConsistentType( currentInput_.type, match.captured("type"));
    state = INPUT_COMPLETE;
  }

  return state;
}

ShaderInputsParser::InputParseState
ShaderInputsParser::parseInputType(const QRegularExpressionMatch & match)
{
  currentInput_.type_string = match.captured("type");
  currentInput_.type = INPUT_TYPE_TABLE.value( currentInput_.type_string, NodeValue::kNone);

  if (currentInput_.type == NodeValue::kNone) {
    reportError(QObject::tr("type %1 is invalid").arg(currentInput_.type_string));
  }

  // preset 'min' and 'max', in case they are not defined
  currentInput_.min = MINIMUM_TABLE.value( currentInput_.type, QVariant());
  currentInput_.max = MAXIMUM_TABLE.value( currentInput_.type, QVariant());

  return PARSING;
}


ShaderInputsParser::InputParseState
ShaderInputsParser::parseInputFlags(const QRegularExpressionMatch & line_match)
{
  // more flags can be present in one line
  static const QRegularExpression FLAG_REGEX("(ARRAY|NOT_CONNECTABLE|NOT_KEYFRAMABLE|HIDDEN|MAIN_INPUT)");

  static const QMap<QString, InputFlag> FLAG_TABLE{{"ARRAY", kInputFlagArray},
                                                   {"NOT_CONNECTABLE", kInputFlagNotConnectable},
                                                   {"NOT_KEYFRAMABLE", kInputFlagNotKeyframable},
                                                   {"HIDDEN", kInputFlagHidden}
                                                  };


  QString flags_line = line_match.captured("flags");
  QRegularExpressionMatchIterator flag_match = FLAG_REGEX.globalMatch( flags_line);

  while (flag_match.hasNext()) {
    QRegularExpressionMatch flag = flag_match.next();
    QString flag_str = flag.captured(1);

    currentInput_.flags |= InputFlags(FLAG_TABLE.value( flag_str, kInputFlagNormal));
  }

  return PARSING;
}



ShaderInputsParser::InputParseState
ShaderInputsParser::parseInputValueList(const QRegularExpressionMatch & line_match)
{
  // entries are enclosed in double quotes
  static const QRegularExpression COMBO_ENTRY_REGEX("\"([^\"]+)\"");

  QString values_line = line_match.captured("values");
  QRegularExpressionMatchIterator value_match = COMBO_ENTRY_REGEX.globalMatch( values_line);

  while (value_match.hasNext()) {
    QRegularExpressionMatch value = value_match.next();
    QString value_str = value.captured(1);

    currentInput_.values << value_str;
  }

  return PARSING;
}

ShaderInputsParser::InputParseState ShaderInputsParser::parseInputPointShape(const QRegularExpressionMatch & line_match)
{
  // lookup table for point shape
  static const QMap< QString, PointGizmo::Shape> INPUT_POINT_SHAPE_TABLE{
    {"SQUARE", PointGizmo::kSquare},
    {"CIRCLE", PointGizmo::kCircle},
    {"ANCHOR", PointGizmo::kAnchorPoint}
  };

  if (line_match.hasMatch()) {
    QString shape_str = line_match.captured("shape");

    if (INPUT_POINT_SHAPE_TABLE.contains(shape_str)) {
      currentInput_.pointShape = INPUT_POINT_SHAPE_TABLE[shape_str];
    }
    else {
      currentInput_.pointShape = PointGizmo::kSquare;
      reportError(QObject::tr("'%1' is not a valid point shape.").
                  arg(shape_str));
    }
  }

  return PARSING;
}

ShaderInputsParser::InputParseState ShaderInputsParser::parseInputGizmoColor(const QRegularExpressionMatch & line_match)
{
  if (currentInput_.type == NodeValue::kVec2) {
    QString color_string = line_match.captured("color").trimmed();
    Color ove_color = parseColor( color_string).value<Color>();
    currentInput_.gizmoColor = QColor( int(ove_color.red()*255.0),
                                       int(ove_color.green()*255.0),
                                       int(ove_color.blue()*255.0), 255);
  }
  else {
    reportError( QObject::tr("Attribute 'color' can be used for type POINT, not for %1").
                 arg(currentInput_.type_string));
  }

  return PARSING;
}

ShaderInputsParser::InputParseState
ShaderInputsParser::parseInputMin(const QRegularExpressionMatch & match)
{
  bool ok = false;
  QString min_str = match.captured("min");

  if (currentInput_.type == NodeValue::kFloat) {
    currentInput_.min = min_str.toFloat( &ok);
  }
  else if (currentInput_.type == NodeValue::kInt) {
    currentInput_.min = min_str.toInt( &ok);
  }
  else {
    // this type does not support "min" property
  }

  if (ok == false) {
    reportError(QObject::tr("%1 is not valid as minimum for type %2").
                arg(min_str, currentInput_.type_string));
  }

  return PARSING;
}

ShaderInputsParser::InputParseState
ShaderInputsParser::parseInputMax(const QRegularExpressionMatch & match)
{
  bool ok = false;
  QString max_str = match.captured("max");

  if (currentInput_.type == NodeValue::kFloat) {
    currentInput_.max = max_str.toFloat( &ok);
  }
  else if (currentInput_.type == NodeValue::kInt) {
    currentInput_.max = max_str.toInt( &ok);
  }
  else {
    // this type does not support "max" property
  }

  if (ok == false) {
    reportError(QObject::tr("%1 is not valid as maximum for type %2").
                arg(max_str, currentInput_.type_string));
  }

  return PARSING;
}

ShaderInputsParser::InputParseState
ShaderInputsParser::parseInputDefault(const QRegularExpressionMatch & match)
{
  QString default_string = match.captured("default").trimmed();
  bool ok = false;
  float float_value;
  int int_value;

  switch (currentInput_.type)
  {
  case NodeValue::kFloat:
    float_value = default_string.toFloat( &ok);

    if (ok)
    {
      currentInput_.default_value = QVariant::fromValue<float>(float_value);
    }
    else
    {
      reportError(QObject::tr("%1 is not a valid float").
                  arg(default_string));
    }
    break;

  case NodeValue::kInt:
    int_value = default_string.toInt( &ok);

    if (ok)
    {
      currentInput_.default_value = QVariant::fromValue<int>(int_value);
    }
    else
    {
      reportError(QObject::tr("%1 is not a valid integer").arg(default_string));
    }
    break;

  case NodeValue::kBoolean:
    if (default_string == QString::fromLatin1("false")) {
      currentInput_.default_value = QVariant::fromValue<bool>(false);
    }
    else if (default_string == QString::fromLatin1("true")) {
      currentInput_.default_value = QVariant::fromValue<bool>(true);
    }
    else {
      reportError(QObject::tr("value must be 'true' or 'false'"));
    }
    break;

  case NodeValue::kColor:
    currentInput_.default_value = parseColor( default_string);
    break;

  case NodeValue::kCombo:
    int_value = default_string.toInt( &ok);

    if (ok)
    {
      currentInput_.default_value = QVariant::fromValue<int>(int_value);
    }
    else
    {
      reportError(QObject::tr("Default for SELECTION must be a number from 0 to number of entries minus 1."));
    }
    break;

  case NodeValue::kVec2:
    currentInput_.default_value = parsePoint( default_string);
    break;

  default:
  case NodeValue::kTexture:
  case NodeValue::kNone:
    reportError(QObject::tr("type %1 does not support a default value").arg(currentInput_.type_string));
    break;
  }

  return PARSING;
}

ShaderInputsParser::InputParseState
ShaderInputsParser::parseInputDescription(const QRegularExpressionMatch & match)
{
  currentInput_.description.append( match.captured("description"));
  return PARSING;
}

ShaderInputsParser::InputParseState
ShaderInputsParser::stopParse(const QRegularExpressionMatch & /*match*/)
{
  return SHADER_COMPLETE;
}

// Assume 'line' has format RGBA( r,g,b,a), where 'r,g,b,a' are in range 0.0 .. 1.0
// and stand for 'red', 'green', 'blue', 'alpha'
QVariant ShaderInputsParser::parseColor(const QString& line)
{
  static const QRegularExpression
      COLOR_REGEX("RGBA\\(\\s*(?<r>[\\d\\.]+)\\s*,\\s*(?<g>[\\d\\.]+)\\s*,\\s*(?<b>[\\d\\.]+)\\s*,\\s*(?<a>[\\d\\.]+)\\s*\\)");

  QVariant color;
  QRegularExpressionMatch match = COLOR_REGEX.match(line);
  bool valid = false;

  if (match.hasMatch()) {
    bool ok_r, ok_g, ok_b, ok_a;
    float r,g,b,a;

    r = match.captured("r").toFloat( & ok_r);
    g = match.captured("g").toFloat( & ok_g);
    b = match.captured("b").toFloat( & ok_b);
    a = match.captured("a").toFloat( & ok_a);

    // check ranges
    ok_r &= ((r>=0.0f) && (r <= 1.0f));
    ok_g &= ((g>=0.0f) && (g <= 1.0f));
    ok_b &= ((b>=0.0f) && (b <= 1.0f));
    ok_a &= ((a>=0.0f) && (a <= 1.0f));

    if (ok_r && ok_g && ok_b && ok_a) {
      color = QVariant::fromValue<Color>( Color(r,g,b,a));
      valid = true;
    }
  }

  if (valid == false) {
    reportError(QObject::tr("Color must be in format 'RGBA(r,g,b,a)' "
                            "where r,g,b,a are in range (0,1)"));
  }

  return color;
}

QVariant ShaderInputsParser::parsePoint(const QString &line)
{
  static const QRegularExpression
      POINT_REGEX("\\s*\\(\\s*(?<x>[\\d\\.]+)\\s*,\\s*(?<y>[\\d\\.]+)\\s*\\)");

  QVariant point;
  QRegularExpressionMatch match = POINT_REGEX.match(line);

  if (match.hasMatch()) {
    float x,y;
    bool ok_x, ok_y;

    x = match.captured("x").toFloat( & ok_x);
    y = match.captured("y").toFloat( & ok_y);

    if (ok_x && ok_y) {
      point = QVariant::fromValue<QVector2D>({x,y});
    }
  }

  if (point == QVariant()) {
    reportError(QObject::tr("Point must be in format '(x,y)' where x and y are float values"));
  }

  return point;
}

void ShaderInputsParser::checkConsistentType(const NodeValue::Type &metadata_type, const QString &uniform_type)
{
  static const QMap<QString, QString> UNIFORM_STRING_MAP{{"TEXTURE", "sampler2D"},
                                                         {"COLOR", "vec4"},
                                                         {"FLOAT", "float"},
                                                         {"INTEGER", "int"},
                                                         {"BOOLEAN", "bool"},
                                                         {"SELECTION", "int"},
                                                         {"POINT", "vec2"}
                                                        };

  QString meta_type_string = INPUT_TYPE_TABLE.key( metadata_type, "-");
  QString uniform_type_expected = UNIFORM_STRING_MAP.value( meta_type_string, "-");

  if (uniform_type_expected != uniform_type) {
    reportError(QObject::tr("metadata type %1 requires uniform type %2 and not %3").
                arg( meta_type_string, uniform_type_expected, uniform_type));
  }
}

void ShaderInputsParser::reportError(const QString& error)
{
  Error new_err;
  new_err.line = line_number_;
  new_err.issue = error;

  error_list_.append( new_err);
}

}  // namespace olive
