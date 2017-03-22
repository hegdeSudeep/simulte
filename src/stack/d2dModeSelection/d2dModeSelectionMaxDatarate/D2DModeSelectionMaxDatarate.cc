/*
 * D2DModeSelectionMaxDatarate.cpp
 */

#include <d2dModeSelectionMaxDatarate/D2DModeSelectionMaxDatarate.h>

Define_Module(D2DModeSelectionMaxDatarate);

void D2DModeSelectionMaxDatarate::initialize(int stage) {
    D2DModeSelectionBase::initialize(stage);
    EV << NOW << " D2DModeSelectionMaxDatarate::initialize" << std::endl;
    mOracle = OmniscientEntity::get();
    EV << NOW << "\t" << (mOracle == nullptr ? "Couldn't find oracle." : "Found oracle.") << std::endl;
    if (mOracle == nullptr)
        throw cRuntimeError("D2DModeSelectionMaxDatarate couldn't find the oracle.");
    // Make <from, <to, mode>> map visible to OmniscientEntity.
    mOracle->setModeSelectionMap(peeringModeMap_);
}

void D2DModeSelectionMaxDatarate::doModeSelection() {
    EV << NOW << " D2DModeSelectionMaxDatarate::doModeSelection" << endl;

    // The switch list will contain entries of devices whose mode switches.
    // Clear it to start.
    switchList_.clear();
    // Go through all devices.
    std::map<MacNodeId, std::map<MacNodeId, LteD2DMode>>::iterator peeringMapIterator = peeringModeMap_->begin();
    for (; peeringMapIterator != peeringModeMap_->end(); peeringMapIterator++) {
        // Grab source node.
        MacNodeId srcId = peeringMapIterator->first;

        EV << NOW << "\tChecking for starting node " << srcId << std::endl;
        // Make sure node is in this cell.
        if (binder_->getNextHop(srcId) != mac_->getMacCellId()) {
            EV << NOW << "\t\tskipping. It has left the cell." << std::endl;
            continue;
        }

        // Grab handle to <dest, mode> map.
        std::map<MacNodeId, LteD2DMode>::iterator destModeMapIterator = peeringMapIterator->second.begin();
        for (; destModeMapIterator != peeringMapIterator->second.end(); destModeMapIterator++) {
            // Grab destination.
            MacNodeId dstId = destModeMapIterator->first;

            EV << NOW << "\t\t-> " << dstId << " ";

            // Make sure node is in this cell.
            if (binder_->getNextHop(dstId) != mac_->getMacCellId()) {
                EV << "has left the cell." << std::endl;
                continue;
            }

            // Skip nodes that are performing handover.
            if (binder_->hasUeHandoverTriggered(dstId) || binder_->hasUeHandoverTriggered(srcId)) {
                EV << "is performing handover." << std::endl;
                continue;
            }

            // Calculate capacities for both direct (D2D) and cellular channels.
            double txPower_direct = mOracle->getTransmissionPower(srcId, Direction::D2D);
            double sinr_direct = mOracle->getMean(mOracle->getSINR(srcId, dstId, NOW, txPower_direct));
            double capacity_direct = mOracle->getChannelCapacity(sinr_direct);

            double txPower_cellular = mOracle->getTransmissionPower(srcId, Direction::UL);
            double sinr_cellular = mOracle->getMean(mOracle->getSINR(srcId, mOracle->getEnodeBId(), NOW, txPower_cellular));
            double capacity_cellular = mOracle->getChannelCapacity(sinr_cellular);

            EV << NOW << "Node " << srcId << " has sinr_direct=" << sinr_direct << ", capacity_direct=" << capacity_direct << ", sinr_cellular=" << sinr_cellular << ", capacity_cellular=" << capacity_cellular << std::endl;

            // Choose the better one.
            LteD2DMode oldMode = destModeMapIterator->second;
            LteD2DMode newMode = (capacity_cellular > capacity_direct) ? IM : DM; // IM = Infrastructure Mode, DM = Direct Mode
            EV << NOW << "\tNode " << srcId << " will communicate with node " << dstId << ((capacity_cellular > capacity_direct) ? " via the eNodeB." : " directly.") << std::endl;

            if (newMode != oldMode) {
                // Mark this flow as switching modes.
                FlowId p(srcId, dstId);
                FlowModeInfo info;
                info.flow = p;
                info.oldMode = oldMode;
                info.newMode = newMode;
                switchList_.push_back(info);
                // Update peering map.
                destModeMapIterator->second = newMode;
                EV << NOW << "\tFlow: " << srcId << " --> " << dstId << " switches to " << d2dModeToA(newMode) << " mode." << endl;
            }
        }
    }
}
