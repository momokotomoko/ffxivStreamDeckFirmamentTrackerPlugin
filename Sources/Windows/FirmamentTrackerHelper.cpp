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

	@param[in] url url to html page

	@return true if success 
**/
bool FirmamentTrackerHelper::readFirmamentHTML(const std::string & url)
{
	mHtmlMutex.lock();

	// read html
	bool isSuccess = curlutils::readHTML(url, mHttpData.get(), mHttpCode);
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
std::vector<FirmamentTrackerHelper::restorationRegion_t> FirmamentTrackerHelper::getServerHierarchy()
{
	mHtmlMutex.lock();
	std::vector<restorationRegion_t> serverHierarchy = mServerHierarchy;
	mHtmlMutex.unlock();

	return serverHierarchy;
}

/**
	@brief Get the status for this server

	@param[in] server name of server to get progress for
	@param[out] progress percentage

	@return restorationServerStatus_t struct
**/
const FirmamentTrackerHelper::restorationServerStatus_t FirmamentTrackerHelper::getFirmamentStatus(const std::string & server)
{
	restorationServerStatus_t status;
	mHtmlMutex.lock();
	if (mHttpCode == 200)
	{
		if (mServerStatus.find(server) != mServerStatus.end())
			status = mServerStatus.at(server);
		else
			// http was good but could not parse page for server info
			status.progress = "No Data";
	}
	else {
		// http read was bad
		status.progress = "Error: " + std::to_string(mHttpCode);
	}
	mHtmlMutex.unlock();

	return status;
}

/*
	@brief Parse out server data

	@param[in] liIt iterator to the <li> block to parse
	@param[in] dom the parsed html document object model tree

	@return restorationServerStatus_t struct
*/
FirmamentTrackerHelper::restorationServerStatus_t FirmamentTrackerHelper::parseServerData(tree<htmlcxx::HTML::Node>::post_order_iterator liIt,
	tree<htmlcxx::HTML::Node>& dom)
{
	restorationServerStatus_t status = {};

	// parse world name
	auto worldNameIt = htmlcxxutils::htmlcxxFindNextAttribute("class", "world_name", liIt.begin(), liIt.end());
	if (worldNameIt == htmlcxxutils::pre_order_it(liIt.end())) return status;
	std::string worldName = dom.child(worldNameIt, 0)->text();

	// parse level
	auto levelIt = htmlcxxutils::htmlcxxFindNextAttribute("class", "level", worldNameIt.end(), liIt.end());
	if (levelIt == htmlcxxutils::pre_order_it(liIt.end())) return status;
	std::string level = dom.child(levelIt, 0)->text();

	// parse bar value
	auto barIt = htmlcxxutils::htmlcxxFindNextAttribute("class", "bar", worldNameIt.end(), liIt.end());
	if (barIt == htmlcxxutils::pre_order_it(liIt.end())) return status;
	std::string barValue;
	for (auto it = barIt; it != htmlcxxutils::pre_order_it(barIt.end()); it++)
		barValue += it->text();

	// parse text
	auto textIt = htmlcxxutils::htmlcxxFindNextAttribute("class", "text", worldNameIt.end(), liIt.end());
	std::string text = "";
	if (textIt != htmlcxxutils::pre_order_it(liIt.end())) text = dom.child(textIt, 0)->text();

	// convert bar value to progress
	std::string progress = "nan";
	float progressF = 0.0;
	// check for completion string
	const std::string completionWord = "Completed";
	std::size_t completionPos = barValue.find(completionWord);
	if (completionPos != std::string::npos)
	{
		progress = "Completed";
		progressF = 100.0;
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
				progress = barValue.substr(startPos, endPos - startPos);
				progressF = std::stof(progress);
				progress += "%";
			}
		}
	}
	status = { worldName, progress, level, text, progressF, true };
	return status;
}

/*
	@brief Parse restoration html to extract server data

	@param[out] serverHierarchy how the servers are organized
	@param[out] serverStatus parsed info for each server
	@param[in] dom the parsed html document object model tree

	@return true if success
*/
bool FirmamentTrackerHelper::parseRestorationServerHtml(std::vector<restorationRegion_t>& serverHierarchy,
	std::unordered_map<std::string, restorationServerStatus_t>& serverStatus,
	tree<htmlcxx::HTML::Node>& dom)
{
	serverHierarchy.clear();
	serverStatus.clear();

	// load the region names into vector
	// the region names are in the html before everything else, hence we have
	// to gather the names here first
	auto regionIt = htmlcxxutils::htmlcxxFindNextAttribute("class", "report-region_select", dom.begin(), dom.end());

	if (regionIt == dom.end()) return false;

	for (auto subRegionIt = regionIt; subRegionIt != htmlcxxutils::pre_order_it(regionIt.end()); subRegionIt++)
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
			dcIt != htmlcxxutils::pre_order_it(nextRegionIt.end());
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
			if (worldListIt == htmlcxxutils::pre_order_it(nextRegionIt.end())) return false;

			// go through each "li" tag and parse out the info we need
			for (auto liIt = htmlcxxutils::htmlcxxFindNextTag("li", worldListIt.begin(), worldListIt.end());
				liIt != htmlcxxutils::pre_order_it(worldListIt.end());
				liIt = htmlcxxutils::htmlcxxFindNextTag("li", liIt.end(), worldListIt.end()))
			{
				restorationServerStatus_t status = parseServerData(liIt, dom);

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
FirmamentTrackerHelper::restorationServerStatus_t FirmamentTrackerHelper::parseServerStatus(const std::string& server, tree<htmlcxx::HTML::Node>& dom)
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

				restorationServerStatus_t status = parseServerData(liIt, dom);

				if (!status.isValid) return {};

				return status;
			}
		}
	}

	return {};
}