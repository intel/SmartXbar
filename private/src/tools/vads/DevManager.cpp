/*******************************************************************//**
 * \file DevManager.cpp
 * \brief Manage virtual devices
 * \details
 * \date 03-08-2018
 * \author Jakub Dorzak, jakubx.dorzak@intel.com
 **********************************************************************/


/***********************************************************************
 * INCLUDES
 **********************************************************************/
#include <sstream>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include "avbaudiomodules/internal/audio/common/IasAudioLogging.hpp"
#include "Sink.hpp"
#include "Source.hpp"
#include "DevManager.hpp"

using namespace std;
using namespace boost::property_tree;
using namespace IasAudio;
/***********************************************************************
 * PUBLIC DEFINITIONS
 **********************************************************************/
namespace Vads {

static const std::string cClassName = "DevManager::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"


	
IasAudioCommonResult DevManager::init(ifstream& file) {
	IasAudioLogging::addDltContextItem("VADS", DLT_LOG_INFO, DLT_TRACE_STATUS_ON);
	IasAudioLogging::addDltContextItem("VDEV", DLT_LOG_INFO, DLT_TRACE_STATUS_ON);
	IasAudioLogging::addDltContextItem("VSNK", DLT_LOG_INFO, DLT_TRACE_STATUS_ON);
	IasAudioLogging::addDltContextItem("VSRC", DLT_LOG_INFO, DLT_TRACE_STATUS_ON);
	mDltLog = IasAudioLogging::registerDltContext("VADS", "Virtual ALSA dev simulator");
        ptree confTree;
        read_xml(file, confTree);

	BOOST_FOREACH(ptree::value_type const &v, confTree.get_child("vads.devices")) {
		if(v.first == "dev") {		
			IDev::Params p;
			string s;
			int i;
			
			p.alsaParams = std::make_shared<IasAudioDeviceParams>();
			s = v.second.get<string>("type");
			p.type = this->getType(s);
			p.alsaParams->name = v.second.get<string>("name");
			i = v.second.get<int>("format");
			p.alsaParams->dataFormat = this->getFormat(i);
			p.alsaParams->samplerate = v.second.get<int>("frequency");
			p.alsaParams->numChannels = v.second.get<int>("channelNo");
			s = v.second.get<string>("clock");
			p.alsaParams->clockType = this->getClockType(s);
			p.alsaParams->numPeriods = v.second.get<int>("periodNo");
			p.alsaParams->periodSize = v.second.get<int>("periodSize");
			s = v.second.get<string>("mode");
			p.blockingMode = isBlocking(s);
			p.file = v.second.get<string>("file");			
			// process timing
			ptree timingTree = (ptree)v.second.get_child("timing");
			p.timing = getTiming(timingTree);
			shared_ptr<IDev> dev = nullptr;
			if(eIasDeviceTypeSource == p.type) {
				//dev = new Source();
				dev = make_shared<Sink>(p);
				mSinks.push_back(dev); //that's strange but if set to eIasDeviceTypeSink, aplay doesn't work
				///cout << "Name: " << p.alsaParams->name << endl;
				///cout << "Tick: " << p.timing.tick << endl;
				///cout << "Timing seq: " << p.timing.sequence.size() << endl;
			}
			else if(eIasDeviceTypeSink == p.type) {
				//dev = new Source();
				dev = make_shared<Source>(p);
				mSources.push_back(dev);
				///cout << "Name: " << p.alsaParams->name << endl;
				///cout << "Tick: " << p.timing.tick << endl;
				///cout << "Timing seq: " << p.timing.sequence.size() << endl;
			}
			else {
				cout << "Wrong device type" << endl;
				return eIasResultInitFailed;
			}
		}
	}
	cout << "Found sinks: " << getNumOfSinks() << endl;		
	cout << "Found sources: " << getNumOfSources() << endl;
	DLT_LOG_CXX(*mDltLog, DLT_LOG_INFO, LOG_PREFIX, "Found sinks: ", getNumOfSinks());
	DLT_LOG_CXX(*mDltLog, DLT_LOG_INFO, LOG_PREFIX, "Found sources: ", getNumOfSources());
	return eIasResultOk;
}



void DevManager::startDevices() {
	DLT_LOG_CXX(*mDltLog, DLT_LOG_INFO, LOG_PREFIX, "Starting devices");
	for (auto snk: mSinks) {
		snk->start();
	}
	for (auto src: mSources) {
		src->start();
	}
}


void DevManager::stopDevices() {
	for (auto snk: mSinks) {
		snk->stop();
	}
	for (auto src: mSources) {
		src->stop();
	}
	DLT_LOG_CXX(*mDltLog, DLT_LOG_INFO, LOG_PREFIX, "All sinks and sources stopped");
}


	
int DevManager::getNumOfSinks() {
	return mSinks.size();
}



int DevManager::getNumOfSources() {
	return mSources.size();
}

/***********************************************************************
 * PRIVATE DEFINITIONS
 **********************************************************************/
IasAudioCommonDataFormat DevManager::getFormat(const int fmt) {
        switch(fmt) {
	case 16:
		return eIasFormatInt16;
	case 32:
		return eIasFormatInt32;
	default:
		DLT_LOG_CXX(*mDltLog, DLT_LOG_ERROR, LOG_PREFIX, "Bad sample format: ", fmt);
		return eIasFormatUndef;
	}
}



IasClockType DevManager::getClockType(const string clk) {
	if ("master" == clk) {
		return eIasClockProvided;
	}
	else if ("slave" == clk) {
		return eIasClockReceived;
	}
	else {
		DLT_LOG_CXX(*mDltLog, DLT_LOG_ERROR, LOG_PREFIX, "Bad clock type: ", clk);
		return eIasClockUndef;
	}
}



IasDeviceType DevManager::getType(const string type) {
        if ("playback" == type) {
		return eIasDeviceTypeSource;
	}
	else if ("capture" == type) {
		return eIasDeviceTypeSink;
	}
	else {
		DLT_LOG_CXX(*mDltLog, DLT_LOG_ERROR, LOG_PREFIX, "Bad device type: ", type);
		return eIasDeviceTypeUndef;
	}
}



bool DevManager::isBlocking(const string mode) {
	bool isBlocking = true;
	if("nonblocking" == mode) {
		isBlocking = false;
		return isBlocking;
	}
	return isBlocking;
}


Timing DevManager::getTiming(ptree tree) {
	Timing timing; //TODO: init
	ptree timingTree = tree;
	string s;
	
	timing.tick = timingTree.get<int>("tick");
	s = timingTree.get<string>("tick.<xmlattr>.prefix");
	if("ms" == s) {
		timing.prefix = TickPrefix::eMiliSecond;
	}
	else if("us" == s) {
		timing.prefix = TickPrefix::eMicroSecond;
	}
	else {
		cout << "Bad prefix" << endl;
		DLT_LOG_CXX(*mDltLog, DLT_LOG_ERROR, LOG_PREFIX, "Bad prefix: ", s);
	}

	//cout << "Got tick: " << timing.tick << ", units: " << s << endl;
	// traverse sequence of ticks
	BOOST_FOREACH(const ptree::value_type &vs, timingTree.get_child("sequence")) {				
		string s = vs.first;
		int tickNumber = 0;
		TimingOption timingOption;
		TimingType type = eUnknown;
				
		if("ticksNumber" == s) {
			int driftVal = 0;
			tickNumber = vs.second.get<int>(""); //"ticksNumber" throws error, only empty string works
			///cout << "Ticks: " << tickNumber << endl;
			//get attributes for every tick
			ptree attrTree = vs.second.get_child("");					
			BOOST_FOREACH(const ptree::value_type &va, attrTree.get_child("<xmlattr>")) {
				string drift = va.second.data();
				stringstream convert(drift);
				convert >> driftVal;
				///cout << va.first << ": " << driftVal << endl;
			}
					
			// push parsed stuff into struct
			if(0 == driftVal) {
				type = ePureTicks;
				timingOption.pureTicks = tickNumber;
			}
			else {
				type = eDriftedTicks;
				timingOption.drift = {driftVal, tickNumber};
			}
					
		}
		else if("ticksEmpty" == s) {
			tickNumber = vs.second.get<int>("");
			///cout << s << ": " << tickNumber << endl;
			type = eEmptyTicks;
			timingOption.emptyTicks = tickNumber;					
		}
		else if("<xmlcomment>" == s) {
			//ignore comment tag otherwise foreach will have iteration for every comment
			continue;
		}
		else {
			DLT_LOG_CXX(*mDltLog, DLT_LOG_ERROR, LOG_PREFIX, "Bad XML entry: ", s);
			//TODO: handle error
			///cout << s << endl;
			///cout << "XML parsing error" << endl;
		}				
		TimingPacket packet = {type, timingOption};
		timing.sequence.push_back(packet);
	}
	return timing;
}

} // Vads
/***********************************************************************
 * END OF FILE
 **********************************************************************/
