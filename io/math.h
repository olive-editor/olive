#ifndef MATH_H
#define MATH_H

int lerp(int a, int b, double t);
float float_lerp(float a, float b, float t);
double double_lerp(double a, double b, double t);
double quad_from_t(double a, double b, double c, double t);
double quad_t_from_x(double x, double a, double b, double c);
double cubic_from_t(double a, double b, double c, double d, double t);
double cubic_t_from_x(double x_target, double a, double b, double c, double d);
double solveCubicBezier(double p0, double p1, double p2, double p3, double x);

// decibel conversion functions
double amplitude_to_db(double amplitude);
double db_to_amplitude(double db);

#endif // MATH_H
