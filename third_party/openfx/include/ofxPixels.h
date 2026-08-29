

#ifndef _ofxPixels_h_
#define _ofxPixels_h_

// Copyright OpenFX and contributors to the OpenFX project.
// SPDX-License-Identifier: BSD-3-Clause


#ifdef __cplusplus
extern "C" {
#endif

/** @file ofxPixels.h
Contains pixel struct definitions
*/

/** @brief Defines an 8 bit per component RGBA pixel */
typedef struct OfxRGBAColourB {
  unsigned char r, g, b, a;
}OfxRGBAColourB;

/** @brief Defines a 16 bit per component RGBA pixel */
typedef struct OfxRGBAColourS {
  unsigned short r, g, b, a;
}OfxRGBAColourS;

/** @brief Defines a floating point component RGBA pixel */
typedef struct OfxRGBAColourF {
  float r, g, b, a;
}OfxRGBAColourF;


/** @brief Defines a double precision floating point component RGBA pixel */
typedef struct OfxRGBAColourD {
  double r, g, b, a;
}OfxRGBAColourD;


/** @brief Defines an 8 bit per component RGB pixel */
typedef struct OfxRGBColourB {
  unsigned char r, g, b;
}OfxRGBColourB;

/** @brief Defines a 16 bit per component RGB pixel */
typedef struct OfxRGBColourS {
  unsigned short r, g, b;
}OfxRGBColourS;

/** @brief Defines a floating point component RGB pixel */
typedef struct OfxRGBColourF {
  float r, g, b;
}OfxRGBColourF;

/** @brief Defines a double precision floating point component RGB pixel */
typedef struct OfxRGBColourD {
  double r, g, b;
}OfxRGBColourD;

#ifdef __cplusplus
}
#endif

#endif
