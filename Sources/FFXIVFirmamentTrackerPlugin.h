//==============================================================================
/**
@file       FFXIVFirmamentTrackerPlugin.h

@brief      FFXIV Firmament Tracker plugin

@copyright  (c) 2020, Momoko Tomoko

**/
//==============================================================================

#include "Common/ESDBasePlugin.h"
#include <mutex>
#include <unordered_map>

class FirmamentTrackerHelper;
class CallBackTimer;
class StreamDeckImageManager;

class FFXIVFirmamentTrackerPlugin : public ESDBasePlugin
{
public:
	
	FFXIVFirmamentTrackerPlugin();
	virtual ~FFXIVFirmamentTrackerPlugin();
	
	void KeyDownForAction(const std::string& inAction, const std::string& inContext, const json &inPayload, const std::string& inDeviceID) override;
	void KeyUpForAction(const std::string& inAction, const std::string& inContext, const json &inPayload, const std::string& inDeviceID) override;
	
	void WillAppearForAction(const std::string& inAction, const std::string& inContext, const json &inPayload, const std::string& inDeviceID) override;
	void WillDisappearForAction(const std::string& inAction, const std::string& inContext, const json &inPayload, const std::string& inDeviceID) override;
	
	void DeviceDidConnect(const std::string& inDeviceID, const json &inDeviceInfo) override;
	void DeviceDidDisconnect(const std::string& inDeviceID) override;
	
	void SendToPlugin(const std::string& inAction, const std::string& inContext, const json &inPayload, const std::string& inDeviceID) override;
	void DidReceiveGlobalSettings(const json& inPayload) override;
private:
	
	void UpdateUI(const std::string& inContext);
	
	std::mutex mVisibleContextsMutex;

	// this struct contains a context's saved settings
	struct contextMetaData_t
	{
		std::string onClickUrl; // webpage to open on click, each button can have a different webpage
		std::string server; // name of the server this context is recording
		std::string imageName;
	};
	std::unordered_map<std::string, contextMetaData_t> mContextServerMap;

	contextMetaData_t readJsonIntoMetaData(const json& payload);
	
	std::unique_ptr<FirmamentTrackerHelper> mFirmamentTrackerHelper = std::make_unique <FirmamentTrackerHelper>();
	std::unique_ptr<CallBackTimer> mTimer = std::make_unique <CallBackTimer>();

	std::unique_ptr<StreamDeckImageManager> mStreamDeckImageManager = std::make_unique <StreamDeckImageManager>("Icons/");

	void startTimers();

	std::string mUrl = "https://na.finalfantasyxiv.com/lodestone/ishgardian_restoration/builders_progress_report/";
	bool mFirstRead = true; // if we're on the first read of this url

	bool isInit = false; // on init we need to call GetGlobalSettings
};
