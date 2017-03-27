/*
 * MaxDatarateSorter.h
 */

#ifndef STACK_MAC_SCHEDULING_MODULES_MAX_DATARATE_MAXDATARATESORTER_H_
#define STACK_MAC_SCHEDULING_MODULES_MAX_DATARATE_MAXDATARATESORTER_H_

#include <vector>
#include <omnetpp.h>
#include <LteCommon.h>

class IdRatePair {
  public:
    IdRatePair(const MacCid& connectionId, const MacNodeId& from, const MacNodeId& to, double txPower, const double rate, const Direction& dir)
        : connectionId(connectionId), from(from), to(to), rate(rate), txPower(txPower), dir(dir) {}

    MacCid connectionId;
    MacNodeId from, to;
    double rate, txPower;
    Direction dir;

    bool operator>(const IdRatePair& other) {
      return rate > other.rate;
    }

    bool operator<(const IdRatePair& other) {
      return !((*this) > other);
    }
};

/**
 * This container can be given <node id, throughput> pairs.
 * It always keeps the internal list sorted according to throughput.
 */
class MaxDatarateSorter {
  public:
    MaxDatarateSorter(size_t numBands);

    void put(const Band& band, const IdRatePair& idRatePair);

    /**
     *
     * @param band
     * @param position
     * @return The xth best node according to throughput.
     */
    const IdRatePair& get(const Band& band, const size_t& position) const;
    const std::vector<IdRatePair> at(const Band& band) const;

    std::string toString() const;
    /**
     * @return The number of bands.
     */
    size_t size() const;


  protected:

  private:
    /**
     * The outer vector corresponds to the bands.
     * Each inner vector holds an always-sorted list of <id, rate> pairs, with the best rate first.
    **/
    std::vector<std::vector<IdRatePair>> mBandToIdRate;
    const size_t mNumBands;
};


#endif //SCHEDULER_MAXDATARATESORTER_HPP

