#ifndef VADS_IDEV_HPP
#define VADS_IDEV_HPP
/*******************************************************************//**
 * \file IDev.hpp
 * \brief Device interface
 * \details Virtual device interface
 * \date 14-08-2018
 * \author Jakub Dorzak, jakubx.dorzak@intel.com
 **********************************************************************/

/***********************************************************************
 * INCLUDES
 **********************************************************************/
#include <string>
#include <atomic>
#include <thread>
#include "avbaudiomodules/audio/common/IasAudioCommonTypes.hpp"
#include "Timing.hpp"

/***********************************************************************
 * TYPES
 **********************************************************************/
namespace Vads {

	class IDev {
	public:
		/*!
		 *  @brief Device parameters.
		 */
		struct Params {
			IasAudio::IasAudioDeviceParamsPtr alsaParams {nullptr};
			IasAudio::IasDeviceType type {};
			bool blockingMode {true};
			std::string file {};
			Timing timing {};
		};

		/*!
		 * @brief Constructor.
		 */
		IDev();

		/*!
		 * @brief Constructor
		 *
		 * @param[in] params Device parameters.
		 */
		IDev(const Params& params);

		/*!
		 * @brief Destructor.
		 */
		virtual ~IDev();

		/*!
		 * @brief Stop device.
		 */
		void stop();

		/*!
		 * @brief Start device.
		 */
		void start();

	protected:
		/*!
		 * @brief Create device.
		 */
		virtual void create() = 0;

		/*!
		 * @brief Get connection name.
		 *
		 * @param[in] deviceType Device type (sink or source).
		 * @param[in] name Device name.
		 */
		std::string getConnName(const IasAudio::IasDeviceType deviceType, const std::string name);

		Params mParams; //!< device's parameters
		bool mIsRunning; //!< flag to indicate if device is running
		std::shared_ptr<std::thread> mWorker; //!< pointer to worker thread
		DltContext  *mDltLog; //!< DLT log context
	};

} //namespace Vads

#endif // VADS_IDEV_HPP
/***********************************************************************
 * END OF FILE
 **********************************************************************/
