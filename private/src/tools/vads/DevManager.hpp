#ifndef VADS_DEV_MANAGER_HPP
#define VADS_DEV_MANAGER_HPP
/*******************************************************************//**
 * \file DevManager.hpp
 * \brief Manage virtual devices
 * \details
 * \date 03-08-2018
 * \author Jakub Dorzak, jakubx.dorzak@intel.com
 **********************************************************************/


/***********************************************************************
 * INCLUDES
 **********************************************************************/
#include <iostream>
#include <vector>
#include <boost/property_tree/ptree.hpp>
#include "audio/common/IasAudioCommonTypes.hpp"
#include "Timing.hpp"
#include "IDev.hpp"

/***********************************************************************
 * TYPES
 **********************************************************************/
namespace Vads {

	class DevManager {
	public:
		/*!
		 * @brief Constructor.
		 */
		DevManager() = default;
		/*!
		 * @brief Destructor.
		 */
		~DevManager() = default;

		/*!
		 * @brief Init DevManager. The method reads a config file and propagate device parameters accordingly.
		 *
		 * @returns eIasResultOk on success, eIasResultInitFailed on failure.
		 *
		 * @param[in] file Input file.
		 */
		IasAudio::IasAudioCommonResult init(std::ifstream& file);

		/*!
		 * @brief Start all devices
		 */
		void startDevices();

		/*!
		 * @brief Stop all devices
		 */
		void stopDevices();
		
		/*!
		 * @brief Get number of sink devices.
		 *
		 * @returns Number of sink devices.
		 */
		int getNumOfSinks();

		/*!
		 * @brief Get number of source devices.
		 *
		 * @returns Number of source devices.
		 */
		int getNumOfSources();

	private:
		/*!
		 * @brief Get sample format.
		 * 
		 * @returns Sample format.
		 *
		 * @param[in] fmt Format.
		 */
		IasAudio::IasAudioCommonDataFormat getFormat(const int fmt);
		
		/*!
		 * @brief Get clock type.
		 * 
		 * @returns Clock type.
		 *
		 * @param[in] clk String describing clock type.
		 */
		IasAudio::IasClockType getClockType(const std::string clk);

		/*!
		 *
		 * @brief Get device type (soruce or sink).
		 *
		 * @returns Device type.
		 *
		 * @param[in] type String describing device type.
		 */
		IasAudio::IasDeviceType getType(const std::string type);

		/*!
		 * @brief Get working mode (blocking or non-blocking).
		 *
		 * @returns True if blocking, false if non-blocking.
		 *
		 * @param[in] mode String describing mode.
		 */
		bool isBlocking(const std::string mode);

		/*!
		 * @brief Get timing section from config file.
		 *
		 * @returns Timing struct.
		 *
		 * @param[in] tree XML timing description.
		 */
		Timing getTiming(boost::property_tree::ptree tree);
		
		std::vector<std::shared_ptr<IDev>> mSinks; //!< vector of pointers to sink devices
		std::vector<std::shared_ptr<IDev>> mSources; //!< vector of pointers to source devices
		DltContext *mDltLog; //!< DLT log context
	};
} //namespace Vads

#endif // VADS_DEV_MANAGER_HPP
/***********************************************************************
 * END OF FILE
 **********************************************************************/
