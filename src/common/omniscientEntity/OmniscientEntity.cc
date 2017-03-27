//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#include <OmniscientEntity.h>

/**
 * Derived class exposes otherwise hidden deployer_ member pointer.
 */
class ExposedLteMacEnb : public LteMacEnb {
public:
    LteDeployer* getDeployer() {
        return LteMacEnb::deployer_;
    }
};

/**
 * Derived class exposes otherwise hidden CQI computation.
 */
class ExposedFeedbackComputer : public LteFeedbackComputationRealistic {
public:
    Cqi getCqi(TxMode txmode, double snr) {
        return LteFeedbackComputationRealistic::getCqi(txmode, snr);
    }
};

OmniscientEntity* OmniscientEntity::SINGLETON = nullptr;

OmniscientEntity::OmniscientEntity()
    : mBinder(getBinder()), // getBinder() provided LteCommon.h
      mSnapshotMsg(new cMessage("OmniscientEnity::snapshot")),
      mConfigMsg(new cMessage("OmniscientEntity::config")),
      mUpdateInterval(0.01) // Will be properly set to provided .NED value in initialize()
    {
    OmniscientEntity::SINGLETON = this;
    }

OmniscientEntity::~OmniscientEntity() {
    if (mMemory != nullptr)
        delete mMemory;
    if (mFeedbackComputer != nullptr)
        delete mFeedbackComputer;
    if (!mConfigMsg->isScheduled())
        delete mConfigMsg;
    if (!mSnapshotMsg->isScheduled())
        delete mSnapshotMsg;
}

double OmniscientEntity::getChannelCapacity(const double sinr) const {
    // Apply Shannon-Hartley theorem.
    double capacity = mBandwidth * log2(1 + sinr);
    return capacity;
}

Cqi OmniscientEntity::getReportedCqi(const MacNodeId from, const MacNodeId to, const uint band, const Direction direction, const Remote antenna, const TxMode transmissionMode) const {
    if (from == to)
        throw cRuntimeError(std::string("OmniscientEntity::getReportedCqi shouldn't be called for from==to!").c_str());
    if (mAmc == nullptr)
        throw cRuntimeError(std::string("OmniscientEntity::getCqi called before the AMC was registered. You should call this method after final configuration is done.").c_str());

    // Cellular link.
    if (from == mENodeBId) {
        return mAmc->getFeedback(to, antenna, transmissionMode, direction).getCqi(0, band);
    } else if (to == mENodeBId) {
        return mAmc->getFeedback(from, antenna, transmissionMode, direction).getCqi(0, band);
    // D2D link.
    } else {
        return mAmc->getFeedbackD2D(from, antenna, transmissionMode, to).getCqi(0, band);
    }
}

Cqi OmniscientEntity::getCqi(const TxMode transmissionMode, const double sinr) const {
    return mFeedbackComputer->getCqi(transmissionMode, sinr);
}

Cqi OmniscientEntity::getCqi(const MacNodeId from, const MacNodeId to, const SimTime time, const TxMode transmissionMode) const {
    if (from == to)
        throw cRuntimeError(std::string("OmniscientEntity::getCqi shouldn't be called for from==to!").c_str());
    // getSINR handles distinction of computing a current value or querying the memory.
    double txPower = (to == mENodeBId ? getTransmissionPower(from, Direction::UL) : getTransmissionPower(from, Direction::D2D));
    double meanSINR = getMean(getSINR(from, to, time, txPower));
    return getCqi(transmissionMode, meanSINR);
}


