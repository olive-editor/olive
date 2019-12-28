#ifndef BEZIER_H
#define BEZIER_H


class Bezier
{
public:
  static double QuadraticXtoT(double x, double a, double b, double c);

  static double QuadraticTtoY(double a, double b, double c, double t);

  static double CubicXtoT(double x_target, double a, double b, double c, double d);

  static double CubicTtoY(double a, double b, double c, double d, double t);
};

#endif // BEZIER_H
