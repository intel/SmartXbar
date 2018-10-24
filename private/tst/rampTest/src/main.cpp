/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/*
 * main.cpp
 *
 *  Created on: Aug 17, 2012
 */

#include "gtest/gtest.h"


#include "audio/smartx/rtprocessingfwx/IasProcessingTypes.hpp"
#include "helper/IasRamp.hpp"

#define ONE  (1.0f)
#define HALF (0.5f)
#define ZERO (0.0f)
#define MUTE (6.309573444801930e-8f)


using namespace IasAudio;

class IasAudioRampTest : public ::testing::Test
{
  protected:
    virtual void SetUp()
    {
    }

    virtual void TearDown()
    {
    }
};

int main(int argc, char* argv[])
{

  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}

TEST_F(IasAudioRampTest, create_delete)
{
  IasRamp* myRamp = new IasRamp(48000, 192);
  ASSERT_TRUE(myRamp != nullptr);
  delete myRamp;
}

TEST_F(IasAudioRampTest, setRamp)
{
  IasRamp* myRamp = new IasRamp(0, 192);
  ASSERT_TRUE(myRamp != nullptr);
  IasRampErrors err = myRamp->setTimedRamp(0.0f,1.0f,100,eIasRampShapeLinear);
  ASSERT_TRUE(err == eIasRampErrorInitParams);
  delete myRamp;

  myRamp = new IasRamp(48000, 0);
  ASSERT_TRUE(myRamp != nullptr);
  err = myRamp->setTimedRamp(0.0f,1.0f,100,eIasRampShapeLinear);
  ASSERT_TRUE(err == eIasRampErrorInitParams);
  delete myRamp;

  myRamp = new IasRamp(48000, 192);
  ASSERT_TRUE(myRamp != nullptr);
  err = myRamp->setTimedRamp(0.0f,1.0f,0,eIasRampShapeLinear);
  ASSERT_TRUE(err == eIasRampErrorZeroFadeTime);

  err = myRamp->setTimedRamp(0.0f,1.0f,1000,eIasMaxNumRampShapes);
  ASSERT_TRUE(err == eIasRampErrorUnknownRampShape);

  err = myRamp->setTimedRamp(0.0f,1.0f,100,eIasRampShapeLinear);
  ASSERT_TRUE(err == eIasRampErrorNoError);

  float *data = new float[192*4];
  uint32_t stillToRamp = 0;

  do{
  err = myRamp->getRampValues(data, 0, 2, &stillToRamp);
  ASSERT_TRUE(err == eIasRampErrorNoError);
  std::cout << "still to ramp: " << stillToRamp <<std::endl;
  }while(stillToRamp != 0);

  err = myRamp->setTimedRamp(0.0f,0.5f,100,eIasRampShapeExponential);
  ASSERT_TRUE(err == eIasRampErrorNoError);
  stillToRamp = 0;
  do{
    err = myRamp->getRampValues(data, 0, 2, &stillToRamp);
    ASSERT_TRUE(err == eIasRampErrorNoError);
    std::cout << "still to ramp: " << stillToRamp <<std::endl;
  }while(stillToRamp != 0);

  err = myRamp->setTimedRamp(0.5f,0.5f,100,eIasRampShapeLinear);
  ASSERT_TRUE(err == eIasRampErrorNoError);
  stillToRamp = 0;
  do{
    err = myRamp->getRampValues(data, 0, 2, &stillToRamp);
    ASSERT_TRUE(err == eIasRampErrorNoError);
    std::cout << "still to ramp: " << stillToRamp <<std::endl;
  }while(stillToRamp != 0);


  err = myRamp->setTimedRamp(0.5f,1.0f,100,eIasRampShapeExponential);
  ASSERT_TRUE(err == eIasRampErrorNoError);
  stillToRamp = 0;
  do{
    err = myRamp->getRampValues(data, 0, 2, &stillToRamp);
    ASSERT_TRUE(err == eIasRampErrorNoError);
    std::cout << "still to ramp: " << stillToRamp <<std::endl;
  }while(stillToRamp != 0);

  err = myRamp->setTimedRamp(1.0f,0.0f,100,eIasRampShapeExponential);
  ASSERT_TRUE(err == eIasRampErrorNoError);
  stillToRamp = 0;
  do{
    err = myRamp->getRampValues(data, 0, 2, &stillToRamp);
    ASSERT_TRUE(err == eIasRampErrorNoError);
    std::cout << "still to ramp: " << stillToRamp <<std::endl;
  }while(stillToRamp != 0);


  err = myRamp->getRampValues(data, 0, 2, &stillToRamp);
  ASSERT_TRUE(err == eIasRampErrorNoError);

  err = myRamp->setTimedRamp(1.0f,0.0f,100,eIasRampShapeExponential);
  ASSERT_TRUE(err == eIasRampErrorNoError);
  stillToRamp = 0;
  err = myRamp->getRampValues(nullptr, 0, 2, &stillToRamp);
  ASSERT_TRUE(err == eIasRampErrorDataPointer);

  err = myRamp->getRampValues(data, 0, 2, nullptr);
  ASSERT_TRUE(err == eIasRampErrorDataPointer);

  err = myRamp->getRampValues(data, 4, 2, &stillToRamp);
  ASSERT_TRUE(err == eIasRampErrorDataPointer);

  err = myRamp->getRampValues(data, 0, 5, &stillToRamp);
  ASSERT_TRUE(err == eIasRampErrorDataPointer);

  err = myRamp->setTimedRamp(1.0f,0.0f,100,eIasRampShapeExponential);
  ASSERT_TRUE(err == eIasRampErrorNoError);
  stillToRamp = 0;
  float currentValue;
  do{
      stillToRamp = myRamp->getNumSamples2Ramp();
      err = myRamp->getRampNewValue(data);
      ASSERT_TRUE(err == eIasRampErrorNoError);
      myRamp->getCurrentValue(&currentValue);
      std::cout << "still to ramp: " << stillToRamp <<std::endl;
    }while(stillToRamp != 0);


  delete myRamp;
}

TEST_F(IasAudioRampTest, getRampValues)
{
  const uint32_t cIasFrameLength = 192;
  const uint32_t cIasSampleRate = 48000;
  float startValue = 0.0f;
  float targetValue = 1.0f;
  float *data = nullptr;

  IasRamp* myRamp = new IasRamp(cIasSampleRate, cIasFrameLength);
  ASSERT_TRUE(myRamp != nullptr);

  //data is not allocated yet, eIasRampErrorDataPointer expected
  IasRampErrors err = myRamp->getRampValues(data);
  ASSERT_TRUE(err == eIasRampErrorDataPointer);

  //allocate memory for data
  data = new float[cIasFrameLength];

  //no ramp is set, eIasRampErrorRampNotSet error expected
  err = myRamp->getRampValues(data);
  ASSERT_TRUE(err == eIasRampErrorRampNotSet);

  //set timed ramp linear
  err = myRamp->setTimedRamp(startValue, targetValue, 100, eIasRampShapeLinear);
  ASSERT_TRUE(err == eIasRampErrorNoError);

  do{
    err = myRamp->getRampValues(data);
    ASSERT_TRUE(err == eIasRampErrorNoError);
  }while(data[cIasFrameLength-1] != targetValue);

  delete[] data;
  delete myRamp; 
}
