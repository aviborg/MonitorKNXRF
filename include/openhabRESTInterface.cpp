#include <iostream>
#include <curl/curl.h>
#include "jsmn.h"
#include "openhabRESTInterface.h"

size_t CurlWrite_CallbackFunc_StdString(void *contents, size_t size, size_t nmemb, std::string *s)
{
    size_t newLength = size*nmemb;
    size_t oldLength = s->size();
    try
    {
        s->resize(oldLength + newLength);
    }
    catch(std::bad_alloc &e)
    {
        //handle memory problem
        return 0;
    }

    std::copy((char*)contents,(char*)contents+newLength,s->begin()+oldLength);
    return size*nmemb;
}

OpenhabItem::OpenhabItem(void) {
	name = "";
	state = 0;  // assumes the item is a number
	next = NULL;
}

uint32_t OpenhabItem::getSize(void) {
	if (this->next) {
		return (1U+this->next->getSize());
	} else {
		return 1U;
	}
}

std::string getOpenhabItems(std::string tags) {
	CURL *curl;
	CURLcode res;
	std::string s = "";
	
	// init
	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();
	if(curl) {
		struct curl_slist *list = NULL;
		// Read out items with defined tag by calling the REST api
		tags = "http://localhost:8080/rest/items?tags=" + tags + "&recursive=false&fields=name%2C%20editable"; 
		curl_easy_setopt(curl, CURLOPT_URL, tags.c_str());
		list = curl_slist_append(list, "Accept: application/json");		// we're expecting json
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWrite_CallbackFunc_StdString);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
        // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L); //remove this to disable verbose output
		// Do the call
		res = curl_easy_perform(curl);
		/* Check for errors */ 
		if(res != CURLE_OK)
			s = "";

		/* always cleanup */
		curl_slist_free_all(list);
		
	}
	curl_easy_cleanup(curl);
	curl_global_cleanup();
	return s;
}

void parseOpenhabItems(std::string s,  OpenhabItem *&itemList) {
	if (s.length() > 1) {
		std::string tempStr;
		int r;
		jsmn_parser p;
		jsmntok_t t[1024]; /* We expect no more than 1024 tokens */
		jsmn_init(&p);
		int endPos;
		if (s.substr(0,1).compare("[") == 0) {
			s = s.substr(1, s.length()-2); // curl (or openhab?) adds [] to start and end of string, remove those
		}
		r = jsmn_parse(&p, s.c_str(), s.length(), t, sizeof(t)/sizeof(t[0]));
		if (s.length() > 0 && r > 0) {
			OpenhabItem *currentItem;
			while (itemList) {  // Clear itemlist first
				currentItem = itemList;
				itemList = itemList->next;
				delete currentItem;
			}
			itemList = new OpenhabItem;
			currentItem = itemList;
			endPos = t[0].end;
			for (int i=0; i < r; ++i) {
				if (t[i].start > endPos) { // is there a new item?
					currentItem->next = new OpenhabItem;
					currentItem = currentItem->next;
					currentItem->next = NULL;
					endPos = t[i].end;
				}
				if (s.substr(t[i].start, t[i].end-t[i].start).compare("name") == 0) {
					currentItem->name.assign(s.substr(t[i+1].start, t[i+1].end-t[i+1].start));
				}
			}
		}
	}
}

void putOpenhabItem(std::string itemName, std::string itemState) {
	CURL *curl;
	// CURLcode res;
	char dataBuffer[256];
	
	// init
	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();
	if(curl) {
		struct curl_slist *list = NULL;
		list = curl_slist_append(list, "Content-Type: text/plain");
		list = curl_slist_append(list, "Accept: application/json");
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT"); /* !!! */
		// curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L); //remove this to disable verbose output
		sprintf(dataBuffer,"http://localhost:8080/rest/items/%s/state", itemName.c_str());
		curl_easy_setopt(curl, CURLOPT_URL, dataBuffer);  
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, sprintf(dataBuffer,"%s", itemState.c_str()));
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, dataBuffer); /* data goes here */			
		curl_easy_perform(curl);		
	}
	curl_easy_cleanup(curl);
	curl_global_cleanup();
}
