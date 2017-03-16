/*
 * LteMaxDatarate.cpp
 *
 *  Created on: Mar 16, 2017
 *      Author: kunterbunt
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
}

void LteMaxDatarate::commitSchedule() {
    EV_STATICCONTEXT;
    EV << "LteMaxDatarate::commitSchedule" << std::endl;
}
