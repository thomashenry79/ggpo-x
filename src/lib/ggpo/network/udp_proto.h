/* -----------------------------------------------------------------------
 * GGPO.net (http://ggpo.net)  -  Copyright 2009 GroundStorm Studios, LLC.
 *
 * Use of this software is governed by the MIT license that can be found
 * in the LICENSE file.
 */

#ifndef _UDP_PROTO_H_
#define _UDP_PROTO_H_

#include "poll.h"
#include "udp.h"
#include "udp_msg.h"
#include "game_input.h"
#include "timesync.h"
#include "ggponet.h"
#include "ring_buffer.h"
#include <functional>
#include <vector>
#include <string>
#include <map>
#include <memory>
class UdpProtocol : public IPollSink
{
public:
   struct Stats {
      int                 ping;
      float                 remote_frame_advantage;
      float                 local_frame_advantage;
      float                 av_remote_frame_advantage;
      float                 av_local_frame_advantage;
      int                 send_queue_len;
      Udp::Stats          udp;
   };

   struct Event {
      enum class Type {
         Unknown = -1,
         Connected,
         Synchronizing,
         Synchronzied,
         Input,
         Disconnected,
         NetworkInterrupted,
         NetworkResumed,
         NetworkError,
      };

      Type      type;
      union {
         struct {
            GameInput   input;
         } input;
         struct {
            int         total;
            int         count;
         } synchronizing;
         struct {
            int         disconnect_timeout;
         } network_interrupted;
         struct {
             int errorCode;
         } network_error;
         struct {
             int localFrameDelay;
             int remoteFrameDelay;
         } syncInfo;
      } u;

      Event(Event::Type t = Event::Type::Unknown) : type(t) { };
   };

public:
   virtual bool OnLoopPoll() override;

public:
   UdpProtocol();
   virtual ~UdpProtocol();

   void Init(Udp *udp, Poll &p, int queue, char *ip, u_short port, UdpMsg::connect_status *status,float fps);

   void Synchronize();
   bool GetPeerConnectStatus(int id, int *frame);
   bool IsInitialized() { return _udp != NULL; }
   bool IsSynchronized() { return _current_state == Running; }
   bool IsRunning() { return _current_state == Running; }
   void SendInput(GameInput &input);
   void SendChat(const char* message);
   void SendInputAck();
   bool HandlesMsg(sockaddr_in &from, UdpMsg *msg);
   void OnMsg(UdpMsg *msg, int len);
   void Disconnect();
  
   void GetNetworkStats(struct GGPONetworkStats *stats);
   bool GetEvent(UdpProtocol::Event &e);
   void SetLocalFrameNumber(int num);
   float RecommendFrameDelay();
   int RemoteFrameDelay()const;
   void SetDisconnectTimeout(int timeout);
   void SetDisconnectNotifyStart(int timeout);
   void SetFrameDelay(int delay);
   void ConsumeChat(std::function<void(const char*)> onChat);
   void ApplyToEvents(std::function<void(UdpProtocol::Event&)> cb);
   void StartPollLoop();
   void EndPollLoop();
   std::map<int, uint16> _remoteCheckSums;
   std::map<int, uint16> _remoteCheckSumsThisFrame;
   UdpProtocol(UdpProtocol&&) = default;
protected:
   enum State {
      Syncing,
      Synchronzied,
      Running,
      Disconnected
   };
   struct QueueEntry {
      int         queue_time;
      sockaddr_in dest_addr;
      std::unique_ptr<UdpMsg>      msg;

      QueueEntry() {}
      QueueEntry(int time, sockaddr_in &dst, std::unique_ptr<UdpMsg>&&  m) : queue_time(time), dest_addr(dst), msg(std::move(m)) { }
   };

   void UpdateNetworkStats(void);
   void QueueEvent(const UdpProtocol::Event &evt);
   void ClearSendQueue(void);
   void Log(const char *fmt, ...);
   void LogMsg(const char *prefix, UdpMsg *msg);
   void LogEvent(const char *prefix, const UdpProtocol::Event &evt);
   void SendSyncRequest();
   void SendMsg(std::unique_ptr<UdpMsg>&& msg);
   void PumpSendQueue();
   void SendPendingOutput();
   bool OnInvalid(UdpMsg *msg, int len);
   bool OnSyncRequest(UdpMsg *msg, int len);
   bool OnSyncReply(UdpMsg *msg, int len);
   bool OnInput(UdpMsg *msg, int len);
   bool OnInputAck(UdpMsg *msg, int len);
   bool OnQualityReport(UdpMsg *msg, int len);
   bool OnQualityReply(UdpMsg *msg, int len);
   bool OnKeepAlive(UdpMsg *msg, int len);
   bool OnChat(UdpMsg *msg, int len);
  
protected:
   /*
    * Network transmission information
    */
   Udp            *_udp= nullptr;
   sockaddr_in    _peer_addr; 
   uint16         _magic_number=0;
   int            _queue=-1;
   uint16         _remote_magic_number=0;
   bool           _connected=false;
   int            _send_latency;
   int            _oop_percent;
   struct {
      int         send_time;
      sockaddr_in dest_addr;
      UdpMsg*     msg;
   }              _oo_packet;
   RingBuffer<QueueEntry, 128> _send_queue;

   /*
    * Stats
    */
   double            _round_trip_time = 0;
   int            _packets_sent=0;
   int            _bytes_sent=0;
   int            _kbps_sent=0;
   int            _stats_start_time=0;

   /*
    * The state machine
    */
   UdpMsg::connect_status *_local_connect_status;
   UdpMsg::connect_status _peer_connect_status[UDP_MSG_MAX_PLAYERS];

   State          _current_state;
   union {
      struct {
         uint32   roundtrips_remaining;
         uint32   random;
      } sync;
      struct {
         uint32   last_quality_report_time;
         uint32   last_network_stats_interval;
         uint32   last_input_packet_recv_time;
      } running;
   } _state;

   /*
    * Fairness.
    */
   float               _local_frame_advantage=0;
   float               _remote_frame_advantage=0;
   float _fps;
   /*
    * Packet loss...
    */
   RingBuffer<GameInput, 128>  _pending_output;
   GameInput                  _last_received_input;
   GameInput                  _last_sent_input;
   GameInput                  _last_acked_input;
   unsigned int               _last_send_time=0;
   unsigned int               _last_recv_time=0;
   unsigned int               _shutdown_timeout=0;
   unsigned int               _disconnect_event_sent=0;
   unsigned int               _disconnect_timeout=0;
   unsigned int               _disconnect_notify_start=0;
   bool                       _disconnect_notify_sent=false;

   uint16                     _next_send_seq=0;
   uint16                     _next_recv_seq=0;

   /*
    * Rift synchronization.
    */
   TimeSync                   _timesync;

   /*
    * Event queue
    */
   RingBuffer<UdpProtocol::Event, 128>  _event_queue;
   std::vector<std::string> _chatMessages;
   UdpProtocol(const UdpProtocol&) = delete;
};

#endif
