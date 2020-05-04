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
private:
	
	void UpdateUI();
	
	std::mutex mVisibleContextsMutex;
	std::unordered_map<std::string, std::string> mContextServerMap;
	
	FirmamentTrackerHelper *mFirmamentTrackerHelper = nullptr;
	CallBackTimer *mTimer;
};
