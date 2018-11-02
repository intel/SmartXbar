/*******************************************************************//**
 * \file Timing.cpp
 * \brief Timing definitions
 * \details Virtual device timing
 * \date 20-08-2018
 * \author Jakub Dorzak, jakubx.dorzak@intel.com
 **********************************************************************/

/***********************************************************************
 * INCLUDES
 **********************************************************************/
#include <iostream>
#include "avbaudiomodules/internal/audio/common/IasAudioLogging.hpp"
#include "Timing.hpp"

using namespace std;
using namespace IasAudio;
/***********************************************************************
 * PUBLIC DEFINITIONS
 **********************************************************************/
namespace Vads {

static const std::string cClassName = "Timing::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"
	
Timings::Timings(Timing t):
	mTiming(t),
	mDltLog(IasAudioLogging::registerDltContext("TMNG", "Timing")) {
			    
	//mDltLog = IasAudioLogging::registerDltContext("TMNG", "Timing");
}

vector<Timings::TimerSequence> Timings::getSequence() {
	createSequence();
	return mTimerSequence;
}
 
/***********************************************************************
 * PRIVATE DEFINITIONS
 **********************************************************************/
void Timings::createSequence() {
	TimerSequence timerSeq;
	
	for(vector<TimingPacket>::iterator it = mTiming.sequence.begin(); it != mTiming.sequence.end(); ++it) {
		TimingType type = (*it).type;
		switch(type) {
		case ePureTicks:
			timerSeq.tick = mTiming.tick;
			timerSeq.tickNumber = (*it).timing.pureTicks;
			break;
		case eDriftedTicks:
			timerSeq.tick = mTiming.tick + (*it).timing.drift.value;
			timerSeq.tickNumber = (*it).timing.drift.ticks;
			break;
		case eEmptyTicks:
			timerSeq.tick = mTiming.tick;
			timerSeq.tickNumber = (*it).timing.emptyTicks;
			break;
		case eUnknown:
			break;
		}

		timerSeq.type = type;
		mTimerSequence.push_back(timerSeq);
	}
}

} // namespace Vads
/***********************************************************************
 * END OF FILE
 **********************************************************************/
