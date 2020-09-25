//==============================================================================
/**
@file       CurlUtils.hpp
@brief      utilities for downloading html pages with curl
@copyright  (c) 2020, Momoko Tomoko
**/
//==============================================================================

#pragma once
#define CURL_STATICLIB
#include <curl\curl.h>

#include <string>

namespace curlutils
{
	/**
		@brief Callback function used for curl read
	**/
	static std::size_t callback(
		const char* in,
		std::size_t size,
		std::size_t num,
		std::string* out)
	{
		const std::size_t totalBytes(size * num);
		out->append(in, totalBytes);
		return totalBytes;
	}

	/**
		@brief download url's html data into string

		@param[in] html the url to download from
		@param[out] data html data downloaded

		@return true if success
	**/
	static bool readHTML(const std::string& html, std::string* data, long& httpCode)
	{
		CURL* curl;

		curl = curl_easy_init();
		curl_easy_setopt(curl, CURLOPT_URL, html);
		// Hide progress bar
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
		// Don't wait forever, time out after 10 seconds.
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);

		data->clear();

		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, data);

		// grab raw html
		curl_easy_perform(curl);
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

		curl_easy_cleanup(curl);

		if (httpCode == 200)
		{
			return true;
		}
		return false;
	}
}