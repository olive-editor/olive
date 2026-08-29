

#ifndef _ofxMessage_h_
#define _ofxMessage_h_

#include "ofxCore.h"

// Copyright OpenFX and contributors to the OpenFX project.
// SPDX-License-Identifier: BSD-3-Clause


#ifdef __cplusplus
extern "C" {
#endif

#define kOfxMessageSuite "OfxMessageSuite"


/** @brief String used to type fatal error messages

    Fatal error messages should only be posted by a plugin when it can no longer continue operation.
 */
#define kOfxMessageFatal "OfxMessageFatal"

/** @brief String used to type error messages 

    Ordinary error messages should be posted when there is an error in operation that is recoverable by
    user intervention.
*/
#define kOfxMessageError "OfxMessageError"

/** @brief String used to type warning messages 

    Warnings indicate states that allow for operations to proceed, but are not necessarily optimal.
*/
#define kOfxMessageWarning "OfxMessageWarning"

/** @brief String used to type simple ordinary messages 

    Ordinary messages simply convey information from the plugin directly to the user.
*/
#define kOfxMessageMessage "OfxMessageMessage"

/** @brief String used to type log messages 

    Log messages are written out to a log and not to the end user.
*/
#define kOfxMessageLog "OfxMessageLog"

/** @brief String used to type yes/no messages

    The host is to enter a modal state which waits for the user to respond yes or no. 
The OfxMessageSuiteV1::message function which posted the message will only return after 
the user responds. When asking a question, the OfxStatus code returned by the message function will be,
    - kOfxStatReplyYes - if the user replied 'yes' to the question
    - kOfxStatReplyNo - if the user replied 'no' to the question
    - some error code if an error was encounterred

    It is an error to post a question message if the plugin is not in an interactive session.
 */
#define kOfxMessageQuestion "OfxMessageQuestion"

/** @brief The OFX suite that allows a plug-in to pass messages back to a user. The V2 suite extends on this
    in a backwards compatible manner.
 */
typedef struct OfxMessageSuiteV1 {

  /** @brief Post a message on the host, using printf style varargs

      \arg \c handle     effect handle (descriptor or instance) the message should be associated with, may be NULL
      \arg \c messageType string describing the kind of message to post, one of the kOfxMessageType* constants
      \arg \c messageId plugin specified id to associate with this message. If overriding the message in XML resource, the message is identified with this, this may be NULL, or "", in which case no override will occur,
      \arg \c format    printf style format string
      \arg \c ...       printf style varargs list to print

\returns 
  - ::kOfxStatOK - if the message was sucessfully posted 
  - ::kOfxStatReplyYes - if the message was of type  kOfxMessageQuestion and the user reply yes
  - ::kOfxStatReplyNo - if the message was of type kOfxMessageQuestion and the user reply no
  - ::kOfxStatFailed - if the message could not be posted for some reason

   */
  OfxStatus (*message)(void *handle,
		       const char *messageType,
		       const char *messageId,
		       const char *format,
		       ...);

} OfxMessageSuiteV1;

/** @brief The OFX suite that allows a plug-in to pass messages back to a user.

    This extends OfxMessageSuiteV1, and should be considered a replacement to version 1. 

    Note that this suite has been extended in backwards compatible manner, so that a host can return this struct
    for both V1 and V2.
 */
typedef struct OfxMessageSuiteV2 {

  /** @brief Post a transient message on the host, using printf style varargs. Same as the V1 message suite call.

      \arg \c handle     effect handle (descriptor or instance) the message should be associated with, may be null
      \arg \c messageType string describing the kind of message to post, one of the kOfxMessageType* constants
      \arg \c messageId plugin specified id to associate with this message. If overriding the message in XML resource, the message is identified with this, this may be NULL, or "", in which case no override will occur,
      \arg \c format    printf style format string
      \arg \c ...       printf style varargs list to print

      \returns 
      - ::kOfxStatOK - if the message was sucessfully posted 
      - ::kOfxStatReplyYes - if the message was of type  kOfxMessageQuestion and the user reply yes
      - ::kOfxStatReplyNo - if the message was of type kOfxMessageQuestion and the user reply no
      - ::kOfxStatFailed - if the message could not be posted for some reason
   */
  OfxStatus (*message)(void *handle,
		       const char *messageType,
		       const char *messageId,
		       const char *format,
		       ...);

  /** @brief Post a persistent message on an effect, using printf style varargs, and set error states. New for V2 message suite.

      \arg \c handle     effect instance handle the message should be associated with, may NOT be null,
      \arg \c messageType string describing the kind of message to post, should be one of...
            - kOfxMessageError
            - kOfxMessageWarning
            - kOfxMessageMessage            
      \arg \c messageId plugin specified id to associate with this message. If overriding the message in XML resource, the message is identified with this, this may be NULL, or "", in which case no override will occur,
      \arg \c format    printf style format string
      \arg \c ...       printf style varargs list to print

      \returns 
        - ::kOfxStatOK - if the message was sucessfully posted 
        - ::kOfxStatErrBadHandle - the handle was rubbish
        - ::kOfxStatFailed - if the message could not be posted for some reason

     Persistent messages are associated with an effect handle until explicitly cleared by an effect. So if an error message is posted the error state, and associated message will persist and be displayed on the effect appropriately. (eg: draw a node in red on a node based compostor and display the message when clicked on).

     If \e messageType is error or warning, associated error states should be flagged on host applications. Posting an error message implies that the host cannot proceeed, a warning allows the host to proceed, whilst a simple message should have no stop anything.
   */
  OfxStatus (*setPersistentMessage)(void *handle,
                                    const char *messageType,
                                    const char *messageId,
                                    const char *format,
                                    ...);

  
  /** @brief Clears any persistent message on an effect handle that was set by OfxMessageSuiteV2::setPersistentMessage. New for V2 message suite.

      \arg \c handle     effect instance handle messages should be cleared from.
      \arg \c handle     effect handle (descriptor or instance) 

      \returns 
        - ::kOfxStatOK - if the message was sucessfully cleared
        - ::kOfxStatErrBadHandle - the handle was rubbish
        - ::kOfxStatFailed - if the message could not be cleared for some reason

     Clearing a message will clear any associated error state.
  */
  OfxStatus (*clearPersistentMessage)(void *handle);

} OfxMessageSuiteV2;

/** @file ofxMessage.h
    This file contains the Host API for end user message communication.
*/

#ifdef __cplusplus
}
#endif

#endif
