
#include <IasAudioDevicesComparer.hpp>
#include "model/IasAudioSinkDevice.hpp"
#include "model/IasAudioSourceDevice.hpp"
#include <iostream>

namespace IasAudio {

IasAudioDevicesComparer::IasAudioDevicesComparer(IasISetup* setup) :
    sink{setup->getAudioSinkDevices()},
    source{setup->getAudioSourceDevices()}
{
}

IasAudioSinkDeviceVector IasAudioDevicesComparer::getSinkDevices() const
{
  return sink;
}

IasAudioSourceDeviceVector IasAudioDevicesComparer::getSourceDevices() const
{
  return source;
}

bool operator==(IasAudioDeviceParams lhs, IasAudioDeviceParams rhs)
{
  if(lhs.clockType != rhs.clockType) return false;
  if(lhs.name != rhs.name) return false;
  if(lhs.numChannels != rhs.numChannels ) return false;
  if(lhs.samplerate != rhs.samplerate ) return false;
  if(lhs.dataFormat != rhs.dataFormat ) return false;
  if(lhs.clockType != rhs.clockType ) return false;
  if(lhs.periodSize != rhs.periodSize ) return false;
  if(lhs.numPeriods != rhs.numPeriods ) return false;
  if(lhs.numPeriodsAsrcBuffer  != rhs.numPeriodsAsrcBuffer ) return false;
  return true;
}

bool operator!=(IasAudioDeviceParams lhs, IasAudioDeviceParams rhs)
{
  return !(lhs == rhs);
}

bool operator==(IasAudioSinkDeviceVector lhs, IasAudioSinkDeviceVector rhs)
{
  if(lhs.size() != rhs.size())
  {
    return false;
  }

  for(auto l = std::begin(lhs), r = std::begin(rhs), e = std::end(lhs); l != e; ++l, ++r)
  {
    const auto lparamPtr = l->get()->getDeviceParams();
    const auto rparamPtr = r->get()->getDeviceParams();

    const IasAudioDeviceParams lparam = *lparamPtr.get();
    const IasAudioDeviceParams rparam = *rparamPtr.get();

    if(lparam != rparam)
    {
      return false;
    }
  }
  return true;
}

bool operator!=(IasAudioSinkDeviceVector lhs, IasAudioSinkDeviceVector rhs)
{
  return !(lhs == rhs);
}

bool operator==(IasAudioSourceDeviceVector lhs, IasAudioSourceDeviceVector rhs)
{
  if(lhs.size() != rhs.size())
  {
    return false;
  }

  for(auto l = std::begin(lhs), r = std::begin(rhs), e = std::end(lhs); l != e; ++l, ++r)
  {
    const auto lparamPtr = l->get()->getDeviceParams();
    const auto rparamPtr = r->get()->getDeviceParams();

    const IasAudioDeviceParams lparam = *lparamPtr.get();
    const IasAudioDeviceParams rparam = *lparamPtr.get();

    if(lparam != rparam)
    {
      return false;
    }
  }
  return true;
}

bool operator!=(IasAudioSourceDeviceVector lhs, IasAudioSourceDeviceVector rhs)
{
  return !(lhs == rhs);
}

bool operator==(IasAudioDevicesComparer lhs, IasAudioDevicesComparer rhs)
{
  if(lhs.getSinkDevices() != rhs.getSinkDevices() ) return false;
  if(lhs.getSourceDevices() != rhs.getSourceDevices() ) return false;
  return true;
}

bool operator!=(IasAudioDevicesComparer lhs, IasAudioDevicesComparer rhs)
{
  return !(lhs == rhs);
}


} /* namespace IasAudio */


