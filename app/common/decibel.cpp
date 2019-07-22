#include "decibel.h"

#include <QtMath>

double amplitude_to_db(double amplitude) {
  return (20.0*(qLn(amplitude)/qLn(10.0)));
}

double db_to_amplitude(double db) {
  return qPow(M_E, (db*qLn(10.0))/20.0);
}
