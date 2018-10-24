/*******************************************************************//**
* \file main.cpp
* \brief Virtual alsa devices main.cpp
* \details
* \date 19-07-2018
* \author Jakub Dorzak, jakubx.dorzak@intel.com
**********************************************************************/

/***********************************************************************
 * INCLUDES
 **********************************************************************/
#include <fstream>
#include <iostream>
#include <thread>
#include <boost/program_options.hpp>
#include "audio/common/IasAudioCommonTypes.hpp"
#include "smartx/IasConfigFile.hpp"
#include "smartx/IasSmartXClient.hpp"
#include "internal/audio/common/audiobuffer/IasAudioRingBuffer.hpp"
#include "internal/audio/common/IasAudioLogging.hpp"
#include "IasRingBufferTestReader.hpp"
#include "IasRingBufferTestWriter.hpp"
#include "DevManager.hpp"

using namespace std;
using namespace IasAudio;
using namespace Vads;

/***********************************************************************
 * PRIVATE VARIABLES
 **********************************************************************/
static string config;

/***********************************************************************
 * PRIVATE FUNCTIONS
 **********************************************************************/
static int parseArgs(int argc, char* argv[]) {

	const string defaultConfig = "default.xml";
	po::options_description desc("Allowed options:");
	desc.add_options()
		("help,h", "print usage message")
		("config,c", po::value<string>(&config)->implicit_value(defaultConfig), "read config xml") //vector'd be more generic, but don't know how to do this
		("list,l", "list devices")
		("quit,q", "Quit program");
	
	po::variables_map vm;
	po::store(parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (vm.count("help")) {
		cout << desc << endl;
		exit(0);
	}

	if (vm.count("config")) {
		string f = vm["config"].as<string>();
		cout << "Config file " << f << endl;
   	}
	else if (vm.count("list")){
		cout << "Listing devices" << endl;
	}
	else {
		cout << "Unknown option." << endl;
		cout << desc << endl;
		exit(-1);
	}

	ifstream ifs(config.c_str());
	if (!ifs) {
		cout << "No such config file: " << config << endl;
		cout << "Please select config file or use: " << argv[0] << " -c " << defaultConfig << endl;
		exit(-1);
	}
	else {
		cout << "Found config: " << config << endl;
	}

	return 0;
}

/***********************************************************************
 * ENTRY
 **********************************************************************/
int main(int argc, char* argv[]) {
	DLT_REGISTER_APP("VADS", "Virtual ALSA device simulator");
	//IasAudioLogging::addDltContextItem("VADS", DLT_LOG_INFO, DLT_TRACE_STATUS_ON);
	
	IasConfigFile *cfgFile = IasConfigFile::getInstance();
	if (nullptr == cfgFile) {
		cout << "Can't instantiate config file" << endl;
	}
	cfgFile->load();

	parseArgs(argc, argv);

	DevManager dm;
	ifstream cfg(config.c_str());
	IasAudioCommonResult res = dm.init(cfg);
	if (eIasResultOk != res) {
		cout << "Init failed" << endl;
		exit(-1);
	}

	dm.startDevices();
	
	for(;;) {
		int c = getchar();
		if(c == 'q') {
			dm.stopDevices();
			break;
		}
	}
	
	return 0;
}
/***********************************************************************
 * END OF FILE
 **********************************************************************/
