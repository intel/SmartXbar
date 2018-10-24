/*
 * * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/*
 * IasRingBufferTestWriter.cpp
 *
 *  Created 2015
 */

#include <stdio.h>
#include "IasRingBufferTestWriter.hpp"
#include "internal/audio/common/audiobuffer/IasAudioRingBuffer.hpp"

namespace IasAudio
{

	IasRingBufferTestWriter::IasRingBufferTestWriter(IasAudioRingBuffer* buffer,
							 std::uint32_t periodSize,
							 std::uint32_t numChannels,
							 IasAudioCommonDataFormat dataType,
							 std::string fileName)
		:mRingBuffer(buffer)
		,mPeriodSize(periodSize)
		,mNumChannels(numChannels)
		,mSampleSize(0)
		,mDataType(dataType)
		,mWriteBuf(NULL)
		,mDataFile(NULL)
		,mNumBufferChannels(0)
		,mFileName(fileName)
		,mInitialized(false)
		,mSamplesReadFromFile(0)
	{

	}

	IasAudioCommonResult IasRingBufferTestWriter::init()
	{
		mSampleSize = toSize(mDataType);

		std::uint32_t tmp = posix_memalign(&mWriteBuf,16,mPeriodSize*mSampleSize*mNumChannels);
		(void)tmp;

		if( (mSampleSize == -1) || (mRingBuffer == NULL) || (mPeriodSize == 0) || (mNumChannels == 0) )
		{
			return eIasResultInitFailed;
		}
		mNumBufferChannels = mRingBuffer->getNumChannels();
		SF_INFO  sourceInfo;
		mDataFile = sf_open(mFileName.c_str(),SFM_READ,&sourceInfo);
		if (mDataFile == nullptr)
		{
			fprintf(stderr,"Cannot open WAVE file %s\n", mFileName.c_str());
			return eIasResultInitFailed;
		}

		if((std::uint32_t)sourceInfo.channels > mNumBufferChannels)
		{
			fprintf(stderr,"Wave file has more channels (%u) than ringbuffer (%u)\n",sourceInfo.channels,mNumBufferChannels);
			return eIasResultInitFailed;
		}

		if(mDataFile == NULL)
		{
			return eIasResultInitFailed;
		}

		if( (sourceInfo.format == (SF_FORMAT_WAV |SF_FORMAT_PCM_32)) && (mDataType != eIasFormatInt32) )
		{
			fprintf(stderr,"data format of file (Int32) does not match data format of ringbuffer: format is %x\n",sourceInfo.format & SF_FORMAT_PCM_32);

			return eIasResultInitFailed;
		}
		if( (sourceInfo.format & (SF_FORMAT_WAV |SF_FORMAT_PCM_16)) && (mDataType != eIasFormatInt16) )
		{
			fprintf(stderr,"data format of file (Int16) does not match data format of ringbuffer: format is %x\n",sourceInfo.format & SF_FORMAT_PCM_16);
			return eIasResultInitFailed;
		}
		if( (sourceInfo.format == (SF_FORMAT_WAV |SF_FORMAT_FLOAT)) && (mDataType != eIasFormatFloat32) )
		{
			fprintf(stderr,"data format of file (Float32) does not match data format of ringbuffer: format is %x\n",sourceInfo.format & SF_FORMAT_FLOAT);
			return eIasResultInitFailed;
		}
		mInitialized = true;

		return eIasResultOk;
	}


	IasRingBufferTestWriter::~IasRingBufferTestWriter()
	{
		if(mDataFile)
		{
			sf_close(mDataFile);
		}
		free(mWriteBuf);

	}
	template<typename T>
	IasAudioCommonResult IasRingBufferTestWriter::copy( std::uint32_t numSamples, std::uint32_t chanIdx)
	{
		std::uint32_t samples = numSamples;
		std::uint32_t offset = 0;

		mRingBuffer->beginAccess(eIasRingBufferAccessWrite,&mAreas,&offset,&samples);

		T* src = static_cast<T*>(mWriteBuf);

		for(std::uint32_t i=0; i<samples ; i++)
		{
			for(std::uint32_t j=chanIdx; j< (mNumChannels+chanIdx);j++)
			{
				char* tmp = (char*)mAreas[j].start +  offset*sizeof(T) + (mAreas[j].first +i*mAreas[j].step)/8;
				T* sink = reinterpret_cast<T*>(tmp);
				*sink = *src;
				src++;
			}
		}

		mRingBuffer->endAccess(eIasRingBufferAccessWrite,offset,samples);

		return eIasResultOk;
	}


	IasAudioCommonResult IasRingBufferTestWriter::read( std::uint32_t* numReadSamples, std::int16_t* readDest)
	{
		*numReadSamples = (std::uint32_t) sf_readf_short(mDataFile, readDest, mPeriodSize);
		return eIasResultOk;
	}

	IasAudioCommonResult IasRingBufferTestWriter::read( std::uint32_t* numReadSamples, std::int32_t* readDest)
	{
		*numReadSamples = (std::uint32_t) sf_readf_int(mDataFile, readDest, mPeriodSize);
		return eIasResultOk;
	}

	IasAudioCommonResult IasRingBufferTestWriter::read( std::uint32_t* numReadSamples, float* readDest)
	{
		*numReadSamples = (std::uint32_t) sf_readf_float(mDataFile, readDest, mPeriodSize);
		return eIasResultOk;
	}

	IasAudioCommonResult IasRingBufferTestWriter::writeToBuffer(std::uint32_t chanIdx)
	{
		if(mInitialized == false)
		{
			return eIasResultNotInitialized;
		}

		std::uint32_t nReadSamples = 0;
		IasAudioCommonResult result = eIasResultOk;
		static IasAudioCommonResult prevRes = eIasResultNotInitialized;

		if(mDataFile == NULL)
		{
			return eIasResultInitFailed;
		}

		if( (chanIdx + mNumChannels) > mNumBufferChannels)
		{
			fprintf(stderr,"Invalid channel index\n");
			return eIasResultInvalidParam;
		}

		std::uint32_t samples = 0;

		mRingBuffer->updateAvailable(eIasRingBufferAccessWrite,&samples);
		if(samples >= mPeriodSize)
		{

			if(mDataType == eIasFormatInt32)
			{
				result = read(&nReadSamples,static_cast<std::int32_t*>(mWriteBuf));
			}
			else if(mDataType == eIasFormatInt16)
			{
				result = read(&nReadSamples,static_cast<std::int16_t*>(mWriteBuf));
			}
			else if(mDataType == eIasFormatFloat32)
			{
				result = read(&nReadSamples,static_cast<float*>(mWriteBuf));
			}
			else
			{
				fprintf(stderr,"Data type %s not supported, quit now!\n", toString(mDataType).c_str());
				result = eIasResultInvalidParam;
			}

			if (result != eIasResultOk)
			{
				return result;
			}

			if(nReadSamples == mPeriodSize)
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
				mSamplesReadFromFile += mPeriodSize;
			}
			else
			{
				return eIasResultBufferEmpty;
			}

		}
		else
		{
			if (result != prevRes) {
				fprintf(stderr,"IasRingBufferTestWriter::no space in destination buffer\n");
				prevRes = result;
			}
			result = eIasResultBufferFull;
		}

		return result;
	}

}
