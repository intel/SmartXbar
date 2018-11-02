/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasRoutingZone.cpp
 * @date   2015
 * @brief
 */

#include <string>

#include "avbaudiomodules/internal/audio/common/IasAudioLogging.hpp"
#include "avbaudiomodules/internal/audio/common/audiobuffer/IasAudioRingBufferFactory.hpp"
#include "model/IasAudioPort.hpp"
#include "model/IasAudioSinkDevice.hpp"
#include "model/IasRoutingZoneWorkerThread.hpp"
#include "model/IasRoutingZone.hpp"
#include "switchmatrix/IasSwitchMatrix.hpp"

namespace IasAudio {

static const std::string cClassName = "IasRoutingZone::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"
#define LOG_ZONE "zone=" + mParams->name + ":"

IasRoutingZone::IasRoutingZone(IasRoutingZoneParamsPtr params)
  :mLog(IasAudioLogging::registerDltContext("RZN", "Routing Zone"))
  ,mParams(params)
  ,mIsDerivedZone(false)
  ,mWorkerThread(nullptr)
  ,mSwitchMatrix(nullptr)
  ,mRoutingZonePtr(nullptr)
  ,mBaseZone(nullptr)
{
  IAS_ASSERT(params != nullptr)
}


IasRoutingZone::~IasRoutingZone()
{
  DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX, LOG_ZONE);

  if (mWorkerThread != nullptr)
  {
    stop();
  }
  mSwitchMatrix = nullptr;


  // Call the destructor of the worker thread object.
  mWorkerThread.reset();
}


IasRoutingZone::IasResult IasRoutingZone::init()
{
  IAS_ASSERT(mParams != nullptr);

  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_ZONE, "Initialization of Routing Zone.");

  if(mWorkerThread == nullptr)
  {
    mWorkerThread = std::make_shared<IasRoutingZoneWorkerThread>(mParams);
    IAS_ASSERT(mWorkerThread != nullptr);
    IasRoutingZoneWorkerThread::IasResult result = mWorkerThread->init();
    IAS_ASSERT(result == IasRoutingZoneWorkerThread::eIasOk);
    (void)result;
  }

  return eIasOk;
}


const IasRoutingZoneParamsPtr IasRoutingZone::getRoutingZoneParams() const
{
  return mParams;
}


IasRoutingZone::IasResult IasRoutingZone::linkAudioSinkDevice(IasAudioSinkDevicePtr sinkDevice)
{
  if (sinkDevice == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE, "Parameter == nullptr");
    return eIasInvalidParam;
  }
  mSinkDevice = sinkDevice;

  IasAudioDeviceParamsPtr sinkDeviceParams = mSinkDevice->getDeviceParams();
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_ZONE, "Sink device parameters are:");
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_ZONE, "name        = ", sinkDeviceParams->name);
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_ZONE, "numChannels = ", sinkDeviceParams->numChannels);
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_ZONE, "samplerate  = ", sinkDeviceParams->samplerate);
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_ZONE, "periodSize  = ", sinkDeviceParams->periodSize);
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_ZONE, "clockType   = ", toString(sinkDeviceParams->clockType));

  IasRoutingZoneWorkerThread::IasResult result = mWorkerThread->linkAudioSinkDevice(sinkDevice);
  IAS_ASSERT(result == IasRoutingZoneWorkerThread::eIasOk);
  (void)result;
  return eIasOk;
}


void IasRoutingZone::unlinkAudioSinkDevice()
{
  IAS_ASSERT(isActive() == false); // already checked in IasSetupImpl::unlink()

  mWorkerThread->unlinkAudioSinkDevice();
  mSinkDevice.reset();
}