std::vector<double> OmniscientEntity::getSINR(const MacNodeId from, const MacNodeId to, const SimTime time, const double txPower) const {
    if (from == to)
        throw cRuntimeError(std::string("OmniscientEntity::getSINR shouldn't be called for from==to!").c_str());

    if (time >= NOW) {
        EV << "OmniscientEntity::getSINR computes current value." << std::endl;
        Direction direction = determineDirection(from, to);
        LteAirFrame frame("feedback_pkt");
        UserControlInfo uinfo;
        uinfo.setFrameType(FEEDBACKPKT);
        uinfo.setIsCorruptible(false);
        uinfo.setCoord(getPosition(from));

        uinfo.setSourceId(from);
        uinfo.setD2dTxPeerId(from);
        uinfo.setDestId(to);
        uinfo.setD2dRxPeerId(to);

        uinfo.setDirection(direction);
        uinfo.setTxPower(txPower);
        uinfo.setD2dTxPower(txPower);
        return mChannelModel->getSINR_D2D(&frame, &uinfo, to, getPosition(to), mENodeBId);
    } else {
        EV << "OmniscientEntity::getSINR fetches value from memory." << std::endl;
        return mMemory->get(time, from, to);
    }
}

std::vector<double> OmniscientEntity::getSINROld(MacNodeId from, MacNodeId to, const SimTime time, const double transmissionPower, const Direction direction) const {
    if (from == to)
        throw cRuntimeError(std::string("OmniscientEntity::getSINR shouldn't be called for from==to!").c_str());
    // Make sure eNodeB is the target.
//    if (from == mENodeBId) {
//        MacNodeId temp = from;
//        from = to;
//        to = temp;
//    }
    // Compute current value.
    if (time >= NOW) {
        EV << "OmniscientEntity::getSINR computes current value." << std::endl;
        LteAirFrame* frame = new LteAirFrame("feedback_pkt");
        UserControlInfo* uinfo = new UserControlInfo();
        uinfo->setFrameType(FEEDBACKPKT);
        uinfo->setIsCorruptible(false);
        uinfo->setSourceId(from);
        uinfo->setDirection(direction);
        uinfo->setTxPower(transmissionPower);
        uinfo->setDestId(to);
        std::vector<double> SINRs;

        // UE <-> eNodeB cellular channel.
        if (to == mENodeBId) {
            // I am pretty sure that there's a mistake in the simuLTE source code.
            // It incorrectly sets the eNodeB's position for its calculations. I am here setting the UE's position as the eNodeB's,
            // so that the distance is correct which is all that matters. (or maybe this is how you're supposed to do it...)
//            uinfo->setCoord(mENodeBPosition);
            uinfo->setCoord(getPosition(from));


            SINRs = mChannelModel->getSINR(frame, uinfo);
        // UE <-> UE D2D channel.
        } else {
            uinfo->setCoord(getPosition(from));
            uinfo->setD2dTxPeerId(from);
            uinfo->setD2dRxPeerId(to);
            uinfo->setD2dTxPower(transmissionPower);

            SINRs = mChannelModel->getSINR_D2D(frame, uinfo, to, getPosition(to), mENodeBId);
        }

        delete uinfo;
        delete frame;
        return SINRs;
    // Retrieve from memory.
    } else {
        EV << "OmniscientEntity::getSINR fetches value from memory." << std::endl;
        return mMemory->get(time, from, to);
    }
}

//std::vector<double> OmniscientEntity::getSINR(MacNodeId from, MacNodeId to, SimTime time) const {
//    // Determine direction.
//    Direction dir = determineDirection(from, to);
//
//    // Make sure eNodeB is the target so that getDeviceInfo() is not given the eNodeB's ID.
//    if (from == mENodeBId) {
//        MacNodeId temp = from;
//        from = to;
//        to = temp;
//    }
//
//    double transmissionPower = getTransmissionPower(from, dir);
//    return getSINR(from, to, time, transmissionPower, dir);
//}

IMobility* OmniscientEntity::getMobility(const MacNodeId& device) const {
    cModule *host = nullptr;
    try {
        UeInfo* ueInfo = getDeviceInfo(device);
        host = ueInfo->ue;
    } catch (const cRuntimeError &e) {
        // Getting the mobility of an eNodeB?
        EnbInfo* enbInfo = getENodeBInfo(device);
        host = enbInfo->eNodeB;
    }
    if (host == nullptr)
        throw cRuntimeError(std::string("OmniscientEntity::getMobility couldn't find the device's cModule!").c_str());
    IMobility* mobilityModule = check_and_cast<IMobility*>(host->getSubmodule("mobility"));
    if (mobilityModule == nullptr)
        throw cRuntimeError(std::string("OmniscientEntity::getMobility couldn't find a mobility module!").c_str());
    return mobilityModule;
}

