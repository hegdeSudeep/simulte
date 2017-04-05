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

#ifndef STACK_MAC_SCHEDULING_MODULES_MAX_DATARATE_SCHEDULINGMEMORY_H_
#define STACK_MAC_SCHEDULING_MODULES_MAX_DATARATE_SCHEDULINGMEMORY_H_

#include <omnetpp.h>
#include <LteCommon.h>

/**
 * Maps a node id to its assigned bands and transmission direction.
 */
class SchedulingMemory {
  public:
    /**
     * Notify a 'band' being assigned to 'id'.
     * @param id
     * @param band
     */
    void put(const MacNodeId id, const Band band);

    /**
     * @param id
     * @return The number of bands currently assigned to 'id'.
     */
    std::size_t getNumberAssignedBands(const MacNodeId& id) const;

    const std::vector<Band>& getBands(const MacNodeId& id) const;

  private:
    /**
     * A memory item holds the assigned bands per node id
     * as well as the transmission direction this node wants to transmit in.
     */
    class MemoryItem {
      private:
        MacNodeId _id;
        std::vector<Band> _assignedBands;
        Direction _dir;

      public:
        MemoryItem(MacNodeId id) : _id(id) {}

        void putBand(Band band) {
          _assignedBands.push_back(band);
        }
        std::size_t getNumberOfAssignedBands() const {
          return _assignedBands.size();
        }
        const std::vector<Band>& getBands() const {
          return _assignedBands;
        }

        void setDir(Direction dir) {
          _dir = dir;
        }
        const Direction& getDir() const {
          return _dir;
        }

        const MacNodeId& getId() const {
          return _id;
        }
    };

  protected:
    const MemoryItem& get(const MacNodeId& id) const;
    MemoryItem& get(const MacNodeId& id);

    std::vector<MemoryItem> _memory;
};

#endif /* STACK_MAC_SCHEDULING_MODULES_MAX_DATARATE_SCHEDULINGMEMORY_H_ */
