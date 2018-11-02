/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file  IasDrift.hpp
 * @brief Simulation of time-dependent drifts.
 */

#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdlib.h>
#include <iostream>

#include "avbaudiomodules/audio/common/IasAudioCommonTypes.hpp"
#include "IasDrift.hpp"

using namespace std;

namespace IasAudio {

IasDrift::IasDrift()
  :mCurrentElement(0)
  ,mCurrentDrift(0)
{
}

IasDrift::~IasDrift()
{
}


IasDrift::IasResult IasDrift::init(const std::string &driftFileName)
{
  std::ifstream infile;

  IasDriftSpecElement tmp;
  cout << "Open drift simulation file: " << driftFileName.c_str() << endl;
  infile.open(driftFileName, std::ifstream::in);
  if (infile.is_open())
  {
    while (infile.good())
    {
      std::string line;
      if (!getline( infile, line ))
      {
        break;
      }
      istringstream ss( line );
      while (ss.good())
      {
        string parseLine;
        if (!getline( ss, parseLine, ',' ))
        {
          break;
        }
        tmp.timestamp = atoi(parseLine.c_str());
        if (!getline( ss, parseLine, ',' ))
        {
          break;
        }
        tmp.drift = atoi(parseLine.c_str());
        if (!getline( ss, parseLine, ',' ))
        {
          break;
        }
        tmp.gradient = atoi(parseLine.c_str());
        mDriftSpecVec.push_back(tmp);
      }
    }
    infile.close();
  }
  else
  {
    cout << "ERROR: Drift simulation file can not be opened!" <<endl;
    return eIasFileError;
  }

  // Dump the content of the Drift Vector
  cout << "----------------------------------------------------------------------" << endl;
  for (uint32_t cnt = 0; cnt < mDriftSpecVec.size(); cnt++)
  {
    cout << "timestamp ="      << setw(8) << mDriftSpecVec[cnt].timestamp
         << " ms, drift ="     << setw(6) << mDriftSpecVec[cnt].drift
         << " ppm, gradient =" << setw(6) << mDriftSpecVec[cnt].gradient
         << " ppm/sec " << endl;
  }
  cout << "----------------------------------------------------------------------" << endl;

  this->rewind();

  return eIasOk;
}


void IasDrift::updateCurrentDrift(uint32_t  time,
                                  int32_t  *drift)
{
  // Return default value, if the drift spec vector is empty.
  if (mDriftSpecVec.size() == 0)
  {
    *drift = 0;
    return;
  }

  // Return default value as long as the current time is smaller than the
  // timestamp of the first element in the vector.
  if (time < mDriftSpecVec[0].timestamp)
  {
    *drift = 0;
    return;
  }

  // Check if there is another element in the DriftSpecVec.
  if (furtherDriftAvail())
  {
    // Check if the current time has already reched the timestamp of the next element.
    uint32_t nextElement = mCurrentElement+1;
    if (time >= mDriftSpecVec[nextElement].timestamp)
    {
      // Increase the index, so that the next element becomes the active one.
      mCurrentElement = mCurrentElement+1;
    }
  }

  IAS_ASSERT(time >= mDriftSpecVec[mCurrentElement].timestamp);

  // Calculate the current drift value. This is based on a linear interpolation,
  // which starts from the current drift spec plus a term that depends on the
  // current gradient multiplied with the difference between the current time
  // and the timestamp of the current drift spec.
  mCurrentDrift = (mDriftSpecVec[mCurrentElement].drift +
                   (mDriftSpecVec[mCurrentElement].gradient *
                    static_cast<int32_t>(time - mDriftSpecVec[mCurrentElement].timestamp)) / 1000);
  *drift = mCurrentDrift;
}


void IasDrift::rewind()
{
  mCurrentElement = 0;
  mCurrentDrift   = 0;
}

/*
 * Function to get a IasDrift::IasResult as string.
 */
#define STRING_RETURN_CASE(name) case name: return std::string(#name); break
#define DEFAULT_STRING(name) default: return std::string(name)
std::string toString(const IasDrift::IasResult& type)
{
  switch(type)
  {
    STRING_RETURN_CASE(IasDrift::eIasOk);
    STRING_RETURN_CASE(IasDrift::eIasFileError);
    DEFAULT_STRING("Invalid IasAlsaHandlerWorkerThread::IasResult => " + std::to_string(type));
  }
}



} // namespace IasAudio
