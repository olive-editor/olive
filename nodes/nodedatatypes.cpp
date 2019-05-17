#include "nodedatatypes.h"

QString olive::nodes::DataTypeToString(DataType type) {
  switch (type) {
  case kFloat:
    return "DOUBLE";
  case kVec2:
    return "VEC2";
  case kVec3:
    return "VEC3";
  case kVec4:
    return "VEC4";
  case kArray:
    return "ARRAY";
  case kColor:
    return "COLOR";
  case kString:
    return "STRING";
  case kBoolean:
    return "BOOL";
  case kCombo:
    return "COMBO";
  case kFont:
    return "FONT";
  case kFile:
    return "FILE";
  case kInteger:
    return "INTEGER";
  case kTexture:
    return "TEXTURE";
  case kMatrix:
    return "MATRIX";
  case kBlock:
    return "CLIP";
  default:
    return QString();
  }
}

olive::nodes::DataType olive::nodes::StringToDataType(const QString &s)
{
  if (s == "DOUBLE") {
    return kFloat;
  } else if (s == "VEC2") {
    return kVec2;
  } else if (s == "VEC3") {
    return kVec3;
  } else if (s == "VEC4") {
    return kVec4;
  } else if (s == "ARRAY") {
    return kArray;
  } else if (s == "COLOR") {
    return kColor;
  } else if (s == "STRING") {
    return kString;
  } else if (s == "BOOL") {
    return kBoolean;
  } else if (s == "COMBO") {
    return kCombo;
  } else if (s == "FONT") {
    return kFont;
  } else if (s == "FILE") {
    return kFile;
  } else if (s == "INTEGER") {
    return kInteger;
  } else if (s == "TEXTURE") {
    return kTexture;
  } else if (s == "MATRIX") {
    return kMatrix;
  } else if (s == "CLIP") {
    return kBlock;
  }
  return kInvalid;
}
