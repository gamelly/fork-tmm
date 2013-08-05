/*
 * Project.h
 *
 *  Created on: 05/02/2013
 *      Author: Felippe Nagato
 */

#ifndef PROJECT_H_
#define PROJECT_H_

#include "inputdata/InputData.h"
#include "npt/NPTProject.h"
#include "carousel/PCarousel.h"
#include "si/PAit.h"
#include "si/PTot.h"
#include "si/PSdt.h"
#include "si/PNit.h"
#include "si/PEit.h"
#include "si/PMTView.h"
#include "timeline/Timeline.h"
#include "Stc.h"

#include <inttypes.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string>
#include <map>

using namespace std;
using namespace br::pucrio::telemidia::mpeg2::dsmcc;

namespace br {
namespace pucrio {
namespace telemidia {
namespace tool {

class Project {
	#define VBV_BUFFER 1.00

	private:

	protected:
		string filename;
		string projectName;
		string projectDescription;

		bool isLoop;
		string destination;
		string providerName;
		int tsid;
		int tsBitrate;
		int64_t stcBegin; //27 MHz - Minimum value = 270000000 (10s)
		double vbvBuffer;
		unsigned char ttl;

		unsigned short originalNetworkId;
		string tsName;
		unsigned short broadcastFrequency;
		unsigned char virtualChannel;
		unsigned char guardInterval;
		unsigned char transmissionMode;
		unsigned short packetsInBuffer;

		map<int, ProjectInfo*>* projectList;
		//TODO: EIT, CAT

	public:
		Project();
		virtual ~Project();

		void setFilename(string filename);
		string getFilename();
		string getProjectName();
		string getProjectDescription();
		bool mountCarousels();
		int configAit(PAit* ait, unsigned int ctag, string aName, string lang,
					  string baseDir, string initClass, unsigned int orgId,
					  unsigned short appId, unsigned int appcode);

		void setDestination(string dest);
		string getDestination();
		void setProviderName(string name);
		string getProviderName();
		void setTsid(int id);
		int getTsid();
		void setTsBitrate(int rate);
		int getTsBitrate();
		void setStcBegin(int64_t stc);
		int64_t getStcBegin();
		map<int, ProjectInfo*>* getProjectList();
		ProjectInfo* findProject(int id);
		double getVbvBuffer();
		bool getIsLoop();
		unsigned char getTTL();
		unsigned short getOriginalNetworkId();
		string getTsName();
		unsigned short getBroadcastFrequency();
		unsigned char getVirtualChannel();
		unsigned char getGuardInterval();
		unsigned char getTransmissionMode();
		unsigned short getPacketsInBuffer();

		virtual int readFile()=0;

};

}
}
}
}


#endif /* PROJECT_H_ */
