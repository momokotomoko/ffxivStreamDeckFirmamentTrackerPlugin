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
				mConnectionManager->SetTitle(context.second + "\nLoading", context.first, kESDSDKTarget_HardwareAndSoftware);

			bool isSuccess = mFirmamentTrackerHelper->ReadFirmamentHTML();

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
			if (mContextServerMap.at(inContext).length() > 0)
			{
				std::string progress;
				bool isServerParsed = mFirmamentTrackerHelper->GetFirmamentProgress(mContextServerMap.at(inContext), progress);
				// Server name \n progress%
				if (isServerParsed)
					mConnectionManager->SetTitle(mContextServerMap.at(inContext) + "\n" + progress, inContext, kESDSDKTarget_HardwareAndSoftware);
				else
					mConnectionManager->SetTitle(mContextServerMap.at(inContext) + "\nNo Data", inContext, kESDSDKTarget_HardwareAndSoftware);
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
	// if key is pressed, open firmament page
	mConnectionManager->OpenUrl(mFirmamentTrackerHelper->html);
}

void FFXIVFirmamentTrackerPlugin::KeyUpForAction(const std::string& inAction, const std::string& inContext, const json &inPayload, const std::string& inDeviceID)
{
	// Nothing to do
}

/**
	@brief Runs when app shows up on streamdeck profile
**/
void FFXIVFirmamentTrackerPlugin::WillAppearForAction(const std::string& inAction, const std::string& inContext, const json &inPayload, const std::string& inDeviceID)
{
	// read payload for any saved settings
	std::string server = "";
	if (inPayload["settings"].find("Server") != inPayload["settings"].end())
	{
		server = inPayload["settings"]["Server"].get<std::string>();
	}

	// Remember the context and the saved server name for this app
	mVisibleContextsMutex.lock();
	// if this is the first plugin to be displayed, boot up the timers
	bool isEmpty = mContextServerMap.empty();
	if (isEmpty)
		startTimers();

	mContextServerMap.insert({ inContext, server });

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
	// check for the init signal
	if (inPayload.find("Init") != inPayload.end())
	{
		// if HTML read was good, setup the dropdown menu by sending all possible settings
		mInitMutex.lock();
		if (mFirmamentTrackerHelper->isHtmlGood())
		{
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

			mConnectionManager->SetGlobalSettings(j);
			#ifdef LOGGING
			mConnectionManager->LogMessage(j.dump(4));
			#endif
		}
		mInitMutex.unlock();
	}

	// PI dropdown menu has saved new settings for this context (aka server name changed), load those
	mVisibleContextsMutex.lock();
	if (inPayload.find("Server") != inPayload.end())
	{
		mContextServerMap.at(inContext) = inPayload["Server"].get<std::string>();
	}
	// update UI after changing server for this app
	this->UpdateUI(inContext);
	mVisibleContextsMutex.unlock();
}