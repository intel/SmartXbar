#ifndef VADS_SINK_HPP
#define VADS_SINK_HPP
/*******************************************************************//**
 * \file Sink.hpp
 * \brief Sink header
 * \details Sink header
 * \date 15-08-2018
 * \author Jakub Dorzak, jakubx.dorzak@intel.com
 **********************************************************************/

/***********************************************************************
 * INCLUDES
 **********************************************************************/
#include "IDev.hpp"

/***********************************************************************
 * TYPES
 **********************************************************************/

namespace Vads {
	class Sink: public IDev {
	public:
		/*!
		 * @brief Constructor
		 */
		Sink();
		
		/*!
		 * @brief Constructor
		 *
		 * @param[in] params Device parameters.
		 */
		Sink(const Params& params);
	private:
		/*!
		 * @brief Create a device
		 */
		void create();
	};
} //namespace Vads

#endif // VADS_SINK_HPP
/***********************************************************************
 * END OF FILE
 **********************************************************************/
