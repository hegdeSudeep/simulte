/*
 * LteMaxDatarate.cpp
 */

#include <LteMaxDatarate.h>
#include <LteSchedulerEnb.h>

LteMaxDatarate::LteMaxDatarate() {
    mOracle = OmniscientEntity::get();
    EV_STATICCONTEXT;
    EV << "LteMaxDatarate::constructor" << std::endl;
    EV << "\t" << (mOracle == nullptr ? "Couldn't find oracle." : "Found oracle.") << std::endl;
    if (mOracle == nullptr)
        throw cRuntimeError("LteMaxDatarate couldn't find the oracle.");
}

LteMaxDatarate::~LteMaxDatarate() {}

MacNodeId LteMaxDatarate::determineD2DEndpoint(MacNodeId srcNode) const {
    EV_STATICCONTEXT;
    EV << NOW << " LteMaxDatarate::determineD2DEndpoint" << std::endl;
    const std::map<MacNodeId, std::map<MacNodeId, LteD2DMode>>* map = mOracle->getModeSelectionMap();
    if (map == nullptr)
        throw cRuntimeError("LteMaxDatarate::determineD2DEndpoint couldn't get a mode selection map from the oracle.");
    const std::map<MacNodeId, LteD2DMode> destinationModeMap = map->at(srcNode);
    if (destinationModeMap.size() <= 0)
        throw cRuntimeError(std::string("LteMaxDatarate::determineD2DEndpoint couldn't find an end point from node " + std::to_string(srcNode)).c_str());
    MacNodeId dstNode = 0;
    bool foundIt = false;
    for (std::map<MacNodeId, LteD2DMode>::const_iterator iterator = destinationModeMap.begin();
            iterator != destinationModeMap.end(); iterator++) {
        if ((*iterator).second == Direction::D2D) {
            foundIt = true;
            dstNode = (*iterator).first;
            break;
        }
    }
    if (!foundIt)
        throw cRuntimeError(std::string("LteMaxDatarate::determineD2DEndpoint couldn't find an end point from node " + std::to_string(srcNode)).c_str());
    EV << NOW << " LteMaxDatarate::determineD2DEndpoint found " << srcNode << " --> " << dstNode << "." << std::endl;
    return dstNode;
}

void LteMaxDatarate::prepareSchedule() {
    EV_STATICCONTEXT;
    EV << NOW << "LteMaxDatarate::prepareSchedule" << std::endl;

    // Copy currently active connections to a working copy.
    activeConnectionTempSet_ = activeConnectionSet_;
    // Go through all connections.
    for (ActiveSet::iterator iterator = activeConnectionTempSet_.begin(); iterator != activeConnectionTempSet_.end (); ++iterator) {
        MacCid currentConnection = *iterator;
        MacNodeId nodeId = MacCidToNodeId(currentConnection);

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
        // We want to find the channel capacity for all bands.
        int maxBands = eNbScheduler_->mac_->getAmc()->getSystemNumBands();
        // The priority_queue auto-sorts.
        std::priority_queue<SortedDesc<Band, double>> sortedBands;
        // We need the SINR values for each band.
        std::vector<double> SINRs;
        switch (dir) {
            case Direction::UL: {
                SINRs = mOracle->getSINR(nodeId, mOracle->getEnodeBId(), NOW, maxTransmitPower);
                EV << "From node " << nodeId << " to eNodeB " << mOracle->getEnodeBId() << " (Uplink)" << std::endl;
                break;
            }
            case Direction::DL: {
                SINRs = mOracle->getSINR(mOracle->getEnodeBId(), nodeId , NOW, maxTransmitPower);
                EV << "From eNodeB " << mOracle->getEnodeBId() << " to node " << nodeId << " (Downlink)" << std::endl;
                break;
            }
            case Direction::D2D: {
                MacNodeId destinationId = determineD2DEndpoint(nodeId);
                SINRs = mOracle->getSINR(nodeId, destinationId, NOW, maxTransmitPower);
                EV << "From node " << nodeId << " to node " << destinationId << " (Direct Link)" << std::endl;
                break;
            }
            default: {
                // This can't happen, dir is specifically set above. Just for the sake of completeness.
                throw cRuntimeError(std::string("LteMaxDatarate::prepareSchedule sees undefined direction: " + std::to_string(dir)).c_str());
            }
        }

//        for (std::set<Band>::const_iterator it = usableBands.begin(); it != usableBands.end(); it++) {
        // Go through all bands ...
        for (size_t i = 0; i < (unsigned int) maxBands; i++) {
            Band currentBand = Band(i);
            // Estimate cellular throughput.
            double associatedSinr = SINRs.at(currentBand);
            double estimatedThroughput = mOracle->getChannelCapacity(associatedSinr);
            SortedDesc<Band, double> bandEntry(currentBand, estimatedThroughput);
            sortedBands.push(bandEntry);
        }

        EV << NOW << "\t\tBands are now sorted by their estimated throughput:" << std::endl;
        for (size_t i = 0; i < sortedBands.size(); i++) {
            SortedDesc<Band, double> bandEntry = sortedBands.top();
            sortedBands.pop();
            EV << NOW << "\t\t\tBand " << bandEntry.x_ << " has throughput " << bandEntry.score_ << "." << std::endl;
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