Coord OmniscientEntity::getPosition(const MacNodeId& device) const {
    return getMobility(device)->getCurrentPosition();
}

/**
 * @param device The device's node ID.
 * @return The device's physical current speed. Coord.{x,y,z} are publicly available.
 */
Coord OmniscientEntity::getSpeed(const MacNodeId& device) const {
    return getMobility(device)->getCurrentSpeed();
}

double OmniscientEntity::getMean(const std::vector<double>& values) const {
    double sum = 0.0;
    for (size_t i = 0; i < values.size(); i++)
        sum += values.at(i);
    return (sum / ((double) values.size()));
}

Direction OmniscientEntity::determineDirection(const MacNodeId& from, const MacNodeId& to) const {
    Direction dir;
    if (from == mENodeBId)
        dir = Direction::DL; // eNodeB -DL-> UE.
    else if (to == mENodeBId)
        dir = Direction::UL; // UE -UL-> eNodeB.
    else
        dir = Direction::D2D; // UE -D2D-> UE.
    return dir;
}

double OmniscientEntity::getTransmissionPower(const MacNodeId& device, Direction dir) const {
    double transmissionPower;
    std::string modulePath = getDeviceInfo(device)->ue->getFullPath() + ".nic.phy";
    cModule *mod = getModuleByPath(modulePath.c_str()); // Get a pointer to the device's module.
    try {
        if (dir == Direction::D2D)
            transmissionPower = mod->par("d2dTxPower");
        else
            transmissionPower = mod->par("ueTxPower");
    } catch (const cRuntimeError& e) {
        // @TODO This error shouldn't happen! If there's more than one UE in UED2DTX array, then for the second one there's no parameter 'd2dTxPower'.
        EV_ERROR << "OmniscientEntity::getTransmissionPower(MacNodeId= " << device << ") encountered exception:" << e.what()
                << std::endl << " Using default value \"transmissionPower=24.14973348\"" << std::endl;
        transmissionPower = 24.14973348;
    }
    return transmissionPower;
}

MacNodeId OmniscientEntity::getEnodeBId() const {
    return mENodeBId;
}

const std::map<MacNodeId, std::map<MacNodeId, LteD2DMode>>* OmniscientEntity::getModeSelectionMap() const {
    return mModeSelectionMap;
}

void OmniscientEntity::setModeSelectionMap(const std::map<MacNodeId, std::map<MacNodeId, LteD2DMode>>* map) {
    mModeSelectionMap = map;
}

double euclideanDistance(Coord from, Coord to) {
    return sqrt(pow(from.x - to.x, 2) + pow(from.y - to.y, 2) + pow(from.z - to.z, 2));
}

int OmniscientEntity::getNumberOfBands() const {
    if (mAmc == nullptr)
        throw cRuntimeError("OmniscientEntity::getNumberOfBands called before it could set the AMC reference!");
    return mAmc->getSystemNumBands();
}

void OmniscientEntity::initialize() {
    EV << "OmniscientEntity::initialize" << std::endl;
    // This entity is being initialized before a lot of other entities, like the eNodeBs and UEs, are deployed.
    // That's why final configuration needs to take place a bit later.
    mConfigTimepoint = par("configTimepoint").doubleValue();
    mUpdateInterval = par("updateInterval").doubleValue();
    mBandwidth = par("resourceBlockBandwidth").doubleValue();
    mMemory = new Memory(mUpdateInterval, 15.0, this);
    scheduleAt(mConfigTimepoint, mConfigMsg);
    // Schedule first update.
    scheduleAt(mConfigTimepoint + mUpdateInterval, mSnapshotMsg);
}

/**
 * Final configuration at some time point when other network devices are deployed and accessible.
 */
