#ifndef VADS_SOURCE_HPP
#define VADS_SOURCE_HPP
/*******************************************************************//**
 * \file Source.hpp
 * \brief Source header
 * \details Source header
 * \date 16-08-2018
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
	
	class Source: public IDev {
	public:
		/*!
		 * @brief Constructor
		 */
		Source();

		/*!
		 * @brief Constructor
		 *
		 * @param[in] params Device parameters.
		 */
		Source(const Params& params);
		
	private:
		/*!
		 * @brief Create a device
		 */
		void create();
	};
	
} //namespace Vads
#endif // VADS_SOURCE_HPP
/***********************************************************************
 * END OF FILE
 **********************************************************************/
