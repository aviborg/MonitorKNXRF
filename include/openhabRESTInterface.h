#ifndef _OPENHABRESTINTERFACE_H
#define _OPENHABRESTINTERFACE_H

#include <string>
#include <stddef.h>
#include <stdint.h>

size_t CurlWrite_CallbackFunc_StdString(void *contents, size_t size, size_t nmemb, std::string *s);

class OpenhabItem {
	public:
		OpenhabItem(void);
		std::string name;
		float state;  // assumes the item is a number
		uint32_t getSize(void);
		OpenhabItem *next;
}; 

std::string getOpenhabItems(std::string tags);

void parseOpenhabItems(std::string s, OpenhabItem *&itemList);

void putOpenhabItem(std::string itemName, std::string itemState);



#endif