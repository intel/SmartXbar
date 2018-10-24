/*******************************************************************//**
 * \file Source.cpp
 * \brief Source definition
 * \details Source definition
 * \date 16-08-2018
 * \author Jakub Dorzak, jakubx.dorzak@intel.com
 **********************************************************************/

/***********************************************************************
 * INCLUDES
 **********************************************************************/
#include <iostream>
#include "smartx/IasSmartXClient.hpp"
#include "internal/audio/common/audiobuffer/IasAudioRingBufferResult.hpp"
#include "internal/audio/common/audiobuffer/IasAudioRingBuffer.hpp"
#include "internal/audio/common/IasAudioLogging.hpp"
#include "IasRingBufferTestWriter.hpp"
#include "Timing.hpp"
#include "Timer.hpp"
#include "Source.hpp"

using namespace std;
using namespace IasAudio;
/***********************************************************************
 * PUBLIC DEFINITIONS
 **********************************************************************/
namespace Vads {

static const std::string cClassName = "Source::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"
	
Source::Source() {
	//mDltLog = IasAudioLogging::registerDltContext("VSRC", "Virtual Source");
	//empty body
}



Source::Source(const Params& params): IDev(params) {
}
/***********************************************************************
 * PRIVATE DEFINITIONS
 **********************************************************************/
void Source::create() {
	unique_ptr<IasSmartXClient>client(new IasSmartXClient(mParams.alsaParams));
	if (nullptr == client) {
		DLT_LOG_CXX(*mDltLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't instantiate SmartX client");
		cout << "Can't instantiate IasSmartXClient" << endl;
	}
	IasSmartXClient::IasResult res = client->init(mParams.type);
	if (IasSmartXClient::eIasFailed == res) {
		DLT_LOG_CXX(*mDltLog, DLT_LOG_ERROR, LOG_PREFIX, "SmartX client init failed");
		cout << "IasSmartXClient init failed" << endl;
	}
	unique_ptr<IasAlsaPluginShmConnection>shmConn(new IasAlsaPluginShmConnection());
	if (nullptr == shmConn) {
		DLT_LOG_CXX(*mDltLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't instantiate SHM connection");
		cout << "Can't instantiate SHM connection" << endl;
	}
	std::string conn = getConnName(mParams.type, mParams.alsaParams->name);
	IasAudioCommonResult r = shmConn->findConnection(conn);
	if (eIasResultOk != r) {
		DLT_LOG_CXX(*mDltLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't find SHM connection");
		cout << "Can't find SHM connection" << endl;
	}
	//get handle to ringbuffer
	IasAudioRingBuffer *buff = nullptr;
	res = client->getRingBuffer(&buff);
	if (res == IasSmartXClient::eIasFailed) {
		DLT_LOG_CXX(*mDltLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't find buffer handle");
		cout << "Can't find buffer handle" << endl;
	}
	// by default ringbuffer is in blocking mode
	// check if we have non-blocking
	if(false == mParams.blockingMode) {
		IasAudioRingBufferResult res = buff->setNonBlockMode(true);
		if (eIasRingBuffOk != res) {
			DLT_LOG_CXX(*mDltLog, DLT_LOG_ERROR, LOG_PREFIX, "Non blocking mode failed");
			cout << "Non blocking mode failed" << endl;
		}
		else {
			DLT_LOG_CXX(*mDltLog, DLT_LOG_INFO, LOG_PREFIX, "Non-blocking mode active");
		}		
	}
	shared_ptr<IasRingBufferTestWriter> writer(new IasRingBufferTestWriter(
							   buff,
							   mParams.alsaParams->periodSize,
							   mParams.alsaParams->numChannels,
							   mParams.alsaParams->dataFormat,
							   mParams.file));
	if (nullptr == writer) {
		DLT_LOG_CXX(*mDltLog, DLT_LOG_ERROR, LOG_PREFIX, "Failed to instantiate writer");
		cout << "Failed to instantiate writer" << endl;
	}
	r = writer->init();
	if (eIasResultOk != r) {
		DLT_LOG_CXX(*mDltLog, DLT_LOG_ERROR, LOG_PREFIX, "Failed to init writer");
		cout << "Failed to init writer" << endl;
	}
	DLT_LOG_CXX(*mDltLog, DLT_LOG_INFO, LOG_PREFIX, "Source usage: arecord -Dplug:smartx: ", mParams.alsaParams->name, " <file>, input: ", mParams.file);
	cout << "Source usage: arecord -Dplug:smartx:" << mParams.alsaParams->name << " <file>, input: " << mParams.file <<  endl;
	//IasAudioCommonResult buffState;

	Timings t(mParams.timing);
	vector<Timings::TimerSequence> seq = t.getSequence();
#if 0
	cout << "Timing sequence size: " << seq.size() << endl;
	int i = 0;
	for(auto it = seq.begin(); it != seq.end(); ++it) {
		cout << mParams.alsaParams->name << " seq[" << i << "].tick=" << (*it).tick << endl;
		cout << mParams.alsaParams->name << " seq[" << i << "].tickNumber=" << (*it).tickNumber << endl;
		cout << mParams.alsaParams->name << " seq[" << i << "].tickType=" << (int)(*it).type << endl;
		i++;
	}
#endif
	while(mIsRunning) {
		for(auto it = seq.begin(); it != seq.end(); ++it) {
			if (eUnknown == (*it).type) {
				cout << "Tick type not recognized" << endl;
				exit(-1);
			}
			else if (eEmptyTicks == (*it).type) {
				//cout << "Empty ticks" << endl;
				int tickNumber = (*it).tickNumber;
				int tickDelay = (*it).tick;
				for(int i = 0; i < tickNumber; ++i) {
					//cout << "tick no= " << i << endl;
					Timer t(tickDelay, &IasAudio::IasRingBufferTestWriter::dummyWrite, writer);
				}
			}
			else {
				//cout << "Ticks" << endl;
				//cout << "Sequence: " << cnt++ << endl;
				//cout << mParams.alsaParams->name << ": seq[" << cnt << "].";
				int tickNumber = (*it).tickNumber;
				int tickDelay = (*it).tick;
				for(int i = 0; i < tickNumber; ++i) {
					//cout << "tick no= " << i << endl;
					Timer t(tickDelay, &IasAudio::IasRingBufferTestWriter::writeToBuffer, writer, 0);
				}
			}
		}
	}
	cout << "Stopping device: " << mParams.alsaParams->name << endl;	
}

} // namespace Vads
/***********************************************************************
 * END OF FILE
 **********************************************************************/
