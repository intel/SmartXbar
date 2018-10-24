/*
 * Ports.hpp
 *
 *  Created on: Jun 7, 2017
 *      Author: mkielanx
 */

#ifndef AUDIO_DAEMON2_PRIVATE_TST_CONFIGPARSERTEST_INC_IASPORTSCOMPARER_HPP_
#define AUDIO_DAEMON2_PRIVATE_TST_CONFIGPARSERTEST_INC_IASPORTSCOMPARER_HPP_


#include "audio/smartx/IasISetup.hpp"

namespace IasAudio {



class IasPortsComparer
{
  IasAudioPortVector input;
  IasAudioPortVector output;

public:

  IasPortsComparer(IasISetup* setup);

  IasAudioPortVector getInputPorts() const;
  IasAudioPortVector getOutputPorts() const;

  friend bool operator==(IasAudioPortParams lhs, IasAudioPortParams rhs);
  friend bool operator!=(IasAudioPortParams lhs, IasAudioPortParams rhs);

  friend bool operator==(IasAudioPortVector lhs, IasAudioPortVector rhs);
  friend bool operator!=(IasAudioPortVector lhs, IasAudioPortVector rhs);

  friend bool operator==(IasPortsComparer lhs, IasPortsComparer rhs);
  friend bool operator!=(IasPortsComparer lhs, IasPortsComparer rhs);
};

}  /* namespace IasAudio */


#endif /* AUDIO_DAEMON2_PRIVATE_TST_CONFIGPARSERTEST_INC_IASPORTSCOMPARER_HPP_ */
