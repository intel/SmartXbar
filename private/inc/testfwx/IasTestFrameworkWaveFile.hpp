/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file   IasTestFrameworkWaveFile.hpp
 * @date   2017
 * @brief  Class for reading / writing WAVE files.
 */
#ifndef IAS_TEST_FRAMEWORK_WAVE_FILE_HPP_
#define IAS_TEST_FRAMEWORK_WAVE_FILE_HPP_

#include <sndfile.h>

#include "audio/common/IasAudioCommonTypes.hpp"
#include "testfwx/IasTestFrameworkTypes.hpp"
#include "model/IasAudioPin.hpp"


namespace IasAudio {

class IasTestFrameworkWaveFile
{
  public:

    enum IasResult
    {
      eIasOk,               //!< Ok, Operation successful
      eIasFailed,           //!< operation failed, log will give information about error
      eIasFinished,         //!< operation finished
      eIasFileNotOpen,      //!< file has not been opened
      eIasAlreadyOpen       //!< file is already open
    };

    /**
     *  @brief Constructor.
     *
     * @param[in] params Wave file parameters incl. file name
     */
    IasTestFrameworkWaveFile(IasTestFrameworkWaveFileParamsPtr params);

    /**
     *  @brief Destructor
     */
    ~IasTestFrameworkWaveFile();

    /**
     * @brief Open file for reading
     *
     * @param[in] bufferSize Buffer size in terms of numPeriods* periodSize [frames]
     *
     * @returns error code
     * @retval eIasFailed operation failed
     * @retval eIasOk     everything went well
     */
    IasResult openForReading(uint32_t bufferSize);

    /**
     * @brief Open file for writing
     *
     * @param[in] bufferSize Buffer size in terms of numPeriods* periodSize [frames]
     *
     * @returns error code
     * @retval eIasFailed operation failed
     * @retval eIasOk     everything went well
     */
    IasResult openForWriting(uint32_t bufferSize);

    /**
     * @brief Close file
     */
    void close();

    /**
     * @brief Read data from wave file
     *
     * If the wave file provides less frames than requested, the end of the buffer is filled with zeros.
     *
     * @param[in]  area Area where the data should be written
     * @param[in]  offset Offset for the area
     * @param[in]  numFrames Number of frames to be read
     * @param[out] numFramesRead Number of frames successfully read
     *
     * @returns error code
     * @retval eIasFailed    operation failed
     * @retval eIasOk        everything went well
     * @retval eIasEndOfFile read was finished because end of file was reached
     */
    IasResult readFrames(IasAudioArea* area, uint32_t offset, uint32_t numFrames, uint32_t& numFramesRead);

    /**
     * @brief Write data into wave file
     *
     * @param[in] area              Data to be written into file
     * @param[in] offset            Offset for the area
     * @param[in] numFrames         Number of frames to be written
     * @param[out] numFramesWritten Number of frames successfully written
     *
     * @returns error code
     * @retval eIasFailed operation failed
     * @retval eIasOk     everything went well
     */
    IasResult writeFrames(IasAudioArea* area, uint32_t offset, uint32_t numFrames, uint32_t& numFramesWritten);

    /**
     * @brief Link wave file with audio input or output pin of pipeline
     *
     * @param[in] audioPin Audio pin to be linked
     *
     * @returns error code
     * @retval eIasFailed operation failed
     * @retval eIasOk     everything went well
     */
    IasResult linkAudioPin(IasAudioPinPtr audioPin);

    /**
     * @brief Get wave file name
     *
     * @return wave file name
     */
    std::string getFileName() const {return mParams->fileName;}

    /**
     * @brief Get dummy source device
     *
     * @return dummy source device
     */
    IasAudioSourceDevicePtr getDummySourceDevice() const {return mDummySourceDevice;}

    /**
     * @brief Get dummy sink device
     *
     * @return dummy sink device
     */
    IasAudioSinkDevicePtr getDummySinkDevice() const {return mDummySinkDevice;}

    /**
     * @brief Check if file is linked with input port
     *
     * @return true if linked with input port, false otherwise
     */
    bool isInputFile() const;

  private:

    /*!
     *  @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasTestFrameworkWaveFile(IasTestFrameworkWaveFile const &other);

    /*!
     *  @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasTestFrameworkWaveFile& operator=(IasTestFrameworkWaveFile const &other);

    void reset();

    IasResult allocateMemory();

    DltContext*                        mLog;
    IasTestFrameworkWaveFileParamsPtr  mParams;             //!< wave file params
    IasAudioCommonDataFormat           mDataFormat;

    uint32_t                        mBufferSize;         //!< the size of one read or write block for a file
    float*                      mIntermediateBuffer; //!< local buffer for intermediate data buffering
    IasAudioArea*                      mArea;               //!< local IasAudioArea, uses mIntermediateBuffer

    bool                          mFileIsOpen;         //!< flag to indicate if file is open
    SNDFILE*                           mFilePtr;            //!< pointer to the file
    SF_INFO                            mFileInfo;           //!< file info structure

    IasAudioSourceDevicePtr            mDummySourceDevice;  //!< dummy source device
    IasAudioSinkDevicePtr              mDummySinkDevice;    //!< dummy sink device
    IasAudioPinPtr                     mAudioPin;           //!< linked audio pin
};


/**
 * @brief Function to get a IasTestFrameworkWaveFile::IasResult as string.
 *
 * @return String carrying the result message.
 */
std::string toString(const IasTestFrameworkWaveFile::IasResult& type);


} // namespace IasAudio

#endif /* IAS_TEST_FRAMEWORK_WAVE_FILE_HPP_ */
