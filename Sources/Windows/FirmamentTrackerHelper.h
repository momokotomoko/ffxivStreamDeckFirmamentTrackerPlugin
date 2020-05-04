//==============================================================================
/**
@file       FirmamentTrackerHelper.h

@brief		Load firmament html page and parse progress

@copyright  (c) 2020, Momoko Tomoko

**/
//==============================================================================

#pragma once

#include <mutex>

class FirmamentTrackerHelper
{
public:
	const std::string html = "https://na.finalfantasyxiv.com/lodestone/ishgardian_restoration/builders_progress_report";
	FirmamentTrackerHelper();
	~FirmamentTrackerHelper() {};

	bool GetFirmamentProgress(const std::string& server, std::string& progress);
	bool ReadFirmamentHTML();

private:
	std::unique_ptr<std::string> mHttpData;
	long mHttpCode = 0;

	std::mutex mHtmlMutex;
};
