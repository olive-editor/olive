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

#include "testutil.h"

#include "node/filter/shader/shaderinputsparser.h"

// shortcut for std::string
#define STR(x)   QString(x).toStdString()

//#include <QDebug>

namespace olive {

// Script header information.
OLIVE_ADD_TEST(HeaderTest)
{
  QString code(
        "#version 150""\n"
        "//OVE shader_name: slide""\n"
        "//OVE shader_description: slide transition""\n"
        "//OVE shader_version: 0.1""\n"
        "//OVE number_of_iterations: 2""\n"
        );

  ShaderInputsParser parser(code);
  parser.Parse();

  OLIVE_ASSERT_EQUAL( parser.ErrorList().size(), 0);
  OLIVE_ASSERT_EQUAL( STR(parser.ShaderName()), STR("slide"));
  OLIVE_ASSERT_EQUAL( STR(parser.ShaderDescription()), STR("slide transition"));
  OLIVE_ASSERT_EQUAL( STR(parser.ShaderVersion()), STR("0.1"));
  OLIVE_ASSERT_EQUAL( parser.NumberOfIterations(), 2);

  OLIVE_TEST_END;
}

// Script description over multiple lines
OLIVE_ADD_TEST(DescriptionTest)
{
  QString code(
        "#version 150""\n"
        "//OVE shader_name: slide""\n"
        "//OVE shader_description:   slide transition   ""\n"
        "//OVE shader_description:   that works fine.   ""\n"
        "//OVE shader_version: 0.1""\n"
        );

  ShaderInputsParser parser(code);
  parser.Parse();

  OLIVE_ASSERT_EQUAL( parser.ErrorList().size(), 0);
  OLIVE_ASSERT_EQUAL( STR(parser.ShaderName()), STR("slide"));
  OLIVE_ASSERT_EQUAL( STR(parser.ShaderDescription()), STR("slide transition\nthat works fine."));
  OLIVE_ASSERT_EQUAL( STR(parser.ShaderVersion()), STR("0.1"));

  OLIVE_TEST_END;
}


// The main input does not count in the inputs list
OLIVE_ADD_TEST(MainInputTest)
{
  QString code(
        "//OVE main_input_name: dummy""\n"
        "uniform sampler2D dummy;""\n"
        );

  ShaderInputsParser parser(code);
  parser.Parse();

  OLIVE_ASSERT_EQUAL( parser.ErrorList().size(), 0);
  OLIVE_ASSERT_EQUAL( parser.InputList().size(), 0);

  OLIVE_TEST_END;
}

// An input of kind Texture
OLIVE_ADD_TEST(InputTextureTest)
{
  QString code(
        "//OVE name: Base image""\n"
        "//OVE type: TEXTURE""\n"
        "//OVE flag: NOT_KEYFRAMABLE""\n"
        "//OVE description: the base image""\n"
        "uniform sampler2D tex_base;""\n"
        );

  ShaderInputsParser parser(code);
  parser.Parse();

  OLIVE_ASSERT_EQUAL( parser.ErrorList().size(), 0);
  OLIVE_ASSERT_EQUAL( parser.InputList().size(), 1);

  const ShaderInputsParser::InputParam & param = parser.InputList().at(0);

  OLIVE_ASSERT_EQUAL( STR(param.human_name), STR("Base image"));
  OLIVE_ASSERT_EQUAL( STR(param.description), STR("the base image"));
  OLIVE_ASSERT_EQUAL( param.flags, InputFlags(kInputFlagNotKeyframable));
  OLIVE_ASSERT_EQUAL( STR(param.uniform_name), STR("tex_base"));
  OLIVE_ASSERT_EQUAL( STR(param.type_string), STR("TEXTURE"));
  OLIVE_ASSERT_EQUAL( param.type, NodeValue::kTexture);

  OLIVE_TEST_END;
}

// An input of kind Float
OLIVE_ADD_TEST(InputFloatTest)
{
  QString code(
        "//OVE name: Tolerance %%""\n"
        "//OVE type: FLOAT""\n"
        "//OVE min: 1.0""\n"
        "//OVE default: 5.5""\n"
        "//OVE max: 100""\n"
        "//OVE description:   the tolerance of filter""\n"
        "uniform float toler_in;""\n"
        );

  ShaderInputsParser parser(code);
  parser.Parse();

  OLIVE_ASSERT_EQUAL( parser.ErrorList().size(), 0);
  OLIVE_ASSERT_EQUAL( parser.InputList().size(), 1);

  const ShaderInputsParser::InputParam & param = parser.InputList().at(0);

  OLIVE_ASSERT_EQUAL( STR(param.human_name), STR("Tolerance %%"));
  OLIVE_ASSERT_EQUAL( STR(param.description), STR("the tolerance of filter"));
  OLIVE_ASSERT_EQUAL( param.flags, InputFlags(0));
  OLIVE_ASSERT_EQUAL( STR(param.uniform_name), STR("toler_in"));
  OLIVE_ASSERT_EQUAL( STR(param.type_string), STR("FLOAT"));
  OLIVE_ASSERT( param.type == NodeValue::kFloat);
  OLIVE_ASSERT_EQUAL( param.min.toDouble(), 1.);
  OLIVE_ASSERT_EQUAL( param.max.toDouble(), 100.);
  OLIVE_ASSERT_EQUAL( param.default_value.toDouble(), 5.5);

  OLIVE_TEST_END;
}

// An input of kind Float with 'min' and 'max' not specified
OLIVE_ADD_TEST(InputFloatTestBoundary)
{
  QString code(
        "//OVE name: Tolerance""\n"
        "//OVE type: FLOAT""\n"
        "uniform float toler_in;""\n"
        "//OVE name: Number of bars""\n"
        "//OVE type: INTEGER""\n"
        "uniform int bars_in;""\n"
        );

  ShaderInputsParser parser(code);
  parser.Parse();

  OLIVE_ASSERT_EQUAL( parser.ErrorList().size(), 0);
  OLIVE_ASSERT_EQUAL( parser.InputList().size(), 2);

  const ShaderInputsParser::InputParam & param1 = parser.InputList().at(0);

  OLIVE_ASSERT_EQUAL( STR(param1.human_name), STR("Tolerance"));
  OLIVE_ASSERT( param1.min.toDouble() < -1e30);
  OLIVE_ASSERT( param1.max.toDouble() > 1e30);

  const ShaderInputsParser::InputParam & param2 = parser.InputList().at(1);

  OLIVE_ASSERT_EQUAL( STR(param2.human_name), STR("Number of bars"));
  OLIVE_ASSERT( param2.min.toInt() < -1000000000);
  OLIVE_ASSERT( param2.max.toInt() > 1000000000);

  OLIVE_TEST_END;
}

bool operator == (const Color & rhs, const Color & lhs)
{
  bool r_ok = (abs(rhs.red() - lhs.red()) < 0.01) ;
  bool g_ok = (abs(rhs.green() - lhs.green()) < 0.01) ;
  bool b_ok = (abs(rhs.blue() - lhs.blue()) < 0.01) ;
  bool a_ok = (abs(rhs.alpha() - lhs.alpha()) < 0.01);

  return r_ok && g_ok && b_ok && a_ok;
}

// An input of kind Color
OLIVE_ADD_TEST(InputColorTest)
{
  QString code(
        "//OVE name: Final tone""\n"
        "//OVE type: COLOR""\n"
        "//OVE default: RGBA( 0.5,0.4 ,0.1,  1)""\n"
        "//OVE description: color applied to the grayscale""\n"
        "uniform vec4 tone_in;""\n"
        );

  ShaderInputsParser parser(code);
  parser.Parse();

  OLIVE_ASSERT_EQUAL( parser.ErrorList().size(), 0);
  OLIVE_ASSERT_EQUAL( parser.InputList().size(), 1);

  const ShaderInputsParser::InputParam & param = parser.InputList().at(0);

  OLIVE_ASSERT_EQUAL( STR(param.human_name), STR("Final tone"));
  OLIVE_ASSERT_EQUAL( STR(param.description), STR("color applied to the grayscale"));
  OLIVE_ASSERT_EQUAL( STR(param.uniform_name), STR("tone_in"));
  OLIVE_ASSERT_EQUAL( STR(param.type_string), STR("COLOR"));
  OLIVE_ASSERT_EQUAL( param.type, NodeValue::kColor);
  OLIVE_ASSERT( param.default_value.value<Color>() == Color(0.5f, 0.4f, 0.1f, 1.0f));

  OLIVE_TEST_END;
}

// An input of kind Color with components outside [0..1]
OLIVE_ADD_TEST(ColorOutOfRangeTest)
{
  QString code(
        "//OVE name: Final tone""\n"
        "//OVE type: COLOR""\n"
        "//OVE default: RGBA( 2, 0, 0,  1)""\n"                  //  <-- 2 is outisde range
        "//OVE description: color applied to the grayscale""\n"
        "uniform vec4 tone_in;""\n"
        );

  ShaderInputsParser parser(code);
  parser.Parse();

  OLIVE_ASSERT_EQUAL( parser.ErrorList().size(), 1);
  OLIVE_ASSERT_EQUAL( parser.InputList().size(), 1);  // input is still present

  OLIVE_ASSERT_EQUAL( STR(parser.ErrorList().at(0).issue), STR("Color must be in format 'RGBA(r,g,b,a)' "
                                                               "where r,g,b,a are in range (0,1)"));
  OLIVE_ASSERT_EQUAL( parser.ErrorList().at(0).line, 3);

  OLIVE_TEST_END;
}

// An input of kind Boolean
OLIVE_ADD_TEST(InputBooleanTest)
{
  QString code(
        "//OVE name: bypass effect""\n"
        "//OVE type: BOOLEAN""\n"
        "//OVE flag: NOT_CONNECTABLE""\n"
        "//OVE default: false""\n"
        "//OVE description: when True, the input is passed to output as is.""\n"
        "uniform bool disable_in;""\n"
        );

  ShaderInputsParser parser(code);
  parser.Parse();

  OLIVE_ASSERT_EQUAL( parser.ErrorList().size(), 0);
  OLIVE_ASSERT_EQUAL( parser.InputList().size(), 1);

  const ShaderInputsParser::InputParam & param = parser.InputList().at(0);

  OLIVE_ASSERT_EQUAL( STR(param.human_name), STR("bypass effect"));
  OLIVE_ASSERT_EQUAL( STR(param.description), STR("when True, the input is passed to output as is."));
  OLIVE_ASSERT_EQUAL( param.flags, InputFlags(kInputFlagNotConnectable));
  OLIVE_ASSERT_EQUAL( STR(param.uniform_name), STR("disable_in"));
  OLIVE_ASSERT_EQUAL( STR(param.type_string), STR("BOOLEAN"));
  OLIVE_ASSERT_EQUAL( param.type, NodeValue::kBoolean);
  OLIVE_ASSERT_EQUAL( param.default_value.toBool(), false);

  OLIVE_TEST_END;
}

// An input of kind Selection
OLIVE_ADD_TEST(InputSelectionTest)
{
  QString code(
        "//OVE name: mode""\n"
        "//OVE type: SELECTION""\n"
        "//OVE values:  \"ADD\", \"AVERAGE\", \"COLOR BURN\"""\n"
        "//OVE values:  \"COLOR DODGE\"  \"DARKEN\"  \"DIFFERENCE\"""\n"
        "//OVE values:  \"EXCLUSION\" \"GLOW\"""\n"
        "//OVE default: 3""\n"
        "//OVE description: the blend mode""\n"
        "uniform int mode_in; ""\n"
        );

  ShaderInputsParser parser(code);
  parser.Parse();

  OLIVE_ASSERT_EQUAL( parser.ErrorList().size(), 0);
  OLIVE_ASSERT_EQUAL( parser.InputList().size(), 1);

  const ShaderInputsParser::InputParam & param = parser.InputList().at(0);

  OLIVE_ASSERT_EQUAL( STR(param.human_name), STR("mode"));
  OLIVE_ASSERT_EQUAL( STR(param.description), STR("the blend mode"));
  OLIVE_ASSERT_EQUAL( STR(param.uniform_name), STR("mode_in"));
  OLIVE_ASSERT_EQUAL( STR(param.type_string), STR("SELECTION"));
  OLIVE_ASSERT_EQUAL( param.type, NodeValue::kCombo);
  OLIVE_ASSERT_EQUAL( param.default_value.toInt(), 3);

  OLIVE_TEST_END;
}

// An input of kind Point
OLIVE_ADD_TEST(InputPointTest)
{
  QString code(
        "//OVE name: From""\n"
        "//OVE type: POINT""\n"
        "//OVE default: (0.4,0.2)""\n"
        "//OVE color: RGBA( 0.5,0.5 ,0.0,  1)""\n"
        "//OVE description: gradient start point""\n"
        "uniform vec2 from_in;""\n"
        );

  ShaderInputsParser parser(code);
  parser.Parse();

  OLIVE_ASSERT_EQUAL( parser.ErrorList().size(), 0);
  OLIVE_ASSERT_EQUAL( parser.InputList().size(), 1);

  const ShaderInputsParser::InputParam & param = parser.InputList().at(0);

  OLIVE_ASSERT_EQUAL( STR(param.human_name), STR("From"));
  OLIVE_ASSERT_EQUAL( STR(param.description), STR("gradient start point"));
  OLIVE_ASSERT_EQUAL( param.flags, InputFlags(0));
  OLIVE_ASSERT_EQUAL( STR(param.uniform_name), STR("from_in"));
  OLIVE_ASSERT_EQUAL( STR(param.type_string), STR("POINT"));
  OLIVE_ASSERT_EQUAL( param.type, NodeValue::kVec2);
  OLIVE_ASSERT( param.gizmoColor == QColor(127,127,0));

  OLIVE_TEST_END;
}

// An input with multiple flags (with different separators)
OLIVE_ADD_TEST(MultipleFlagsTest)
{
  QString code(
        "//OVE name: From""\n"
        "//OVE type: POINT""\n"
        "//OVE default: (0.4,0.2)""\n"
        "//OVE flag: NOT_KEYFRAMABLE, NOT_CONNECTABLE  HIDDEN""\n"
        "//OVE description: gradient start point""\n"
        "uniform vec2 from_in;""\n"
        );

  ShaderInputsParser parser(code);
  parser.Parse();

  OLIVE_ASSERT_EQUAL( parser.ErrorList().size(), 0);
  OLIVE_ASSERT_EQUAL( parser.InputList().size(), 1);

  const ShaderInputsParser::InputParam & param = parser.InputList().at(0);

  OLIVE_ASSERT_EQUAL( STR(param.human_name), STR("From"));
  OLIVE_ASSERT_EQUAL( param.flags, InputFlags(kInputFlagNotKeyframable | kInputFlagNotConnectable | kInputFlagHidden));

  OLIVE_TEST_END;
}


// input type typo error. Check that errors are detected
OLIVE_ADD_TEST(InputTypoTest)
{
  QString code(
        "//OVE name: Tolerance %%""\n"
        "//OVE type: FLOA""\n"                 //  <-- typo: "FLOA" for "FLOAT"
        "//OVE min: 1.0""\n"
        "//OVE default: 5.5""\n"
        "//OVE max: 100""\n"
        "//OVE description:   the tolerance of filter""\n"
        "uniform float toler_in;""\n"
        );

  ShaderInputsParser parser(code);
  parser.Parse();

  OLIVE_ASSERT_EQUAL( parser.ErrorList().size(), 4);

  OLIVE_ASSERT_EQUAL( STR(parser.ErrorList().at(0).issue),
                      STR("type FLOA is invalid") );
  OLIVE_ASSERT_EQUAL( parser.ErrorList().at(0).line, 2);

  OLIVE_ASSERT_EQUAL( STR(parser.ErrorList().at(1).issue),
                      STR("1.0 is not valid as minimum for type FLOA") );
  OLIVE_ASSERT_EQUAL( parser.ErrorList().at(1).line, 3);

  OLIVE_ASSERT_EQUAL( STR(parser.ErrorList().at(2).issue),
                      STR("type FLOA does not support a default value") );
  OLIVE_ASSERT_EQUAL( parser.ErrorList().at(2).line, 4);

  OLIVE_ASSERT_EQUAL( STR(parser.ErrorList().at(3).issue),
                      STR("100 is not valid as maximum for type FLOA") );
  OLIVE_ASSERT_EQUAL( parser.ErrorList().at(3).line, 5);

  OLIVE_TEST_END;
}


// an input defined after "//OVE end"
// The input is not counted
OLIVE_ADD_TEST(InputAfterEndTest)
{
  QString code(
        "//OVE end""\n"                       // <- end
        "//OVE name: Tolerance %%""\n"
        "//OVE type: FLOAT""\n"
        "//OVE description:   the tolerance of filter""\n"
        "uniform float toler_in;""\n"
        );

  ShaderInputsParser parser(code);
  parser.Parse();

  OLIVE_ASSERT_EQUAL( parser.ErrorList().size(), 0);
  OLIVE_ASSERT_EQUAL( parser.InputList().size(), 0);

  OLIVE_TEST_END;
}


// presence of multiple inputs
OLIVE_ADD_TEST(InputManyTest)
{
  QString code(
        "//OVE name: Base image""\n"
        "//OVE type: TEXTURE""\n"
        "//OVE flag: NOT_KEYFRAMABLE""\n"
        "//OVE description: the base image""\n"
        "uniform sampler2D tex_base;""\n"
        "\n"
        "//OVE name: Tolerance %%""\n"
        "//OVE type: FLOAT""\n"
        "//OVE description:   the tolerance of filter""\n"
        "uniform float toler_in;""\n"
        "\n"
        "//OVE name: base color""\n"
        "//OVE type: COLOR""\n"
        "//OVE description:   the final color""\n"
        "uniform vec4 tone_in;""\n"
        "\n"
        );

  ShaderInputsParser parser(code);
  parser.Parse();

  OLIVE_ASSERT_EQUAL( parser.ErrorList().size(), 0);
  OLIVE_ASSERT_EQUAL( parser.InputList().size(), 3);

  OLIVE_TEST_END;
}

// An input whose type is not consistent with the uniform type
OLIVE_ADD_TEST(InconsistentTypeTest_1)
{
  QString code(
        "//OVE name: Base image""\n"
        "//OVE type: TEXTURE""\n"
        "//OVE description: the base image""\n"
        "uniform float tex_base;""\n"            // <-- 'float' vs 'TEXTURE'
        );

  ShaderInputsParser parser(code);
  parser.Parse();

  OLIVE_ASSERT_EQUAL( parser.ErrorList().size(), 1);
  OLIVE_ASSERT_EQUAL( parser.InputList().size(), 1);  // input is still present

  OLIVE_ASSERT_EQUAL( parser.ErrorList().at(0).line, 4);
  OLIVE_ASSERT_EQUAL( STR(parser.ErrorList().at(0).issue),
                      STR("metadata type TEXTURE requires uniform type sampler2D and not float"));


  OLIVE_TEST_END;
}

// An input whose type is not consistent with the uniform type
OLIVE_ADD_TEST(InconsistentTypeTest_2)
{
  QString code(
        "//OVE name: Gain""\n"
        "//OVE type: FLOAT""\n"
        "//OVE description: the base image""\n"
        "uniform vec2 gain_in;""\n"            // <-- 'vec2' vs 'FLOAT'
        );

  ShaderInputsParser parser(code);
  parser.Parse();

  OLIVE_ASSERT_EQUAL( parser.ErrorList().size(), 1);
  OLIVE_ASSERT_EQUAL( parser.InputList().size(), 1);  // input is still present

  OLIVE_ASSERT_EQUAL( parser.ErrorList().at(0).line, 4);
  OLIVE_ASSERT_EQUAL( STR(parser.ErrorList().at(0).issue),
                      STR("metadata type FLOAT requires uniform type float and not vec2"));


  OLIVE_TEST_END;
}

// Illegal number of iterations
OLIVE_ADD_TEST(WrongNumOfIterations)
{
  QString code(
        "#version 150""\n"
        "//OVE shader_name: slide""\n"
        "//OVE shader_description: slide transition""\n"
        "//OVE shader_version: 0.1""\n"
        "//OVE number_of_iterations: two""\n"
        );

  ShaderInputsParser parser(code);
  parser.Parse();

  OLIVE_ASSERT_EQUAL( parser.ErrorList().size(), 1);
  OLIVE_ASSERT_EQUAL( parser.ErrorList().at(0).line, 5);
  OLIVE_ASSERT_EQUAL( STR(parser.ErrorList().at(0).issue),
                      STR("Number of iterations must be a number greater or equal to 1"));

  OLIVE_TEST_END;
}


}  // olive