void OmniscientEntity::configure() {
    EV << "OmniscientEntity::configure" << std::endl;
    // Get the eNodeB.
    std::vector<EnbInfo*>* enbInfo = getEnbInfo();
    if (enbInfo->size() == 0)
        throw cRuntimeError("OmniscientEntity::configure can't get AMC pointer because I couldn't find an eNodeB!");
    // -> its ID -> the node -> cast to the eNodeB class
    mENodeBId = enbInfo->at(0)->id;
    ExposedLteMacEnb *eNodeB = (ExposedLteMacEnb*) getMacByMacNodeId(mENodeBId);
    // -> get the AMC.
    mAmc = eNodeB->getAmc();
    if (mAmc == nullptr)
        throw cRuntimeError("OmniscientEntity::configure couldn't find an AMC.");
    else
        EV << "\tFound AMC." << endl;

    // Remember eNodeB position.
    mENodeBPosition = getPosition(mENodeBId);

    // Get deployer pointer.
    mDeployer = eNodeB->getDeployer();
    if (mDeployer == nullptr)
        throw cRuntimeError("OmniscientEntity::configure couldn't find the deployer.");
    EV << "\tFound deployer." << endl;

    // Print info about all network devices.
    // UEs...
    std::vector<UeInfo*>* ueInfo = getUeInfo();
    EV << "\tThere are " << ueInfo->size() << " UEs in the network: " << std::endl;
    for (size_t i = 0; i < ueInfo->size(); i++) {
        Coord position = getPosition(ueInfo->at(i)->id);
        EV << "\t\t#" << i+1 << ": has MacNodeId " << ueInfo->at(i)->id << " and OmnetID " << getId(ueInfo->at(i)->id)
           << " and sits at position (" << position.x << ", " << position.y << ")." << std::endl;
    }
    // eNodeB...
    std::vector<EnbInfo*>* EnbInfo = getEnbInfo();
    EV << "\tThere are " << EnbInfo->size() << " EnBs in the network: " << std::endl;
    for (size_t i = 0; i < EnbInfo->size(); i++) {
        Coord position = getPosition(EnbInfo->at(i)->id);
        EV << "\t\t#" << i+1 << ": has MacNodeId " << EnbInfo->at(i)->id << " and OmnetID " << getId(EnbInfo->at(i)->id)
           << " and sits at position (" << position.x << ", " << position.y << ")." << std::endl;
    }

    // Get a pointer to the channel model.
    mChannelModel = ueInfo->at(0)->realChan;
    if (mChannelModel != nullptr)
        EV << "\tFound channel model." << std::endl;
    else
        throw cRuntimeError("OmniscientEntity::configure couldn't find a channel model.");

    // Construct a feedback computer.
    mFeedbackComputer = getFeedbackComputation();
    if (mFeedbackComputer == nullptr)
        throw cRuntimeError("OmniscientEntity::configure couldn't construct the feedback computer.");
    else
        EV << "\tConstructed feedback computer." << endl;

    EV << "Testing Computations..." << std::endl;
    std::vector<double> tx1Vec, tx2Vec, rx1Vec, rx2Vec;
    MacNodeId tx1 = ueInfo->at(0)->id, tx2 = ueInfo->at(1)->id,
                rx1 = ueInfo->at(2)->id, rx2 = ueInfo->at(3)->id;
    double txPower_d2d = getTransmissionPower(tx1, Direction::D2D),
           txPower_cellular = getTransmissionPower(tx1, Direction::UL);
    tx1Vec.push_back(getMean(getSINR(tx1, mENodeBId, NOW, txPower_cellular)));
    tx1Vec.push_back(getMean(getSINR(tx1, tx2, NOW, txPower_d2d)));
    tx1Vec.push_back(getMean(getSINR(tx1, rx1, NOW, txPower_d2d)));
    tx1Vec.push_back(getMean(getSINR(tx1, rx2, NOW, txPower_d2d)));

    tx2Vec.push_back(getMean(getSINR(tx2, mENodeBId, NOW, txPower_cellular)));
    tx2Vec.push_back(getMean(getSINR(tx2, tx1, NOW, txPower_d2d)));
    tx2Vec.push_back(getMean(getSINR(tx2, rx1, NOW, txPower_d2d)));
    tx2Vec.push_back(getMean(getSINR(tx2, rx2, NOW, txPower_d2d)));

    rx1Vec.push_back(getMean(getSINR(rx1, mENodeBId, NOW, txPower_cellular)));
    rx1Vec.push_back(getMean(getSINR(rx1, tx1, NOW, txPower_d2d)));
    rx1Vec.push_back(getMean(getSINR(rx1, tx2, NOW, txPower_d2d)));
    rx1Vec.push_back(getMean(getSINR(rx1, rx2, NOW, txPower_d2d)));

    rx2Vec.push_back(getMean(getSINR(rx2, mENodeBId, NOW, txPower_cellular)));
    rx2Vec.push_back(getMean(getSINR(rx2, tx1, NOW, txPower_d2d)));
    rx2Vec.push_back(getMean(getSINR(rx2, tx2, NOW, txPower_d2d)));
    rx2Vec.push_back(getMean(getSINR(rx2, rx1, NOW, txPower_d2d)));

    EV << "CALCTEST Tx1->eNB=" << tx1Vec.at(0) << std::endl;
    EV << "CALCTEST Tx1->Tx2=" << tx1Vec.at(1) << std::endl;
    EV << "CALCTEST Tx1->Rx1=" << tx1Vec.at(2) << std::endl;
    EV << "CALCTEST Tx1->Rx2=" << tx1Vec.at(3) << std::endl;

    EV << "CALCTEST Tx2->eNB=" << tx2Vec.at(0) << std::endl;
    EV << "CALCTEST Tx2->Tx1=" << tx2Vec.at(1) << std::endl;
    EV << "CALCTEST Tx2->Rx1=" << tx2Vec.at(2) << std::endl;
    EV << "CALCTEST Tx2->Rx2=" << tx2Vec.at(3) << std::endl;

    EV << "CALCTEST Rx1->eNB=" << rx1Vec.at(0) << std::endl;
    EV << "CALCTEST Rx1->Tx1=" << rx1Vec.at(1) << std::endl;
    EV << "CALCTEST Rx1->Tx2=" << rx1Vec.at(2) << std::endl;
    EV << "CALCTEST Rx1->Rx2=" << rx1Vec.at(3) << std::endl;

    EV << "CALCTEST Rx2->eNB=" << rx2Vec.at(0) << std::endl;
    EV << "CALCTEST Rx2->Tx1=" << rx2Vec.at(1) << std::endl;
    EV << "CALCTEST Rx2->Tx2=" << rx2Vec.at(2) << std::endl;
    EV << "CALCTEST Rx2->Rx1=" << rx2Vec.at(3) << std::endl;
//    for (size_t i = 0; i < ueInfo->size(); i++) {
//        MacNodeId from = ueInfo->at(i)->id;
////        for (size_t j = 0; j < ueInfo->size(); j++) {
////            if (i == j)
////                continue;
////            MacNodeId to = ueInfo->at(j)->id;
////            double txPower_d2d = getTransmissionPower(from, Direction::D2D);
////            double sinr_d2d = getMean(getSINR(from, to, NOW, txPower_d2d, Direction::D2D));
////            EV << "CALCTEST: SINR(UE[" << i << ", id=" << ueInfo->at(i)->id << "]-D2D->" << "UE[" << j << ", id=" << ueInfo->at(j)->id << ") = " << sinr_d2d << std::endl;
////            EV << "CALCTEST: d(UE[" << i << ", id=" << ueInfo->at(i)->id << "], UE[" << j << ", id=" << ueInfo->at(j)->id << "]) = " << euclideanDistance(getPosition(from), getPosition(to)) << std::endl;
////        }
//        double txPower_cellular = getTransmissionPower(from, Direction::UL);
//        double sinr_cellular = getMean(getSINR(from, mENodeBId, NOW, txPower_cellular, Direction::UL));
//        EV << "CALCTEST: SINR(UE[" << i << ", id=" << ueInfo->at(i)->id << "]-UL->" << "eNB[0, id=" << mENodeBId << "]) = " << sinr_cellular << std::endl;
//        EV << "CALCTEST: d(UE[" << i << ", id=" << ueInfo->at(i)->id << "], eNB[0, id=" << mENodeBId << "]) = " << euclideanDistance(getPosition(from), getPosition(mENodeBId)) << std::endl;
//    }

    // Test the different interfaces.
//        MacNodeId from = ueInfo->at(0)->id,
//                  to = ueInfo->at(1)->id;
//        EV << "Testing CQI(UE" << from << ", UE" << to << ")" << std::endl;
//        Cqi cqi_computed_d2d = getCqi(from, to, NOW, TxMode::SINGLE_ANTENNA_PORT0),
//            cqi_reported_d2d = getReportedCqi(from, to, 0, Direction::D2D, Remote::MACRO, TxMode::SINGLE_ANTENNA_PORT0),
//            cqi_computed_cellular_UL = getCqi(from, mENodeBId, NOW, TxMode::SINGLE_ANTENNA_PORT0),
//            cqi_reported_cellular_UL = getReportedCqi(from, mENodeBId, 0, Direction::UL, Remote::MACRO, TxMode::SINGLE_ANTENNA_PORT0),
//            cqi_computed_cellular_DL = getCqi(mENodeBId, to, NOW, TxMode::SINGLE_ANTENNA_PORT0),
//            cqi_reported_cellular_DL = getReportedCqi(mENodeBId, to, 0, Direction::DL, Remote::MACRO, TxMode::SINGLE_ANTENNA_PORT0);
//        EV << "CQI_computed_d2d=" << cqi_computed_d2d << " CQI_reported_d2d=" << cqi_reported_d2d << std::endl;
//        EV << "CQI_computed_cellular_UL=" << cqi_computed_cellular_UL << " CQI_reported_cellular_UL=" << cqi_reported_cellular_UL << std::endl;
//        EV << "CQI_computed_cellular_DL=" << cqi_computed_cellular_DL << " CQI_reported_cellular_DL=" << cqi_reported_cellular_DL << std::endl;
//        if (cqi_computed_d2d != cqi_reported_d2d || cqi_computed_cellular_UL != cqi_reported_cellular_UL || cqi_computed_cellular_DL != cqi_reported_cellular_DL)
//            throw cRuntimeError(std::string("OmniscientEntity::configure didn't find the same values for reported and calculated CQI!").c_str());
//        double sinr_d2d = getMean(getSINR(from, to, NOW));
//        double sinr_ul = getMean(getSINR(from, mENodeBId, NOW));
//        double sinr_dl = getMean(getSINR(mENodeBId, from, NOW));
//        EV << "SINR_d2d=" << sinr_d2d << std::endl;
//        EV << "SINR_UL=" << sinr_ul << std::endl;
//        EV << "SINR_DL=" << sinr_dl << std::endl;
}

