/*
 * LteMaxDatarate.cpp
 */

#include <LteMaxDatarate.h>

LteMaxDatarate::LteMaxDatarate() {
    EV_STATICCONTEXT;
    EV << "LteMaxDatarate::constructor" << std::endl;
}

LteMaxDatarate::~LteMaxDatarate() {}

void LteMaxDatarate::prepareSchedule() {
    EV_STATICCONTEXT;
    EV << "LteMaxDatarate::prepareSchedule" << std::endl;

    activeConnectionTempSet_ = activeConnectionSet_;
    EV << "SIZE=" << activeConnectionTempSet_.empty() << std::endl;
    for (ActiveSet::iterator iterator = activeConnectionTempSet_.begin(); iterator != activeConnectionTempSet_.end (); ++iterator) {
        MacCid cid = *iterator;
        MacNodeId nodeId = MacCidToNodeId(cid);
        EV << "MacCid=" << cid << " MacNodeId=" << nodeId << std::endl;
    }
}

void LteMaxDatarate::commitSchedule() {
    EV_STATICCONTEXT;
    EV << "LteMaxDatarate::commitSchedule" << std::endl;
}
