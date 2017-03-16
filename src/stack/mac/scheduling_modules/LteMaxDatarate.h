/*
 * LteMaxDatarate.h
 */

#ifndef STACK_MAC_SCHEDULING_MODULES_LTEMAXDATARATE_H_
#define STACK_MAC_SCHEDULING_MODULES_LTEMAXDATARATE_H_

#include <LteScheduler.h>
#include "LteCommon.h"

class LteMaxDatarate : public virtual LteScheduler {
public:
    LteMaxDatarate();
    virtual ~LteMaxDatarate();
};

#endif /* STACK_MAC_SCHEDULING_MODULES_LTEMAXDATARATE_H_ */
