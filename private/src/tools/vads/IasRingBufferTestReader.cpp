/*
 * * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/*
 * IasRingBufferTestReader.cpp
 *
 *  Created 2015
 */

#include <stdio.h>
#include <stdlib.h>
#include "IasRingBufferTestReader.hpp"
#include "internal/audio/common/audiobuffer/IasAudioRingBuffer.hpp"

namespace IasAudio
{

	IasRingBufferTestReader::IasRingBufferTestReader(IasAudioRingBuffer* buffer,
							 std::uint32_t periodSize,
							 std::uint32_t numChannels,
							 IasAudioCommonDataFormat dataType,
							 std::string fileName)
		:mRingBuffer(buffer)
		,mPeriodSize(periodSize)
		,mNumChannels(numChannels)
		,mSampleSize(0)
		,mDataType(dataType)
		,mReadBuf(NULL)
		,mDataFile(NULL)
		,mNumBufferChannels(0)
		,mFileName(fileName)
		,mInitialized(false)
		,mSamplesWrittenToFile(0)
	{


	}


	IasAudioCommonResult IasRingBufferTestReader::init()
	{
		mSampleSize = toSize(mDataType);

		if( (mSampleSize == -1) || (mRingBuffer == NULL) || (mPeriodSize == 0) || (mNumChannels == 0) )
		{
			return eIasResultInitFailed;
		}

		std::uint32_t tmp = posix_memalign(&mReadBuf,16,mPeriodSize*sizeof(std::int32_t)*mNumChannels);
		(void)tmp;

		mNumBufferChannels = mRingBuffer->getNumChannels();
		if(mNumBufferChannels < mNumChannels)
		{
			return eIasResultInitFailed;
		}

		SF_INFO  sinkInfo;
		sinkInfo.channels = mNumChannels;
		sinkInfo.samplerate = 48000;
		sinkInfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_32;
		mDataFile = sf_open(mFileName.c_str(),SFM_WRITE,&sinkInfo);
		if(mDataFile == NULL)
		{
			return eIasResultInitFailed;
		}

		mInitialized = true;
		return eIasResultOk;
	}


	IasRingBufferTestReader::~IasRingBufferTestReader()
	{

		if(mDataFile)
		{
			sf_close(mDataFile);
		}

		free(mReadBuf);

	}

	IasAudioCommonResult IasRingBufferTestReader::readFromBuffer(std::uint32_t chanIdx)
	{

		std::uint32_t nReadSamples = 0;
		IasAudioCommonResult result = eIasResultOk;
		static IasAudioCommonResult prevRes = eIasResultNotInitialized;

		if(mInitialized == false)
		{
			return eIasResultNotInitialized;
		}

		if( (chanIdx + mNumChannels) > mNumBufferChannels)
		{
			fprintf(stderr,"Invalid channel index\n");
			return eIasResultInvalidParam;
		}

		std::uint32_t samples = 0;

		mRingBuffer->updateAvailable(eIasRingBufferAccessRead,&samples);
		if(samples >= mPeriodSize)
		{
			if(mDataType == eIasFormatInt32)
			{
				copy<std::int32_t>(mPeriodSize, chanIdx);
			}
			else if(mDataType == eIasFormatInt16)
			{
				copy<std::int16_t>(mPeriodSize, chanIdx);
			}
			else if(mDataType == eIasFormatFloat32)
			{
				copy<float>(mPeriodSize, chanIdx);
			}


			if(mDataType == eIasFormatInt32)
			{
				result = write((sf_count_t*)&nReadSamples,static_cast<std::int32_t*>(mReadBuf));
			}
			else if(mDataType == eIasFormatInt16)
			{
				result = write((sf_count_t*)&nReadSamples,static_cast<std::int16_t*>(mReadBuf));
			}
			else
			{
				fprintf(stderr,"Data type %s not supported, quit now!\n", toString(mDataType).c_str());
				result = eIasResultInvalidParam;
			}
			mSamplesWrittenToFile+=mPeriodSize;

		}
		else
		{
			if (prevRes != result) {
				fprintf(stderr,"IasRingBufferTestReader::no data in source buffer\n");
				prevRes = result;
			}
			result = eIasResultBufferEmpty;
		}
		return result;
	}

	template<typename T>
	IasAudioCommonResult IasRingBufferTestReader::copy( std::uint32_t numSamples, std::uint32_t chanIdx)
	{
		std::uint32_t samples = numSamples;
		std::uint32_t offset = 0;

		mRingBuffer->beginAccess(eIasRingBufferAccessRead,&mAreas,&offset,&samples);
		std::int32_t* sink= static_cast<std::int32_t*>(mReadBuf);

		for(std::uint32_t i=0; i<samples ; i++)
		{
			for(std::uint32_t j=chanIdx; j< (mNumChannels+chanIdx);j++)
			{
				char* tmp =  (char*)mAreas[j].start +  offset*sizeof(T) + (mAreas[j].first +i*mAreas[j].step)/8;
				T* src = reinterpret_cast<T*>(tmp);
				if(sizeof(T) == sizeof(std::int32_t))
				{
					*sink = static_cast<std::int32_t>(*src) ;
				}
				else
				{
					*sink =  (static_cast<std::int32_t>(*src))<<16;
				}
				sink++;
			}
		}
		mRingBuffer->endAccess(eIasRingBufferAccessRead,offset,samples);
		return eIasResultOk;
	}



	IasAudioCommonResult IasRingBufferTestReader::write(sf_count_t* numWrittenSamples,std::int32_t* src)
	{
		*numWrittenSamples = sf_writef_int(mDataFile, src, mPeriodSize);
		return eIasResultOk;
	}

	IasAudioCommonResult IasRingBufferTestReader::write(sf_count_t* numWrittenSamples,std::int16_t* src)
	{
		*numWrittenSamples = sf_writef_short(mDataFile, src, mPeriodSize);
		return eIasResultOk;
	}

	IasAudioCommonResult IasRingBufferTestReader::write(sf_count_t* numWrittenSamples,float* src)
	{
		*numWrittenSamples = sf_writef_float(mDataFile, src, mPeriodSize);
		return eIasResultOk;
	}

}
