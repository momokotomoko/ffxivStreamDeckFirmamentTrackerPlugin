//==============================================================================
/**
@file       FFXIVFirmamentTrackerPlugin.cpp

@brief      FFXIV Firmament Tracker plugin

@copyright  (c) 2020, Momoko Tomoko

**/
//==============================================================================

#include "FFXIVFirmamentTrackerPlugin.h"
#include <atomic>

#include "Windows/FirmamentTrackerHelper.h"
#include "Windows/CallBackTimer.h"

#include "Common/ESDConnectionManager.h"

//#define LOGGING


FFXIVFirmamentTrackerPlugin::FFXIVFirmamentTrackerPlugin()
{
	mFirmamentTrackerHelper = new FirmamentTrackerHelper();
	mTimer = new CallBackTimer();
}

FFXIVFirmamentTrackerPlugin::~FFXIVFirmamentTrackerPlugin()
{
	if(mTimer != nullptr)
	{
		mTimer->stop();
		
		delete mTimer;
		mTimer = nullptr;
	}
	
	if(mFirmamentTrackerHelper != nullptr)
	{
		delete mFirmamentTrackerHelper;
		mFirmamentTrackerHelper = nullptr;
	}
}

/**
	@brief Starts the callback timers for this plugin
**/
void FFXIVFirmamentTrackerPlugin::startTimers()
{
	mTimer->stop();

    #ifdef LOGGING
	mConnectionManager->LogMessage("Starting timers...");
    #endif

	// timer that is called every hour on the 1 minute mark to grab raw html
	std::set<int> triggerMinuteOfTheHour = { 1 };
	mTimer->start(triggerMinuteOfTheHour, [this]()
		{
            #ifdef LOGGING
			mConnectionManager->LogMessage("Reading HTML...");
            #endif

			// read HTML and update UI once triggered
			mVisibleContextsMutex.lock();
			for (const auto& context : mContextServerMap)
				mConnectionManager->SetTitle(context.second.server + "\nLoading", context.first, kESDSDKTarget_HardwareAndSoftware);

			bool isSuccess = mFirmamentTrackerHelper->ReadFirmamentHTML(mUrl);

			for (const auto& context : mContextServerMap)
				this->UpdateUI(context.first);
			mVisibleContextsMutex.unlock();

            #ifdef LOGGING
			mConnectionManager->LogMessage("Reading status: " + std::to_string(status));
            #endif
			return isSuccess;
		});
}

/**
	@brief Updates all visible contexts for this app by parsing firmament progress from stored raw html
**/
void FFXIVFirmamentTrackerPlugin::UpdateUI(const std::string& inContext)
{
	// warning: lock mVisibleContextsMutex before calling!

	if(mConnectionManager != nullptr)
	{
		bool isSuccessful = true;
		// go through all our visible contexts and set the title to show the firmament progress
		if (mContextServerMap.find(inContext) != mContextServerMap.end())
		{
			if (mContextServerMap.at(inContext).server.length() > 0)
			{
				std::string progress;
				bool isServerParsed = mFirmamentTrackerHelper->GetFirmamentProgress(mContextServerMap.at(inContext).server, progress);
				// Server name \n progress%
				if (isServerParsed)
					mConnectionManager->SetTitle(mContextServerMap.at(inContext).server + "\n" + progress, inContext, kESDSDKTarget_HardwareAndSoftware);
				else
					mConnectionManager->SetTitle(mContextServerMap.at(inContext).server + "\nNo Data", inContext, kESDSDKTarget_HardwareAndSoftware);
			}
			else
				mConnectionManager->SetTitle("", inContext, kESDSDKTarget_HardwareAndSoftware);
		}

		// re-load html if something failed
		if (!isSuccessful)
		{
			#ifdef LOGGING
			mConnectionManager->LogMessage("Something went wrong parsing progress percentage from HTML, attempt reloading page...");
			#endif
			mTimer->wake();
		}
	}
}


void FFXIVFirmamentTrackerPlugin::KeyDownForAction(const std::string& inAction, const std::string& inContext, const json &inPayload, const std::string& inDeviceID)
{
	mVisibleContextsMutex.lock();
	std::string url = mContextServerMap.at(inContext).onClickUrl;
	mVisibleContextsMutex.unlock();

	if (url.find("https://") != 0 && url.find("http://") != 0)
		url = "https://" + url;
	mConnectionManager->OpenUrl(url);
}

void FFXIVFirmamentTrackerPlugin::KeyUpForAction(const std::string& inAction, const std::string& inContext, const json &inPayload, const std::string& inDeviceID)
{
	// Nothing to do
}

/**
	@brief Reads a payload into a contextMetaData_t struct

	@param[in] payload the json payload

	@return the contextMetaData_t struct
**/
FFXIVFirmamentTrackerPlugin::contextMetaData_t FFXIVFirmamentTrackerPlugin::readJsonIntoMetaData(const json& payload)
{
#ifdef LOGGING
	mConnectionManager->LogMessage(payload.dump(4));
#endif
	contextMetaData_t data{};
	if (payload.find("Server") != payload.end())
	{
		data.server = payload["Server"].get<std::string>();
	}
	if (payload.find("OnClickUrl") != payload.end())
	{
		data.onClickUrl = payload["OnClickUrl"].get<std::string>();
	}
	return data;
}

