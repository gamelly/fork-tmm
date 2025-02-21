/******************************************************************************
TeleMídia Multiplexer
Copyright (C) 2016 TeleMídia Lab/PUC-Rio
https://github.com/TeleMidia/tmm-mpeg2ts/graphs/contributors

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU Affero General Public License as published by the Free
Software Foundation, either version 3 of the License, or (at your option) any
later version. This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License
for more details. You should have received a copy of the GNU Affero General
Public License along with this program. If not, see http://www.gnu.org/licenses/.

*******************************************************************************/

/*
 * ISDBTInformation.cpp
 *
 *  Created on: 16/09/2013
 *      Author: Felippe Nagato
 */

#include "tsparser/ISDBTInformation.h"

namespace br {
namespace pucrio {
namespace telemidia {
namespace tool {
namespace isdbt {

ISDBTInformation::ISDBTInformation() {
	init();
}

ISDBTInformation::ISDBTInformation(char* data) {
	init();
	memcpy(stream, data, 8);
	TMCCIdentifier = (stream[0] & 0xC0) >> 6;
	bufferResetControlFlag = (stream[0] & 0x10) >> 4;
	switchOnControlFlagForEmergencyBroadcasting = (stream[0] & 0x08) >> 3;
	initializationTimingHeadPacketFlag = (stream[0] & 0x04) >> 2;
	frameHeadPacketFlag = (stream[0] & 0x02) >> 1;
	frameIndicator = (stream[0] & 0x01);

	layerIndicator = (stream[1] & 0xF0) >> 4;
	countDownIndex = (stream[1] & 0x0F);

	ACDataInvalidFlag = (stream[2] & 0x80) >> 7;
	ACDataEffectiveBytes = (stream[2] & 0x60) >> 5;
	TSPCounter = (stream[2] & 0x1F) << 8;

	TSPCounter |= (stream[3] & 0xFF);

	if (!ACDataInvalidFlag) {
		if (ACData) delete ACData;
		ACData = new char[4];
		memcpy(ACData, stream + 4, 4);
	}
}

ISDBTInformation::~ISDBTInformation() {
	if (stream) delete stream;
	if (ACData) delete ACData;
}

void ISDBTInformation::init() {
	TMCCIdentifier = TERRESTRIAL_DIGITAL_TV;
	stream = new char[8];
	ACData = NULL;
	bufferResetControlFlag = false;
	switchOnControlFlagForEmergencyBroadcasting = false;
	initializationTimingHeadPacketFlag = false;
	frameHeadPacketFlag = false;
	frameIndicator = false;
	ACDataInvalidFlag = true;
	countDownIndex = 0x0F; //?
	ACDataLength = 0;
	TSPCounter = 0;
}

void ISDBTInformation::setTMCCIdentifier(unsigned char id) {
	TMCCIdentifier = id;
}

unsigned char ISDBTInformation::getTMCCIdentifier() {
	return TMCCIdentifier;
}

void ISDBTInformation::setBufferResetControlFlag(bool flag) {
	bufferResetControlFlag = flag;
}

bool ISDBTInformation::getBufferResetControlFlag() {
	return bufferResetControlFlag;
}

void ISDBTInformation::setSwitchOnControlFlagForEmergencyBroadcasting(bool flag) {
	switchOnControlFlagForEmergencyBroadcasting = flag;
}

bool ISDBTInformation::getSwitchOnControlFlagForEmergencyBroadcasting() {
	return switchOnControlFlagForEmergencyBroadcasting;
}

void ISDBTInformation::setInitializationTimingHeadPacketFlag(bool flag) {
	initializationTimingHeadPacketFlag = flag;
}

bool ISDBTInformation::getInitializationTimingHeadPacketFlag() {
	return initializationTimingHeadPacketFlag;
}

void ISDBTInformation::setFrameHeadPacketFlag(bool flag) {
	frameHeadPacketFlag = flag;
}

bool ISDBTInformation::getFrameHeadPacketFlag() {
	return frameHeadPacketFlag;
}

void ISDBTInformation::setFrameIndicator(bool indicator) {
	frameIndicator = indicator;
}

bool ISDBTInformation::getFrameIndicator() {
	return frameIndicator;
}

void ISDBTInformation::setLayerIndicator(unsigned char layer) {
	layerIndicator = layer;
}

unsigned char ISDBTInformation::getLayerIndicator() {
	return layerIndicator;
}

void ISDBTInformation::setCountDownIndex(unsigned char cindex) {
	countDownIndex = cindex;
}

unsigned char ISDBTInformation::getCountDownIndex() {
	return countDownIndex;
}

void ISDBTInformation::setACDataInvalidFlag(bool flag) {
	ACDataInvalidFlag = flag;
}

bool ISDBTInformation::getACDataInvalidFlag() {
	return ACDataInvalidFlag;
}

void ISDBTInformation::setACDataEffectiveBytes(unsigned char acDataEB) {
	ACDataEffectiveBytes = acDataEB;
}

unsigned char ISDBTInformation::getACDataEffectiveBytes() {
	return ACDataEffectiveBytes;
}

unsigned short ISDBTInformation::incrementTSPCounter() {
	return ++TSPCounter;
}

void ISDBTInformation::setTSPCounter(unsigned short counter) {
	TSPCounter = counter;
}

unsigned short ISDBTInformation::getTSPCounter() {
	return (TSPCounter & 0x1FFF);
}

void ISDBTInformation::setACData(char* acDatasrc, unsigned short length) {
	if (ACData) delete[] ACData;
	ACData = new char[length];
	memcpy(ACData, acDatasrc, length);
	ACDataLength = length;
}

unsigned short ISDBTInformation::getACData(char** acData) {
	if (ACData) {
		*acData = ACData;
		return ACDataLength;
	}
	return 0;
}

unsigned short ISDBTInformation::getACDataLength() {
	return ACDataLength;
}

int ISDBTInformation::updateStream() {
	int pos = 0;

	stream[pos] = 0x20;
	stream[pos] = stream[pos] | ((TMCCIdentifier << 6) & 0xC0);
	stream[pos] = stream[pos] | ((bufferResetControlFlag << 4) & 0x10);
	stream[pos] = stream[pos] | ((switchOnControlFlagForEmergencyBroadcasting << 3) & 0x08);
	stream[pos] = stream[pos] | ((initializationTimingHeadPacketFlag << 2) & 0x04);
	stream[pos] = stream[pos] | ((frameHeadPacketFlag << 1) & 0x02);
	stream[pos] = stream[pos] | frameIndicator;

	pos++;

	stream[pos] = ((layerIndicator << 4) & 0xF0);
	stream[pos] = stream[pos] | (countDownIndex & 0x0F);

	pos++;

	stream[pos] = ((ACDataInvalidFlag << 7) & 0x80);
	stream[pos] = stream[pos] | ((ACDataEffectiveBytes << 5) & 0x60);
	stream[pos] = stream[pos] | ((TSPCounter >> 8) & 0x1F);

	pos++;

	stream[pos] = (TSPCounter & 0xFF);

	pos++;

	if (ACDataInvalidFlag) {
		memset(stream+pos, 0xFF, 4);
	} else {
		if (ACData) {
			memcpy(stream+pos, ACData, 4);
		}
	}

	pos += 4;

	return pos;
}

int ISDBTInformation::getStream(char** dataStream) {
	if (dataStream != NULL) *dataStream = stream;
	return 8;
}


}
}
}
}
}


