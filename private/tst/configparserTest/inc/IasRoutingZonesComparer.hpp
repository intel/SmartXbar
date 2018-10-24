/*
 * IasRoutingZonesComparer.hpp
 *
 *  Created on: Jun 7, 2017
 *      Author: mkielanx
 */

#ifndef AUDIO_DAEMON2_PRIVATE_TST_CONFIGPARSERTEST_INC_IASROUTINGZONESCOMPARER_HPP_
#define AUDIO_DAEMON2_PRIVATE_TST_CONFIGPARSERTEST_INC_IASROUTINGZONESCOMPARER_HPP_


#include "audio/smartx/IasISetup.hpp"

namespace IasAudio {



class IasRoutingZonesComparer
{
  IasRoutingZoneVector rzn;

public:

  IasRoutingZonesComparer(IasISetup* setup);

  IasRoutingZoneVector getRoutingZones();

  friend bool operator==(IasRoutingZonePtr lhs, IasRoutingZonePtr rhs);
  friend bool operator!=(IasRoutingZonePtr lhs, IasRoutingZonePtr rhs);

  friend bool operator==(IasRoutingZoneVector lhs, IasRoutingZoneVector rhs);
  friend bool operator!=(IasRoutingZoneVector lhs, IasRoutingZoneVector rhs);

  friend bool operator==(IasRoutingZonesComparer lhs, IasRoutingZonesComparer rhs);
  friend bool operator!=(IasRoutingZonesComparer lhs, IasRoutingZonesComparer rhs);
};

}  /* namespace IasAudio */




#endif /* AUDIO_DAEMON2_PRIVATE_TST_CONFIGPARSERTEST_INC_IASROUTINGZONESCOMPARER_HPP_ */
