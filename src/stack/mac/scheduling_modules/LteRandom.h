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

    virtual void prepareSchedule();

    virtual void commitSchedule();

    // *****************************************************************************************

    void notifyActiveConnection(MacCid cid);

    void removeActiveConnection(MacCid cid);

    void updateSchedulingInfo();
};




#endif /* STACK_MAC_SCHEDULING_MODULES_LTERANDOM_H_ */
