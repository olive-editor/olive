#ifndef DECIBEL_H
#define DECIBEL_H

/**
 * @brief Converts an amplitude into a value in decibels
 *
 * Converts a 0.0-1.0 amplitude into an infinity-0.0 decibel value
 */
double amplitude_to_db(double amplitude);

/**
 * @brief Converts decibels into an amplitude value
 *
 * Converts an infinity-0.0 decibel value into a 0.0-1.0 amplitude
 */
double db_to_amplitude(double db);

#endif // DECIBEL_H