IasRoutingZone::IasResult IasRoutingZone::createConversionBuffer(const IasAudioPortPtr    audioPort,
                                                                 IasAudioCommonDataFormat dataFormat,
                                                                 IasAudioRingBuffer**     conversionBuffer)
{
  IAS_ASSERT(mWorkerThread != nullptr);

  if (audioPort == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "audioPort == nullptr");
    return eIasInvalidParam;
  }
  if (conversionBuffer == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "conversionBuffer == nullptr");
    return eIasInvalidParam;
  }
  if (mSinkDevice == nullptr)
  {
    /**
     * @log link(IasRoutingZonePtr, IasAudioSinkDevicePtr) method of IasISetup interface was not called before.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "No sink device linked to routing zone");
    return eIasNotInitialized;
  }
  *conversionBuffer = nullptr;

  IasAudioPortParamsPtr   audioPortParams      = audioPort->getParameters();
  IasAudioDeviceParamsPtr sinkDeviceParams     = mSinkDevice->getDeviceParams();
  std::string             conversionBufferName = "IasRoutingZone_mConversionBuffer_" + audioPortParams->name;

  // Verify that we do not already have a conversion buffer for this audio port.
  if (mWorkerThread->getConversionBuffer(audioPort) != nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE,
                "Error: routing zone already includes a conversion buffer for audio port", audioPortParams->name);
    return eIasFailed;
  }

  // If the caller has not defined a data format, adopt the data format from the sink device.
  if (dataFormat == eIasFormatUndef)
  {
    dataFormat = sinkDeviceParams->dataFormat;
  }

  // Now we create the conversion buffer.
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_ZONE,
              "Creating ring buffer, periodSize =", sinkDeviceParams->periodSize,
              "numPeriods =", sinkDeviceParams->numPeriods,
              "numChannels =", audioPortParams->numChannels,
              "dataFormat =",  toString(dataFormat));

  IasAudioRingBufferFactory* ringBufferFactory = IasAudioRingBufferFactory::getInstance();
  IasAudioCommonResult result = ringBufferFactory->createRingBuffer(conversionBuffer,
                                                                    sinkDeviceParams->periodSize,
                                                                    sinkDeviceParams->numPeriods,
                                                                    audioPortParams->numChannels,
                                                                    dataFormat,
                                                                    eIasRingBufferLocalReal,
                                                                    conversionBufferName);
  if (result != eIasResultOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE,
                "Error while creating conversion buffer:", conversionBufferName,
                ":", toString(result));
    return eIasFailed;
  }

  // Tell the worker thread that it shall add the new conversion buffer.
  mWorkerThread->addConversionBuffer(audioPort, *conversionBuffer);

  return eIasOk;
}


IasRoutingZone::IasResult IasRoutingZone::destroyConversionBuffer(const IasAudioPortPtr audioPort)
{
  IAS_ASSERT(mWorkerThread != nullptr);

  if (audioPort == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "audioPort == nullptr");
    return eIasInvalidParam;
  }

  IasAudioRingBuffer* conversionBuffer = mWorkerThread->getConversionBuffer(audioPort);
  if (conversionBuffer == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "conversionBuffer == nullptr");
    return eIasFailed;
  }

  // Tell the worker thread that it must not use the conversion buffer anymore.
  mWorkerThread->deleteConversionBuffer(audioPort);

  IasAudioRingBufferFactory* ringBufferFactory = IasAudioRingBufferFactory::getInstance();
  ringBufferFactory->destroyRingBuffer(conversionBuffer);

  return eIasOk;
}





IasRoutingZone::IasResult IasRoutingZone::linkAudioPorts(IasAudioPortPtr zoneInputPort, IasAudioPortPtr sinkDeviceInputPort)
{
  if (mWorkerThread == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE, "Error, zone has not been initialized (mWorkerThread == nullptr)");
    return eIasNotInitialized;
  }

  IasRoutingZoneWorkerThread::IasResult result = mWorkerThread->linkAudioPorts(zoneInputPort, sinkDeviceInputPort);
  if (result != IasRoutingZoneWorkerThread::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE, "Error during IasRoutingZoneWorkerThread::linkAudioPorts()");
    return eIasFailed;
  }

  return eIasOk;
}

IasRoutingZone::IasResult IasRoutingZone::getLinkedSinkPort(IasAudioPortPtr zoneInputPort, IasAudioPortPtr *sinkDeviceInputPort)
{
  if (mWorkerThread == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE, "Error, zone has not been initialized (mWorkerThread == nullptr)");
    return eIasNotInitialized;
  }

  mWorkerThread->getLinkedSinkPort(zoneInputPort, sinkDeviceInputPort);
  return eIasOk;
}

void IasRoutingZone::unlinkAudioPorts(IasAudioPortPtr zoneInputPort, IasAudioPortPtr sinkDeviceInputPort)
{
  if (mWorkerThread != nullptr)
  {
    mWorkerThread->unlinkAudioPorts(zoneInputPort, sinkDeviceInputPort);
  }
}


IasRoutingZone::IasResult IasRoutingZone::addPipeline(IasPipelinePtr pipeline)
{
  if (mWorkerThread == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE, "Error, zone has not been initialized (mWorkerThread == nullptr)");
    return eIasNotInitialized;
  }

  IasRoutingZoneWorkerThread::IasResult result = mWorkerThread->addPipeline(pipeline);
  if (result != IasRoutingZoneWorkerThread::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE, "Error during IasRoutingZoneWorkerThread::addPipeline()");
    return eIasFailed;
  }

  return eIasOk;
}


void IasRoutingZone::deletePipeline()
{
  if (mWorkerThread != nullptr)
  {
    mWorkerThread->deletePipeline();
  }
}


IasRoutingZone::IasResult IasRoutingZone::addBasePipeline(IasPipelinePtr pipeline)
{
  if (mWorkerThread == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE, "Error, zone has not been initialized (mWorkerThread == nullptr)");
    return eIasNotInitialized;
  }

  IasRoutingZoneWorkerThread::IasResult result = mWorkerThread->addBasePipeline(pipeline);
  if (result != IasRoutingZoneWorkerThread::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE, "Error during IasRoutingZoneWorkerThread::addBasePipeline()");
    return eIasFailed;
  }

  return eIasOk;
}


void IasRoutingZone::deleteBasePipeline()
{
  if (mWorkerThread != nullptr)
  {
    mWorkerThread->deleteBasePipeline();
  }
}


IasRoutingZone::IasResult IasRoutingZone::addDerivedZone(IasRoutingZonePtr derivedZone)
{
  if (derivedZone == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE, "Error, parameter derivedZone == nullptr");
    return eIasInvalidParam;
  }

  if (mWorkerThread == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE, "Error, base zone has not been initialized (mWorkerThread == nullptr)");
    return eIasNotInitialized;
  }

  // Verify that the current zone is a base zone. It is not allowed to add derived zones to a derived zone.
  if (mIsDerivedZone)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE,
                "Error, we cannot add derived zones to a zone that is already a derived zone.");
    return eIasFailed;
  }

  // Inform the derived zone that it shall become a derived zone.
  derivedZone->setZoneDerived(true, mRoutingZonePtr);

  mDerivedZones.push_back(derivedZone);

  // Tell worker thread of the base zone that it has to execute also the worker thread of the derived zone.
  IasRoutingZoneWorkerThread::IasResult result = mWorkerThread->addDerivedZoneWorkerThread(derivedZone->getWorkerThread());
  if (result != IasRoutingZoneWorkerThread::eIasOk)
  {
    /**
     * @log A more detailed description of the error is provided by the method IasRoutingZoneWorkerThread::addDerivedZoneWorkerThread.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE,
                "Error while adding the derived zone's worker thread object to the base zone's worker thread object:",
                toString(result));
    return eIasFailed;
  }

  // Tell the derived zone that it will be served by the switch matrix worker thread of the base zone.
  derivedZone->setSwitchMatrix(mSwitchMatrix);

  return eIasOk;
}


void IasRoutingZone::deleteDerivedZone(IasRoutingZonePtr derivedZone)
{
  if (derivedZone != nullptr)
  {
    const std::string &derivedName = derivedZone->getRoutingZoneParams()->name;
    IasRoutingZoneWorkerThreadPtr derivedZoneWorkerThread = derivedZone->getWorkerThread();
    if (derivedZoneWorkerThread != nullptr)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Derived zone", derivedName, "has routing zone worker thread. Going to delete it now");
      mWorkerThread->deleteDerivedZoneWorkerThread(derivedZoneWorkerThread);
    }
    mDerivedZones.remove(derivedZone);
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Creating new switchmatrix worker thread for derived zone", derivedName, "again");
    IasSwitchMatrixPtr switchMatrix = std::make_shared<IasSwitchMatrix>();
    if (switchMatrix == nullptr)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Out of memory");
      return;
    }

    if (derivedZone->hasLinkedAudioSinkDevice())
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Derived zone", derivedName, "has linked audio sink device. Switchmatrix worker thread will be initialized now");
      // In case we have a linked audio sink device we now have to initialize the switchmatrix worker thread
      IasSwitchMatrix::IasResult comres = switchMatrix->init(derivedZone->getRoutingZoneParams()->name + "_worker", derivedZone->getPeriodSize(), derivedZone->getSampleRate());
      IAS_ASSERT(comres == IasSwitchMatrix::eIasOk);
      (void)comres;
    }
    derivedZone->setSwitchMatrix(switchMatrix);

    // Inform the derived zone that it is not a derived zone anymore.
    derivedZone->setZoneDerived(false, mRoutingZonePtr);
  }
}


void IasRoutingZone::setZoneDerived(bool isDerivedZone, IasRoutingZonePtr baseZone)
{
  if (isDerivedZone)
  {
    // Stop the worker thread, if it is already running
    this->stop();
    // Remember the base zone
    mBaseZone = baseZone;
  }

  // If the zone was a derived zone and if it shall become a base zone,
  // mark it as inactive, because it is not scheduled by the former base zone anymore.
  if (mIsDerivedZone && !isDerivedZone)
  {
    setActive(false);
    // Forget the base zone
    mBaseZone = nullptr;
  }

  mIsDerivedZone = isDerivedZone;
  if(mWorkerThread != nullptr)
  {
    mWorkerThread->setDerived(isDerivedZone);
  }
}


IasPipelinePtr IasRoutingZone::getPipeline() const
{
  if (mWorkerThread == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE, "Error, routing zone has not been initialized (mWorkerThread == nullptr)");
    return nullptr;
  }
  return mWorkerThread->getPipeline();
}

bool IasRoutingZone::hasPipeline(IasPipelinePtr pipeline) const
{
  if (mWorkerThread == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE, "Error, routing zone has not been initialized (mWorkerThread == nullptr)");
    return false;
  }
  return mWorkerThread->hasPipeline(pipeline);
}


IasRoutingZonePtr IasRoutingZone::getBaseZone() const
{
  return mBaseZone;
}

bool IasRoutingZone::isDerivedZone() const
{
  return mIsDerivedZone;
}

IasRoutingZone::IasResult IasRoutingZone::prepareStates()
{
  if (mWorkerThread == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE, "Error, routing zone has not been initialized (mWorkerThread == nullptr)");
    return eIasNotInitialized;
  }

  // Prepare all states that are needed for the worker thread.
  IasRoutingZoneWorkerThread::IasResult result = mWorkerThread->prepareStates();
  if (result != IasRoutingZoneWorkerThread::eIasOk)
  {
    /**
     * @log A more detailed description of the error is provided by the method IasRoutingZoneWorkerThread::prepareStates.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE, "Error during IasRoutingZoneWorkerThread::prepareStates:", toString(result));
    return eIasFailed;
  }
  return eIasOk;
}


IasRoutingZone::IasResult IasRoutingZone::start()
{
  if (mWorkerThread == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE, "Error, routing zone has not been initialized (mWorkerThread == nullptr)");
    return eIasNotInitialized;
  }

  if (mSinkDevice == nullptr)
  {
    /**
     * @log link(IasRoutingZonePtr, IasAudioSinkDevicePtr) method of IasISetup interface was not called before.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE, "No sink device linked to routing zone");
    return eIasNotInitialized;
  }

  if (mIsDerivedZone == false)
  {
    // Since this is a base routing zone, we now verify that the clockType of the sink device
    // is eIasClockReceived. Otherwise, the routing zone would spin without any trigger mechanism!
    const IasAudioDeviceParamsPtr sinkDeviceParams = mSinkDevice->getDeviceParams();
    IAS_ASSERT(sinkDeviceParams != nullptr);  // already checked in constructor of IasAudioDevice.
    if (sinkDeviceParams->clockType != eIasClockReceived)
    {
      /**
       * @log Linking a base routing zone to a sink, whose clock type is not eIasClockReceived, results in an undefined clock.
       */
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE,
                  "Base routing zones must not be linked with sink devices, whose clock type is",
                  toString(sinkDeviceParams->clockType));
      return eIasFailed;
    }
  }

  // Prepare all states of the routing zone for real-time processing
  IasResult result = prepareStates();
  if (result != eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE,
                "Error during IasRoutingZone::prepareStates method of routing zone:", toString(result));
    return eIasFailed;
  }

  if (mIsDerivedZone == false)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_ZONE, "Start the worker thread for this base zone");
    // The worker thread shall only be started for a base zone and not for a derived zone. The execution method
    // of a derived zone worker thread is called in the context of its base zone
    IasRoutingZoneWorkerThread::IasResult rzwtResult = mWorkerThread->start();
    if (rzwtResult != IasRoutingZoneWorkerThread::eIasOk)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE, "Error during IasRoutingZoneWorkerThread::start:", toString(rzwtResult));
      return eIasFailed;
    }
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_ZONE, "No need to start the worker thread for this derived zone, just set active");
  }

  // Mark this zone as active.
  setActive(true);
  return result;
}


