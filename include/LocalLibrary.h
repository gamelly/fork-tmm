/*
 * LocalLibrary.h
 *
 *  Created on: 04/02/2014
 *      Author: Felippe Nagato
 */

#ifndef LOCALLIBRARY_H_
#define LOCALLIBRARY_H_

#ifdef _WIN32
#include <windows.h>
#else
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include <string>
#include <cctype>

#include "project/tinyxml2.h"

using namespace std;
using namespace tinyxml2;

namespace br {
namespace pucrio {
namespace telemidia {
namespace tool {

class LocalLibrary {

	private:

	protected:

	public:
		LocalLibrary();
		virtual ~LocalLibrary();

		static bool executeApp(const string& filename, string parameters,
							   unsigned int* pid);
		static bool killApp(unsigned int pid);

		static string getAttribute(XMLElement* e, const string& name);
		static string getElementText(XMLElement* e);
		static string extractBaseId(const string& filename);
		static string upperCase(const string& text);

};

}
}
}
}


#endif /* LOCALLIBRARY_H_ */
