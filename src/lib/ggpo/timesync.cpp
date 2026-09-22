/* -----------------------------------------------------------------------
 * GGPO.net (http://ggpo.net)  -  Copyright 2009 GroundStorm Studios, LLC.
 *
 * Use of this software is governed by the MIT license that can be found
 * in the LICENSE file.
 */

#include "timesync.h"
#include <string>
#include <vector>
TimeSync::TimeSync() : _local(), _remote()
{
}

TimeSync::~TimeSync()
{
}
void TimeSync::SetFrameDelay(int frame)
{
    _frameDelay2 = frame;
}
void
TimeSync::advance_frame(GameInput &input, float advantage, float radvantage)
{
    advantage -= _remoteFrameDelay;
    radvantage *= -1;
   // Remember the last frame and frame advantage
    _local[input.frame % ARRAY_SIZE(_local)] = advantage;
   _remote[input.frame % ARRAY_SIZE(_remote)] = radvantage;
   
  
   _avgLocal = ((nFrame * _avgLocal) + advantage) / (nFrame + 1);
   _avgRemote = ((nFrame * _avgRemote) + radvantage) / (nFrame + 1);
  
   nFrame++;   
   //Clear after first 3 seconds, as this is a bit crazy
   if (!clearedInitial && nFrame == 300)
   {
       _avgLocal = _avgRemote = 0;
       clearedInitial = true;
       nFrame = 0;
   }

}

#include <algorithm>
template<typename T>
float median(const T& floatyCollection)
{
    std::vector<float> sorted(std::begin(floatyCollection), std::end(floatyCollection));
    std::sort(sorted.begin(), sorted.end());
    return sorted[sorted.size() / 2];
}

float TimeSync::LocalAdvantage() const
{
 /*   float advantage=0;
    for (auto local : _local) {
        advantage += local;
    }
    advantage /=(float)(_local).size();
    return (advantage);*/
    return median(_local);
}

float TimeSync::RemoteAdvantage() const
{
    float advantage = 0;
    for (auto local : _remote) {
        advantage += local;
    }
    advantage /= (float)(_remote).size();
    return (advantage);
}

float
TimeSync::recommend_frame_wait_duration(bool )
{
   
   auto advantage = LocalAdvantage();

   auto radvantage = RemoteAdvantage();

   
   float sleep_frames = -(((radvantage + advantage) / 2.0f));
   if (abs(advantage - radvantage) > 1.0f)
       return 0.0f;
   if (advantage > 0.75f && radvantage > 0.75f)
       return sleep_frames;
   if (advantage < 0.75f && radvantage < 0.75f)
       return sleep_frames;
   return 0.0f;

   // See if someone should take action.  The person furthest ahead
   // needs to slow down so the other user can catch up.
   // Only do this if both clients agree on who's ahead!!
    
 
 //  if (advantage  >= radvantage) {
      
   //   return 0;
  // }
  // float sleep_frames = -(((radvantage + advantage) / 2.0f));


//return sleep_frames > 0  ? (float)MIN(sleep_frames, MAX_FRAME_ADVANTAGE) : (float)MAX(sleep_frames, -MAX_FRAME_ADVANTAGE);
}
