/*
 * IasRoutingZonesComparer.cpp
 *
 *  Created on: Jun 7, 2017
 *      Author: mkielanx
 */

#include "audio/smartx/IasISetup.hpp"
#include "model/IasRoutingZone.hpp"
#include "IasRoutingZonesComparer.hpp"

namespace IasAudio {

IasRoutingZonesComparer::IasRoutingZonesComparer(IasISetup* setup) :
  rzn{setup->getRoutingZones()}
{
}

IasRoutingZoneVector IasRoutingZonesComparer::getRoutingZones()
{
  return rzn;
}

bool operator==(IasRoutingZonePtr lhs, IasRoutingZonePtr rhs)
{
  if(lhs->getName() != rhs->getName()) return false;
  if(lhs->getPeriodSize() != rhs->getPeriodSize()) return false;
  if(lhs->getSampleRate() != rhs->getSampleRate()) return false;
  return true;
}

bool operator!=(IasRoutingZonePtr lhs, IasRoutingZonePtr rhs)
{
  return !(lhs == rhs);
}

bool operator==(IasRoutingZoneVector& lhs, IasRoutingZoneVector& rhs)
{
  if(lhs.size() != rhs.size())
  {
    return false;
  }

  else
  {
    for(auto l = std::begin(lhs), r = std::begin(rhs), e = std::end(lhs); l != e; ++l, ++r)
    {
      if(l != r)
      {
        return false;
      }
    }
    return true;
  }
}

bool operator!=(IasRoutingZoneVector& lhs, IasRoutingZoneVector& rhs)
{
  return !(lhs == rhs);
}

bool operator==(IasRoutingZonesComparer lhs, IasRoutingZonesComparer rhs)
{
  if(lhs.getRoutingZones() != rhs.getRoutingZones() ) return false;
  return true;
}

bool operator!=(IasRoutingZonesComparer lhs, IasRoutingZonesComparer rhs)
{
  return !(lhs == rhs);
}

} /* namespace IasAudio */

