//==============================================================================
/**
@file       FirmamentTrackerHelper.cpp

@brief		Load firmament html page and parse progress

@copyright  (c) 2020, Momoko Tomoko

**/
//==============================================================================

#include "pch.h"
#include "FirmamentTrackerHelper.h"

FirmamentTrackerHelper::FirmamentTrackerHelper()
{
	mHttpData.reset(new std::string());
}

/**
	@brief Read the html page and store it into htmlData string

	@return true if success 
**/
bool FirmamentTrackerHelper::ReadFirmamentHTML()
{
	mHtmlMutex.lock();

	// read html
	bool isSuccess = curlutils::readHTML(html, mHttpData.get(), mHttpCode);
	if (isSuccess)
	{
		// generate the dom tree
		htmlcxx::HTML::ParserDom parser;
		mDom = parser.parseTree(mHttpData.get()->c_str());

		isSuccess = parseRestorationServerHtml(mServerHierarchy, mServerStatus, mDom);
	}

	mIsSuccess = isSuccess;

	mHtmlMutex.unlock();

	return isSuccess;
}

/**
	@brief Check to see if the html read was good

	@return true if html was successfully read
**/
bool FirmamentTrackerHelper::isHtmlGood()
{
	mHtmlMutex.lock();
	bool isSuccess = mIsSuccess;
	mHtmlMutex.unlock();

	return isSuccess;
}

/**
	@brief Get the server hierarchy

	@return server hierarchy
**/
std::vector<FirmamentTrackerHelper::restorationRegion> FirmamentTrackerHelper::getServerHierarchy()
{
	mHtmlMutex.lock();
	std::vector<restorationRegion> serverHierarchy = mServerHierarchy;
	mHtmlMutex.unlock();

	return serverHierarchy;
}

/**
	@brief Parse raw html for firmament progress

	@param[in] server name of server to get progress for
	@param[out] progress percentage

	@return true if successful
**/
bool FirmamentTrackerHelper::GetFirmamentProgress(const std::string & server, std::string& progress)
{
	bool isSuccessful = false;
	mHtmlMutex.lock();
	if (mHttpCode == 200)
	{
		if (mServerStatus.find(server) != mServerStatus.end())
		{
			isSuccessful = mServerStatus.at(server).isValid;
			progress = mServerStatus.at(server).progress;
		}
		else
		{
			// http was good but something went wrong...
			progress = "No Data";
		}
	}
	else {
		// http read was bad
		progress = "Error: " + std::to_string(mHttpCode);
	}

	mHtmlMutex.unlock();

	return isSuccessful;
}

/*
	@brief Parse out server data

	@param[in] liIt iterator to the <li> block to parse
	@param[in] dom the parsed html document object model tree

	@return restorationServerStatus struct
*/
FirmamentTrackerHelper::restorationServerStatus FirmamentTrackerHelper::parseServerData(tree<htmlcxx::HTML::Node>::post_order_iterator liIt,
	tree<htmlcxx::HTML::Node>& dom)
{
	restorationServerStatus status;

	// parse world name
	auto worldNameIt = htmlcxxutils::htmlcxxFindNextAttribute("class", "world_name", liIt.begin(), liIt.end());
	if (worldNameIt == liIt.end()) return status;
	std::string worldName = dom.child(worldNameIt, 0)->text();

	// parse level
	auto levelIt = htmlcxxutils::htmlcxxFindNextAttribute("class", "level", worldNameIt.end(), liIt.end());
	if (levelIt == liIt.end()) return status;
	std::string level = dom.child(levelIt, 0)->text();

	// parse bar value
	auto barIt = htmlcxxutils::htmlcxxFindNextTag("span", levelIt.end(), liIt.end());
	if (barIt == liIt.end()) return status;
	barIt->parseAttributes();
	std::string barValue = barIt->attribute("style").second;

	// parse text
	auto textIt = htmlcxxutils::htmlcxxFindNextTag("p", barIt.end(), liIt.end());
	if (textIt == liIt.end()) return status;
	std::string text = dom.child(textIt, 0)->text();

	// convert bar value to progress
	std::string progress = "nan";
	// check for completion string
	const std::string completionWord = "Works Complete";
	std::size_t completionPos = barValue.find(completionWord);
	if (completionPos != std::string::npos)
	{
		progress = "Completed";
	}
	else
	{
		// find the size of progress bar if not completed
		const std::string keyWord = "width: ";
		std::size_t progressPos = barValue.find("width: ");
		if (progressPos != std::string::npos)
		{
			// find the end % sign
			std::size_t startPos = progressPos + keyWord.length();
			std::size_t endPos = barValue.find("%", startPos);
			if (startPos < endPos)
			{
				// extract the substring
				progress = barValue.substr(startPos, endPos - startPos + 1);
			}
		}
	}

	status = { worldName, progress, level, text, true };
	return status;
}