/**
	@brief Runs when app shows up on streamdeck profile
**/
void FFXIVFirmamentTrackerPlugin::WillAppearForAction(const std::string& inAction, const std::string& inContext, const json &inPayload, const std::string& inDeviceID)
{
	// read payload for any saved settings
	contextMetaData_t data{};
	if (inPayload.find("settings") != inPayload.end())
	{
		data = readJsonIntoMetaData(inPayload["settings"]);
	}
	
	mVisibleContextsMutex.lock();
	// if just launched, try to grab any global settings
	if (!isInit)
	{
		mConnectionManager->GetGlobalSettings();
		isInit = true;
	}

	// if this is the first plugin to be displayed, boot up the timers
	bool isEmpty = mContextServerMap.empty();
	if (isEmpty)
		startTimers();

	// Remember the context and the saved metadata
	mContextServerMap.insert({ inContext, data });

	// update the UI with firmament percentages
	if (!isEmpty)
	{
		this->UpdateUI(inContext);
	}
	mVisibleContextsMutex.unlock();
}

/**
	@brief Runs when app is hidden streamdeck profile
**/
void FFXIVFirmamentTrackerPlugin::WillDisappearForAction(const std::string& inAction, const std::string& inContext, const json &inPayload, const std::string& inDeviceID)
{
	// Remove this particular context so we don't have to process it when updating UI
	mVisibleContextsMutex.lock();
	mContextServerMap.erase(inContext);

	// if we have no active plugin displayed, kill the timers to save cpu cycles
	if (mContextServerMap.empty())
	{
		mTimer->stop();
	}
	mVisibleContextsMutex.unlock();
}

void FFXIVFirmamentTrackerPlugin::DeviceDidConnect(const std::string& inDeviceID, const json &inDeviceInfo)
{
	// Nothing to do
}

void FFXIVFirmamentTrackerPlugin::DeviceDidDisconnect(const std::string& inDeviceID)
{
	// Nothing to do
}

/**
	@brief Runs when app recieves payload from PI, which is when PI dropdown menu changes
**/
void FFXIVFirmamentTrackerPlugin::SendToPlugin(const std::string& inAction, const std::string& inContext, const json &inPayload, const std::string& inDeviceID)
{
	mVisibleContextsMutex.lock();
	// PI dropdown menu has saved new settings for this context, load those
	if (mContextServerMap.find(inContext) != mContextServerMap.end())
	{
		// updated stored settings
		mContextServerMap.at(inContext) = readJsonIntoMetaData(inPayload);
	}
	else
	{
		mConnectionManager->LogMessage("Error: SendToPlugin: could not find stored context: " + inContext + "\nPayload:\n" + inPayload.dump(4));
		mConnectionManager->LogMessage(inContext);
	}
	// update UI after changing server for this app
	this->UpdateUI(inContext);

	mVisibleContextsMutex.unlock();
}

/**
	@brief Runs when app recieves global settings
**/
void FFXIVFirmamentTrackerPlugin::DidReceiveGlobalSettings(const json& inPayload)
{
	mVisibleContextsMutex.lock();
	// check for change in firmament website
	json j = inPayload["settings"];
	if (j.find("FirmamentUrl") != j.end())
	{
		std::string url = j["FirmamentUrl"].get<std::string>();
		if (url != mUrl)
		{
			mUrl = url;
			mFirstRead = true;
		}
	}

	// do a test read and grab the server hierarchy here
	if (mFirstRead) // only do this once per new url
	{
		bool isSuccess = mFirmamentTrackerHelper->ReadFirmamentHTML(mUrl);
		if (isSuccess)
		{
			if (mFirmamentTrackerHelper->isHtmlGood())
			{
				// generate the hierarcchy and send as global setting
				json j;
				std::vector<FirmamentTrackerHelper::restorationRegion> serverHierarchy = mFirmamentTrackerHelper->getServerHierarchy();
				for (const auto region : serverHierarchy)
				{
					for (const auto dc : region.dc)
					{
						for (const auto server : dc.second.servers)
						{
							j["menu"][region.name][dc.first][server];
						}
					}
				}
				j["FirmamentUrl"] = mUrl;

				mConnectionManager->SetGlobalSettings(j);
                #ifdef LOGGING
				mConnectionManager->LogMessage(j.dump(4));
                #endif

				mFirstRead = false;
			}
		}
	}

	// send reload command now that global settings are sent
	for (const auto& context : mContextServerMap)
	{
		json j;
		j["reload"];
		mConnectionManager->SendToPropertyInspector("", context.first, j);
	}

	// wake timer to update all UI elements
	mTimer->wake();
	mVisibleContextsMutex.unlock();
}