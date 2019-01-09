/* 
 * Olive. Olive is a free non-linear video editor for Windows, macOS, and Linux.
 * Copyright (C) 2018  {{ organization }}
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 *along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
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

#endif // MATH_H
