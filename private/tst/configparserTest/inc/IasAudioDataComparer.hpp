/*
 * Params.hpp
 *
 *  Created on: Jun 7, 2017
 *      Author: mkielanx
 */

#ifndef AUDIO_DAEMON2_PRIVATE_TST_CONFIGPARSERTEST_INC_IASAUDIODATACOMPARER_HPP_
#define AUDIO_DAEMON2_PRIVATE_TST_CONFIGPARSERTEST_INC_IASAUDIODATACOMPARER_HPP_

#include <IasAudioDevicesComparer.hpp>
#include <IasPortsComparer.hpp>
#include <IasRoutingZonesComparer.hpp>
#include "audio/smartx/IasSmartX.hpp"

namespace IasAudio {


class IasAudioDataComparer
{
  IasPortsComparer        ports;
  IasAudioDevicesComparer devices;
  IasRoutingZonesComparer rzn;

public:

  IasAudioDataComparer(IasSmartX* smartx);

  IasPortsComparer getPortsComparer() const;
  IasAudioDevicesComparer getDevicesComparer() const;
  IasRoutingZonesComparer getRoutingZonesComparer() const;

  friend bool operator==(IasAudioDataComparer lhs, IasAudioDataComparer rhs);
  friend bool operator!=(IasAudioDataComparer lhs, IasAudioDataComparer rhs);
};


}

#endif /* AUDIO_DAEMON2_PRIVATE_TST_CONFIGPARSERTEST_INC_IASAUDIODATACOMPARER_HPP_ */
