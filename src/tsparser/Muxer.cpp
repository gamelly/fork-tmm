/*
 * Muxer.cpp
 *
 *  Created on: 05/02/2013
 *      Author: Felippe Nagato
 */

#include "tsparser/Muxer.h"

namespace br {
namespace pucrio {
namespace telemidia {
namespace tool {

Muxer::Muxer(unsigned short packetsInBuffer) {
	stc = SYSTEM_CLOCK_FREQUENCY * 10;
	tsBitrate = 19000000;
	packetSize = TSPacket::TS_PACKET_SIZE;
	stcOffset = 0;
	pFile = NULL;
	bitrateErrorCounter = 0;
	server = NULL;
	buffer = new Buffer;
	streamBufferSize = packetSize * packetsInBuffer;
	this->packetsInBuffer = packetsInBuffer;
	streamBuffer = new char[streamBufferSize];
	streamBufferLength = 0;
}

Muxer::~Muxer() {
	close();
	delete buffer;
	if (server) delete server;
	if (streamBuffer) delete streamBuffer;
	removeAllElementaryStreams();
}

unsigned int Muxer::gcd(unsigned int a, unsigned int b) {
	if (b == 0) return a; else return gcd(b, a % b);
}

void Muxer::setTsBitrate(unsigned int rate) {
	tsBitrate = rate;
}

unsigned int Muxer::getTsBitrate() {
	return tsBitrate;
}

void Muxer::setPacketSize(unsigned char size) {
	packetSize = size;
	if (streamBuffer) delete streamBuffer;
	streamBufferSize = packetSize * packetsInBuffer;
	streamBuffer = new char[streamBufferSize];
	streamBufferLength = 0;
}

unsigned char Muxer::getPacketSize() {
	return packetSize;
}

void Muxer::setDestination(string dest) {
	destination = dest;
	if (!dest.size()) return;
	isFileMode = true;
	if ((dest[0] >= '0') && (dest[0] <= '9')) {
		isFileMode = false;
	}
	unsigned found = dest.find("://");
	if (found != std::string::npos) {
		isFileMode = false;
	}
}

string Muxer::getDestination() {
	return destination;
}

void Muxer::setStcReference(int64_t stcBegin) {
	if (stcBegin < SYSTEM_CLOCK_FREQUENCY * 10) stc = SYSTEM_CLOCK_FREQUENCY * 10;
		else stc = stcBegin;
	this->stcBegin = stc;
	stc_s.setReference(stc);
}

int64_t Muxer::getCurrentStc() {
	return stc + stcOffset;
}

int64_t Muxer::getStcBegin() {
	return stcBegin;
}

int64_t Muxer::getRelativeStc() {
	return getCurrentStc() - stcBegin;
}

double Muxer::getPktStc() {
	return pktStc;
}

void Muxer::setTTL(unsigned char t) {
	ttl = t;
}

int Muxer::open() {
	if (isFileMode) {
		pFile = fopen(destination.c_str(), "wb");
		if (pFile == NULL ) {
			cout << "Muxer::open - Unable to open file: " << destination << endl;
			return -1;
		}
	} else {
		string ip, port;
		unsigned found = destination.find("://");
		ip.assign(destination, found+3, 25);
		found = ip.find(":");
		port.assign(ip, found+1, 5);
		ip.assign(ip, 0, found);
		int num = atol(port.c_str());
		server = new MulticastServer(ip.c_str(), num);
		if (!server->createSocket()) {
			cout << "Muxer::open - Unable to open socket: " << destination << endl;
			delete server;
			server = NULL;
			return -2;
		}
		server->setTTL(ttl);
	}
	return 0;
}

int Muxer::close() {
	if (isFileMode) {
		if (pFile) fclose(pFile);
		pFile = NULL;
	}
	return 0;
}

int Muxer::sendStreamBuffer() {
	if (streamBufferLength == streamBufferSize) {
		streamBufferLength = 0;
		if (isFileMode) {
			int rw;
			rw = fwrite(streamBuffer, 1, streamBufferSize, pFile);
			if (rw != streamBufferSize) {
				return -1;
			}
		} else {
			Stc::waitFor(stc + stcOffset - stc_s.getStc());
			return server->send(streamBuffer, streamBufferSize);
		}
	}
	return 0;
}

bool Muxer::calculateStcStep() {
	map<unsigned short, unsigned short>::iterator it;
	unsigned int cd = 0;
	unsigned int i = 0;
	bool found = false;

	if (listOfAllPossiblePcrsFrequencies.size()) {
		if (listOfAllPossiblePcrsFrequencies.size() == 1) {
			cd = listOfAllPossiblePcrsFrequencies[0];
		} else {
			while (i < listOfAllPossiblePcrsFrequencies.size()) {
				if (cd == 0) {
					cd = gcd(listOfAllPossiblePcrsFrequencies[i],
							 listOfAllPossiblePcrsFrequencies[i+1]);
					i = i + 2;
				} else {
					cd = gcd(cd, listOfAllPossiblePcrsFrequencies[i]);
					i++;
				}
			}
		}

		for (i = 0; i < listOfAllPossiblePcrsFrequencies.size(); ++i) {
			if (listOfAllPossiblePcrsFrequencies[i] == cd) {
				for (unsigned int j = 2; j < cd/2; ++j) {
					if ((cd % j) == 0) {
						cd /= j;
						found = true;
						break;
					}
				}
				break;
			}
		}
		if (!found) {
			cout << "Muxer::calculateStcStep - PCR frequencies not suitable." << endl;
			return false;
		}

		stcStep = Stc::secondToStc((double) cd / 1000);

		return true;
	}

	return false;
}

int Muxer::reachedNextPcrSend(int64_t stc) {
	map<unsigned short, int64_t>::iterator it;

	it = nextPcrSendList.begin();
	while (it != nextPcrSendList.end()) {
		if (stc >= it->second) {
			return it->first;
		}
		++it;
	}

	return -1;
}

void Muxer::fillAllBufferes() {
	map<unsigned short, vector<Stream*>*>::iterator it;
	vector<Stream*>::iterator itStr;

	for (it = streamList.begin(); it != streamList.end(); ++it) {
		for (itStr = it->second->begin(); itStr != it->second->end(); ++itStr) {
			if (*itStr) {
				(*itStr)->fillBuffer();
			} else {
				cout << "Muxer::fillAllBufferes - No stream available for pid = " <<
						it->first << endl;
			}
		}
	}
}

int64_t Muxer::getNextStcStreamToMultiplex(unsigned short *pid,
										   int *index,
										   bool *streamScheduled) {
	map<unsigned short, vector<Stream*>*>::iterator it;
	vector<Stream*>::iterator itStr;
	int64_t nextSend = 0x7FFFFFFFFFFFFFFF;
	unsigned int idx;

	for (it = streamList.begin(); it != streamList.end(); ++it) {
		idx = -1;
		for (itStr = it->second->begin(); itStr != it->second->end(); ++itStr) {
			idx++;
			if ((*itStr)->getNextSend() < nextSend) {
				nextSend = (*itStr)->getNextSend();
				*pid = it->first;
				*index = idx;
				*streamScheduled = true;
			}
		}
	}

	return nextSend;
}

//Notice: Never use the same stream for different PIDs.
bool Muxer::addElementaryStream(unsigned short pid, Stream* stream) {
	vector<Stream*>* strList;
	vector<Stream*>::iterator itStr;
	map<unsigned short, vector<Stream*>*>::iterator it = streamList.begin();

	if (!streamList.count(pid)) {
		strList = new vector<Stream*>();
		streamList.insert(
				it, std::pair<unsigned short, vector<Stream*>*>(pid, strList));
	} else {
		strList = streamList[pid];
	}
	for (itStr = strList->begin(); itStr != strList->end(); ++itStr) {
		if (*itStr == stream) return false;
	}
	strList->push_back(stream);

	return true;
}

bool Muxer::removeElementaryStream(unsigned short pid) {
	map<unsigned short, vector<Stream*>*>::iterator it;

	it = streamList.find(pid);
	if (it != streamList.end()){
		delete it->second;
		streamList.erase(it);
		return true;
	}

	return false;
}

bool Muxer::removeAllElementaryStreams() {
	map<unsigned short, vector<Stream*>*>::iterator it;

	for (it = streamList.begin(); it != streamList.end(); ++it) {
		delete (it->second);
	}
	streamList.clear();

	return true;
}

bool Muxer::addToListOfAllPossiblePcrsFrequencies(unsigned short freq) {
	vector<unsigned short>::iterator it;

	if (freq < 10) return false;

	it = listOfAllPossiblePcrsFrequencies.begin();
	while (it != listOfAllPossiblePcrsFrequencies.end()) {
		if (*it == freq) return true;
		++it;
	}
	listOfAllPossiblePcrsFrequencies.push_back(freq);

	return true;
}

bool Muxer::addPcrPid(unsigned short pid, unsigned short frequency) {
	if (frequency < 10) return false;
	if (pcrList.count(pid)) return true;
	pcrList[pid] = frequency;
	nextPcrSendList[pid] = stc;
	return true;
}

bool Muxer::removePcrPid(unsigned short pid) {
	if (pcrList.count(pid)) {
		pcrList.erase(pid);
		nextPcrSendList.erase(pid);
		return true;
	}
	return false;
}

map<unsigned short, unsigned short>* Muxer::getPcrList() {
	return &pcrList;
}

map<unsigned short, vector<Stream*>*>* Muxer::getStreamList() {
	return &streamList;
}

bool Muxer::prepareMultiplexer(int64_t stcBegin) {

	if (!calculateStcStep()) return false;

	pktNumSinceLastStep = 0;
	calculateBitrate();
	stcOffset = 0;

	if (stcBegin < SYSTEM_CLOCK_FREQUENCY * 10) stc = SYSTEM_CLOCK_FREQUENCY * 10;
		else stc = stcBegin;
	this->stcBegin = stc;

	for (int i = 0; i < 8192; i++) pktControl[i] = 0;

	if (open() < 0) {
		cout << "Muxer::prepareMultiplexer - Destination not set." << endl;
		return false;
	}

	for (unsigned int idx = 0; idx < nextPcrSendList.size(); idx++) {
		nextPcrSendList[idx] = stc;
	}

	cout << "Destination: " << destination << endl;
	cout << "STC begins at " << this->stcBegin << " (" <<
			Stc::stcToSecond(this->stcBegin) << "s)" << endl;

	stc_s.setReference(stc);

	return true;
}

void Muxer::newStcStep() {
	stc += stcStep;
	pktNumSinceLastStep = 0;
	stcOffset = 0;
}

int Muxer::mainLoop() {
	int64_t nextSend;
	unsigned short pid = 0xFFFF;
	int idx;
	bool streamScheduled = false;
	Stream* stream;

	fillAllBufferes();
	stcOffset = 0;

	idx = -1;
	//get next stream to multiplex
	nextSend = getNextStcStreamToMultiplex(&pid, &idx, &streamScheduled);
	if (idx < 0) return -1;
	stream = streamList[pid]->at(idx);

	//wait for it
	while (stc < nextSend) {
		processNullPackets();
		processPcr();
		if (!isFileMode) Stc::waitFor(stc - stc_s.getStc());
		newStcStep();
	};

	while (streamScheduled) {
		vector<Buffer*>::iterator it;
		streamScheduled = false;

		stream->setCurrStc(stc + stcOffset);
		if (stream->getBuffer(&buffer)) {
			//write it as a TS
			writeTsStream(pid, stream->getType());
		} else {
			bitrateErrorCounter++;
			if (bitrateErrorCounter >= 5) {
				if (tsBitrate <= 19000000) {
					bitrateErrorCounter = 0;
					tsBitrate += 1000000;
					calculateBitrate();
					cout << "Muxer::mainLoop - Increasing TS bitrate in 1 mbps. Total = "
							<< tsBitrate << " bps." << endl;
				}
				/*if (bitrateErrorCounter >= 100) {
					cout << "Muxer::mainLoop - Too many errors. Stopping application." << endl;
					exit(0);
				}*/
			}
		}
		stream->disposeBuffer();
		stream->updateNextSend(stc + stcOffset);
		idx = -1;
		nextSend = getNextStcStreamToMultiplex(&pid, &idx, &streamScheduled);
		if (idx < 0) return -1;
		stream = streamList[pid]->at(idx);
		stream->fillBuffer();
		if (stc + stcOffset < nextSend) break;
	}

	return 0;
}

int Muxer::processPcr() {
	int reachedPcrPid;

	reachedPcrPid = reachedNextPcrSend(stc + stcOffset);
	while (reachedPcrPid >= 0) {
		//send a PCR now
		writeTsPcr(stc + stcOffset, reachedPcrPid);
		reachedPcrPid = reachedNextPcrSend(stc + stcOffset);
	}
	return reachedPcrPid;
}

int Muxer::writeTsPcr(int64_t pcr, unsigned short pid) {
	TSPacket* packet;
	TSAdaptationField* tsaf;
	char* pktBuffer;
	int rw;

	packet = new TSPacket();
	tsaf = new TSAdaptationField(pcr);
	packet->setAdaptationField(tsaf);
	packet->setAdaptationFieldControl(TSPacket::NO_PAYLOAD);
	packet->setPid(pid);
	packet->setSectionType(false);
	if (streamList.count(pid)) {
		packet->setStartIndicator(0);
	} else {
		packet->setStartIndicator(1);
	}
	packet->setContinuityCounter(pktControl[pid]);
	packet->getStream(&pktBuffer);
	//if (packetSize == 204) addRS();

	rw = writeStream(pktBuffer);

	delete packet;

	nextPcrSendList[pid] += pcrList[pid] * 27000;
	pktNumSinceLastStep++;
	stcOffset = (int64_t) ((pktStc * pktNumSinceLastStep) + 0.5f);

	return rw;
}

int Muxer::writeTsStream(unsigned short pid, unsigned char type) {
	unsigned int pos = 0;
	int rw = 0;
	int reachedPcrPid;
	unsigned char payloadLength;
	TSPacket* packet;
	TSAdaptationField* tsaf;
	char* pktBuffer;

	if (type > 1) {
		cout << "Muxer::writeTsStream - Type not implemented." << endl;
		return -1;
	}

	while (pos < buffer->size) {
		packet = NULL;
		tsaf = NULL;

		reachedPcrPid = reachedNextPcrSend(stc + stcOffset);
		if (reachedPcrPid == pid) {
			//adaptation field and payload
			tsaf = new TSAdaptationField(stc + stcOffset);
			nextPcrSendList[pid] += pcrList[pid] * 27000;
			processPcr();
			newStcStep();
		} else if (reachedPcrPid >= 0) {
			//adaptation field only
			processPcr();
			newStcStep();
			continue;
		}

		if (buffer->size - pos >= 184) {
			payloadLength = 184;
		} else {
			payloadLength = buffer->size - pos;
			if ((type == 0) && (tsaf == NULL)) {
				tsaf = new TSAdaptationField();
				tsaf->setAdaptationFieldLength(183 - payloadLength);
			}
		}

		packet = new TSPacket(type, buffer->buffer + pos, payloadLength, tsaf);
		if (pos == 0 && buffer->startIndicator) {
			packet->setStartIndicator(1);
		} else {
			packet->setStartIndicator(0);
		}
		packet->setPid(pid);
		packet->setContinuityCounter(++pktControl[pid]);
		//the payloadLength actually used calling packet->getStream()
		payloadLength = packet->getStream(&pktBuffer);
		pos += payloadLength;
		//if (packetSize == 204) addRS();
		rw = writeStream(pktBuffer);

		delete packet;
		pktNumSinceLastStep++;
		stcOffset = (int64_t) ((pktStc * pktNumSinceLastStep) + 0.5f);
		if (pktNumSinceLastStep == pktPerStepInterval) {
			//particular case when this values become equal: a pcr is not sent.
			//but if you try to send it, jitters occur during all TS.
			if (reachedNextPcrSend(stc + stcOffset) == -1) {
				//processPcr();
				newStcStep();
			}
		}
	}
	return rw;
}

int Muxer::processNullPackets() {
	map<unsigned short, int64_t>::iterator it;
	TSPacket* packet;
	char* pktBuffer;
	int rw = 0, pcrCount = 0;

	it = nextPcrSendList.begin();
	while (it != nextPcrSendList.end()) {
		if (stc >= it->second) {
			pcrCount++;
		}
		++it;
	}

	if (pktPerStepInterval - (pktNumSinceLastStep + pcrCount) > pktPerStepInterval) {
		cout << "Ops! stcOffset to long!" << endl;
	}

	while (pktNumSinceLastStep < (pktPerStepInterval - pcrCount)) {

		packet = new TSPacket(0, NULL, 0, NULL);
		packet->setAdaptationFieldControl(1);
		packet->setStartIndicator(0);
		packet->setPid(8191);
		packet->getStream(&pktBuffer);

		//if (packetSize == 204) addRS();
		writeStream(pktBuffer);
		delete packet;
		pktNumSinceLastStep++;
		stcOffset = (int64_t) ((pktStc * pktNumSinceLastStep) + 0.5f);
	}

	return rw;
}

int Muxer::writeStream(char* pktBuffer) {
	int rw;
	memcpy(streamBuffer + streamBufferLength, pktBuffer, packetSize);
	streamBufferLength += packetSize;
	rw = sendStreamBuffer();
	if ((rw != 0) && (rw != streamBufferSize)) {
		cout << "Muxer::writeStream - Packet not sent." << endl;
		return -2;
	}
	return rw;
}

void Muxer::calculateBitrate() {
	double value;
	pktPerSec = (unsigned int)(((double)(tsBitrate / 8) / packetSize) + 0.5f);
	value = (((double) tsBitrate / 8) * ((double) stcStep / SYSTEM_CLOCK_FREQUENCY)) / packetSize;
	pktPerStepInterval = (unsigned int)(value + 0.5f);
	value = ((double) pktPerStepInterval * ((double)SYSTEM_CLOCK_FREQUENCY/stcStep)) * packetSize * 8;
	tsBitrate = (unsigned int)(value + 0.5f);
	pktStc = (double) stcStep * ((double) 1 / pktPerStepInterval);
}

}
}
}
}


