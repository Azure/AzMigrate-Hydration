#include <string>
#include <iostream>
#include <vector>
#include "svutilities.h"

svector_t split(const std::string arg,
				const std::string delim,
				const int numFields) 
{

	svector_t splitList;
	int delimetersFound = 0;
	std::string::size_type delimLength = delim.length();
	std::string::size_type argEndIndex   = arg.length();
	std::string::size_type front = 0;

	std::string::size_type back = arg.find(delim, front);

	while( back != std::string::npos ) {
		++delimetersFound;

		if( (numFields) && (delimetersFound >= numFields) ) {
			break;
		}

		splitList.push_back(arg.substr(front, (back - front)));
		front = back + delimLength;
		back = arg.find(delim, front);
	}

	if( front < argEndIndex ) {
		splitList.push_back(arg.substr(front, (argEndIndex - front)));
	}

	return splitList;
}
