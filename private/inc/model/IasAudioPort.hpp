/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file
 */

#ifndef IASAUDIOPORT_HPP_
#define IASAUDIOPORT_HPP_


#include "avbaudiomodules/audio/common/IasAudioCommonTypes.hpp"
#include "smartx/IasAudioTypedefs.hpp"
#include "avbaudiomodules/internal/audio/common/IasAudioLogging.hpp"

/*!
 * @brief namespace IasAudio
 */
namespace IasAudio {

class IasAudioRingBuffer;
class IasAudioDevice;
class IasSwitchMatrix;

typedef struct{
    void* start;
    uint32_t numChannels;
    uint32_t index;
    uint32_t periodSize;
    uint32_t sampleRate;
    IasAudioCommonDataFormat dataFormat;
    IasAudioArea* areas;
  }IasAudioPortCopyInformation;


/*!
 * @brief Documentation for class IasAudioPort
 */
class IAS_AUDIO_PUBLIC IasAudioPort
{

  public:
    /**
     * @brief The result type for the IasAudioPort methods
     */
    enum IasResult
    {
      eIasOk,               //!< Operation successful
      eIasFailed            //!< Operation failed
    };

    /*!
     *  @brief Constructor.
     */
    IasAudioPort(IasAudioPortParamsPtr params);

    /*!
     *  @brief Destructor, virtual by default.
     */
    virtual ~IasAudioPort();

    /*!
     * @brief function to attach the port to its data buffer
     *
     * @param[in] buffer the ringbuffer pointer
     *
     * @returns error code
     * @retval eIasOk everything ok
     * @retval eIasFailed error occured, infos about error can be seen in DLT log
     */
    IasResult setRingBuffer(IasAudioRingBuffer* buffer);

    /*!
     * @brief function to remove the attachment of the port to its data buffer
     */
    void clearRingBuffer();

    /*!
     * @brief function to get the pointer to the data buffer
     *
     * @param[in,out] buffer the ringbuffer pointer
     *
     * @returns error code
     * @retval eIasOk everything ok
     * @retval eIasFailed error occured, infos about error can be seen in DLT log
     */
    IasResult getRingBuffer(IasAudioRingBuffer** buffer) const;

    /*!
     * @brief function to attach the port to its owner
     *
     * @param[in] owner the pointer to the owner
     *
     * @returns error code
     * @retval eIasOk everything ok
     * @retval eIasFailed error occured, infos about error can be seen in DLT log
     */
    IasResult setOwner(IasAudioPortOwnerPtr owner);

    /*!
     * @brief function to get the pointer to the owner
     *
     * @param[in,out] owner the pointer to the owner
     *
     * @returns error code
     * @retval eIasOk everything ok
     * @retval eIasFailed error occured, infos about error can be seen in DLT log
     */
    IasResult getOwner(IasAudioPortOwnerPtr* owner);

    /**
     * @brief Remove the owner from the port
     */
    void clearOwner();

    /*!
     * @brief function to get copy infos from audio port
     *
     * @param[in,out] copyInfos the pointer to the copy inforamtion
     *
     * @returns error code
     * @retval eIasOk everything ok
     * @retval eIasFailed error occured, infos about error can be seen in DLT log
     */
    IasResult getCopyInformation(IasAudioPortCopyInformation* copyInfos) const;

    /**
     * @brief Get access to the port parameters
     *
     * @return A read-only pointer to the port parameters
     */
    const IasAudioPortParamsPtr getParameters() const { return mParams; }

    /**
     * @brief Let the audio port keep in mind that it is connected with the specified switchMatrix.
     *
     * This method is relevant for source output ports, in order to avoid that one source
     * output port can be connected to several independent (non synchronous) routing
     * zones. To support this, this method returns the error code eIasFailed
     * if the audio port is already connected with a different switchMatrix.
     *
     * @param[in] switchMatrix   Handle of the switch matrix,
     *                           the audio port shall be connected to.
     *
     * @returns   Error code.
     * @retval    eIasFailed           If the audio port is an output port already connetced to a different switchMatrix.
     * @retval    eIasOk               If the audio port is not connected yet, or if this is an input audio port
     *                                 or if this is an output port and the existing connection uses the
     *                                 same switchMatrixWorker.
     */
    IasResult storeConnection(IasSwitchMatrixPtr switchMatrix);

    /**
     * @brief Let the audio port forget that it was connected with the specified switchMatrix.
     *
     * If the source output audio port has connections to several sink input audio ports
     * of the same routing zone, the internal counter for the number of connections is
     * decremented.
     *
     * @param[in] switchMatrix   Handle of the switch matrix,
     *                           whose connection shall be removed.
     *
     * @returns   Error code.
     * @retval    eIasFailed     If the audio port is not connected to a switchMatrix or it's an output port
     *                           and is not connected to the specified switchMatrix.
     * @retval    eIasOk         On success.
     */
    IasResult forgetConnection(IasSwitchMatrixPtr switchMatrix);

    /**
     * @brief Check if a port has an active connection
     *
     * One port can only be connected to one switch matrix at a time, but it can have multiple
     * connections to dependent routing zones, which all share the same switch matrix.
     * The number of the connections to one switch matrix is stored in the member mNumconnections
     * The function returns false if mNumConnections is zero, otherwise it returns true.
     *
     * @returns boolean value
     * @retval  true    port is connected to at least one other port
     * @retval  false   port is not connected
     */
    inline bool isConnected(){ return mNumConnections > 0 ? true : false;};

    /**
     * @brief Return the switch matrix
     *
     * @return The read only pointer to the switch matrix
     */
    IasSwitchMatrixPtr getSwitchMatrix() const { return mSwitchMatrix; }

    /**
     * @brief Helper to clear switchmatrix shared pointer
     */
    void clearSwitchMatrix(){mSwitchMatrix = nullptr;};

  private:
    /*!
     *  @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasAudioPort(IasAudioPort const &other);

    /*!
     *  @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasAudioPort& operator=(IasAudioPort const &other);

    DltContext                     *mLog;
    IasAudioPortParamsPtr           mParams;
    IasAudioRingBuffer*             mRingbuffer;
    IasAudioPortOwnerPtr            mOwner;
    uint32_t                     mNumConnections;
    IasSwitchMatrixPtr              mSwitchMatrix;
};

} // namespace IasAudio

#endif // IASAUDIOPORT_HPP_
