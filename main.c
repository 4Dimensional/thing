// Library
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>

// not important
struct MemoryStruct
{
	char*  memory;
	size_t size;
};

// do something
static size_t WriteMemoryCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
	size_t				 realsize = size * nmemb;
	struct MemoryStruct* mem	  = (struct MemoryStruct*) userp;

	char* ptr = realloc(mem->memory, mem->size + realsize + 1);
	if (!ptr)
	{
		printf("Not enough memory! (realloc returned NULL)\n");
		return 0;
	}

	mem->memory = ptr;
	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}

// why is this here?
char* replace_char(char* str, char find, char replace)
{
	char* current_pos = strchr(str, find);
	while (current_pos)
	{
		*current_pos = replace;
		current_pos	 = strchr(current_pos, find);
	}
	return str;
}

// Takes an item, a unix timestamp, and a output file and writes the prices to it in CSV format
// Why did I name it this?
void GetFromTime(unsigned long int timestamp, unsigned int item_id, const char* filename)
{
	char URL[1024] = "https://prices.runescape.wiki/api/v1/osrs/1h?timestamp=";
	char TIMESTAMP_CHAR[1024];
	sprintf(TIMESTAMP_CHAR, "%lu", timestamp);
	strcat(URL, TIMESTAMP_CHAR);

	CURL*	 curl_handle;
	CURLcode res;

	struct MemoryStruct chunk;

	chunk.memory = malloc(1);
	chunk.size	 = 0;

	curl_global_init(CURL_GLOBAL_ALL);

	curl_handle = curl_easy_init();

	curl_easy_setopt(curl_handle, CURLOPT_URL, URL);

	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void*) &chunk);

	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

	res = curl_easy_perform(curl_handle);

	if (res != CURLE_OK)
	{
		fprintf(stderr, "curl_easy_preform() failed: %s\n", curl_easy_strerror(res));
	}
	else
	{
		printf("%lu bytes retrieved: %lu\n", (unsigned long) chunk.size, timestamp);

		cJSON* json = cJSON_Parse(chunk.memory);
		if (!json)
		{
			printf("Error parsing JSON: %s\n", cJSON_GetErrorPtr());
		}
		else
		{
			char key[64];  //"9243" = Diamond bolts (e)
			sprintf(key, "%u", item_id);
			cJSON* selection0	  = cJSON_GetObjectItem(json, "data");
			cJSON* desiredSection = cJSON_GetObjectItem(selection0, key);
			if (!desiredSection)
			{
				printf("Key not found in JSON\n");
				cJSON_Delete(json);
				curl_easy_cleanup(curl_handle);
				free(chunk.memory);
				curl_global_cleanup();
				return;
			}
			else
			{
				cJSON* newJson = cJSON_CreateObject();
				cJSON_AddItemToObject(newJson, key, cJSON_Duplicate(desiredSection, 1));

				const char* newFilename	   = filename;
				char*		newJson_string = cJSON_Print(newJson);

				FILE* returnFile = fopen(newFilename, "a");
				if (returnFile)
				{
					cJSON* specific = cJSON_GetObjectItem(newJson, key);
					int	   itemHigh = cJSON_GetObjectItem(specific, "avgHighPrice")->valueint;
					int	   itemLow	= cJSON_GetObjectItem(specific, "avgLowPrice")->valueint;
					fprintf(returnFile, "%lu;%i;%i;\n", timestamp, itemHigh, itemLow);
					fclose(returnFile);
				}
				else
				{
					perror("Could not write file.\n");
				}
				cJSON_Delete(newJson);
			}
		}

		cJSON_Delete(json);
	}

	curl_easy_cleanup(curl_handle);

	free(chunk.memory);

	curl_global_cleanup();
}

// Convert unix timesteps to times that wiki api can handle
unsigned long int ConvertTo5M(unsigned long int t) { return t - (t % 300) - 300; }
unsigned long int ConvertTo1H(unsigned long int t) { return t - (t % 3600) - 3600; }

// an important comment
int main(int argc, char* argv[])
{
	const char*		   CSV_NAME	 = "data.csv"; // output file name
	const unsigned int ITEM_ID	 = 9243; // Item id (diamond bolts (e) that i lost all my money on)
	unsigned long int  TIMESTAMP = (unsigned long int) time(NULL); // End recording time
	unsigned long int  PAST_TIME = 1615733400; // Start recording time
	unsigned int	   TIME_STEP = 3600 * 24; // 3600 = 1 hr, so this makes it step forward a day every iteration

	unsigned long int TIMESTAMP_START = ConvertTo1H(PAST_TIME);
	unsigned long int TIMESTAMP_5M	  = ConvertTo1H(TIMESTAMP); // I named this before i created the 1H function

	printf("Starting: %lu\nEnding: %lu\n", TIMESTAMP_START, TIMESTAMP_5M);

	FILE* returnFile = fopen(CSV_NAME, "w");
	fprintf(returnFile, "time;highprice;lowprice;\n");
	fclose(returnFile);

	// loop through two periods of time and put the wiki data in the file
	for (unsigned long int i = TIMESTAMP_START; i < TIMESTAMP_5M; i += TIME_STEP)
	{
		GetFromTime(i, ITEM_ID, CSV_NAME);
	}

	// hooray!
	printf("Complete!\n");

	return 0;
}