void IasRoutingZone::setActive(bool isActive)
{
  if (isActive == true)
  {
    mWorkerThread->changeState(IasRoutingZoneWorkerThread::eIasPrepare);
    if (mIsDerivedZone == false)
    {
      // The base zone can be directly activated without the need to synchronize
      // to any other routing zone
      mWorkerThread->changeState(IasRoutingZoneWorkerThread::eIasActivate);
    }
  }
  else
  {
    mWorkerThread->changeState(IasRoutingZoneWorkerThread::eIasInactivate);
  }
}

bool IasRoutingZone::isActive() const
{
  return mWorkerThread->isActive();
}

bool IasRoutingZone::isActivePending() const
{
  return mWorkerThread->isActivePending();
}

IasRoutingZone::IasResult IasRoutingZone::stop()
{
  if (mWorkerThread == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE, "Error, routing zone has not been initialized (mWorkerThread == nullptr)");
    return eIasNotInitialized;
  }
  IasRoutingZoneWorkerThread::IasResult result = mWorkerThread->stop();
  if (result != IasRoutingZoneWorkerThread::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE, "Error during IasRoutingZoneWorkerThread::stop:", toString(result));
    return eIasFailed;
  }

  // Mark this zone as inactive.
  setActive(false);

  return eIasOk;
}

