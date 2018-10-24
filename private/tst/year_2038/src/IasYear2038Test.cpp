/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

/**
 * @file IasYear2038Test.cpp
 * @brief Unit test to cover all uncovered Audio model methods
 */

#include <iostream>
#include <pthread.h>

#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread_time.hpp>

#include "gtest/gtest.h"

#include "audio/common/IasAudioCommonTypes.hpp"


using namespace IasAudio;
using namespace std;

class IasYear2038Test : public ::testing::Test
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

TEST_F(IasYear2038Test, boost)
{
  using namespace boost::interprocess;
  interprocess_mutex myMutex;
  scoped_lock<interprocess_mutex> lock(myMutex);
  interprocess_condition myCond;
  for (uint32_t count=0; count<10; ++count)
  {
    bool result = myCond.timed_wait(lock, boost::get_system_time() + boost::posix_time::millisec(100));
    if (result == false)
    {
      cout << "Timeout(" << count << ")" << endl;
    }
  }
}

TEST_F(IasYear2038Test, pthread)
{
  int res;
  pthread_condattr_t cond_attr;
  res = pthread_condattr_init(&cond_attr);
  if (res != 0)
  {
    cerr << "Error initializing cond_attr" << endl;
  }
  ASSERT_EQ(0, res);
  res = pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED);
  if (res != 0)
  {
     pthread_condattr_destroy(&cond_attr);
     cerr << "Error during cond_attr setpshared" << endl;
  }
  ASSERT_EQ(0, res);
  pthread_cond_t   myCond;
  res = pthread_cond_init(&myCond, &cond_attr);
  if (res != 0)
  {
    cerr << "Error during pthread_cond_init" << endl;
  }
  ASSERT_EQ(0, res);
  pthread_condattr_destroy(&cond_attr);

  pthread_mutexattr_t mutex_attr;
  res = pthread_mutexattr_init(&mutex_attr);
  if (res != 0)
  {
    cerr << "Error initializing mutex_attr" << endl;
  }
  ASSERT_EQ(0, res);
  res = pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
  if (res != 0)
  {
     pthread_mutexattr_destroy(&mutex_attr);
     cerr << "Error during mutex_attr setpshared" << endl;
  }
  ASSERT_EQ(0, res);
  pthread_mutex_t myMutex;
  res = pthread_mutex_init(&myMutex, &mutex_attr);
  if (res != 0)
  {
    cerr << "Error during pthread_mutex_init" << endl;
  }
  ASSERT_EQ(0, res);

  for (uint32_t count=0; count<10; ++count)
  {
    timeval tv;
    res = gettimeofday(&tv, NULL);
    if (res != 0)
    {
      cerr << "Error during gettimeofday" << endl;
    }
    ASSERT_EQ(0, res);
    tv.tv_usec += 100000;
    timespec ts;
    ts.tv_sec = tv.tv_sec;
    ts.tv_nsec = tv.tv_usec * 1000;
    int res = pthread_cond_timedwait(&myCond, &myMutex, &ts);
    if (res != 0)
    {
      cerr << "Timeout(" << count << ")" << endl;
    }
  }
}
