//==============================================================================
/**
@file       FirmamentTrackerHelper.h

@brief		Load firmament html page and parse progress

@copyright  (c) 2020, Momoko Tomoko

**/
//==============================================================================

#pragma once

#include <mutex>
#include "HtmlcxxUtils.hpp"
#include "CurlUtils.hpp"

class FirmamentTrackerHelper
{
public:
	/*
		struct to store server hierarchy
		Hierarchy: Region->DC->server
	*/
	struct restorationDC
	{
		std::set<std::string> servers;
	};
	struct restorationRegion
	{
		std::string name = "";
		std::map<std::string, FirmamentTrackerHelper::restorationDC> dc = {};
	};

	// struct to store parsed server data
	struct restorationServerStatus
	{
		std::string name = "";
		std::string progress = "";
		std::string level = "";
		std::string text = "";
		bool isValid = false;
	};

	const std::string html = "https://na.finalfantasyxiv.com/lodestone/ishgardian_restoration/builders_progress_report";
	FirmamentTrackerHelper();
	~FirmamentTrackerHelper() {};

	bool GetFirmamentProgress(const std::string& server, std::string& progress);
	bool ReadFirmamentHTML();
	bool isHtmlGood();

	std::vector<FirmamentTrackerHelper::restorationRegion> getServerHierarchy();

private:
	std::unique_ptr<std::string> mHttpData; // raw html string
	long mHttpCode = 0; // error code from curl after downloading html string
	tree<htmlcxx::HTML::Node> mDom; // tree structure for parsed html data
	bool mIsSuccess = false; // status of previous read

	std::mutex mHtmlMutex;

	// the html doesn't wrap the regions, it's just in order that it appears,
	// so don't use unordered_map here since we need to preserve the order we loaded the regions
	std::vector<restorationRegion> mServerHierarchy = {};
	std::unordered_map<std::string, restorationServerStatus> mServerStatus;

	restorationServerStatus parseServerStatus(const std::string& server, tree<htmlcxx::HTML::Node>& dom);
	restorationServerStatus parseServerData(tree<htmlcxx::HTML::Node>::post_order_iterator liIt,
											tree<htmlcxx::HTML::Node>& dom);
	bool parseRestorationServerHtml(std::vector<restorationRegion>& serverHierarchy,
		std::unordered_map<std::string, restorationServerStatus>& serverStatus,
		tree<htmlcxx::HTML::Node>& dom);
};
