/*
 * PSdt.cpp
 *
 *  Created on: 18/06/2013
 *      Author: Felippe Nagato
 */

#include "project/si/PSdt.h"

namespace br {
namespace pucrio {
namespace telemidia {
namespace tool {

PSdt::PSdt() {
	ProjectInfo();
	projectType = PT_SDT;
	layer = 0x01; //HIERARCHY_A
}

PSdt::~PSdt() {

}

}
}
}
}


