/*
 * Devices.hpp
 *
 *  Created on: Jun 7, 2017
 *      Author: mkielanx
 */

#ifndef AUDIO_DAEMON2_PRIVATE_TST_CONFIGPARSERTEST_INC_IASAUDIODEVICESCOMPARER_HPP_
#define AUDIO_DAEMON2_PRIVATE_TST_CONFIGPARSERTEST_INC_IASAUDIODEVICESCOMPARER_HPP_

#include "audio/smartx/IasISetup.hpp"

namespace IasAudio {

class IasAudioDevicesComparer
{
  IasAudioSinkDeviceVector   sink;
  IasAudioSourceDeviceVector source;

public:

  IasAudioDevicesComparer(IasISetup* setup);

  IasAudioSinkDeviceVector getSinkDevices() const;
  IasAudioSourceDeviceVector getSourceDevices() const;

  friend bool operator==(IasAudioSinkDeviceVector lhs, IasAudioSinkDeviceVector rhs);
  friend bool operator!=(IasAudioSinkDeviceVector lhs, IasAudioSinkDeviceVector rhs);

  friend bool operator==(IasAudioSourceDeviceVector lhs, IasAudioSourceDeviceVector rhs);
  friend bool operator!=(IasAudioSourceDeviceVector lhs, IasAudioSourceDeviceVector rhs);

  friend bool operator==(IasAudioDevicesComparer lhs, IasAudioDevicesComparer rhs);
  friend bool operator!=(IasAudioDevicesComparer lhs, IasAudioDevicesComparer rhs);
};

} /* namespace IasAudio */

#endif /* AUDIO_DAEMON2_PRIVATE_TST_CONFIGPARSERTEST_INC_IASAUDIODEVICESCOMPARER_HPP_ */
