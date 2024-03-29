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
#ifndef __FFMPEG_ATTACHMENT_INPUTCONTROLLER_H_
#define __FFMPEG_ATTACHMENT_INPUTCONTROLLER_H_

#include <memory>

#include <Utils/Attachment/AttachmentReader.h>
#include <Utils/AudioFormat.h>

#include "FFmpegInputControllerInterface.h"

struct AVIOContext;
struct AVInputFormat;
struct AVDictionary;

namespace aisdk {
namespace mediaPlayer {
namespace ffmpeg {

/**
 * This class provides the FFmpegDecoder input access to the content of an attachment reader.
 *
 * This class only support one media input and it cannot provide multiple tracks / repeat.
 */
class FFmpegAttachmentInputController : public FFmpegInputControllerInterface {
public:
    /**
     * Creates an input reader object.
     *
     * @param reader A pointer to the attachment reader.
     * @param format The audio format to be used to interpret raw audio data. This can be @c nullptr.
     * @return A pointer to the @c FFmpegAttachmentInputController if succeed; @c nullptr otherwise.
     */
    static std::unique_ptr<FFmpegAttachmentInputController> create(
        std::shared_ptr<utils::attachment::AttachmentReader> reader,
        const utils::AudioFormat* format = nullptr);

    /// @name FFmpegInputControllerInterface methods
    /// @{
    bool hasNext() const override;
    bool next() override;
	AVFormatContext* createNewFormatContext() override;
    std::tuple<Result, std::shared_ptr<AVFormatContext>, std::chrono::milliseconds> getCurrentFormatContextOpen() override;
    /// @}

    /**
     * Destructor.
     */
    ~FFmpegAttachmentInputController();

private:
    /**
     * Constructor. The @c inputFormat and @c inputOptions are used for raw input.
     *
     * @param reader A pointer to the attachment reader.
     * @param inputFormat Optional parameter that can be used to force an input format.
     * @param inputOptions Optional parameter that can be used to force an set codec options.
     */
    FFmpegAttachmentInputController(
        std::shared_ptr<utils::attachment::AttachmentReader> reader,
        std::shared_ptr<AVInputFormat> inputFormat = nullptr,
        std::shared_ptr<AVDictionary> inputOptions = nullptr);

    /**
     * Function used to provide input data to the decoder.
     *
     * @param buffer Buffer to copy the data to.
     * @param bufferSize The buffer size in bytes.
     * @return The size read if the @c read call succeeded or the AVError code.
     */
    int read(uint8_t* buffer, int bufferSize);

    /**
     * Feed AvioBuffer with some data from the input controller.
     *
     * @param userData A pointer to the input controller instance used to read the encoded input.
     * @param buffer Buffer to copy the data to.
     * @param bufferSize The buffer size in bytes.
     * @return The size read if the @c read call succeeded or the AVError code.
     */
    static int feedBuffer(void* userData, uint8_t* buffer, int bufferSize);

	/// Wait tryagain count.
	/// pretry again duration is READ_TIMEOUT. so toal wait timeout is m_tryCount*tryagain.
	int m_tryCount;

	/**
	 * This flag indicates that at least one frame of vaild data has been detected.
	 * It will allow the task following the @c'avformat_open_input'function to
	 * continue executing, otherwise it will end the following task.
	 */
	bool m_hasProbedVaildData;

    /// Pointer to the data input.
    std::shared_ptr<utils::attachment::AttachmentReader> m_reader;

    /// Optional input format that can be used to force a format. If nullptr, use ffmpeg auto detect.
    std::shared_ptr<AVInputFormat> m_inputFormat;

    /// Optional input format options that can be used to force some format parameters.
    std::shared_ptr<AVDictionary> m_inputOptions;

    /// Keep a pointer to the avioContext to avoid memory leaks.
    std::shared_ptr<AVIOContext> m_ioContext;

	/// The current format context original point @c AVFormatContext.
	AVFormatContext* m_avFormatContext;
};

}  // namespace ffmpeg
}  // namespace mediaPlayer
}  // namespace aisdk

#endif  // __FFMPEG_ATTACHMENT_INPUTCONTROLLER_H_
