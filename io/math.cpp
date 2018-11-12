#include "math.h"

int lerp(int a, int b, double t) {
    return ((1.0 - t) * a) + (t * b);
}

double double_lerp(double a, double b, double t) {
    return ((1.0 - t) * a) + (t * b);
}
