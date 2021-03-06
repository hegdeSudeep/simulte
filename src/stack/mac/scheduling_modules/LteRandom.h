/*
 * LteRandom.h
 *
 *  Created on: 10-Jul-2017
 *      Author: Sudeep
 */

#ifndef STACK_MAC_SCHEDULING_MODULES_LTERANDOM_H_
#define STACK_MAC_SCHEDULING_MODULES_LTERANDOM_H_

#include "LteScheduler.h"
#include <random>

class LteRandom : public virtual LteScheduler
{
  public:

   /*
    * Constructor
    */

    LteRandom(){
        activeConnectionSet_.clear();
        numActiveConnections_ = mac_->registerSignal("numActiveConnections");
    }

    virtual void prepareSchedule();

    virtual void commitSchedule();

    // *****************************************************************************************

    //Returns a random number in the range 0 and 'limit'
    int getRandomInt(int limit);

    void notifyActiveConnection(MacCid cid);

    void removeActiveConnection(MacCid cid);

    void updateSchedulingInfo();
};




#endif /* STACK_MAC_SCHEDULING_MODULES_LTERANDOM_H_ */
