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

#include "Common/ESDConnectionManager.h"

//#define LOGGING

/**
	@brief Timer that triggers on a specified minute of every hour, or on wake
**/
class CallBackTimer
{
public:
	CallBackTimer()
	{
		lock();
	}

	~CallBackTimer()
	{
		stop();
	}

	/**
		@brief Unlocks mutex and stops/joins the thread
	**/
	void stop()
	{
		running = false;
		if (locked)
		{
			unlock();
		}
		if (thd.joinable())
			thd.join();
	}

	/**
		@brief Unlocks mutex and stops/joins the thread

		@param[in] triggerMinuteOfTheHour the minute of the hour to trigger on (0-59)
		@param[in] func the function to trigger
	**/
	void start(unsigned int triggerMinuteOfTheHour, std::function<bool(void)> func)
	{
		triggerMinuteOfTheHour = min(0, triggerMinuteOfTheHour);
		running = true;

		// start the timer thread
		thd = std::thread([this, triggerMinuteOfTheHour, func]()
			{
				while (running)
				{
					// get current time and set the minute to the trigger minute
					time_t now = time(0);
					struct tm newTime {};
					localtime_s(&newTime, &now);
					newTime.tm_sec = 0;
					newTime.tm_min = triggerMinuteOfTheHour;
					time_t nextTriggerTime = mktime(&newTime);

					// if the new time is behind us, it means the next trigger minute is in an hour
					if (difftime(nextTriggerTime, now) < 1)
					{
						nextTriggerTime += 3600; // increment by an hour
					}

					// call the desired function
					int status = func();

					int waitTime = 0;
					if (status == false) // function failed, retry in 5s
					{
						waitTime = 5;
					}
					else
					{
						// function may be slow, recompute current time, diff maybe be negative which in that case we immeadiatly unlock mutex
						now = time(0);
						waitTime = (int)(difftime(nextTriggerTime, now));
					}

					// wait
					if (timerMutex.try_lock_for(std::chrono::seconds(waitTime)))
					{
						unlock();
					}
					if (!locked)
					{
						lock();
					}
				}
			});
	}

	/**
		@brief checks if thread is running

		@return true if thread is running
	**/
	bool is_running() const noexcept
	{
		return (running && thd.joinable());
	}

	/**
		@brief force a wakeup even if trigger time not met
	**/
	void wake()
	{
		if (is_running() && locked)
		{
			unlock();
		}
	}
private:
	std::atomic_bool running = false;
	std::thread thd;

	std::timed_mutex timerMutex;
	std::atomic_bool locked = false;

	void lock()
	{
		timerMutex.lock();
		locked = true;
	}

	void unlock()
	{
		locked = false;
		timerMutex.unlock();
	}
};

FFXIVFirmamentTrackerPlugin::FFXIVFirmamentTrackerPlugin()
{
	mFirmamentTrackerHelper = new FirmamentTrackerHelper();
	mTimer = new CallBackTimer();

	// timer that is called every hour on the 1 minute mark to grab raw html
	const int triggerMinuteOfTheHour = 1;
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
	ShellExecuteA(NULL, "open", mFirmamentTrackerHelper->html.c_str(), NULL, NULL, SW_SHOWNORMAL);
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