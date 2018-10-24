/*
 * * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/*
 * IasRingBufferTestWriter.hpp
 *
 *  Created 2015
 */

#ifndef IASRINGBUFFERTESTWRITER_HPP_
#define IASRINGBUFFERTESTWRITER_HPP_

#include <sndfile.h>
#include "audio/common/IasAudioCommonTypes.hpp"

namespace IasAudio
{

	class IasAudioRingBuffer;

	class IAS_AUDIO_PUBLIC IasRingBufferTestWriter
	{

	public:

		IasRingBufferTestWriter(IasAudioRingBuffer* buffer,
					std::uint32_t periodSize,
					std::uint32_t numChannels,
					IasAudioCommonDataFormat dataType,
					std::string fileName);


		~IasRingBufferTestWriter();

		IasAudioCommonResult init();

		IasAudioCommonResult writeToBuffer(std::uint32_t chanIdx);
		inline void dummyWrite(){};

	private:

		IasRingBufferTestWriter(IasRingBufferTestWriter const &other);

		IasRingBufferTestWriter& operator=(IasRingBufferTestWriter const &other);

		IasAudioCommonResult read( std::uint32_t* numReadSamples, std::int16_t* readDest);

		IasAudioCommonResult read( std::uint32_t* numReadSamples, std::int32_t* readDest);

		IasAudioCommonResult read( std::uint32_t* numReadSamples, float* readDest);

		template< typename T>
			IasAudioCommonResult copy( std::uint32_t numSamples, std::uint32_t chanIdx);

		IasAudioRingBuffer*      mRingBuffer;
		std::uint32_t              mPeriodSize;
		std::uint32_t              mNumChannels;
		std::int32_t               mSampleSize;
		IasAudioCommonDataFormat mDataType;
		void*                    mWriteBuf;
		SNDFILE*                 mDataFile;
		IasAudioArea*            mAreas;
		std::uint32_t              mNumBufferChannels;
		std::string              mFileName;
		bool                mInitialized;
		std::uint64_t              mSamplesReadFromFile;

	};

}
#endif
