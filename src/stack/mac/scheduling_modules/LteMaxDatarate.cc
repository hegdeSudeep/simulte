/*
 * LteMaxDatarate.cpp
 */

#include <LteMaxDatarate.h>
#include <LteSchedulerEnb.h>

LteMaxDatarate::LteMaxDatarate() {
    EV_STATICCONTEXT;
    EV << "LteMaxDatarate::constructor" << std::endl;
}

LteMaxDatarate::~LteMaxDatarate() {}

void LteMaxDatarate::prepareSchedule() {
    EV_STATICCONTEXT;
    EV << "LteMaxDatarate::prepareSchedule" << std::endl;

    activeConnectionTempSet_ = activeConnectionSet_;
    EV << "EMPTY=" << activeConnectionTempSet_.empty() << std::endl;
    EV <<"RBs available: " << eNbScheduler_->readTotalAvailableRbs() << std::endl;
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

void LteMaxDatarate::notifyActiveConnection(MacCid cid) {
    EV << NOW << "LteMaxDatarate::notifyActiveConnection(MacCid=" << cid<< " [MacNodeId=" << MacCidToNodeId(cid) << "])" << std::endl;
    activeConnectionSet_.insert(cid);
}

void LteMaxDatarate::removeActiveConnection(MacCid cid) {
    EV << NOW << "LteMaxDatarate::removeActiveConnection(MacCid=" << cid<< " [MacNodeId=" << MacCidToNodeId(cid) << "])" << std::endl;
    activeConnectionSet_.erase(cid);
}
