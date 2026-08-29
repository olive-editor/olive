

#ifndef _ofxDraw_h_
#define _ofxDraw_h_

#include "ofxCore.h"
#include "ofxPixels.h"

// Copyright OpenFX and contributors to the OpenFX project.
// SPDX-License-Identifier: BSD-3-Clause


#ifdef __cplusplus
extern "C" {
#endif

/** @file ofxDrawSuite.h
API for host- and GPU API-independent drawing.
@version Added in OpenFX 1.5
*/


/** @brief the string that names the DrawSuite, passed to OfxHost::fetchSuite */
#define kOfxDrawSuite "OfxDrawSuite"

/** @brief Blind declaration of an OFX drawing context
 */
typedef struct OfxDrawContext *OfxDrawContextHandle;

/** @brief The Draw Context handle

 - Type - pointer X 1
 - Property Set - read only property on the inArgs of the following actions...
 - ::kOfxInteractActionDraw
 */
#define kOfxInteractPropDrawContext "OfxInteractPropDrawContext"

/** @brief Defines valid values for OfxDrawSuiteV1::getColour */
typedef enum OfxStandardColour
{
	kOfxStandardColourOverlayBackground,
	kOfxStandardColourOverlayActive,
	kOfxStandardColourOverlaySelected,
	kOfxStandardColourOverlayDeselected,
	kOfxStandardColourOverlayMarqueeFG,
	kOfxStandardColourOverlayMarqueeBG,
	kOfxStandardColourOverlayText
} OfxStandardColour;

/** @brief Defines valid values for OfxDrawSuiteV1::setLineStipple */
typedef enum OfxDrawLineStipplePattern
{
	kOfxDrawLineStipplePatternSolid,	// -----
	kOfxDrawLineStipplePatternDot,		// .....
	kOfxDrawLineStipplePatternDash,		// - - -
	kOfxDrawLineStipplePatternAltDash,	//  - - -
	kOfxDrawLineStipplePatternDotDash	// .-.-.-
} OfxDrawLineStipplePattern;

/** @brief Defines valid values for OfxDrawSuiteV1::draw */

typedef enum OfxDrawPrimitive
{
	kOfxDrawPrimitiveLines,
	kOfxDrawPrimitiveLineStrip,
	kOfxDrawPrimitiveLineLoop,
	kOfxDrawPrimitiveRectangle,
	kOfxDrawPrimitivePolygon,
	kOfxDrawPrimitiveEllipse
} OfxDrawPrimitive;

/** @brief Defines text alignment values for OfxDrawSuiteV1::drawText */
typedef enum OfxDrawTextAlignment
{
	kOfxDrawTextAlignmentLeft     = 0x0001,
	kOfxDrawTextAlignmentRight    = 0x0002,
	kOfxDrawTextAlignmentTop      = 0x0004,
	kOfxDrawTextAlignmentBottom   = 0x0008,
	kOfxDrawTextAlignmentBaseline = 0x0010,
	kOfxDrawTextAlignmentCenterH  = (kOfxDrawTextAlignmentLeft | kOfxDrawTextAlignmentRight),
	kOfxDrawTextAlignmentCenterV  = (kOfxDrawTextAlignmentTop | kOfxDrawTextAlignmentBaseline)
} OfxDrawTextAlignment;

/** @brief OFX suite that allows an effect to draw to a host-defined display context.

*/
typedef struct OfxDrawSuiteV1 {
	/** @brief Retrieves the host's desired draw colour for

	 \arg \c context  draw context
	 \arg \c std_colour desired colour type
	 \arg \c colour      returned RGBA colour

	 @returns
	 - ::kOfxStatOK - the colour was returned
	 - ::kOfxStatErrValue - std_colour was invalid
	 - ::kOfxStatFailed - failure, e.g. if function is called outside kOfxInteractActionDraw
	 */
	OfxStatus (*getColour)(OfxDrawContextHandle context, OfxStandardColour std_colour, OfxRGBAColourF *colour);

	/** @brief Sets the colour for future drawing operations (lines, filled shapes and text)

	 \arg \c context  draw context
	 \arg \c colour      RGBA colour

	 The host should use "over" compositing when using a non-opaque colour.
	
	 @returns
	 - ::kOfxStatOK - the colour was changed
	 - ::kOfxStatFailed - failure, e.g. if function is called outside kOfxInteractActionDraw
	 */
	OfxStatus (*setColour)(OfxDrawContextHandle context, const OfxRGBAColourF *colour);

	/** @brief Sets the line width for future line drawing operations

	 \arg \c context  draw context
	 \arg \c width     line width

	 Use width 0 for a single pixel line or non-zero for a smooth line of the desired width

	 The host should adjust for screen density.

	 @returns
	 - ::kOfxStatOK - the width was changed
	 - ::kOfxStatFailed - failure, e.g. if function is called outside kOfxInteractActionDraw
	 */
	OfxStatus (*setLineWidth)(OfxDrawContextHandle context, float width);

	/** @brief Sets the stipple pattern for future line drawing operations

	 \arg \c context  draw context
	 \arg \c pattern  desired stipple pattern

	 @returns
	 - ::kOfxStatOK - the pattern was changed
	 - ::kOfxStatErrValue - pattern was not valid
	 - ::kOfxStatFailed - failure, e.g. if function is called outside kOfxInteractActionDraw
	 */
	OfxStatus (*setLineStipple)(OfxDrawContextHandle context, OfxDrawLineStipplePattern pattern);

	/** @brief Draws a primitive of the desired type

	 \arg \c context  draw context
	 \arg \c primitive  desired primitive
	 \arg \c points  array of points in the primitive
	 \arg \c point_count  number of points in the array

	 kOfxDrawPrimitiveLines - like GL_LINES, n points draws n/2 separated lines
	 kOfxDrawPrimitiveLineStrip - like GL_LINE_STRIP, n points draws n-1 connected lines
	 kOfxDrawPrimitiveLineLoop - like GL_LINE_LOOP, n points draws n connected lines
	 kOfxDrawPrimitiveRectangle - draws an axis-aligned filled rectangle defined by 2 opposite corner points
	 kOfxDrawPrimitivePolygon - like GL_POLYGON, draws a filled n-sided polygon
	 kOfxDrawPrimitiveEllipse - draws a axis-aligned elliptical line (not filled) within the rectangle defined by 2 opposite corner points

	 @returns
	 - ::kOfxStatOK - the draw was completed
	 - ::kOfxStatErrValue - invalid primitive, or point_count not valid for primitive
	 - ::kOfxStatFailed - failure, e.g. if function is called outside kOfxInteractActionDraw
	 */
	OfxStatus (*draw)(OfxDrawContextHandle context, OfxDrawPrimitive primitive, const OfxPointD *points, int point_count);


	/** @brief Draws text at the specified position

	 \arg \c context  draw context
	 \arg \c text  text to draw (UTF-8 encoded)
	 \arg \c pos  position at which to align the text
	 \arg \c alignment  text alignment flags (see kOfxDrawTextAlignment*)

	 The text font face and size are determined by the host.

	 @returns
	 - ::kOfxStatOK - the text was drawn
	 - ::kOfxStatErrValue - text or pos were not defined
	 - ::kOfxStatFailed - failure, e.g. if function is called outside kOfxInteractActionDraw
	 */
	OfxStatus (*drawText)(OfxDrawContextHandle context, const char *text, const OfxPointD *pos, int alignment);

} OfxDrawSuiteV1;

#ifdef __cplusplus
}
#endif

#endif
