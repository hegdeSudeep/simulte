/*
 * D2DModeSelectionMaxDatarate.h
 */

#ifndef STACK_D2DMODESELECTION_D2DMODESELECTIONMAXDATARATE_D2DMODESELECTIONMAXDATARATE_H_
#define STACK_D2DMODESELECTION_D2DMODESELECTIONMAXDATARATE_D2DMODESELECTIONMAXDATARATE_H_

#include "D2DModeSelectionBase.h"
#include "OmniscientEntity.h"

/**
 * For each connection choose direct mode if the estimated channel capacity is larger.
 */
class D2DModeSelectionMaxDatarate : public D2DModeSelectionBase {
public:
    D2DModeSelectionMaxDatarate() {}
    virtual ~D2DModeSelectionMaxDatarate() {}

    virtual void initialize(int stage) override;

protected:
    OmniscientEntity* mOracle = nullptr;

    virtual void doModeSelection();
};

#endif /* STACK_D2DMODESELECTION_D2DMODESELECTIONMAXDATARATE_D2DMODESELECTIONMAXDATARATE_H_ */
