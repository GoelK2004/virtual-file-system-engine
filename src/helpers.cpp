#include "helpers.h"

namespace helpers{
	bool isValidFileName(const std::string& fileName){

		if (fileName.empty() || fileName.size() > 31)
			return false;
		if (fileName == "." || fileName == "..")
			return false;
		return std::all_of(fileName.begin(), fileName.end(), [](char ch){
			return std::isalnum(ch) || ch == '.' || ch == '_';
		});
	}
}
