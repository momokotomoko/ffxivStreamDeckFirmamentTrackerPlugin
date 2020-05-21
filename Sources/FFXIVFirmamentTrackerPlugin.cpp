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

	// timer that is called every hour on the 1 minute mark to grab raw html
	const int triggerMinuteOfTheHour = { 1 };
	mTimer->start(triggerMinuteOfTheHour, [this]()
	{
		#ifdef LOGGING
			mConnectionManager->LogMessage("Reading HTML...");
		#endif
		// read HTML and update UI once triggered
		bool status = mFirmamentTrackerHelper->ReadFirmamentHTML();
		this->UpdateUI();
		#ifdef LOGGING
			mConnectionManager->LogMessage("Reading status: " + std::to_string(status));
		#endif
		return status;
	});
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
	@brief Updates all visible contexts for this app by parsing firmament progress from stored raw html
**/
void FFXIVFirmamentTrackerPlugin::UpdateUI()
{
	if(mConnectionManager != nullptr)
	{
		bool isSuccessful = true;
		mVisibleContextsMutex.lock();
		// go through all our visible contexts and set the title to show the firmament progress
		for (const auto & context : mContextServerMap)
		{
			if (context.second.length() > 0)
			{
				std::string progress;
				if (!mFirmamentTrackerHelper->GetFirmamentProgress(context.second, progress))
				{
					isSuccessful = false;
				}
				// Server name \n progress%
				mConnectionManager->SetTitle(context.second + "\n" + progress, context.first, kESDSDKTarget_HardwareAndSoftware);
			}
		}
		mVisibleContextsMutex.unlock();

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
	mContextServerMap.insert({ inContext, server });
	mVisibleContextsMutex.unlock();

	// update the UI with firmament percentages
	this->UpdateUI();
}

/**
	@brief Runs when app is hidden streamdeck profile
**/
void FFXIVFirmamentTrackerPlugin::WillDisappearForAction(const std::string& inAction, const std::string& inContext, const json &inPayload, const std::string& inDeviceID)
{
	// Remove this particular context so we don't have to process it when updating UI
	mVisibleContextsMutex.lock();
	mContextServerMap.erase(inContext);
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
	// PI dropdown menu has saved new settings for this context (aka server name changed), load those
	mVisibleContextsMutex.lock();
	if (inPayload.find("Server") != inPayload.end())
	{
		mContextServerMap.at(inContext) = inPayload["Server"].get<std::string>();
	}
	mVisibleContextsMutex.unlock();

	// update UI after changing server for this app
	this->UpdateUI();
}