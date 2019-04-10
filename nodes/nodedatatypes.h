#ifndef NODEDATATYPES_H
#define NODEDATATYPES_H

#include <QString>

namespace olive {
namespace nodes {

/**
 * @brief The EffectFieldType enum
 *
 * Predetermined types of fields. Used throughout Olive to identify what kind of data to expect from GetValueAt().
 *
 * This enum is also currently used to match an external XML effect's fields with the correct derived class (e.g.
 * EFFECT_FIELD_DOUBLE matches to DoubleInput).
 */
enum DataType {
  /** Invalid data type. Used only for error handling. */
  kInvalid,

  /** Values are doubles. Also corresponds to DoubleInput. */
  kFloat,

  /** Value is an 2-component vector of floats. */
  kVec2,

  /** Value is an 3-component vector of floats. */
  kVec3,

  /** Value is an 4-component vector of floats. */
  kVec4,

  /** Value is an array of floats. This cannot be an input field, and can only be passed between nodes. */
  kArray,

  /** Values are colors. Equivalent to kVec4 but represents as a color. Corresponds to ColorInput. */
  kColor,

  /** Values are strings. Also corresponds to StringInput. */
  kString,

  /** Values are booleans. Also corresponds to BoolInput. */
  kBoolean,

  /** Values are arbitrary data. Also corresponds to ComboInput. */
  kCombo,

  /** Values are font family names (in string). Also corresponds to FontInput. */
  kFont,

  /** Values are filenames (in string). Also corresponds to FileInput. */
  kFile,

  /** Values are integers. */
  kInteger,

  /** Value is a texture. This cannot be an input field, and can only be passed between nodes. */
  kTexture,

  /** Value is a 4x4 matrix. This cannot be an input field, and can only be passed between nodes. */
  kMatrix,

  /** Values is a UI object with no data. Corresponds to nothing. */
  kUI,

  /** Total count of valid node data types. Never use this as an actual data type. */
  kDataTypeCount
};

/**
 * @brief Convert a node data type to a unique string that can be saved to an XML file
 *
 * @param type
 *
 * The data type to convert to string
 *
 * @return
 *
 * The unique string identifier for this data type.
 */
QString DataTypeToString(DataType type);

/**
 * @brief Convert a string to a node data type.
 *
 * Generally this string should be a string that was received from DataTypeToString() to ensure compatibility and
 * correctness.
 *
 * @param s
 *
 * The string to convert to a data type
 *
 * @return
 *
 * The data type that this string represents
 */
DataType StringToDataType(const QString& s);

}
}

#endif // NODEDATATYPES_H
