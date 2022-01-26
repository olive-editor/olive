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
QRegularExpression INPUT_DEFAULT_REGEX("^\\s*//OVE\\s*default:\\*(?<def>.*)\\s*$");
// description. So far not used by application
QRegularExpression INPUT_DESCRIPTION_REGEX("^\\s*//OVE\\s*description:\\s*(?<description>.*)\\s*$");

// when this is found, parsing is stopped
QRegularExpression STOP_PARSE_REGEX("^\\s*//OVE\\s*end\\s*$");

// lookup table for kind of input
const QMap<QString, NodeValue::Type> INPUT_TYPE_TABLE{{"TEXTURE", NodeValue::kTexture},
                                                      {"COLOR", NodeValue::kColor},
                                                      {"FLOAT", NodeValue::kFloat},
                                                      {"INTEGER", NodeValue::kInt},
                                                      {"BOOLEAN", NodeValue::kBoolean}
                                                     };


// features of the input that's currently being parsed
ShaderInputsParser::InputParam currentInput;

void clearCurrentInput()
{
  currentInput.readable_name.clear();
  currentInput.uniform_name.clear();
  currentInput.description.clear();
  currentInput.type = NodeValue::kNone;
  currentInput.flags = InputFlags(kInputFlagNormal);
  currentInput.min = QVariant();
  currentInput.max = QVariant();
  currentInput.default_value = QVariant();
}

}  // namespace


ShaderInputsParser::ShaderInputsParser( const QString & shader_code) :
  shader_code_(shader_code)
{
  clearCurrentInput();

  // build the table that matches an OVE markup comment with its parse function
  INPUT_PARAM_PARSE_TABLE.insert( & SHADER_NAME_REGEX, & ShaderInputsParser::parseShaderName);
  INPUT_PARAM_PARSE_TABLE.insert( & SHADER_DESCRIPTION_REGEX, & ShaderInputsParser::parseShaderDescription);
  INPUT_PARAM_PARSE_TABLE.insert( & SHADER_VERSION_REGEX, & ShaderInputsParser::parseShaderVerion);

  INPUT_PARAM_PARSE_TABLE.insert( & INPUT_NAME_REGEX, & ShaderInputsParser::parseInputName);
  INPUT_PARAM_PARSE_TABLE.insert( & INPUT_UNIFORM_REGEX, & ShaderInputsParser::parseInputUniform);
  INPUT_PARAM_PARSE_TABLE.insert( & INPUT_TYPE_REGEX, & ShaderInputsParser::parseInputType);
  INPUT_PARAM_PARSE_TABLE.insert( & INPUT_FLAG_REGEX, & ShaderInputsParser::parseInputFlags);
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

  QRegularExpression LINE_BREAK = QRegularExpression("[\\n\\r]+");

  input_list_.clear();

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

  // iterate over all regular expressions up to the first match.
  // return as soon as the first match is found
  for (it = INPUT_PARAM_PARSE_TABLE.begin(); it != INPUT_PARAM_PARSE_TABLE.end(); ++it) {

    match = it.key()->match( line);

    if (match.hasMatch()) {
      LineParseFunction parseFunction = it.value();
      new_state = (this->*parseFunction)( match);

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
ShaderInputsParser::parseShaderVerion(const QRegularExpressionMatch & /*match*/)
{
  // nothing to do. SO far.
  return PARSING;
}

ShaderInputsParser::InputParseState
ShaderInputsParser::parseInputName(const QRegularExpressionMatch & match)
{
  currentInput.readable_name = match.captured("name");
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
  currentInput.type = INPUT_TYPE_TABLE.value( match.captured("type"), NodeValue::kNone);

  if (currentInput.type == NodeValue::kNone) {
    // TODO_ report error
    qDebug() << "invalid type: " << match.captured("type");
  }

  return PARSING;
}

ShaderInputsParser::InputParseState
ShaderInputsParser::parseInputFlags(const QRegularExpressionMatch & /*match*/)
{
  // TODO_
  return PARSING;
}

ShaderInputsParser::InputParseState
ShaderInputsParser::parseInputMin(const QRegularExpressionMatch & match)
{
  bool ok = true;

  if (currentInput.type == NodeValue::kFloat) {
    currentInput.min = match.capturedRef("min").toFloat( &ok);
  }
  else {
    currentInput.min = match.capturedRef("min").toInt( &ok);
  }

  if (ok == false) {
    // TODO_ report error
  }

  return PARSING;
}

ShaderInputsParser::InputParseState
ShaderInputsParser::parseInputMax(const QRegularExpressionMatch & match)
{
  bool ok = true;

  if (currentInput.type == NodeValue::kFloat) {
    currentInput.max = match.capturedRef("max").toFloat( &ok);
  }
  else {
    currentInput.max = match.capturedRef("max").toInt( &ok);
  }

  if (ok == false) {
    // TODO_ report error
  }

  return PARSING;
}

ShaderInputsParser::InputParseState
ShaderInputsParser::parseInputDefault(const QRegularExpressionMatch & /*match*/)
{
  // TODO_
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

}  // namespace olive
