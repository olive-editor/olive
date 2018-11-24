#include "math.h"

#include <QtMath>

int lerp(int a, int b, double t) {
    return qRound(((1.0 - t) * a) + (t * b));
}

float float_lerp(float a, float b, float t) {
    return ((1.0F - t) * a) + (t * b);
}

double double_lerp(double a, double b, double t) {
    return ((1.0 - t) * a) + (t * b);
}
