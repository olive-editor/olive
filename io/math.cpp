#include "math.h"

#include <QtMath>
#include <QVector>

#include "debug.h"

int lerp(int a, int b, double t) {
	return qRound(((1.0 - t) * a) + (t * b));
}

float float_lerp(float a, float b, float t) {
	return ((1.0F - t) * a) + (t * b);
}

double double_lerp(double a, double b, double t) {
	return ((1.0 - t) * a) + (t * b);
}

double quad_from_t(double a, double b, double c, double t) {
	return qPow(1.0 - t, 2)*a + 2*(1.0 - t)*t*b + qPow(t, 2)*c;
}

double quad_t_from_x(double x, double a, double b, double c) {
	return (a - b + qSqrt(a*x + c*x - 2*b*x + qPow(b, 2) - a*c))/(a - 2*b + c);
	// alt: return (a - b - qSqrt(a*x + c*x - 2*b*x + qPow(b, 2) - a*c))/(a - 2*b + c);
}

double cubic_from_t(double a, double b, double c, double d, double t) {
	return qPow(1.0 - t, 3)*a + 3*qPow(1.0 - t, 2)*t*b + 3*(1.0 - t)*qPow(t, 2)*c + qPow(t, 3)*d;
}

double cubic_t_from_x(double x_target, double a, double b, double c, double d) {
	double tolerance = 0.0001;

	double lower = 0.0;
	double upper = 1.0;

	double percent = 0.5;
	double x = cubic_from_t(a, b, c, d, percent);

	while (qAbs(x_target - x) > tolerance) {
		if (x_target > x) {
			lower = percent;
		} else {
			upper = percent;
		}

		percent = (upper + lower) / 2.0;
		x = cubic_from_t(a, b, c, d, percent);
	}

	return percent;
}
