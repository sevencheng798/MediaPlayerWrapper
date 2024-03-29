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
#include <cstring>
#include <Utils/Logging/Logger.h>
#include "Application/PortAudioMicrophoneWrapper.h"

static const std::string TAG{"PortAudioMicrophoneWrapper"};
#define LX(event) aisdk::utils::logging::LogEntry(TAG, event)

namespace aisdk {
namespace application {
#ifdef KWD_SOUNDAI
static const int NUM_INPUT_CHANNELS = 8;
static const std::string DEFAULE_MICROPHONE{"microphone"};
#else
static const int NUM_INPUT_CHANNELS = 1;
/// for rk3308
static const std::string DEFAULE_MICROPHONE{"6mic_loopback"};
#endif

static const int16_t SAMPLE_SILENCE = 0;
static const int NUM_OUTPUT_CHANNELS = 0;
static const double SAMPLE_RATE = 16000;
static const unsigned long PREFERRED_SAMPLES_PER_CALLBACK = paFramesPerBufferUnspecified;
//static const std::string DEFAULE_MICROPHONE{"microphone"};
//static const std::string DEFAULE_MICROPHONE{"6mic_loopback"};

std::unique_ptr<PortAudioMicrophoneWrapper> PortAudioMicrophoneWrapper::create(
	std::shared_ptr<utils::sharedbuffer::SharedBuffer> stream) {
	if(!stream) {
		AISDK_ERROR(LX("CreatedFailed").d("reason", "Invalid stream passed to PortAudioMicrophoneWrapper"));
		return nullptr;
	}

	std::unique_ptr<PortAudioMicrophoneWrapper> wrapper(new PortAudioMicrophoneWrapper(stream));
	if(!wrapper->initialize()) {
		AISDK_ERROR(LX("CreatedFailed").d("reason", "Failed to initialize PortAudioMicrophone."));
		return nullptr;
	}

	return wrapper;
}

PortAudioMicrophoneWrapper::PortAudioMicrophoneWrapper(
	std::shared_ptr<utils::sharedbuffer::SharedBuffer> stream):
    m_audioInputStream{stream},
    m_paStream{nullptr} {
}

PortAudioMicrophoneWrapper::~PortAudioMicrophoneWrapper() {
    Pa_StopStream(m_paStream);
    Pa_CloseStream(m_paStream);
    Pa_Terminate();
}

bool PortAudioMicrophoneWrapper::initialize() {
    m_writer = m_audioInputStream->createWriter(utils::sharedbuffer::Writer::Policy::NONBLOCKABLE);
    if (!m_writer) {
        AISDK_CRITICAL(LX("Failed to create stream writer"));
        return false;
    }
    PaError err;
    err = Pa_Initialize();
    if (err != paNoError) {
        AISDK_CRITICAL(LX("Failed to initialize PortAudio").d("errorCode", err));
        return false;
    }

 	PaTime suggestedLatency = 0.15;	// default value
	auto latencyInConfig = true;
	if (!latencyInConfig) {
		err = Pa_OpenDefaultStream(
			&m_paStream,
			NUM_INPUT_CHANNELS,
			NUM_OUTPUT_CHANNELS,
			paInt16,
			SAMPLE_RATE,
			PREFERRED_SAMPLES_PER_CALLBACK,
			PortAudioCallback,
			this);
	} else {
		AISDK_INFO(
			LX("PortAudio suggestedLatency has been configured to ").d("Seconds", std::to_string(suggestedLatency)));
		int numDevices; 
		int index;
		numDevices = Pa_GetDeviceCount(); 
		if( numDevices < 0 ) {
		  AISDK_ERROR(LX("PortAudioFindDevicesError ").d("reason", "notFoundMicrophoneDevices."));
		  err = numDevices; 
		  return false;
		}
		
		AISDK_INFO(LX("PortAudio find total microphone devices ").d("numDevices", numDevices));

		//const PaDeviceInfo *devInfo;
		for(index=0; index < numDevices; index++) {
			const PaDeviceInfo *devInfo = Pa_GetDeviceInfo(index);
			if(devInfo) {
				#if 0
				printf("name: %s\n", devInfo->name);
				printf("structVersion: 0x%02x\n", devInfo->structVersion);
				printf("maxInputChannels: %d\n", devInfo->maxInputChannels);
				printf("maxOutputChannels: %d\n", devInfo->maxOutputChannels);
				printf("defaultSampleRate: %ld\n", devInfo->defaultSampleRate);
				#endif
				if(devInfo->name == DEFAULE_MICROPHONE)
					break;
			}
		}

		if(index == numDevices) {
			AISDK_ERROR(LX("PortAudioFindDevicesError ")
					.d("reason", "notFoundMicrophoneDevices.")
					.d("expected", DEFAULE_MICROPHONE));
			return false;
		}
		AISDK_INFO(LX("Find a suitable MIC device ").d("deviceIndex", index));
		
		PaStreamParameters inputParameters;
        std::memset(&inputParameters, 0, sizeof(inputParameters));
        inputParameters.device = index; //Pa_GetDefaultInputDevice();
        inputParameters.channelCount = NUM_INPUT_CHANNELS;
        inputParameters.sampleFormat = paInt16;
        inputParameters.suggestedLatency = suggestedLatency;
        inputParameters.hostApiSpecificStreamInfo = nullptr;

        err = Pa_OpenStream(
            &m_paStream,
            &inputParameters,
            nullptr,
            SAMPLE_RATE,
            PREFERRED_SAMPLES_PER_CALLBACK,
            paNoFlag,
            PortAudioCallback,
            this);
	}
	
    if (err != paNoError) {
        AISDK_CRITICAL(LX("Failed to open PortAudio default stream").d("errorCode", err));
        return false;
    }
    return true;
}

bool PortAudioMicrophoneWrapper::startStreamingMicrophoneData() {
    std::lock_guard<std::mutex> lock{m_mutex};
    PaError err = Pa_StartStream(m_paStream);
    if (err != paNoError) {
        AISDK_CRITICAL(LX("Failed to start PortAudio stream"));
        return false;
    }
    return true;
}

bool PortAudioMicrophoneWrapper::stopStreamingMicrophoneData() {
    std::lock_guard<std::mutex> lock{m_mutex};
    PaError err = Pa_StopStream(m_paStream);
    if (err != paNoError) {
        AISDK_CRITICAL(LX("Failed to stop PortAudio stream"));
        return false;
    }
    return true;
}

void PortAudioMicrophoneWrapper::portAudioStreamPreprocessing(
	void *outFrames,
	const void *inputFrames,
	unsigned long framesPreBuffer) {
	int16_t *rdptr = (int16_t*)inputFrames;
	auto *rwptr = static_cast<int16_t*>(outFrames);
	int16_t instant;
	unsigned long index;
	int ch;
	for(index=0; index<framesPreBuffer; index++) {
		for(ch=0; ch<NUM_INPUT_CHANNELS; ch++) {
			if(ch == 0) {
				*rwptr++ = *rdptr++;	
			} else if(ch == 1) {
				//instant = *rdptr++;
				(void)*rdptr++;
				instant = *rdptr;
				*rwptr++ = instant;  // 2
			} else if(ch == 2) {
				instant = *rdptr++;		// 4
				//(void)*rdptr++;
				instant = *(rdptr+1);
				*rwptr++ = instant;
			} else if(ch == 3) {
				(void)*rdptr++;
				instant = *(rdptr+3);
				*rwptr++ = instant;
			}else {
				(void)*rdptr++;
				*rwptr++ = SAMPLE_SILENCE;
			}
		}
	}
}

int PortAudioMicrophoneWrapper::PortAudioCallback(
    const void* inputBuffer,
    void* outputBuffer,
    unsigned long numSamples,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void* userData) {
    PortAudioMicrophoneWrapper* wrapper = static_cast<PortAudioMicrophoneWrapper*>(userData);
	auto allFramesToCalc = numSamples*NUM_INPUT_CHANNELS;
	
#ifdef KWD_SOUNDAI
	std::vector<int16_t> pushToData(allFramesToCalc);
	wrapper->portAudioStreamPreprocessing(pushToData.data(), inputBuffer, numSamples);
	ssize_t returnCode = wrapper->m_writer->write(pushToData.data(), allFramesToCalc);
	if (returnCode <= 0) {
		AISDK_CRITICAL(LX("Failed to write to stream."));
		return paAbort;
	}
#else
	ssize_t returnCode = wrapper->m_writer->write(inputBuffer, allFramesToCalc);
    if (returnCode <= 0) {
        AISDK_CRITICAL(LX("Failed to write to stream."));
        return paAbort;
    }
#endif
	
    return paContinue;
}

} // namespace application
} // namespace aisdk