void OmniscientEntity::handleMessage(cMessage *msg) {
    EV << "OmniscientEntity::handleMessage" << std::endl;
    if (msg == mSnapshotMsg)
        snapshot();
    else if (msg == mConfigMsg)
        configure();
}

void OmniscientEntity::snapshot() {
    EV << "OmniscientEntity::snapshot" << std::endl;
    std::vector<UeInfo*>* ueInfo = getUeInfo();
    // For all UEs...
    for (size_t i = 0; i < ueInfo->size(); i++) {
        MacNodeId from = ueInfo->at(i)->id;
        // Find SINR to the eNodeB (cellular uplink).
        std::vector<double> sinrs_eNodeB = getSINR(from, mENodeBId, NOW, getTransmissionPower(from, Direction::UL));
        mMemory->put(NOW, from, mENodeBId, sinrs_eNodeB);
        // And for all other UEs...
        for (size_t j = 0; j < ueInfo->size(); j++) {
            MacNodeId to = ueInfo->at(j)->id;
            // Ignore link to current node.
            if (from == to)
                continue;
            // Calculate and save the current SINR for the D2D link.
            std::vector<double> sinrs = getSINR(from, to, NOW, getTransmissionPower(from, Direction::D2D));
            mMemory->put(NOW, from, to, sinrs);
        }
    }

    // Print current memory.
//    EV << mMemory->toString() << std::endl;

    // Schedule next snapshot.
    scheduleAt(NOW + mUpdateInterval, mSnapshotMsg);
}

