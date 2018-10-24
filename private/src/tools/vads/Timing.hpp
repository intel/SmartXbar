#ifndef VADS_TIMING_HPP
#define VADS_TIMING_HPP
/*******************************************************************//**
 * \file Timing.hpp
 * \brief Timing header
 * \details Virtual device timing
 * \date 09-08-2018
 * \author Jakub Dorzak, jakubx.dorzak@intel.com
 **********************************************************************/

/***********************************************************************
 * INCLUDES
 **********************************************************************/
#include <vector>
#include "audio/common/IasAudioCommonTypes.hpp"

//using namespace std;

/***********************************************************************
 * TYPES
 **********************************************************************/
namespace Vads {
	struct Drift {
		int value; //!< drift value. At the moment only in us
		int ticks; //!< number of ticks affected by the drift value
	};

	union TimingOption {
		int pureTicks; //!< ticks not affected by drift
		Drift drift; //!< drift definition
		int emptyTicks; //!< send no data for emptyTicks ticks 
	};

	enum TickPrefix {
		eMicroSecond, //!< unit prefix, only us at the moment
		eMiliSecond,
	};

	enum TimingType {
		ePureTicks = 0, //!< three possible tick options
		eDriftedTicks,
		eEmptyTicks,
		eUnknown,
	};
	
	struct TimingPacket {
		TimingType type;
		TimingOption timing;
	};
	
	struct Timing {
		int tick; //!< base clock tick
		TickPrefix prefix;
		std::vector<TimingPacket> sequence; //!< sequence of ticks for a device
	};

	class Timings {
	public:
		/*!
		 * @brief TimerSequence is created from Timing data above. It's passed to the timer so it can start intervals.
		 */
		struct TimerSequence {
			int tick;
			int tickNumber;
			TimingType type;
		};
		
		/*!
		 * @brief Constructor
		 */
		Timings(Timing t);

		/*!
		 * @brief Get timing sequence
		 *
		 * @returns Timing sequence
		 */
		std::vector<TimerSequence> getSequence();		
	private:
		/*!
		 * @brief Create timing sequence
		 */
		void createSequence();
		
		Timing mTiming; //!< Timing info
		std::vector<TimerSequence>mTimerSequence; //!< timing sequence
		DltContext  *mDltLog; //!< DLT log context
	};
	
} //namespace Vads


#endif // VADS_TIMING_HPP
/***********************************************************************
 * END OF FILE
 **********************************************************************/
