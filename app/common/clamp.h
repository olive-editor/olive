#ifndef CLAMP_H
#define CLAMP_H

template<typename T>
/**
 * @brief Clamp a value between a minimum and a maximum value
 *
 * Similar to using min() and max() functions, but performs both at once. If value is less than minimum, this returns
 * minimum. If it is more than maximum, this returns maximum. Otherwise it returns value as-is.
 *
 * @return
 *
 * Will always return a value between minimum and maximum (inclusive).
 */
T clamp(T value, T minimum, T maximum) {
  if (value < minimum) {
    return minimum;
  }

  if (value > maximum) {
    return maximum;
  }

  return value;
}

#endif // CLAMP_H