std::vector<EnbInfo*>* OmniscientEntity::getEnbInfo() const {
    return mBinder->getEnbList();
}
std::vector<UeInfo*>* OmniscientEntity::getUeInfo() const {
    return mBinder->getUeList();
}

OmnetId OmniscientEntity::getId(const MacNodeId id) const {
    return mBinder->getOmnetId(id);
}
MacNodeId OmniscientEntity::getId(const OmnetId id) const {
    return mBinder->getMacNodeIdFromOmnetId(id);
}

UeInfo* OmniscientEntity::getDeviceInfo(const MacNodeId device) const {
  std::vector<UeInfo*>* ueInfo = getUeInfo();
  for (size_t i = 0; i < ueInfo->size(); i++) {
    UeInfo* currentInfo = ueInfo->at(i);
    if (currentInfo->id == device)
      return currentInfo;
  }
  throw cRuntimeError(std::string("OmniscientEntity::getDeviceInfo can't find the requested device ID \"" + std::to_string(device) + "\"").c_str());
}

EnbInfo* OmniscientEntity::getENodeBInfo(MacNodeId id) const {
  std::vector<EnbInfo*>* enbInfo = getEnbInfo();
  for (size_t i = 0; i < enbInfo->size(); i++) {
      EnbInfo* currentInfo = enbInfo->at(i);
    if (currentInfo->id == id)
      return currentInfo;
  }
  throw cRuntimeError("OmniscientEntity::getENodeBInfo can't find the requested eNodeB ID!");
}

