/*******************************************************************//**
 * \file IDev.cpp
 * \brief Device interface
 * \details Virtual device interface
 * \date 15-08-2018
 * \author Jakub Dorzak, jakubx.dorzak@intel.com
 **********************************************************************/

/***********************************************************************
 * INCLUDES
 **********************************************************************/
#include <functional>
#include "avbaudiomodules/internal/audio/common/IasAudioLogging.hpp"
#include "IDev.hpp"

using namespace std;
using namespace IasAudio;
/***********************************************************************
 * PUBLIC DEFINITIONS
 **********************************************************************/
namespace Vads {

static const std::string cClassName = "IDev::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"

IDev::IDev(): mIsRunning(false),
	      mWorker(nullptr),
	      mDltLog(IasAudioLogging::registerDltContext("VDEV", "Virtual Device")){
	mParams.blockingMode = true; //TODO: how to put this to init list?
}



IDev::IDev(const Params& params): mParams(params),
				  mIsRunning(false),
				  mWorker(nullptr),
				  mDltLog(IasAudioLogging::registerDltContext("VDEV", "Virtual Device")){
	mParams.blockingMode = true;
}



IDev::~IDev() {
}



void IDev::stop() {
	mIsRunning = false;
	mWorker->join();
	//delete mWorker;
	DLT_LOG_CXX(*mDltLog, DLT_LOG_INFO, LOG_PREFIX, "Device stopped");
}



void IDev::start() {
	DLT_LOG_CXX(*mDltLog, DLT_LOG_INFO, LOG_PREFIX, "Starting device");
	mIsRunning = true;
	//mWorker = new thread(&IDev::create, this);
	mWorker = make_shared<thread>(bind(&IDev::create, this));
}

/***********************************************************************
 * PRIVATE DEFINITIONS
 **********************************************************************/
std::string IDev::getConnName(const IasDeviceType deviceType, const std::string name) {
	const std::string prefix = "smartx_";
	std::string postfix;
	if (deviceType == eIasDeviceTypeSink) {
		postfix = "_c";
	}
	else {
		postfix = "_p";
	}
	std::string fullName(prefix + name + postfix);
	return fullName;
}

} //namespace Vads
/***********************************************************************
 * END OF FILE
 **********************************************************************/
