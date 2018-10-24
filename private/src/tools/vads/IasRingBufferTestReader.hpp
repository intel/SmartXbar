/*
 * * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/*
 * IasRingBufferTestReader.hpp
 *
 *  Created 2015
 */

#ifndef IASRINGBUFFERTESTREADER_HPP_
#define IASRINGBUFFERTESTREADER_HPP_

#include <sndfile.h>
#include "internal/audio/common/audiobuffer/IasAudioRingBuffer.hpp"

namespace IasAudio
{
	class IasAudioRingBuffer;

	class IAS_AUDIO_PUBLIC IasRingBufferTestReader
	{
	public:

		IasRingBufferTestReader(IasAudioRingBuffer* buffer,
					std::uint32_t periodSize,
					std::uint32_t numChannels,
					IasAudioCommonDataFormat dataType,
					std::string fileName);

		~IasRingBufferTestReader();

		IasAudioCommonResult init();

		IasAudioCommonResult readFromBuffer(std::uint32_t chanIdx);
		inline void dummyRead(){}

	private:

		IasRingBufferTestReader(IasRingBufferTestReader const &other);

		IasRingBufferTestReader& operator=(IasRingBufferTestReader const &other);

		template<typename T>
			IasAudioCommonResult copy( std::uint32_t numSamples, std::uint32_t chanIdx);


		IasAudioCommonResult write(sf_count_t* numWrittenSamples,std::int32_t* src);

		IasAudioCommonResult write(sf_count_t* numWrittenSamples,std::int16_t* src);

		IasAudioCommonResult write(sf_count_t* numWrittenSamples,float* src);

		IasAudioRingBuffer      *mRingBuffer;
		std::uint32_t              mPeriodSize;
		std::uint32_t              mNumChannels;
		std::int32_t               mSampleSize;
		IasAudioCommonDataFormat mDataType;
		void*                    mReadBuf;
		SNDFILE*                 mDataFile;
		IasAudioArea*            mAreas;
		std::uint32_t              mNumBufferChannels;
		std::string              mFileName;
		bool                mInitialized;
		std::uint64_t              mSamplesWrittenToFile;


	};

}
#endif
