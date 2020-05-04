//==============================================================================
/**
@file       FirmamentTrackerHelper.cpp

@brief		Load firmament html page and parse progress

@copyright  (c) 2020, Momoko Tomoko

**/
//==============================================================================

#include "pch.h"
#include "FirmamentTrackerHelper.h"

#define CURL_STATICLIB
#include <curl\curl.h>

FirmamentTrackerHelper::FirmamentTrackerHelper()
{
	mHttpData.reset(new std::string());
}

namespace
{
	/**
		@brief Callback function used for curl html read
	**/
	std::size_t callback(
		const char* in,
		std::size_t size,
		std::size_t num,
		std::string* out)
	{
		const std::size_t totalBytes(size * num);
		out->append(in, totalBytes);
		return totalBytes;
	}
}

/**
	@brief Read the html page and store it into htmlData string

	@return true if success 
**/
bool FirmamentTrackerHelper::ReadFirmamentHTML()
{
	mHtmlMutex.lock();
	CURL* curl;

	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, html);
	// Hide progress bar
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
	// Don't wait forever, time out after 10 seconds.
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
	
	mHttpData->clear();

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, mHttpData.get());

	// grab raw html
	curl_easy_perform(curl);
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &mHttpCode);

	curl_easy_cleanup(curl);
	mHtmlMutex.unlock();

	if (mHttpCode == 200)
	{
		return true;
	}
	return false;
}

/**
	@brief Parse raw html for firmament progress

	@param[in] server name of server to get progress for
	@param[out] progress percentage

	@return true if successful
**/
bool FirmamentTrackerHelper::GetFirmamentProgress(const std::string & server, std::string& progress)
{
	mHtmlMutex.lock();
	if (mHttpCode == 200)
	{
		// find first instance of server name
		std::size_t serverPos = mHttpData.get()->find(server);
		if (serverPos != std::string::npos)
		{
			// find the size of progress bar
			std::string keyWord = "width: ";
			std::size_t progressPos = mHttpData.get()->find("width: ", serverPos);
			if (progressPos != std::string::npos)
			{
				// find the end % sign
				std::size_t startPos = progressPos + keyWord.length();
				std::size_t endPos = mHttpData.get()->find("%", startPos);
				if (startPos < endPos)
				{
					// extract the substring
					progress = mHttpData.get()->substr(startPos, endPos-startPos+1);
					mHtmlMutex.unlock();
					return true;
				}
			}
		}

		// http was good but something went wrong...
		progress = "No Data";
	}

	// http read was bad
	progress = "Error: " + std::to_string(mHttpCode);
	mHtmlMutex.unlock();

	return false;
}