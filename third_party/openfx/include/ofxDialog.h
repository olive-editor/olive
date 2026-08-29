


#ifndef _ofxDialog_h_
#define _ofxDialog_h_

// Copyright OpenFX and contributors to the OpenFX project.
// SPDX-License-Identifier: BSD-3-Clause

#include "ofxCore.h"
#include "ofxProperty.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @file ofxDialog.h

This file contains an optional suite which should be used to popup a native OS dialog
from a host parameter changed action.

When a host uses a fullscreen window and is running the OFX plugins in another thread
it can lead to a lot of conflicts if that plugin will try to open its own window.

This suite will provide the functionality for a plugin to request running its dialog
in the UI thread, and informing the host it will do this so it can take the appropriate
actions needed. (Like lowering its priority etc..)
*/

/** @brief The name of the Dialog suite, used to fetch from a host via
    OfxHost::fetchSuite
 */
#define kOfxDialogSuite		"OfxDialogSuite"

/** @brief Action called after a dialog has requested a 'Dialog'
         The arguments to the action are:
          \arg \c user_data Pointer which was provided when the plugin requested the Dialog

	   When the plugin receives this action it is safe to popup a dialog.
	   It runs in the host's UI thread, which may differ from the main OFX processing thread.
	   Plugin should return from this action when all Dialog interactions are done.
	   At that point the host will continue again.
	   The host will not send any other messages asynchronous to this one.
*/
#define  kOfxActionDialog	"OfxActionDialog"

typedef struct OfxDialogSuiteV1
{
  /** @brief Request the host to send a kOfxActionDialog to the plugin from its UI thread.
  \pre
    - user_data: A pointer to any user data
  \post
  @returns
    - ::kOfxStatOK - The host has queued the request and will send an 'OfxActionDialog'
    - ::kOfxStatFailed - The host has no provisio for this or can not deal with it currently.
  */
  OfxStatus (*RequestDialog)( void *user_data );
  
  /** @brief Inform the host of redraw event so it can redraw itself
      If the host runs fullscreen in OpenGL, it would otherwise not receive
redraw event when a dialog in front would catch all events.
  \pre
  \post
  @returns
    - ::kOfxStatReplyDefault
  */
  OfxStatus (*NotifyRedrawPending)( void );
} OfxDialogSuiteV1;



#ifdef __cplusplus
}
#endif


#endif

