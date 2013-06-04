/*
 * PCarousel.h
 *
 *  Created on: 25/03/2013
 *      Author: Felippe Nagato
 */

#ifndef PCAROUSEL_H_
#define PCAROUSEL_H_

#include "project/ProjectInfo.h"
#include "dsmcc/carousel/Carousel.h"
#include <iostream>

using namespace std;
using namespace br::pucrio::telemidia::mpeg2::dsmcc;

namespace br {
namespace pucrio {
namespace telemidia {
namespace tool {

class PCarousel : public ProjectInfo, public Carousel {

	private:

	protected:
		string outputFile;
		string tempFolder;
	public:
		PCarousel();
		virtual ~PCarousel();

		void setOutputFile(string filename);
		string getOutputFile();
		void setTempFolder(string folder);
		string getTempFolder();

};

}
}
}
}


#endif /* PCAROUSEL_H_ */
