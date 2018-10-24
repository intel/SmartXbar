#ifndef VADS_TIMER_HPP
#define VADS_TIMER_HPP
/*******************************************************************//**
 * \file Timer.hpp
 * \brief Timer header
 * \details Timer header
 * \date 17-08-2018
 * \author Jakub Dorzak, jakubx.dorzak@intel.com
 **********************************************************************/

/***********************************************************************
 * INCLUDES
 **********************************************************************/
#include <functional>

/***********************************************************************
 * TYPES
 **********************************************************************/
namespace Vads {
	class Timer {
	public:
		/*!
		 * @brief Constructor. Triggers a task every n microseconds.
		 *
		 * No logging here as it might influence event timing.
		 */
		template <class callable, class... arguments>
		Timer(const int delay, callable&& f, arguments&&... args) {
			using namespace std;
			function<typename result_of<callable(arguments...)>::type()> task(bind(forward<callable>(f), forward<arguments>(args)...));
			this_thread::sleep_for(chrono::microseconds(delay));
			task();
		}
	};
} //namespace Vads

#endif // VADS_TIMER_HP
/***********************************************************************
 * END OF FILE
 **********************************************************************/