uint32_t IasRoutingZone::getPeriodSize() const
{
  if (mSinkDevice == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE, "No sink device linked to routing zone");
    return 0;
  }
  return mSinkDevice->getDeviceParams()->periodSize;
}

IasAudioCommonDataFormat IasRoutingZone::getSampleFormat() const
{
  if (mSinkDevice == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE, "No sink device linked to routing zone");
    return eIasFormatUndef;
  }
  return mSinkDevice->getDeviceParams()->dataFormat;
}

uint32_t IasRoutingZone::getSampleRate() const
{
  if (mSinkDevice == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE, "No sink device linked to routing zone");
    return 0;
  }
  return mSinkDevice->getDeviceParams()->samplerate;
}

void IasRoutingZone::setSwitchMatrix(IasSwitchMatrixPtr switchMatrix)
{
  if (mWorkerThread != nullptr)
  {
    mSwitchMatrix = switchMatrix;
    mWorkerThread->setSwitchMatrix(switchMatrix);
  }
}

void IasRoutingZone::clearDerivedZones()
{
  mDerivedZones.clear();
}

void IasRoutingZone::clearSwitchMatrix()
{
  mSwitchMatrix = nullptr;
  if (mWorkerThread != nullptr)
  {
    this->stop();
    mWorkerThread->clearSwitchMatrix();
  }
}

/*
 * Function to get a IasRoutingZone::IasResult as string.
 */
#define STRING_RETURN_CASE(name) case name: return std::string(#name); break
#define DEFAULT_STRING(name) default: return std::string(name)
std::string toString(const IasRoutingZone::IasResult& type)
{
  switch(type)
  {
    STRING_RETURN_CASE(IasRoutingZone::eIasOk);
    STRING_RETURN_CASE(IasRoutingZone::eIasInvalidParam);
    STRING_RETURN_CASE(IasRoutingZone::eIasInitFailed);
    STRING_RETURN_CASE(IasRoutingZone::eIasNotInitialized);
    STRING_RETURN_CASE(IasRoutingZone::eIasFailed);
    DEFAULT_STRING("Invalid IasRoutingZone::IasResult => " + std::to_string(type));
  }
}



} //namespace IasAudio
