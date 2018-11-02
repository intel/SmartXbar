/*
 * Ports.cpp
 *
 *  Created on: Jun 7, 2017
 *      Author: mkielanx
 */


#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlmemory.h>
#include <gtest/gtest.h>
#include <dlt/dlt.h>
#include <IasPortsComparer.hpp>

#include "avbaudiomodules/internal/audio/common/IasAudioLogging.hpp"
#include "audio/configparser/IasSmartXconfigParser.hpp"
#include "configparser/IasParseHelper.hpp"
#include "audio/smartx/IasSmartX.hpp"
#include "audio/smartx/IasIRouting.hpp"
#include "audio/smartx/IasISetup.hpp"
#include "audio/smartx/IasIDebug.hpp"
#include "audio/smartx/IasIProcessing.hpp"
#include "audio/smartx/IasSetupHelper.hpp"
#include "audio/smartx/IasEventHandler.hpp"
#include "audio/smartx/IasEvent.hpp"
#include "audio/smartx/IasConnectionEvent.hpp"
#include "audio/smartx/IasSetupEvent.hpp"
#include "model/IasAudioPort.hpp"
#include "core_libraries/foundation/IasTypes.hpp"
#include "core_libraries/foundation/IasDebug.hpp"
#include "boost/filesystem.hpp"

namespace IasAudio {

IasPortsComparer::IasPortsComparer(IasISetup* setup) :
  input{setup->getAudioInputPorts()},
  output{setup->getAudioOutputPorts()}
{

}

IasAudioPortVector IasPortsComparer::getInputPorts() const
{
  return input;
}

IasAudioPortVector IasPortsComparer::getOutputPorts() const
{
  return output;
}

bool operator==(IasAudioPortParams lhs, IasAudioPortParams rhs)
{
  if(lhs.direction != rhs.direction) return false;
  if(lhs.id != rhs.id) return false;     //  should be compare ???
  if(lhs.index != rhs.index) return false;    // ???
  if(lhs.name != rhs.name) return false;
  if(lhs.numChannels != rhs.numChannels) return false;
  return true;
}

bool operator!=(IasAudioPortParams lhs, IasAudioPortParams rhs)
{
  return !(lhs == rhs);
}

bool operator==(IasAudioPortVector lhs, IasAudioPortVector rhs)
{
  if(lhs.size() != rhs.size()) { return false; }

  else
  {
    for(auto l = std::begin(lhs), r = std::begin(rhs), e = std::end(lhs); l != e; ++l, ++r)
    {
      const auto lparamPtr = l->get()->getParameters();
      const auto rparamPtr = r->get()->getParameters();

      const IasAudioPortParams lparam = *lparamPtr.get();
      const IasAudioPortParams rparam = *rparamPtr.get();

      if(lparam != rparam)
      {
        return false;
      }
    }
    return true;
  }
}

bool operator!=(IasAudioPortVector& lhs, IasAudioPortVector& rhs)
{
  return !(lhs == rhs);
}

bool operator==(IasPortsComparer lhs, IasPortsComparer rhs)
{
  if(lhs.getInputPorts() != rhs.getInputPorts() ) return false;
  if(lhs.getOutputPorts() != rhs.getOutputPorts() ) return false;
  return true;
}

bool operator!=(IasPortsComparer lhs, IasPortsComparer rhs)
{
  return !(lhs == rhs);
}

} /* namespace IasAudio */
