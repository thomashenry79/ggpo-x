/* -----------------------------------------------------------------------
 * GGPO.net (http://ggpo.net)  -  Copyright 2009 GroundStorm Studios, LLC.
 *
 * Use of this software is governed by the MIT license that can be found
 * in the LICENSE file.
 */

#ifndef _POLL_H
#define _POLL_H

#include "static_buffer.h"

#define MAX_POLLABLE_HANDLES     64


class IPollSink {
public:
   virtual ~IPollSink() { }
   //virtual bool OnMsgPoll(void*) = 0;//{ return true; }
  // virtual bool OnPeriodicPoll(void*, int) = 0;// { return true; }
   virtual bool OnLoopPoll() = 0;// { return true; }
};

class Poll {
public:
   void RegisterLoop(IPollSink *sink);
   bool Pump();

protected:
   StaticBuffer<IPollSink*, 16>          _loop_sinks;
};

#endif
