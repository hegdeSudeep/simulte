/*
 * LteMaxDatarate.cpp
 */

#include <LteMaxDatarate.h>
#include <LteSchedulerEnb.h>

LteMaxDatarate::LteMaxDatarate() {
    mOracle = OmniscientEntity::get();
    EV_STATICCONTEXT;
    EV << "\t" << (mOracle == nullptr ? "Couldn't find oracle." : "Found oracle.") << std::endl;
}

LteMaxDatarate::~LteMaxDatarate() {}

void LteMaxDatarate::prepareSchedule() {
    EV_STATICCONTEXT;
    EV << NOW << "LteMaxDatarate::prepareSchedule" << std::endl;

    // Copy currently active connections to a working copy.
    activeConnectionTempSet_ = activeConnectionSet_;
    // Go through all connections.
    for (ActiveSet::iterator iterator = activeConnectionTempSet_.begin(); iterator != activeConnectionTempSet_.end (); ++iterator) {
        MacCid currentConnection = *iterator;
        MacNodeId nodeId = MacCidToNodeId(currentConnection);
        EV << NOW << "\tMacCid=" << currentConnection << " MacNodeId=" << nodeId << std::endl;

        // Make sure the current node has not been dynamically removed.
        if (getBinder()->getOmnetId(nodeId) == 0){
            activeConnectionTempSet_.erase(currentConnection);
            EV << NOW << "\t\t has been dynamically removed. Skipping..." << std::endl;
            continue;
        }

        // Determine direction. Uplink and downlink resources are scheduled separately,
        // and LteScheduler's 'direction_' contains the current scheduling direction.
        Direction dir;
        // Uplink may be reused for D2D, so we have to check if the Buffer Status Report (BSR) tells us that this is a D2D link.
        if (direction_ == UL)
            dir = (MacCidToLcid(currentConnection) == D2D_SHORT_BSR) ? D2D : direction_;
        else
            dir = DL;

//        // Compute available bands.
//        const std::set<Band>& usableBands = eNbScheduler_->mac_->getAmc()->computeTxParams(nodeId,dir).readBands();
//        int maxBands = eNbScheduler_->mac_->getAmc()->getSystemNumBands();
//        EV << "\t\tcan use " << usableBands.size() << " out of " << maxBands << " bands: ";
//        for (std::set<Band>::const_iterator it = usableBands.begin(); it != usableBands.end(); it++)
//            EV << "#" << *it << " " ;
//        EV << std::endl;

        // Determine device's maximum transmission power.
        double maxTransmitPower = mOracle->getTransmissionPower(nodeId, dir);
        // Go through all bands and estimate maximum throughput.
        int maxBands = eNbScheduler_->mac_->getAmc()->getSystemNumBands();
        std::priority_queue<SortedDesc<Band, double>> sortedBands;
        // Contains SINR for each band.
        std::vector<double> SINRs = mOracle->getSINR(nodeId, mOracle->getEnodeBId(), NOW, maxTransmitPower, dir);

//        for (std::set<Band>::const_iterator it = usableBands.begin(); it != usableBands.end(); it++) {
        for (size_t i = 0; i < (unsigned int) maxBands; i++) {
            Band currentBand = Band(i);
            double associatedSinr = SINRs.at(currentBand);
            double estimatedThroghput_cellular = mOracle->getChannelCapacity(associatedSinr);
            SortedDesc<Band, double> bandEntry(currentBand, estimatedThroghput_cellular);
            sortedBands.push(bandEntry);
        }

        EV << NOW << "\t\tBands are now sorted by their estimated throughput:" << std::endl;
        for (size_t i = 0; i < sortedBands.size(); i++) {
            SortedDesc<Band, double> bandEntry = sortedBands.top();
            sortedBands.pop();
            EV << NOW << "\t\t\tBand" << i << ": " << bandEntry.x_ << " has throughput " << bandEntry.score_ << "." << std::endl;
        }
    }
}

void LteMaxDatarate::commitSchedule() {
    EV_STATICCONTEXT;
    EV << "LteMaxDatarate::commitSchedule" << std::endl;
}

void LteMaxDatarate::notifyActiveConnection(MacCid conectionId) {
    EV << NOW << "LteMaxDatarate::notifyActiveConnection(MacCid=" << conectionId << " [MacNodeId=" << MacCidToNodeId(conectionId) << "])" << std::endl;
    activeConnectionSet_.insert(conectionId);
}

void LteMaxDatarate::removeActiveConnection(MacCid conectionId) {
    EV << NOW << "LteMaxDatarate::removeActiveConnection(MacCid=" << conectionId << " [MacNodeId=" << MacCidToNodeId(conectionId) << "])" << std::endl;
    activeConnectionSet_.erase(conectionId);
}
