#ifndef CLAMP_H
#define CLAMP_H

template<typename T>
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
