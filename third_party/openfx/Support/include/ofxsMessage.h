

#ifndef _ofxsMessage_H_
#define _ofxsMessage_H_

namespace OFX 
{
  namespace Message 
  {
    enum MessageReplyEnum
    {
      eMessageReplyOK,
      eMessageReplyYes,
      eMessageReplyNo,
      eMessageReplyFailed
    };

    enum MessageTypeEnum
    {
      eMessageFatal,
      eMessageError,
      eMessageMessage,
      eMessageWarning,
      eMessageLog,
      eMessageQuestion
    };
  };
};

#endif
