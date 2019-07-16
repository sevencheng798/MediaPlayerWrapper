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

#include <Utils/Logging/Logger.h>
#include <Utils/Attachment/AttachmentManager.h>
#include <NLP/DomainSequencer.h>
#include <NLP/MessageInterpreter.h>
#include <ASR/MessageConsumer.h>

#include <ASR/AutomaticSpeechRecognizerRegister.h>
#include "Application/AIClient.h"

/// String to identify log entries originating from this file.
static const std::string TAG{"AIClient"};
#define LX(event) aisdk::utils::logging::LogEntry(TAG, event)

namespace aisdk {
namespace application {

std::unique_ptr<AIClient> AIClient::createNew(
	std::shared_ptr<utils::DeviceInfo> deviceInfo,
	std::shared_ptr<utils::mediaPlayer::MediaPlayerInterface> chatMediaPlayer,
	std::shared_ptr<utils::mediaPlayer::MediaPlayerInterface> streamMediaPlayer,
  	std::shared_ptr<utils::mediaPlayer::MediaPlayerInterface> alarmMediaPlayer,
	std::unordered_set<std::shared_ptr<utils::dialogRelay::DialogUXStateObserverInterface>>
    	dialogStateObservers,
    std::unordered_set<std::shared_ptr<dmInterface::AutomaticSpeechRecognizerUIDObserverInterface>>
		asrRefreshUid) {
    std::unique_ptr<AIClient> aiClient(new AIClient());
	if(!aiClient->initialize(
		deviceInfo,
		chatMediaPlayer,
		streamMediaPlayer,
		alarmMediaPlayer,
		dialogStateObservers,
		asrRefreshUid
	)) {
		return nullptr;
	}

	AISDK_DEBUG0(LX("CreateSuccess"));
	
	return aiClient;
}

bool AIClient::initialize(
	std::shared_ptr<utils::DeviceInfo> deviceInfo,
	std::shared_ptr<utils::mediaPlayer::MediaPlayerInterface> chatMediaPlayer,
	std::shared_ptr<utils::mediaPlayer::MediaPlayerInterface> streamMediaPlayer,
	std::shared_ptr<utils::mediaPlayer::MediaPlayerInterface> alarmMediaPlayer,
	std::unordered_set<std::shared_ptr<utils::dialogRelay::DialogUXStateObserverInterface>>
    	dialogStateObservers,
    std::unordered_set<std::shared_ptr<dmInterface::AutomaticSpeechRecognizerUIDObserverInterface>>
		asrRefreshUid) {
	if(!deviceInfo){
		AISDK_ERROR(LX("initializeFailed").d("reason", "nulldeviceInfo"));
		return false;
	}

	if(!chatMediaPlayer){
		AISDK_ERROR(LX("initializeFailed").d("reason", "nullChatMediaPlayer"));
		return false;
	}

	if(!streamMediaPlayer) {
		AISDK_ERROR(LX("initializeFailed").d("reason", "nullStreamMediaPlayer"));
		return false;
	}

	if(!alarmMediaPlayer) {
		AISDK_ERROR(LX("initializeFailed").d("reason", "nullAlarmMediaPlayer"));
		return false;
	}

	m_dialogUXStateRelay = std::make_shared<utils::dialogRelay::DialogUXStateRelay>();
    for (auto observer : dialogStateObservers) {
        m_dialogUXStateRelay->addObserver(observer);
    }

	m_asrRefreshConfig = std::make_shared<asr::ASRRefreshConfiguration>();
	for(auto observer : asrRefreshUid) {
		m_asrRefreshConfig->addObserver(observer);
	}

	/*
	* Creating the Domain directive Sequencer - This is the component that deals with the sequencing and ordering of
	* directives sent from SAI SDK and forwarding them along to the appropriate Domain Proxy that deals with
	* directives in that domain name.
	*/

	/*To-Do implement domainSequencer method*/
	m_domainSequencer = nlp::DomainSequencer::create();
	if(!m_domainSequencer) {
		AISDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateDomainSequencer"));
		return false;
	}
	/// Creating the @c AttachmentManager.
	auto attachmentDocker = std::make_shared<utils::attachment::AttachmentManager>();
	
	/// Creating the domain messageinterpreter.
	auto messageInterpreter = std::make_shared<nlp::MessageInterpreter>(m_domainSequencer, attachmentDocker);

	/// Creating a message consumer
	auto messageConsumer = std::make_shared<asr::MessageConsumer>(messageInterpreter);

	/*
     * Creating the Audio Track Manager
     */
    m_audioTrackManager = std::make_shared<atm::AudioTrackManager>();

#if 0		
#ifdef ENABLE_SOUNDAI_ASR

	/// Creating soundai client engine.
	
	const std::string soundAiConfigPath("/cfg/sai_config");
	m_soundAiEngine = soundai::engine::SoundAiEngine::create(deviceInfo, messageConsumer, soundAiConfigPath);
	if(!m_soundAiEngine) {
		AISDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateSoundAiEngine"));
		return false;
	}

	/*
	 * Creating the keyword observer - This is the commponent that deals with listener the soundai wakeup state.
	 */
	auto keywordObserver = std::make_shared<aisdk::kwd::KeywordDetector>(m_audioTrackManager);

	// Add ai observer to ai engine.
	m_soundAiEngine->addObserver(keywordObserver);
	m_soundAiEngine->addObserver(m_dialogUXStateRelay);
#endif

#endif
	
	if(!attachmentDocker) {
		AISDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateAttachmentDocker"));
		return false;
	}
#ifdef ENABLE_SOUNDAI_ASR
	const std::string soundAiConfigPath("/cfg/sai_config");
	const asr::AutomaticSpeechRecognizerConfiguration config{soundAiConfigPath, 0.45};
	m_asrEngine = asr::AutomaticSpeechRecognizerRegister::create(
		deviceInfo, 
		m_audioTrackManager,
		attachmentDocker,
		messageConsumer,
		m_asrRefreshConfig,
		config);
	if(!m_asrEngine) {
		AISDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateASREngine"));
		return false;
	}
#elif ENABLE_IFLYTEK_AIUI_ASR	
	m_asrEngine = asr::AutomaticSpeechRecognizerRegister::create(
		deviceInfo, 
		m_audioTrackManager,
		attachmentDocker,
		messageConsumer,
		m_asrRefreshConfig);
	if(!m_asrEngine) {
		AISDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateASREngine"));
		return false;
	}
#endif

	m_asrEngine->addASRObserver(m_dialogUXStateRelay);

	/*
	 * Creating the speech synthesizer. This is the commponent that deals with real-time interactive domain.
	 */
	m_speechSynthesizer = domain::speechSynthesizer::SpeechSynthesizer::create(
		chatMediaPlayer,
		m_audioTrackManager,
		m_dialogUXStateRelay);
	if (!m_speechSynthesizer) {
        AISDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateSpeechSynthesizer"));
        return false;
    }

	m_speechSynthesizer->addObserver(m_dialogUXStateRelay);

	/*
	 * Creating the VolumeManager. This is the commponent that deals with real-time interactive domain.
	 */
	m_volumeManager = domain::volumeManager::VolumeManager::create();
	if(!m_volumeManager) {
        AISDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateVolumeManager"));
        return false;
	}
	
	// TODO: Continue to add other domain commponent.
	/// ...
	/// ...
	/// ... 
	
	/*
	 * The following statements show how to register domain relay commponent to the domain directive sequencer.
	 */
	if (!m_domainSequencer->addDomainHandler(m_asrEngine)) {
		AISDK_ERROR(LX("initializeFailed")
						.d("reason", "unableToRegisterDomainHandler")
						.d("domainHandler", "SpeechSynthesizer"));
		return false;
	}
		
	if (!m_domainSequencer->addDomainHandler(m_speechSynthesizer)) {
		AISDK_ERROR(LX("initializeFailed")
						.d("reason", "unableToRegisterDomainHandler")
						.d("domainHandler", "SpeechSynthesizer"));
		return false;
	}

	// TODO: Continue to add other domain commponent.
	/// ...
	/// ...
	/// ...

	/**
	 * This method is the playback control interface. Users can control PLAY and 
	 * PAUSE when a Button is pressed on a physical button. Which modules need to
	 * be controlled should be added into @c m_playbackRouter.
	 */
	m_playbackRouter.insert(m_speechSynthesizer);
	// TODO: Continue to add other playback commponent.
	/// ...
	
	return true;
}

bool AIClient::notifyOfWakeWord(
	std::shared_ptr<utils::sharedbuffer::SharedBuffer> stream,
	utils::sharedbuffer::SharedBuffer::Index beginIndex,
	utils::sharedbuffer::SharedBuffer::Index endIndex) {

	m_asrEngine->recognize(stream, beginIndex, endIndex);
	return true;
}

void AIClient::addDialogStateObserver(
   std::shared_ptr<utils::dialogRelay::DialogUXStateObserverInterface> observer) {
	m_dialogUXStateRelay->addObserver(observer);
}

void AIClient::removeDialogStateObserver(
   std::shared_ptr<utils::dialogRelay::DialogUXStateObserverInterface> observer) {
	m_dialogUXStateRelay->removeObserver(observer);
}

void AIClient::connect() {
#ifdef ENABLE_SOUNDAI_ASR	
	m_asrEngine->start();
#endif
}

void AIClient::disconnect() {
#ifdef ENABLE_SOUNDAI_ASR
	m_asrEngine->stop();
#endif	
}

void AIClient::buttonPressed() {
	for(auto router : m_playbackRouter)
		router->buttonPressedPlayback();
}

AIClient::~AIClient() {
	if(m_domainSequencer) {
		AISDK_DEBUG5(LX("DomainSequencerShutdown"));
        m_domainSequencer->shutdown();
	}

	if(m_speechSynthesizer) {
		AISDK_DEBUG5(LX("SpeechSynthesizerShutdown"));
		m_speechSynthesizer->shutdown();
	}
#if 0	
	if(m_asrEngine) {
		AISDK_DEBUG5(LX("asrEngineShutdown"));
		m_asrEngine->shutdown();
	}
#endif	
}

}  // namespace application
}  // namespace aisdk

