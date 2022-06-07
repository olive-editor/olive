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

#include <QStringRef>
#include <QRegularExpression>
#include <QMap>
#include <limits>
#include <QVector2D>

#include "render/color.h"


namespace olive {

namespace {

// This kind of comments are parsed for input parameters
const char OLIVE_MARKED_COMMENT[] = "//OVE";
// definition of a uniform variable
const char UNIFORM_TAG[] = "uniform";

// per Shader metadata

// title of the shader. This will become part of node name
QRegularExpression SHADER_NAME_REGEX("^\\s*//OVE\\s*shader_name:\\s*(?<name>.*)\\s*$");
// description. So far not used by application
QRegularExpression SHADER_DESCRIPTION_REGEX("^\\s*//OVE\\s*shader_description:\\s*(?<description>.*)\\s*$");
// Version string. So far not used by application
QRegularExpression SHADER_VERSION_REGEX("^\\s*//OVE\\s*shader_version:\\s*(?<version>.*)\\s*$");

// per Input metadata

// human name of the input
QRegularExpression INPUT_NAME_REGEX("^\\s*//OVE\\s*name:\\s*(?<name>.*)\\s*$");
// name of uniform variable
QRegularExpression INPUT_UNIFORM_REGEX("^\\s*uniform\\s+(.*)\\s+(?<name>[A-Za-z0-9_]+)\\s*;");
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

const QMap<NodeValue::Type, QVariant> MAXIMUM_TABLE{{NodeValue::kFloat, QVariant( std::numeric_limits<int>::max())},
                                                    {NodeValue::kInt, QVariant( std::numeric_limits<int>::max())}
                                                   };

// features of the input that's currently being parsed
ShaderInputsParser::InputParam currentInput;

void clearCurrentInput()
{
  currentInput.human_name.clear();
  currentInput.uniform_name.clear();
  currentInput.description.clear();
  currentInput.type_string.clear();
  currentInput.type = NodeValue::kNone;
  currentInput.flags = InputFlags(kInputFlagNormal);
  currentInput.values.clear();
  currentInput.min = QVariant();
  currentInput.max = QVariant();
  currentInput.default_value = QVariant();
  currentInput.is_effect_input = false;
}

}  // namespace


ShaderInputsParser::ShaderInputsParser( const QString & shader_code) :
  shader_code_(shader_code),
  line_number_(1)
{
  clearCurrentInput();

  // build the table that matches an OVE markup comment with its parse function
  INPUT_PARAM_PARSE_TABLE.insert( & SHADER_NAME_REGEX, & ShaderInputsParser::parseShaderName);
  INPUT_PARAM_PARSE_TABLE.insert( & SHADER_DESCRIPTION_REGEX, & ShaderInputsParser::parseShaderDescription);
  INPUT_PARAM_PARSE_TABLE.insert( & SHADER_VERSION_REGEX, & ShaderInputsParser::parseShaderVersion);

  INPUT_PARAM_PARSE_TABLE.insert( & INPUT_NAME_REGEX, & ShaderInputsParser::parseInputName);
  INPUT_PARAM_PARSE_TABLE.insert( & INPUT_UNIFORM_REGEX, & ShaderInputsParser::parseInputUniform);
  INPUT_PARAM_PARSE_TABLE.insert( & INPUT_TYPE_REGEX, & ShaderInputsParser::parseInputType);
  INPUT_PARAM_PARSE_TABLE.insert( & INPUT_FLAG_REGEX, & ShaderInputsParser::parseInputFlags);
  INPUT_PARAM_PARSE_TABLE.insert( & INPUT_VALUES_REGEX, & ShaderInputsParser::parseInputValueList);
  INPUT_PARAM_PARSE_TABLE.insert( & INPUT_MIN_REGEX, & ShaderInputsParser::parseInputMin);
  INPUT_PARAM_PARSE_TABLE.insert( & INPUT_MAX_REGEX, & ShaderInputsParser::parseInputMax);
  INPUT_PARAM_PARSE_TABLE.insert( & INPUT_DEFAULT_REGEX, & ShaderInputsParser::parseInputDefault);
  INPUT_PARAM_PARSE_TABLE.insert( & INPUT_DESCRIPTION_REGEX, & ShaderInputsParser::parseInputDescription);

  INPUT_PARAM_PARSE_TABLE.insert( & STOP_PARSE_REGEX, & ShaderInputsParser::parseInputUniform);
}

void ShaderInputsParser::Parse()
{
  int fragment_size = shader_code_.length();
  int cursor = 0;
  QStringRef line;
  InputParseState state = PARSING;

  static const QRegularExpression LINE_BREAK = QRegularExpression("[\\n\\r]+");

  input_list_.clear();
  error_list_.clear();
  line_number_ = 1;

  // parse line by line
  while (state != SHADER_COMPLETE) {

    int line_end_index = shader_code_.indexOf( LINE_BREAK, cursor);

    if (line_end_index == -1) {
      // last line of file
      line = QStringRef( & shader_code_, cursor, fragment_size - cursor);
      cursor = fragment_size;
    }
    else {
      line = QStringRef( & shader_code_, cursor, line_end_index - cursor);
      cursor = line_end_index;
    }

    line = line.trimmed();

    // check if this line must be parsed
    if ( (line.startsWith(OLIVE_MARKED_COMMENT)) ||
         (line.startsWith(UNIFORM_TAG)) ) {

      state = parseSingleLine( line);

      if (state == INPUT_COMPLETE) {
        input_list_.append( currentInput);
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


ShaderInputsParser::InputParseState ShaderInputsParser::parseSingleLine( const QStringRef & line)
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
ShaderInputsParser::parseShaderDescription(const QRegularExpressionMatch & /*match*/)
{
  // nothing to do. SO far.
  return PARSING;
}

ShaderInputsParser::InputParseState
ShaderInputsParser::parseShaderVersion(const QRegularExpressionMatch & /*match*/)
{
  // nothing to do. So far.
  return PARSING;
}

ShaderInputsParser::InputParseState
ShaderInputsParser::parseInputName(const QRegularExpressionMatch & match)
{
  currentInput.human_name = match.captured("name");
  return PARSING;
}

ShaderInputsParser::InputParseState
ShaderInputsParser::parseInputUniform(const QRegularExpressionMatch & match)
{
  ShaderInputsParser::InputParseState state = PARSING;

  // not all uniforms are exposed as inputs
  if (currentInput.type != NodeValue::kNone) {
    currentInput.uniform_name = match.captured("name");
    state = INPUT_COMPLETE;
  }

  return state;
}

ShaderInputsParser::InputParseState
ShaderInputsParser::parseInputType(const QRegularExpressionMatch & match)
{
  currentInput.type_string = match.captured("type");
  currentInput.type = INPUT_TYPE_TABLE.value( currentInput.type_string, NodeValue::kNone);

  if (currentInput.type == NodeValue::kNone) {
    reportError(QObject::tr("type %1 is invalid").arg(currentInput.type_string));
  }

  // preset 'min' and 'max', in case they are not defined
  currentInput.min = MINIMUM_TABLE.value( currentInput.type, QVariant());
  currentInput.max = MAXIMUM_TABLE.value( currentInput.type, QVariant());

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


  QStringRef flags_line = line_match.capturedRef("flags");
  QRegularExpressionMatchIterator flag_match = FLAG_REGEX.globalMatch( flags_line);

  while (flag_match.hasNext()) {
    QRegularExpressionMatch flag = flag_match.next();
    QString flag_str = flag.captured(1);

    currentInput.flags |= InputFlags(FLAG_TABLE.value( flag_str, kInputFlagNormal));

    if (flag_str == "MAIN_INPUT") {
      // This is not a regular flag for node input. This is the texture passed
      // to output when "Enable" input is not checked.
      setAsMainInput();
    }
  }

  return PARSING;
}


void olive::ShaderInputsParser::setAsMainInput()
{
  if (currentInput.type == NodeValue::kTexture) {
    currentInput.is_effect_input = true;
  }
  else {
    reportError(QObject::tr("Flag MAIN_INPUT is applicable for type TEXTURE only; not for %1.").
                arg(currentInput.type_string));
  }
}


ShaderInputsParser::InputParseState
ShaderInputsParser::parseInputValueList(const QRegularExpressionMatch & line_match)
{
  // entries are enclosed in double quotes
  static const QRegularExpression COMBO_ENTRY_REGEX("\"([^\"]+)\"");

  QStringRef values_line = line_match.capturedRef("values");
  QRegularExpressionMatchIterator value_match = COMBO_ENTRY_REGEX.globalMatch( values_line);

  while (value_match.hasNext()) {
    QRegularExpressionMatch value = value_match.next();
    QString value_str = value.captured(1);

    currentInput.values << value_str;
  }

  return PARSING;
}

ShaderInputsParser::InputParseState
ShaderInputsParser::parseInputMin(const QRegularExpressionMatch & match)
{
  bool ok = false;
  QStringRef min_str = match.capturedRef("min");

  if (currentInput.type == NodeValue::kFloat) {
    currentInput.min = min_str.toFloat( &ok);
  }
  else if (currentInput.type == NodeValue::kInt) {
    currentInput.min = min_str.toInt( &ok);
  }
  else {
    // this type does not support "min" property
  }

  if (ok == false) {
    reportError(QObject::tr("%1 is not valid as minimum for type %2").
                arg(min_str).arg(currentInput.type_string));
  }

  return PARSING;
}

ShaderInputsParser::InputParseState
ShaderInputsParser::parseInputMax(const QRegularExpressionMatch & match)
{
  bool ok = false;
  QStringRef max_str = match.capturedRef("max");

  if (currentInput.type == NodeValue::kFloat) {
    currentInput.max = max_str.toFloat( &ok);
  }
  else if (currentInput.type == NodeValue::kInt) {
    currentInput.max = max_str.toInt( &ok);
  }
  else {
    // this type does not support "max" property
  }

  if (ok == false) {
    reportError(QObject::tr("%1 is not valid as maximum for type %2").
                arg(max_str).arg(currentInput.type_string));
  }

  return PARSING;
}

ShaderInputsParser::InputParseState
ShaderInputsParser::parseInputDefault(const QRegularExpressionMatch & match)
{
  QStringRef default_string = match.capturedRef("default").trimmed();
  bool ok = false;
  float float_value;
  int int_value;

  switch (currentInput.type)
  {
  case NodeValue::kFloat:
    float_value = default_string.toFloat( &ok);

    if (ok)
    {
      currentInput.default_value = QVariant::fromValue<float>(float_value);
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
      currentInput.default_value = QVariant::fromValue<int>(int_value);
    }
    else
    {
      reportError(QObject::tr("%1 is not a valid integer").arg(default_string));
    }
    break;

  case NodeValue::kBoolean:
    if (default_string == "false") {
      currentInput.default_value = QVariant::fromValue<bool>(false);
    }
    else if (default_string == "true") {
      currentInput.default_value = QVariant::fromValue<bool>(true);
    }
    else {
      reportError(QObject::tr("value must be 'true' or 'false'"));
    }
    break;

  case NodeValue::kColor:
    currentInput.default_value = parseColor( default_string);
    break;

  case NodeValue::kCombo:
    int_value = default_string.toInt( &ok);

    if (ok)
    {
      currentInput.default_value = QVariant::fromValue<int>(int_value);
    }
    else
    {
      reportError(QObject::tr("Default for SELECTION must be a number from 0 to number of entries minus 1."));
    }
    break;

  case NodeValue::kVec2:
    currentInput.default_value = parsePoint( default_string);
    break;

  default:
  case NodeValue::kTexture:
  case NodeValue::kNone:
    reportError(QObject::tr("type %1 does not support a default value").arg(currentInput.type_string));
    break;
  }

  return PARSING;
}

ShaderInputsParser::InputParseState
ShaderInputsParser::parseInputDescription(const QRegularExpressionMatch & match)
{
  currentInput.description.append( match.captured("description"));
  return PARSING;
}

ShaderInputsParser::InputParseState
ShaderInputsParser::stopParse(const QRegularExpressionMatch & /*match*/)
{
  return SHADER_COMPLETE;
}

// Assume 'line' has format RGBA( r,g,b,a), where 'r,g,b,a' are in range 0.0 .. 1.0
// and stand for 'red', 'green', 'blue', 'alpha'
QVariant ShaderInputsParser::parseColor(const QStringRef& line)
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

QVariant ShaderInputsParser::parsePoint(const QStringRef &line)
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

void ShaderInputsParser::reportError(const QString& error)
{
  Error new_err;
  new_err.line = line_number_;
  new_err.issue = error;

  error_list_.append( new_err);
}

}  // namespace olive
