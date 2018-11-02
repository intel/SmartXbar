/*
 * IasConfigParserExtractorTest.hpp
 *
 *
 */


#ifndef AUDIO_DAEMON2_PRIVATE_TST_DEBUGFASCADETEST_INC_SMARTXDEBUGFACADETEST_HPP_
#define AUDIO_DAEMON2_PRIVATE_TST_DEBUGFASCADETEST_INC_SMARTXDEBUGFACADETEST_HPP_

#include <gtest/gtest.h>
#include "audio/configparser/IasSmartXDebugFacade.hpp"
#include "avbaudiomodules/internal/audio/common/IasAudioLogging.hpp"


namespace IasAudio {

class SmartXDebugFacadeTest : public ::testing::Test
{
    std::vector<Ias::String>   validXmlFiles;
    std::vector<Ias::String> getFileList(const std::string& path);

protected:

    void SetUp() override;
    void TearDown() override;

    std::vector<Ias::String> getValidXmlFiles();

    class WrapperSmartX
    {
      IasAudio::IasSmartX* mSmartx;
    public:

      WrapperSmartX();
      ~WrapperSmartX();

      IasSmartX* getSmartX();
    };


//    IasISetup::IasResult createAudioSinkDevice(IasISetup *setup, const IasAudioDeviceParams& params, Ias::Int32 id,
//                                               IasAudioSinkDevicePtr* audioSinkDevice,
//                                               IasRoutingZonePtr* routingZone);

};


} // namespace IasAudio


#endif /* AUDIO_DAEMON2_PRIVATE_TST_DEBUGFASCADETEST_INC_SMARTXDEBUGFACADETEST_HPP_ */
