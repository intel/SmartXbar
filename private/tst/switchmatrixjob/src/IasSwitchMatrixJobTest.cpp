/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/*
 * IasSwitchMatrixjobTest.cpp
 *
 *  Created 2016
 */


#include "IasSwitchMatrixJobTest.hpp"
#include "avbaudiomodules/audio/common/IasAudioCommonTypes.hpp"
#include "switchmatrix/IasSwitchMatrixJob.hpp"
#include "switchmatrix/IasBufferTask.hpp"
#include "avbaudiomodules/internal/audio/common/audiobuffer/IasAudioRingBufferFactory.hpp"
#include "smartx/IasAudioTypedefs.hpp"
#include "model/IasAudioSourceDevice.hpp"
#include "model/IasAudioSinkDevice.hpp"
#include "avbaudiomodules/internal/audio/common/audiobuffer/IasAudioRingBuffer.hpp"
namespace IasAudio
{

IasAudioPortParams srcPortParams = IasAudioPortParams("srcPort",
                                                      2,
                                                      0,
                                                      eIasPortDirectionOutput,
                                                      0);

IasAudioPortParams sinkPortParams = IasAudioPortParams("sinkPort",
                                                       2,
                                                       0,
                                                       eIasPortDirectionInput,
                                                       0);

IasAudioDeviceParams sourceDeviceParams = IasAudioDeviceParams("sourceDevice1",
                                                               2,
                                                               48000,
                                                               eIasFormatFloat32,
                                                               eIasClockProvided,
                                                               64,
                                                               4);

IasAudioDeviceParams sinkDeviceParams = IasAudioDeviceParams("sinkDevice1",
                                                             2,
                                                             48000,
                                                             eIasFormatFloat32,
                                                             eIasClockProvided,
                                                             256,
                                                             4);



IasAudioRingBufferFactory* factory = IasAudioRingBufferFactory::getInstance();

IasAudioDeviceParamsPtr srcDevice1ParamsPtr;
IasAudioDeviceParamsPtr srcDevice2ParamsPtr;
IasAudioSourceDevicePtr srcDevice1Ptr;
IasAudioSourceDevicePtr srcDevice2Ptr;
IasAudioDeviceParamsPtr sinkDeviceParamsPtr;
IasAudioSinkDevicePtr   sinkDevicePtr;

void IasSwitchMatrixJobTest::SetUp()
{
  srcDevice1ParamsPtr = std::make_shared<IasAudioDeviceParams>(sourceDeviceParams);
  srcDevice1Ptr       = std::make_shared<IasAudioSourceDevice>(srcDevice1ParamsPtr);

  sourceDeviceParams.name = "sourceDevice2";
  sourceDeviceParams.samplerate = 44100;
  srcDevice2ParamsPtr = std::make_shared<IasAudioDeviceParams>(sourceDeviceParams);
  srcDevice2Ptr       = std::make_shared<IasAudioSourceDevice>(srcDevice2ParamsPtr);

  sourceDeviceParams.name = "sourceDevice1";
  sourceDeviceParams.samplerate = 48000;

  sinkDeviceParamsPtr = std::make_shared<IasAudioDeviceParams>(sinkDeviceParams);
  sinkDevicePtr = std::make_shared<IasAudioSinkDevice>(sinkDeviceParamsPtr);
}

void IasSwitchMatrixJobTest::TearDown()
{
  srcDevice1ParamsPtr = nullptr;
  srcDevice2ParamsPtr = nullptr;
  srcDevice1Ptr = nullptr;
  srcDevice2Ptr = nullptr;
  sinkDevicePtr = nullptr;
}

TEST_F(IasSwitchMatrixJobTest, error_coverage)
{
  uint32_t copySize = 64;

  IasAudioPortParamsPtr srcPortParamsPtr = std::make_shared<IasAudioPortParams>(srcPortParams);
  srcPortParamsPtr->numChannels = 1;
  IasAudioPortParamsPtr sinkPortParamsPtr = std::make_shared<IasAudioPortParams>(sinkPortParams);
  IasAudioPortPtr srcPort = std::make_shared<IasAudioPort>(srcPortParamsPtr);
  IasAudioPortPtr sinkPort = std::make_shared<IasAudioPort>(sinkPortParamsPtr);
  IasSwitchMatrixJobPtr job = std::make_shared<IasSwitchMatrixJob>(srcPort,sinkPort);

  ASSERT_TRUE( job->init(copySize, sinkDeviceParams.samplerate) != 0 );
  job = nullptr;
  srcPort = nullptr;
  srcPortParamsPtr->numChannels = 2;
  srcPort = std::make_shared<IasAudioPort>(srcPortParamsPtr);
  job = std::make_shared<IasSwitchMatrixJob>(srcPort,sinkPort);

  ASSERT_TRUE( job->init(0,sinkDeviceParams.samplerate) != 0 );

  ASSERT_TRUE( job->init(copySize,0) != 0 );
  ASSERT_TRUE( job->init(copySize,sinkDeviceParams.samplerate) != 0 );



  srcPort->setOwner(srcDevice1Ptr);
  sinkPort->setOwner(sinkDevicePtr);

  IasAudioRingBuffer *srcBuffer,*sinkBuffer;
  factory->createRingBuffer(&srcBuffer,64,4,2,eIasFormatFloat32,eIasRingBufferLocalReal,"srcBuffer");
  factory->createRingBuffer(&sinkBuffer,64,4,2,eIasFormatFloat32,eIasRingBufferLocalReal,"sinkBuffer");
  srcPort->setRingBuffer(srcBuffer);
  ASSERT_TRUE( job->init(64,sinkDeviceParams.samplerate) != 0 );
  sinkPort->setRingBuffer(sinkBuffer);
  ASSERT_TRUE( job->init(64,sinkDeviceParams.samplerate) == 0 );

  IasSwitchMatrixJob::IasResult smjres = job->startProbe("dummyRec",
                                                           false,
                                                           5,
                                                           eIasFormatInt32,
                                                           48000,
                                                           2,
                                                           0);
  ASSERT_TRUE(smjres == IasSwitchMatrixJob::eIasOk);

  ASSERT_TRUE( job->updateSrcAreas(nullptr) != 0);




  factory->destroyRingBuffer(srcBuffer);
  factory->destroyRingBuffer(sinkBuffer);
  job = nullptr;
  srcPort = nullptr;
  sinkPort = nullptr;
  srcPortParamsPtr = nullptr;
  sinkPortParamsPtr = nullptr;

}

TEST_F(IasSwitchMatrixJobTest, sample_rate_convert_float_float)
{

  uint32_t framesStillToConsume = 0;
  uint32_t framesConsumed = 0;
  uint32_t copySize = 64;
  IasAudioPortParamsPtr srcPortParamsPtr = std::make_shared<IasAudioPortParams>(srcPortParams);
  IasAudioPortParamsPtr sinkPortParamsPtr = std::make_shared<IasAudioPortParams>(sinkPortParams);
  IasAudioPortPtr srcPort = std::make_shared<IasAudioPort>(srcPortParamsPtr);
  IasAudioPortPtr sinkPort = std::make_shared<IasAudioPort>(sinkPortParamsPtr);
  IasSwitchMatrixJobPtr job = std::make_shared<IasSwitchMatrixJob>(srcPort,sinkPort);

  srcPort->setOwner(srcDevice2Ptr);
  sinkPort->setOwner(sinkDevicePtr);
  IasAudioRingBuffer *srcBuffer,*sinkBuffer;
  factory->createRingBuffer(&srcBuffer,64,4,2,eIasFormatFloat32,eIasRingBufferLocalReal,"srcBuffer");
  factory->createRingBuffer(&sinkBuffer,64,4,2,eIasFormatFloat32,eIasRingBufferLocalReal,"sinkBuffer");
  srcPort->setRingBuffer(srcBuffer);
  sinkPort->setRingBuffer(sinkBuffer);

  ASSERT_TRUE( job->init(copySize,sinkDeviceParams.samplerate) == 0 );
  job->unlock();
  ASSERT_TRUE( job->execute(0, copySize, &framesStillToConsume, &framesConsumed) == 0);
  factory->destroyRingBuffer(srcBuffer);
  factory->destroyRingBuffer(sinkBuffer);
  job = nullptr;
  srcPort = nullptr;
  sinkPort = nullptr;

}

TEST_F(IasSwitchMatrixJobTest, sample_rate_convert_int32_float32)
{

  uint32_t copySize = 64;
  IasAudioPortParamsPtr srcPortParamsPtr = std::make_shared<IasAudioPortParams>(srcPortParams);
  IasAudioPortParamsPtr sinkPortParamsPtr = std::make_shared<IasAudioPortParams>(sinkPortParams);
  IasAudioPortPtr srcPort = std::make_shared<IasAudioPort>(srcPortParamsPtr);
  IasAudioPortPtr sinkPort = std::make_shared<IasAudioPort>(sinkPortParamsPtr);
  IasSwitchMatrixJobPtr job = std::make_shared<IasSwitchMatrixJob>(srcPort,sinkPort);

  srcDevice2Ptr = nullptr;
  srcDevice2ParamsPtr->dataFormat = eIasFormatInt32;
  srcDevice2Ptr = std::make_shared<IasAudioSourceDevice>(srcDevice2ParamsPtr);

  srcPort->setOwner(srcDevice2Ptr);
  sinkPort->setOwner(sinkDevicePtr);
  IasAudioRingBuffer *srcBuffer,*sinkBuffer;
  factory->createRingBuffer(&srcBuffer,64,4,2,eIasFormatInt32,eIasRingBufferLocalReal,"srcBuffer");
  factory->createRingBuffer(&sinkBuffer,64,4,2,eIasFormatFloat32,eIasRingBufferLocalReal,"sinkBuffer");
  srcPort->setRingBuffer(srcBuffer);
  sinkPort->setRingBuffer(sinkBuffer);

  ASSERT_TRUE( job->init(copySize,sinkDeviceParams.samplerate) == IasSwitchMatrixJob::eIasOk );

  factory->destroyRingBuffer(srcBuffer);
  factory->destroyRingBuffer(sinkBuffer);
  job = nullptr;
  srcPort = nullptr;
  sinkPort = nullptr;

}

TEST_F(IasSwitchMatrixJobTest, sample_rate_convert_int32_int32)
{

  uint32_t framesStillToConsume = 0;
  uint32_t framesConsumed = 0;
  uint32_t copySize = 64;
  IasAudioPortParamsPtr srcPortParamsPtr = std::make_shared<IasAudioPortParams>(srcPortParams);
  IasAudioPortParamsPtr sinkPortParamsPtr = std::make_shared<IasAudioPortParams>(sinkPortParams);
  IasAudioPortPtr srcPort = std::make_shared<IasAudioPort>(srcPortParamsPtr);
  IasAudioPortPtr sinkPort = std::make_shared<IasAudioPort>(sinkPortParamsPtr);
  IasSwitchMatrixJobPtr job = std::make_shared<IasSwitchMatrixJob>(srcPort,sinkPort);

  srcDevice2Ptr = nullptr;
  srcDevice2ParamsPtr->dataFormat = eIasFormatInt32;
  srcDevice2Ptr = std::make_shared<IasAudioSourceDevice>(srcDevice2ParamsPtr);

  sinkDevicePtr = nullptr;
  sinkDeviceParamsPtr->dataFormat = eIasFormatInt32;
  sinkDevicePtr = std::make_shared<IasAudioSinkDevice>(sinkDeviceParamsPtr);

  srcPort->setOwner(srcDevice2Ptr);
  sinkPort->setOwner(sinkDevicePtr);
  IasAudioRingBuffer *srcBuffer,*sinkBuffer;
  factory->createRingBuffer(&srcBuffer,64,4,2,eIasFormatInt32,eIasRingBufferLocalReal,"srcBuffer");
  factory->createRingBuffer(&sinkBuffer,64,4,2,eIasFormatInt32,eIasRingBufferLocalReal,"sinkBuffer");
  srcPort->setRingBuffer(srcBuffer);
  sinkPort->setRingBuffer(sinkBuffer);

  ASSERT_TRUE( job->init(copySize,sinkDeviceParams.samplerate) == 0 );
  job->unlock();
  ASSERT_TRUE( job->execute(0, copySize, &framesStillToConsume, &framesConsumed) == 0);
  factory->destroyRingBuffer(srcBuffer);
  factory->destroyRingBuffer(sinkBuffer);
  job = nullptr;
  srcPort = nullptr;
  sinkPort = nullptr;

}


TEST_F(IasSwitchMatrixJobTest, execute_float_float)
{
  uint32_t framesStillToConsume = 0;
  uint32_t framesConsumed = 0;
  uint32_t copySize = 64;
  IasAudioPortParamsPtr srcPortParamsPtr = std::make_shared<IasAudioPortParams>(srcPortParams);
  IasAudioPortParamsPtr sinkPortParamsPtr = std::make_shared<IasAudioPortParams>(sinkPortParams);
  IasAudioPortPtr srcPort = std::make_shared<IasAudioPort>(srcPortParamsPtr);
  IasAudioPortPtr sinkPort = std::make_shared<IasAudioPort>(sinkPortParamsPtr);
  IasSwitchMatrixJobPtr job = std::make_shared<IasSwitchMatrixJob>(srcPort,sinkPort);


  srcPort->setOwner(srcDevice1Ptr);
  sinkPort->setOwner(sinkDevicePtr);

  IasAudioRingBuffer *srcBuffer,*sinkBuffer;
  factory->createRingBuffer(&srcBuffer,64,4,2,eIasFormatFloat32,eIasRingBufferLocalReal,"srcBuffer");
  factory->createRingBuffer(&sinkBuffer,64,4,2,eIasFormatFloat32,eIasRingBufferLocalReal,"sinkBuffer");
  srcPort->setRingBuffer(srcBuffer);
  sinkPort->setRingBuffer(sinkBuffer);

  ASSERT_TRUE( job->init(copySize,sinkDeviceParams.samplerate) == 0 );
  job->unlock();
  ASSERT_TRUE( job->execute(0, copySize, &framesStillToConsume, &framesConsumed) == 0);
  factory->destroyRingBuffer(srcBuffer);
  factory->destroyRingBuffer(sinkBuffer);
  job = nullptr;
  srcPort = nullptr;
  sinkPort = nullptr;
}

TEST_F(IasSwitchMatrixJobTest, execute_in16_float)
{
  uint32_t framesStillToConsume = 0;
  uint32_t framesConsumed = 0;
  uint32_t copySize = 64;
  IasAudioPortParamsPtr srcPortParamsPtr = std::make_shared<IasAudioPortParams>(srcPortParams);
  IasAudioPortParamsPtr sinkPortParamsPtr = std::make_shared<IasAudioPortParams>(sinkPortParams);
  IasAudioPortPtr srcPort = std::make_shared<IasAudioPort>(srcPortParamsPtr);
  IasAudioPortPtr sinkPort = std::make_shared<IasAudioPort>(sinkPortParamsPtr);
  IasSwitchMatrixJobPtr job = std::make_shared<IasSwitchMatrixJob>(srcPort,sinkPort);

  srcDevice1Ptr = nullptr;
  srcDevice1ParamsPtr->dataFormat = eIasFormatInt16;
  srcDevice1Ptr = std::make_shared<IasAudioSourceDevice>(srcDevice1ParamsPtr);


  srcPort->setOwner(srcDevice1Ptr);
  sinkPort->setOwner(sinkDevicePtr);

  IasAudioRingBuffer *srcBuffer,*sinkBuffer;
  factory->createRingBuffer(&srcBuffer,64,4,2,eIasFormatInt16,eIasRingBufferLocalReal,"srcBuffer");
  factory->createRingBuffer(&sinkBuffer,64,4,2,eIasFormatFloat32,eIasRingBufferLocalReal,"sinkBuffer");
  srcPort->setRingBuffer(srcBuffer);
  sinkPort->setRingBuffer(sinkBuffer);

  ASSERT_TRUE( job->init(copySize,sinkDeviceParams.samplerate) == 0 );
  job->unlock();
  ASSERT_TRUE( job->execute(0, copySize, &framesStillToConsume, &framesConsumed) == 0);
  factory->destroyRingBuffer(srcBuffer);
  factory->destroyRingBuffer(sinkBuffer);
  job = nullptr;
  srcPort = nullptr;
  sinkPort = nullptr;

}

TEST_F(IasSwitchMatrixJobTest, execute_float_int16)
{
  uint32_t framesStillToConsume = 0;
  uint32_t framesConsumed = 0;
  uint32_t copySize = 64;
  IasAudioPortParamsPtr srcPortParamsPtr = std::make_shared<IasAudioPortParams>(srcPortParams);
  IasAudioPortParamsPtr sinkPortParamsPtr = std::make_shared<IasAudioPortParams>(sinkPortParams);
  IasAudioPortPtr srcPort = std::make_shared<IasAudioPort>(srcPortParamsPtr);
  IasAudioPortPtr sinkPort = std::make_shared<IasAudioPort>(sinkPortParamsPtr);
  IasSwitchMatrixJobPtr job = std::make_shared<IasSwitchMatrixJob>(srcPort,sinkPort);

  sinkDevicePtr = nullptr;
  sinkDeviceParamsPtr->dataFormat = eIasFormatInt16;
  sinkDevicePtr = std::make_shared<IasAudioSinkDevice>(sinkDeviceParamsPtr);

  srcPort->setOwner(srcDevice1Ptr);
  sinkPort->setOwner(sinkDevicePtr);

  IasAudioRingBuffer *srcBuffer,*sinkBuffer;
  factory->createRingBuffer(&srcBuffer,64,4,2,eIasFormatFloat32,eIasRingBufferLocalReal,"srcBuffer");
  factory->createRingBuffer(&sinkBuffer,64,4,2,eIasFormatInt16,eIasRingBufferLocalReal,"sinkBuffer");
  srcPort->setRingBuffer(srcBuffer);
  sinkPort->setRingBuffer(sinkBuffer);

  ASSERT_TRUE( job->init(copySize,sinkDeviceParams.samplerate) == 0 );
  job->unlock();
  ASSERT_TRUE( job->execute(0, copySize, &framesStillToConsume, &framesConsumed) == 0);
  factory->destroyRingBuffer(srcBuffer);
  factory->destroyRingBuffer(sinkBuffer);
  job = nullptr;
  srcPort = nullptr;
  sinkPort = nullptr;

}

TEST_F(IasSwitchMatrixJobTest, execute_float_int32)
{
  uint32_t framesStillToConsume = 0;
  uint32_t framesConsumed = 0;
  uint32_t copySize = 64;
  IasAudioPortParamsPtr srcPortParamsPtr = std::make_shared<IasAudioPortParams>(srcPortParams);
  IasAudioPortParamsPtr sinkPortParamsPtr = std::make_shared<IasAudioPortParams>(sinkPortParams);
  IasAudioPortPtr srcPort = std::make_shared<IasAudioPort>(srcPortParamsPtr);
  IasAudioPortPtr sinkPort = std::make_shared<IasAudioPort>(sinkPortParamsPtr);
  IasSwitchMatrixJobPtr job = std::make_shared<IasSwitchMatrixJob>(srcPort,sinkPort);

  sinkDevicePtr = nullptr;
  sinkDeviceParamsPtr->dataFormat = eIasFormatInt32;
  sinkDevicePtr = std::make_shared<IasAudioSinkDevice>(sinkDeviceParamsPtr);

  srcPort->setOwner(srcDevice1Ptr);
  sinkPort->setOwner(sinkDevicePtr);

  IasAudioRingBuffer *srcBuffer,*sinkBuffer;
  factory->createRingBuffer(&srcBuffer,64,4,2,eIasFormatFloat32,eIasRingBufferLocalReal,"srcBuffer");
  factory->createRingBuffer(&sinkBuffer,64,4,2,eIasFormatInt32,eIasRingBufferLocalReal,"sinkBuffer");
  srcPort->setRingBuffer(srcBuffer);
  sinkPort->setRingBuffer(sinkBuffer);

  ASSERT_TRUE( job->init(copySize,sinkDeviceParams.samplerate) == 0 );
  job->unlock();
  ASSERT_TRUE( job->execute(0, copySize, &framesStillToConsume, &framesConsumed) == 0);
  factory->destroyRingBuffer(srcBuffer);
  factory->destroyRingBuffer(sinkBuffer);
  job = nullptr;
  srcPort = nullptr;
  sinkPort = nullptr;

}

TEST_F(IasSwitchMatrixJobTest, execute_int32_int16)
{
  uint32_t framesStillToConsume = 0;
  uint32_t framesConsumed = 0;
  uint32_t copySize = 64;
  IasAudioPortParamsPtr srcPortParamsPtr = std::make_shared<IasAudioPortParams>(srcPortParams);
  IasAudioPortParamsPtr sinkPortParamsPtr = std::make_shared<IasAudioPortParams>(sinkPortParams);
  IasAudioPortPtr srcPort = std::make_shared<IasAudioPort>(srcPortParamsPtr);
  IasAudioPortPtr sinkPort = std::make_shared<IasAudioPort>(sinkPortParamsPtr);
  IasSwitchMatrixJobPtr job = std::make_shared<IasSwitchMatrixJob>(srcPort,sinkPort);



  srcDevice1Ptr = nullptr;
  srcDevice1ParamsPtr->dataFormat = eIasFormatInt32;
  srcDevice1Ptr = std::make_shared<IasAudioSourceDevice>(srcDevice1ParamsPtr);

  sinkDevicePtr = nullptr;
  sinkDeviceParamsPtr->dataFormat = eIasFormatInt16;
  sinkDevicePtr = std::make_shared<IasAudioSinkDevice>(sinkDeviceParamsPtr);

  srcPort->setOwner(srcDevice1Ptr);
  sinkPort->setOwner(sinkDevicePtr);

  IasAudioRingBuffer *srcBuffer,*sinkBuffer;
  factory->createRingBuffer(&srcBuffer,64,4,2,eIasFormatInt32,eIasRingBufferLocalReal,"srcBuffer");
  factory->createRingBuffer(&sinkBuffer,64,4,2,eIasFormatInt16,eIasRingBufferLocalReal,"sinkBuffer");
  srcPort->setRingBuffer(srcBuffer);
  sinkPort->setRingBuffer(sinkBuffer);

  ASSERT_TRUE( job->init(copySize,sinkDeviceParams.samplerate) == 0 );
  job->unlock();
  ASSERT_TRUE( job->execute(0, copySize, &framesStillToConsume, &framesConsumed) == 0);
  factory->destroyRingBuffer(srcBuffer);
  factory->destroyRingBuffer(sinkBuffer);
  job = nullptr;
  srcPort = nullptr;
  sinkPort = nullptr;

}

TEST_F(IasSwitchMatrixJobTest, execute_int32_int32)
{
  uint32_t framesStillToConsume = 0;
  uint32_t framesConsumed = 0;
  uint32_t copySize = 64;
  IasAudioPortParamsPtr srcPortParamsPtr = std::make_shared<IasAudioPortParams>(srcPortParams);
  IasAudioPortParamsPtr sinkPortParamsPtr = std::make_shared<IasAudioPortParams>(sinkPortParams);
  IasAudioPortPtr srcPort = std::make_shared<IasAudioPort>(srcPortParamsPtr);
  IasAudioPortPtr sinkPort = std::make_shared<IasAudioPort>(sinkPortParamsPtr);
  IasSwitchMatrixJobPtr job = std::make_shared<IasSwitchMatrixJob>(srcPort,sinkPort);

  srcDevice1Ptr = nullptr;
  srcDevice1ParamsPtr->dataFormat = eIasFormatInt32;
  srcDevice1Ptr = std::make_shared<IasAudioSourceDevice>(srcDevice1ParamsPtr);

  sinkDevicePtr = nullptr;
  sinkDeviceParamsPtr->dataFormat = eIasFormatInt32;
  sinkDevicePtr = std::make_shared<IasAudioSinkDevice>(sinkDeviceParamsPtr);


  srcPort->setOwner(srcDevice1Ptr);
  sinkPort->setOwner(sinkDevicePtr);

  IasAudioRingBuffer *srcBuffer,*sinkBuffer;
  factory->createRingBuffer(&srcBuffer,64,4,2,eIasFormatInt32,eIasRingBufferLocalReal,"srcBuffer");
  factory->createRingBuffer(&sinkBuffer,64,4,2,eIasFormatInt32,eIasRingBufferLocalReal,"sinkBuffer");
  srcPort->setRingBuffer(srcBuffer);
  sinkPort->setRingBuffer(sinkBuffer);

  ASSERT_TRUE( job->init(copySize,sinkDeviceParams.samplerate) == 0 );
  job->unlock();
  IasSwitchMatrixJob::IasResult smjres = job->startProbe("dummyRec",
                                                         false,
                                                         5,
                                                         eIasFormatInt32,
                                                         48000,
                                                         2,
                                                         0);
  ASSERT_TRUE(smjres == IasSwitchMatrixJob::eIasOk);
  ASSERT_TRUE( job->execute(0, copySize, &framesStillToConsume, &framesConsumed) == 0);

  job->stopProbe();
  job->stopProbe();

  factory->destroyRingBuffer(srcBuffer);
  factory->destroyRingBuffer(sinkBuffer);
  job = nullptr;
  srcPort = nullptr;
  sinkPort = nullptr;

}

TEST_F(IasSwitchMatrixJobTest, execute_int32_float32)
{
  uint32_t framesStillToConsume = 0;
  uint32_t framesConsumed = 0;
  uint32_t copySize = 64;
  IasAudioPortParamsPtr srcPortParamsPtr = std::make_shared<IasAudioPortParams>(srcPortParams);
  IasAudioPortParamsPtr sinkPortParamsPtr = std::make_shared<IasAudioPortParams>(sinkPortParams);
  IasAudioPortPtr srcPort = std::make_shared<IasAudioPort>(srcPortParamsPtr);
  IasAudioPortPtr sinkPort = std::make_shared<IasAudioPort>(sinkPortParamsPtr);
  IasSwitchMatrixJobPtr job = std::make_shared<IasSwitchMatrixJob>(srcPort,sinkPort);

  srcDevice1Ptr = nullptr;
  srcDevice1ParamsPtr->dataFormat = eIasFormatInt32;
  srcDevice1Ptr = std::make_shared<IasAudioSourceDevice>(srcDevice1ParamsPtr);

  srcPort->setOwner(srcDevice1Ptr);
  sinkPort->setOwner(sinkDevicePtr);

  IasAudioRingBuffer *srcBuffer,*sinkBuffer;
  factory->createRingBuffer(&srcBuffer,64,4,2,eIasFormatInt32,eIasRingBufferLocalReal,"srcBuffer");
  factory->createRingBuffer(&sinkBuffer,64,4,2,eIasFormatFloat32,eIasRingBufferLocalReal,"sinkBuffer");
  srcPort->setRingBuffer(srcBuffer);
  sinkPort->setRingBuffer(sinkBuffer);

  ASSERT_TRUE( job->init(copySize,sinkDeviceParams.samplerate) == 0 );
  job->unlock();
  ASSERT_TRUE( job->execute(0, copySize, &framesStillToConsume, &framesConsumed) == 0);

  // Test the jetSinkPort() and getSrcPort() functions.
  ASSERT_TRUE( job->getSinkPort()   == sinkPort);
  ASSERT_TRUE( job->getSourcePort() == srcPort);

  factory->destroyRingBuffer(srcBuffer);
  factory->destroyRingBuffer(sinkBuffer);
  job = nullptr;
  srcPort = nullptr;
  sinkPort = nullptr;

}

TEST_F(IasSwitchMatrixJobTest, probing_copy)
{
  uint32_t framesStillToConsume = 0;
  uint32_t framesConsumed = 0;
  uint32_t copySize = 64;
  IasAudioPortParamsPtr srcPortParamsPtr = std::make_shared<IasAudioPortParams>(srcPortParams);
  IasAudioPortParamsPtr sinkPortParamsPtr = std::make_shared<IasAudioPortParams>(sinkPortParams);
  IasAudioPortPtr srcPort = std::make_shared<IasAudioPort>(srcPortParamsPtr);
  IasAudioPortPtr sinkPort = std::make_shared<IasAudioPort>(sinkPortParamsPtr);
  IasSwitchMatrixJobPtr job = std::make_shared<IasSwitchMatrixJob>(srcPort,sinkPort);

  srcDevice1Ptr = nullptr;
  srcDevice1ParamsPtr->dataFormat = eIasFormatInt16;
  srcDevice1ParamsPtr->samplerate = 48000;
  srcDevice1Ptr = std::make_shared<IasAudioSourceDevice>(srcDevice1ParamsPtr);

  srcPort->setOwner(srcDevice1Ptr);
  sinkPort->setOwner(sinkDevicePtr);

  IasAudioRingBuffer *srcBuffer,*sinkBuffer;
  factory->createRingBuffer(&srcBuffer,64,4,2,eIasFormatInt16,eIasRingBufferLocalReal,"srcBuffer");
  factory->createRingBuffer(&sinkBuffer,64,4,2,eIasFormatInt16,eIasRingBufferLocalReal,"sinkBuffer");
  srcPort->setRingBuffer(srcBuffer);
  sinkPort->setRingBuffer(sinkBuffer);

  ASSERT_TRUE( job->init(copySize,sinkDeviceParams.samplerate) == 0 );
  job->unlock();

  ASSERT_TRUE( job->startProbe("probingTest",false,1,eIasFormatInt16,48000,2,0) == IasSwitchMatrixJob::eIasOk);

  uint32_t srcOffset = 0;
  uint32_t sinkOffset = 0;
  uint32_t srcFrames = 0;
  uint32_t sinkFrames = 0;
  IasAudioArea* mySrcArea;
  IasAudioArea* mySinkArea;
  ASSERT_TRUE( sinkBuffer->getAreas(&mySinkArea) == IasAudioRingBufferResult::eIasRingBuffOk);
  ASSERT_TRUE(mySinkArea != nullptr);
  ASSERT_TRUE( srcBuffer->getAreas(&mySrcArea) == IasAudioRingBufferResult::eIasRingBuffOk);
  ASSERT_TRUE(mySrcArea != nullptr);
  uint32_t cnt = 0;

  while(cnt <1001)
  {
    srcBuffer->beginAccess(eIasRingBufferAccessWrite,&mySrcArea,&srcOffset,&srcFrames);
    srcBuffer->endAccess(eIasRingBufferAccessWrite,srcOffset,64);
    ASSERT_TRUE( job->execute(0, copySize, &framesStillToConsume, &framesConsumed) == 0);
    sinkBuffer->beginAccess(eIasRingBufferAccessRead,&mySinkArea,&sinkOffset,&sinkFrames);
    sinkBuffer->endAccess(eIasRingBufferAccessRead,sinkOffset,64);
    cnt++;
  }

  // Test the jetSinkPort() and getSrcPort() functions.
  ASSERT_TRUE( job->getSinkPort()   == sinkPort);
  ASSERT_TRUE( job->getSourcePort() == srcPort);

  factory->destroyRingBuffer(srcBuffer);
  factory->destroyRingBuffer(sinkBuffer);
  job = nullptr;
  srcPort = nullptr;
  sinkPort = nullptr;

}

TEST_F(IasSwitchMatrixJobTest, probing_sample_rate_convert)
{
  uint32_t framesStillToConsume = 0;
  uint32_t framesConsumed = 0;
  uint32_t copySize = 48;
  IasAudioPortParamsPtr srcPortParamsPtr = std::make_shared<IasAudioPortParams>(srcPortParams);
  IasAudioPortParamsPtr sinkPortParamsPtr = std::make_shared<IasAudioPortParams>(sinkPortParams);
  IasAudioPortPtr srcPort = std::make_shared<IasAudioPort>(srcPortParamsPtr);
  IasAudioPortPtr sinkPort = std::make_shared<IasAudioPort>(sinkPortParamsPtr);
  IasSwitchMatrixJobPtr job = std::make_shared<IasSwitchMatrixJob>(srcPort,sinkPort);

  srcDevice1Ptr = nullptr;
  srcDevice1ParamsPtr->dataFormat = eIasFormatInt16;
  srcDevice1ParamsPtr->samplerate = 8000;
  srcDevice1Ptr = std::make_shared<IasAudioSourceDevice>(srcDevice1ParamsPtr);

  srcPort->setOwner(srcDevice1Ptr);
  sinkPort->setOwner(sinkDevicePtr);

  IasAudioRingBuffer *srcBuffer,*sinkBuffer;
  factory->createRingBuffer(&srcBuffer,8,4,2,eIasFormatInt16,eIasRingBufferLocalReal,"srcBuffer");
  factory->createRingBuffer(&sinkBuffer,48,4,2,eIasFormatInt16,eIasRingBufferLocalReal,"sinkBuffer");
  srcPort->setRingBuffer(srcBuffer);
  sinkPort->setRingBuffer(sinkBuffer);

  ASSERT_TRUE( job->init(copySize,sinkDeviceParams.samplerate) == 0 );
  ASSERT_TRUE( job->execute(0, copySize, &framesStillToConsume, &framesConsumed) == 0);

  job->unlock();
  ASSERT_TRUE( job->execute(0, 0, &framesStillToConsume, &framesConsumed) == 0);

  ASSERT_TRUE( job->startProbe("probingTestSRC",false,1,eIasFormatInt16,48000,2,0) == IasSwitchMatrixJob::eIasOk);
  ASSERT_TRUE( job->startProbe("probingTes",true,1,eIasFormatInt16,48000,2,0) == IasSwitchMatrixJob::eIasOk);
  uint32_t srcOffset = 0;
  uint32_t sinkOffset = 0;
  uint32_t srcFrames = 0;
  uint32_t sinkFrames = 0;
  IasAudioArea* mySrcArea;
  IasAudioArea* mySinkArea;
  ASSERT_TRUE( sinkBuffer->getAreas(&mySinkArea) == IasAudioRingBufferResult::eIasRingBuffOk);
  ASSERT_TRUE(mySinkArea != nullptr);
  ASSERT_TRUE( srcBuffer->getAreas(&mySrcArea) == IasAudioRingBufferResult::eIasRingBuffOk);
  ASSERT_TRUE(mySrcArea != nullptr);
  uint32_t cnt = 0;
  uint32_t srcFakeFrames = 8;

  while(cnt <1020)
  {

    srcBuffer->beginAccess(eIasRingBufferAccessWrite,&mySrcArea,&srcOffset,&srcFrames);
    srcBuffer->endAccess(eIasRingBufferAccessWrite,srcOffset,srcFakeFrames);
    ASSERT_TRUE( job->execute(0, copySize, &framesStillToConsume, &framesConsumed) == 0);
    sinkBuffer->beginAccess(eIasRingBufferAccessRead,&mySinkArea,&sinkOffset,&sinkFrames);
    sinkBuffer->endAccess(eIasRingBufferAccessRead,sinkOffset,48);
    if(cnt== 0)
    {
      ASSERT_TRUE( job->startProbe("probingTestSRC",false,1,eIasFormatInt16,48000,2,0) != IasSwitchMatrixJob::eIasOk);
    }
    cnt++;

  }

  // Test the jetSinkPort() and getSrcPort() functions.
  ASSERT_TRUE( job->getSinkPort()   == sinkPort);
  ASSERT_TRUE( job->getSourcePort() == srcPort);

  factory->destroyRingBuffer(srcBuffer);
  factory->destroyRingBuffer(sinkBuffer);
  job = nullptr;
  srcPort = nullptr;
  sinkPort = nullptr;

}


}