/*
	@brief Parse restoration html to extract server data

	@param[out] serverHierarchy how the servers are organized
	@param[out] serverStatus parsed info for each server
	@param[in] dom the parsed html document object model tree

	@return true if success
*/
bool FirmamentTrackerHelper::parseRestorationServerHtml(std::vector<restorationRegion>& serverHierarchy,
	std::unordered_map<std::string, restorationServerStatus>& serverStatus,
	tree<htmlcxx::HTML::Node>& dom)
{
	serverHierarchy.clear();
	serverStatus.clear();

	// load the region names into vector
	// the region names are in the html before everything else, hence we have
	// to gather the names here first
	auto regionIt = htmlcxxutils::htmlcxxFindNextAttribute("class", "report-region_select", dom.begin(), dom.end());

	if (regionIt == dom.end()) return false;

	for (auto subRegionIt = regionIt; subRegionIt != regionIt.end(); subRegionIt++)
	{
		if (subRegionIt->isTag() && htmlcxxutils::strCaseCmp(subRegionIt->tagName(), "A"))
		{
			std::string region = dom.child(subRegionIt, 0)->text();
			if (region.length() > 0)
				serverHierarchy.push_back({ region, {} });
		}
	}

	// the regions are now wrapped in the next few divs at the same depth as "report-region_select"
	// just iterate through the siblings of  "report-region_select"
	auto nextRegionIt = dom.next_sibling(regionIt);
	if (nextRegionIt == dom.end() || nextRegionIt == NULL) return false;

	auto dataIt = serverHierarchy.begin();

	// go through each sibling of "report-region_select"
	for (auto nextRegionIt = dom.next_sibling(regionIt);
		nextRegionIt != NULL;
		nextRegionIt = dom.next_sibling(nextRegionIt))
	{
		// look for instance of div, skip over anything else
		if (!(nextRegionIt->isTag() && (htmlcxxutils::strCaseCmp(nextRegionIt->tagName(), "div")))) continue;

		// return error if we have more divs here than region names
		if (dataIt == serverHierarchy.end()) return false;

		// go through the div looking for the attribute "report-dc_name" for the dc's
		for (auto dcIt = htmlcxxutils::htmlcxxFindNextAttribute("class", "report-dc_name", nextRegionIt.begin(), nextRegionIt.end());
			dcIt != nextRegionIt.end();
			dcIt = htmlcxxutils::htmlcxxFindNextAttribute("class", "report-dc_name", dcIt.begin(), nextRegionIt.end()))
		{
			std::string dcName = dom.child(dcIt, 0)->text();
			if (dcName.length() == 0) return false;

			// return error if dc is repeated
			if (dataIt->dc.find(dcName) != dataIt->dc.end()) return false;

			dataIt->dc.insert({ dcName, {} });

			// The worlds in the dc are listed under the tag with class "report-world_list".
			// They are stored under the "li" tags
			auto worldListIt = htmlcxxutils::htmlcxxFindNextAttribute("class", "report-world_list", dcIt.begin(), nextRegionIt.end());
			if (worldListIt == nextRegionIt.end()) return false;

			// go through each "li" tag and parse out the info we need
			for (auto liIt = htmlcxxutils::htmlcxxFindNextTag("li", worldListIt.begin(), worldListIt.end());
				liIt != worldListIt.end();
				liIt = htmlcxxutils::htmlcxxFindNextTag("li", liIt.end(), worldListIt.end()))
			{
				restorationServerStatus status = parseServerData(liIt, dom);

				if (status.isValid)
				{
					// return error if server is repeated
					if (serverStatus.find(status.name) != serverStatus.end()) return false;

					serverStatus.insert({ status.name, status });
					dataIt->dc.at(dcName).servers.insert(status.name);
				}
			}
		}

		dataIt++;
	}

	return true;
}

/*
	@brief Parse info about a single server

	@param[in] server the server to gather data for
	@param[in] dom the parsed html document object model tree

	@return restoration status
*/
FirmamentTrackerHelper::restorationServerStatus FirmamentTrackerHelper::parseServerStatus(const std::string& server, tree<htmlcxx::HTML::Node>& dom)
{
	for (auto it = dom.begin(); it != dom.end(); it++)
	{
		if ((!it->isTag()) && (!it->isComment()))
		{
			if (htmlcxxutils::strCaseCmp(server, it->text()))
			{
				auto spanIt = dom.parent(it);
				if (spanIt == NULL || spanIt == dom.end()) return {};

				auto liIt = dom.parent(spanIt);
				if (liIt == NULL || liIt == dom.end()) return {};

				if (!liIt->isTag() || !htmlcxxutils::strCaseCmp(liIt->tagName(), "li")) return {};

				restorationServerStatus status = parseServerData(liIt, dom);

				if (!status.isValid) return {};

				return status;
			}
		}
	}

	return {};
}