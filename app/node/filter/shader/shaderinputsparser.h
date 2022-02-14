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

#ifndef SHADERINPUTSPARSER_H
#define SHADERINPUTSPARSER_H

#include <QString>
#include <QVariant>
#include <QList>

#include "node/value.h"
#include "node/param.h"

namespace olive {

/** @brief This class parses a GLSL shader code to find 'uniform' and
 * marked comments for inputs to be exposed to the GUI.
 * The output is a list of inputs with parameters and a list of parsing errors.
 */
class ShaderInputsParser
{
public:

  struct InputParam
  {
    // name that appares in GUI
    QString human_name;
    // name of uniform in code
    QString uniform_name;
    // so far, not used
    QString description;
    // string for input type
    QString type_string;

    NodeValue::Type type;
    InputFlags flags;
    // list of entries for a selection combo
    QStringList values;

    QVariant min;
    QVariant max;
    QVariant default_value;
  };

  struct Error {
     int line;
     QString issue;
  };


  // The constructor accepts the full code of the shader
  ShaderInputsParser( const QString & shader_code);

  ~ShaderInputsParser() {
    input_list_.clear();
  }

  // main function that extracts inputs from shader code
  void Parse();

  // valid after call to 'Parse'
  const QList< InputParam> & InputList() const {
    return input_list_;
  }

  // valid after call to 'Parse'
  const QString & ShaderName() const {
    return shader_name_;
  }

  // list of issue found in parsing inputs
  const QList<Error> & ErrorList() const {
     return error_list_;
  }

private:
  // value returned after parsing one line
  enum InputParseState {
     // adding parameters for the current input
     PARSING = 0,
     // a 'uniform' has been found. An input is complete
     INPUT_COMPLETE,
     // final tag found. Parsing will be stopped
     SHADER_COMPLETE
  };

  InputParseState parseSingleLine( const QStringRef & line);

  // pointer to a function that parse a line that matches an //OVE comment
  typedef InputParseState (ShaderInputsParser::* LineParseFunction)( const QRegularExpressionMatch & match);

  InputParseState parseShaderName( const QRegularExpressionMatch &);
  InputParseState parseShaderDescription( const QRegularExpressionMatch &);
  InputParseState parseShaderVersion( const QRegularExpressionMatch &);
  InputParseState parseInputName( const QRegularExpressionMatch &);
  InputParseState parseInputUniform( const QRegularExpressionMatch &);
  InputParseState parseInputType( const QRegularExpressionMatch &);
  InputParseState parseInputFlags( const QRegularExpressionMatch &);
  InputParseState parseInputValueList( const QRegularExpressionMatch &);
  InputParseState parseInputMin( const QRegularExpressionMatch &);
  InputParseState parseInputMax( const QRegularExpressionMatch &);
  InputParseState parseInputDefault( const QRegularExpressionMatch &);
  InputParseState parseInputDescription( const QRegularExpressionMatch &);
  InputParseState stopParse( const QRegularExpressionMatch &);

  QMap< const QRegularExpression *, LineParseFunction> INPUT_PARAM_PARSE_TABLE;

  QVariant parseColor( const QStringRef & line);
  void reportError( const QString & error);

private:
  const QString & shader_code_;
  QList< InputParam> input_list_;
  QList<Error> error_list_;
  int line_number_;

  QString shader_name_;
};

}

#endif // SHADERINPUTSPARSER_H
