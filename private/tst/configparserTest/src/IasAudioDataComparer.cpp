/*
 * Params.cpp
 *
 *  Created on: Jun 7, 2017
 *      Author: mkielanx
 */


#include <IasAudioDataComparer.hpp>
#include <iostream>

namespace IasAudio {

IasAudioDataComparer::IasAudioDataComparer(IasSmartX* smartx) :
    ports{smartx->setup()},
    devices{smartx->setup()},
    rzn{smartx->setup()}
{
}

IasPortsComparer IasAudioDataComparer::getPortsComparer() const
{
  return ports;
}

IasAudioDevicesComparer IasAudioDataComparer::getDevicesComparer() const
{
  return devices;
}

IasRoutingZonesComparer IasAudioDataComparer::getRoutingZonesComparer() const
{
  return rzn;
}

bool operator==(IasAudioDataComparer lhs, IasAudioDataComparer rhs )
{
  if(lhs.getPortsComparer() != rhs.getPortsComparer()) return false;
  if(lhs.getDevicesComparer() != rhs.getDevicesComparer()) return false;
  if(lhs.getRoutingZonesComparer() != rhs.getRoutingZonesComparer()) return false;
  return true;
}

bool operator!=(IasAudioDataComparer lhs, IasAudioDataComparer rhs )
{
  return !(lhs == rhs);
}

} /* namespace IasAudio */