ExposedFeedbackComputer* OmniscientEntity::getFeedbackComputation() const {
    // We're construction the feedback computer from a description.
    // There's REAL and DUMMY. We want REAL.
    std::string feedbackName = "REAL";
    // The four needed parameters will be supplied in this map.
    std::map<std::string, cMsgPar> parameterMap;

    // Each one must be specified in the OmniscientEntity.ned.
    // @TODO Parse the channel.xml instead because that's the values we want.
    cMsgPar targetBler("targetBler");
    targetBler.setDoubleValue(par("targetBler"));
    parameterMap["targetBler"] = targetBler;

    cMsgPar lambdaMinTh("lambdaMinTh");
    lambdaMinTh.setDoubleValue(par("lambdaMinTh"));
    parameterMap["lambdaMinTh"] = lambdaMinTh;

    cMsgPar lambdaMaxTh("lambdaMaxTh");
    lambdaMaxTh.setDoubleValue(par("lambdaMaxTh"));
    parameterMap["lambdaMaxTh"] = lambdaMaxTh;

    cMsgPar lambdaRatioTh("lambdaRatioTh");
    lambdaRatioTh.setDoubleValue(par("lambdaRatioTh"));
    parameterMap["lambdaRatioTh"] = lambdaRatioTh;

    // Taken from simulte/src/stack/phy/layer/LtePhyEnb.cc line 415.
    return ((ExposedFeedbackComputer*) new LteFeedbackComputationRealistic(
            targetBler, mDeployer->getLambda(), lambdaMinTh, lambdaMaxTh,
            lambdaRatioTh, mDeployer->getNumBands()));
}

std::string OmniscientEntity::vectorToString(const std::vector<double>& vec, const std::string& name) const {
    std::string description = "";
    for (size_t i = 0; i < vec.size(); i++)
        description += name + "[" + std::to_string(i) + "]=" + std::to_string(vec.at(i)) + " ";
    return description;
}

