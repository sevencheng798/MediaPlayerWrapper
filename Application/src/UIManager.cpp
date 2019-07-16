/*
 * Copyright 2019 its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include <iostream>
#include <string>
#include <Utils/Logging/Logger.h>

#include "Application/UIManager.h"
#include "properties.h"

static const std::string TAG{"UIManager"};
#define LX(event) aisdk::utils::logging::LogEntry(TAG, event)

namespace aisdk {
namespace application {
	static const std::string HELP_MESSAGE =
		"\n+----------------------------------------------------------------------------+\n"
		"|                              Options:                                      |\n"
		"| Info:                                                                      |\n"
        "|       Press 'i' followed by Enter at any time to see the help screen.      |\n"
        "| Microphone off:                                                            |\n"
        "|       Press 'm' and Enter to turn on and off the microphone.               |\n"
		"| Playback to control:                                                       |\n"
		"|        Press 'p' and Enter to PLAY and PAUSE state                         |\n"
		"| Stop an interaction:                                                       |\n"
		"|        Press 's' and Enter to stop an ongoing interaction.                 |\n";

static const std::string CONNECTION_MESSAGE =
		"\n#############################################\n"
		"########    Network Disconnected    ###########\n"
		"#############################################\n";

static const std::string IDLE_MESSAGE =
		"\n#############################################\n"
		"########## NLP Client is currently IDLE! ####\n"
		"#############################################\n";

static const std::string LISTEN_MESSAGE =
		"\n#############################################\n"
		"##########    LISTENING          ############\n"
		"#############################################\n";

static const std::string SPEAK_MESSAGE =
		"\n#############################################\n"
		"##########    SPEAKING          #############\n"
		"#############################################\n";

static const std::string THINK_MESSAGE =
		"\n#############################################\n"
		"##########    THINKING          #############\n"
		"#############################################\n";

UIManager::UIManager():
	m_dialogState{DialogUXStateObserverInterface::DialogUXState::IDLE} {

}

void UIManager::onDialogUXStateChanged(DialogUXStateObserverInterface::DialogUXState newState) {
	m_executor.submit([this, newState]() {
			if(m_dialogState == newState)
				return;

			m_dialogState = newState;
			printState();
		});
}

void UIManager::onNetworkStatusChanged(const Status newState) {
	m_executor.submit([this, newState]() {
		if(newState == utils::NetworkStateObserverInterface::Status::DISCONNECTED) {
			AISDK_INFO(LX(CONNECTION_MESSAGE));
			
		}else if(newState == utils::NetworkStateObserverInterface::Status::CONNECTED) {

		}

	//	m_connectionState = newState;
	});

}

bool UIManager::onVolumeChange(dmInterface::VolumeObserverInterface::Type volumeType, int volume) {
	AISDK_DEBUG5(LX("onVolumeChange").d("Type", volumeType).d("volume", volume));
	
	m_executor.submit([this, volumeType, volume]() { adjustVolume(volumeType, volume); });

	return true;
}

void UIManager::asrRefreshConfiguration(
		const std::string &uid,
		const std::string &appid, 
		const std::string &key,
		const std::string &deviceId) {
	AISDK_DEBUG5(LX("asrRefreshConfiguration").d("uid", uid));
	if(uid.empty()) {
		AISDK_ERROR(LX("asrRefreshConfigurationFailed").d("reason", "uidIsEmpty"));
	}else{
		//setprop((char *)"xf.aiui.uid", (char *)uid.c_str());
		// TODO: IPC to notify gm_task
		
	}
}

void UIManager::printErrorScreen() {
    m_executor.submit([]() { AISDK_INFO(LX("Invalid Option")); });
}

void UIManager::printHelpScreen() {
    m_executor.submit([]() { AISDK_INFO(LX(HELP_MESSAGE)); });
}

void UIManager::microphoneOff() {
	// TODO: LED to indicate.
    m_executor.submit([]() { AISDK_INFO(LX("Microphone Off!")); });
}

void UIManager::microphoneOn() {
    m_executor.submit([this]() { printState(); });
}

void UIManager::adjustVolume(dmInterface::VolumeObserverInterface::Type volumeType, int volume) {
	//to-do
}

void UIManager::printState() {
	switch(m_dialogState) {
		case DialogUXStateObserverInterface::DialogUXState::IDLE:
			AISDK_INFO(LX(IDLE_MESSAGE));
			break;
		case DialogUXStateObserverInterface::DialogUXState::LISTENING:
			AISDK_INFO(LX(LISTEN_MESSAGE));
			system("aplay /cfg/sai_config/wakeup/wakeup_2.wav");
			break;
		case DialogUXStateObserverInterface::DialogUXState::LISTEN_EXPECTING:
			AISDK_INFO(LX(LISTEN_MESSAGE));
			break;			
		case DialogUXStateObserverInterface::DialogUXState::THINKING:
			AISDK_INFO(LX(THINK_MESSAGE));
			break;
		case DialogUXStateObserverInterface::DialogUXState::SPEAKING:
			AISDK_INFO(LX(SPEAK_MESSAGE));
			break;
		case DialogUXStateObserverInterface::DialogUXState::FINISHED:
			AISDK_INFO(LX("TIMEOUT"));
			break;
	}
}

} // namespace application
} // namespace aisdk
