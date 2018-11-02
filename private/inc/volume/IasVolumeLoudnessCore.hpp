/*
  * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file IasVolumeLoudnessCore.hpp
 * @date 2012
 * @brief the cor of the volume component
 */

#ifndef IASVOLUMELOUDNESS_HPP_
#define IASVOLUMELOUDNESS_HPP_

#include "helper/IasRamp.hpp"
#include "audio/smartx/rtprocessingfwx/IasAudioStream.hpp"
#include "audio/smartx/rtprocessingfwx/IasGenericAudioCompCore.hpp"
#include "avbaudiomodules/audio/common/IasAudioCommonTypes.hpp"

// disable conversion warnings for tbb
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#include "tbb/tbb.h"
// turn the warnings back on
#pragma GCC diagnostic pop

namespace IasAudio{

const uint32_t cIasAudioLoudnessTableDefaultLength = 10;
const uint32_t cIasAudioMaxSDVTableLength = 40;

const uint32_t cMinLoudnessFilterFreq = 10;
const uint32_t cMaxLoudnessFilterFreq = 20000;

const float cMinLoudnessFilterQual = 0.01f;
const float cMaxLoudnessFilterQual = 100.0f;

const uint32_t cMaxLoudnessFilterOrder = 2;
const uint32_t cMinLoudnessFilterOrder = 1;

const int32_t cMinRampTime = 1;
const int32_t cMaxRampTime = 10000;

const std::list<IasAudioFilterTypes> cLoudnessFilterAllowedTypes = {eIasFilterTypePeak,eIasFilterTypeLowShelving,eIasFilterTypeHighShelving};

class IasVolumeCmdInterface;
class IasVolumeLoudnessCore;
class IasAudioFilter;
struct IasAudioFilterParams;

/**
 *  @brief Struct defining a data point of the loudness table.
 *
 *  The loudness table defines the loudness gain as a function of the volume.
 *  Any intermediate values (which do not ly exactly on one of the data points
 *  are determined by linear interpolation.
 *
 *  The data points within the table have to be sorted in descending order of the
 *  volume.
 */
struct IasVolumeLoudnessTable
{
    std::vector<int32_t>  gains;          //!< Gain of the Loudness filter, expressed linear
    std::vector<int32_t>  volumes;        //!< Volume, expressed linear
};

/**
 * @brief struct defining the table for speed dependent volume
 *
 * this table contains the additional volume gains ( for increasing and decreasing speed)
 * as a function of the current speed. Any intermediate values are determined by linear interpolation
 */
struct IasVolumeSDVTable
{
    std::vector<uint32_t> speed;     //!< the speed values in km/h
    std::vector<uint32_t> gain_inc;  //!< the gain values for increasing speed
    std::vector<uint32_t> gain_dec;  //!< the gain values for decreasing speed
};

/**
 *  @brief Struct defining an element of the queue for setVolume commands
 */
struct IasVolumeQueueEntry
{
  float         volume;
  uint32_t      rampTime;
  IasRampShapes rampShape;
  int32_t       streamId;
};

/**
 *  @brief Struct defining an element of the queue for setMuteState commands
 */
struct IasMuteQueueEntry
{
  bool          mute;
  uint32_t      rampTime;
  IasRampShapes rampShape;
  int32_t       streamId;
};

/**
 *  @brief Struct defining an element of the queue for loudnessState commands
 */
struct IasLoudnessStateQueueEntry
{
  bool    loudnessActive;
  int32_t streamId;
};

/**
 *  @brief Struct defining an element of the queue for sdvState commands
 */
struct IasSDVStateQueueEntry
{
  bool    sdvActive;
  int32_t streamId;
};

/**
 *  @brief Struct defining an element of the queue for setLoudnessTable commands
 */
struct IasLoudnessTableQueueEntry
{
  uint32_t band;
  IasVolumeLoudnessTable table;
};

struct IasLoudnessFilterQueueEntry
{
  uint32_t band;
  IasAudioFilterConfigParams params;
};

/**
 *  @brief Enum datatype for specifying the ramp shape (linear vs. exponential).
 */
enum IasVolumeLoudnessCoreRampShape
{
  eIasVolumeLoudnessCoreRampShapeLinear,      //!< Linear ramp shape
  eIasVolumeLoudnessCoreRampShapeExponential, //!< Exponential ramp shape
};


/**
 *  @brief Enum datatype for specifying the filter type.
 */
enum IasVolumeLoudnessCoreFilterType
{
  eIasVolumeLoudnessCoreFilterFlat,      //!< Flat frequency response (neutral filter)
  eIasVolumeLoudnessCoreFilterPeak,      //!< Peak filter
  eIasVolumeLoudnessCoreFilterLowShelf,  //!< Low-frequency shelving filter
  eIasVolumeLoudnessCoreFilterHighShelf, //!< High-frequency shelving filter
  eIasVolumeLoudnessCoreFilterBandPass,  //!< Band-pass filter
  eIasVolumeLoudnessCoreFilterLowPass,   //!< Low-pass filter
  eIasVolumeLoudnessCoreFilterHighPass   //!< High-pass filter
};


/**
 *  @brief Struct defining the parameters of a (recursive) filter.
 */
struct IasVolumeFilterParams
{
  IasAudio::IasVolumeLoudnessCoreFilterType  mFilterType; //!< filter type (enum datatype)
  float                                      mFrequency;  //!< center or cut-off frequency
  float                                      mQuality;    //!< quality (Q value) for band-pass and peak filters
  uint32_t                                   mOrder;      //!< filter order
};


struct IasVolumeRampParams
{
  uint32_t bundleIndex;
  uint32_t channelIndex;
  uint32_t nChannels;
  IasRamp* rampVol;
  IasRamp* rampSDV;
  IasRamp* rampMute;
};

/**
 * @brief enum defining the reason why a volume ramp is triggered
 * the reason for a volume ramp can be a volume command or a mute/unmute command
 * Depending on the reason different actions have you to be taken
 */
enum IasVolumeRampReason
{
  eIasRampReasonVolume = 0,
  eIasRampReasonMute,
  eIasRampReasonNotSet = 0xFF
};

/**
 * @brief structure for the handling of the fade ramps
 */
struct IasVolumeParams
{
  float                         currentVolume;
  float                         muteVolume;
  float                         destinationVolume;
  float                         currentSDVGain;
  float                         currentMuteGain;
  uint32_t                      numSamplesLeftToRamp;
  uint32_t                      numSDVSamplesLeftToRamp;
  uint32_t                      numMuteSamplesLeftToRamp;
  bool                          isMuted;
  bool                          volRampActive;
  bool                          sdvRampActive;
  bool                          muteRampActive;
  bool                          isSDVon;
  bool                          lastRunVol;
  bool                          lastRunSDV;
  bool                          lastRunMute;
  bool                          activeLoudness;
  std::vector<IasVolumeRampParams>    mVolumeRampParamsVector;
  std::vector<IasAudioFilter**> filters;
};

/**
 * @brief structure to mark the callbacks for the command interface
 */
struct IasVolumeCallbackParams
{
  int32_t streamId;
  float volume;
  bool    markerVolume;
  bool    loudnessState;
  bool    markerLoudness;
  bool    muteState;
  bool    markerMute;
  bool    markerSDV;
  bool    stateSDV;
};

/**
 * @brief general type defintions for map or list or vector types
 */
using IasVolumeCallbackMap = std::map<int32_t,IasVolumeCallbackParams>;
using IasVolumeRampMap = std::map<int32_t,IasVolumeParams>;
using IasVolumeActiveRampMap = std::map<uint32_t,bool>;
using IasVolumeStreamToFilter = std::map<uint32_t, IasVolumeFilterParams*>;
using IasBundleIndexMap = std::map<IasAudioChannelBundle*,uint32_t>;
using IasBundleIndexPair = std::pair< IasAudioChannelBundle*,uint32_t >;
using IasFilterMap = std::map<uint32_t,IasAudioFilter*>;
using IasFilterPair = std::pair< uint32_t, IasAudioFilter*>;
using IasVolumeFilterVector = std::vector<IasAudioFilter**>;

/** \class IasVolumeLoudnessCore
 *  This class is the core of the volume component
 */
class IAS_AUDIO_PUBLIC IasVolumeLoudnessCore : public IasGenericAudioCompCore
{
  public:
    /**
     * @brief Constructor.
     *
     * @param config pointer to the component configuration.
     * @param componentName unique component name.
     */
    IasVolumeLoudnessCore(const IasIGenericAudioCompConfig *config, const std::string &componentName);

    /*!
     *  @brief Destructor, virtual by default.
     */
    virtual ~IasVolumeLoudnessCore();

    /*!
     *  @brief Set volume.
     *
     *  @param[in] sinkId    Index of the sink whose volume shall be set.
     *  @param[in] volume    Volume (expressed in dB).
     *  @param[in] rampTime  Length of the linear ramp or time constant of the exponential ramp.
     *  @param[in] rampShape Ramp shape: linear or exponential.
     *  @return    error     error code
     *  @retval    eIasAudioProcOK everything ok
     *  @retval    eIasAudioProcInvalidParam  an input paramter was not correct
     */
    IasAudioProcessingResult setVolume(int32_t   Id,
                                       float volume,
                                       int32_t   rampTime,
                                       IasAudio::IasRampShapes rampShape);

    /*!
     *  @brief Set mute state.
     *
     *  @param[in] sinkId   Index of the sink whose mute state shall be set.
     *  @param[in] isMuted  Indicates whether the sink shall be muted (if true) or unmuted (if false).
     *  @return    error    error code
     *  @retval    eIasAudioProcOK everything ok
     *  @retval    eIasAudioProcInvalidParam  an input paramter was not correct
     */
    IasAudioProcessingResult setMuteState(int32_t Id,
                                          bool  isMuted,
                                          uint32_t rampTime,
                                          IasAudio::IasRampShapes rampShape);


    /*!
     *  @brief Set loudness function on or off.
     *
     *  @param[in] sinkId  Index of the sink whose loudness function shall be set.
     *  @param[in] isOn    Indicates whether the loudness function shall be on or off.
     *  @return    error    error code
     *  @retval    eIasAudioProcOK everything ok
     *  @retval    eIasAudioProcInvalidParam  the input stream id was not ok
     */
    IasAudioProcessingResult setLoudnessOnOff(int32_t Id,
                                              bool  isOn);

    /*!
     *  @brief Set loudness filter parameters.
     *
     *  @param[in] streamId Index of the sink whose loudness filter parameters shall be set.
     *  @param[in] band     Index of the filter stage whose parameters shall be set.
     *  @param[in] params   Pointer to the struct of filter params that shall be adopted.
     *  @return    error    The current implementation only returns eIasAudioProcOK
     */
    IasAudioProcessingResult setLoudnessFilter( int32_t streamId,
                                                uint32_t band,
                                                IasAudioFilterParams *params);


    /*!
     *  @brief The function sets all loudness filters one defined loudness band
     *
     *  @param[in] band         the filter band
     *  @param[in] filterParams the pointer to the filter paramaters
     *  @param[in] directInit   Directly initialize and don't send command via queue
     *  @return error the error value
     *  @retval eIasAudioProcOK everything ok
     *  @retval eIasAudioProcInitializationFailed setting for one stream did fail
     */
    IasAudioProcessingResult setLoudnessFilterAllStreams(uint32_t band,
                                                         IasAudio::IasAudioFilterConfigParams *filterParams,
                                                         bool directInit = false);

    /*!
     *  @brief Set loudness table.
     *
     *  The specified loudness table is used for all sinks and for the stage that is
     *  defined by the parameter band (e.g. 0 for loudness-low or 1 for loudness-high).
     *
     *  @param[in] band           Index of the filter stage whose table shall be set.
     *  @param[in] loudnessTable  Vector containing the loudness table.
     *  @param[in] directInit     Directly initialize and don't send command via queue
     *  @return error             error code
     *  @retval eIasAudioProcOK   everything ok
     *  @retval eIasAudioProcInvalidParam an input parameter was not correct
     *
     */
    IasAudioProcessingResult setLoudnessTable(uint32_t band,
                                              IasVolumeLoudnessTable *loudnessTable,
                                              bool directInit = false);
    /*!
     *  @brief The function return the setting of the loudness filter for one defined loudness band
     *
     *  @param band  the filter band
     *  @param filterParams the pointer to the filter paramaters
     *  @return error the error value
     *  @retval eIasAudioProcOK everything ok
     *  @retval eIasAudioProcInvalidParams a paramter was not correct
     */
    IasAudioProcessingResult getLoudnessFilterAllStreams(uint32_t band,
                                                         IasAudio::IasAudioFilterConfigParams &filterParams);
    /*!
     *  @brief Get loudness table.
     *
     *  The specified loudness table is used for all sinks and for the stage that is
     *  defined by the parameter band (e.g. 0 for loudness-low or 1 for loudness-high).
     *
     *  @param[in] band The band for which the loudness table shall be retrieved
     *  @param[out] loudnessTable  Reference to location to store the loudnessTable
     *  @return         error           error code
     *  @retval         eIasAudioProcOK everything ok
     *  @retval         eIasAudioProcInvalidParam an input parameter was not correct
     *
     */
    IasAudioProcessingResult getLoudnessTable(uint32_t band, IasVolumeLoudnessTable &loudnessTable);

    /*!
     *  @brief Get sdv table.
     *
     *  The specified sdv table is unique in the module and used for all streams.
     *
     *  @param[out] sdvTable Reference to location to store sdvTable
     *  @return         error           error code
     *  @retval         eIasAudioProcOK everything ok
     *  @retval         eIasAudioProcInvalidParam an input parameter was not correct
     *
     */
    IasAudioProcessingResult getSDVTable(IasVolumeSDVTable &sdvTable);


    /*!
     *  @brief set speed controlled volume mode
     *
     *  This function sets the mode for the speed controlled volume.
     *
     *  @param[in] sinkId the sink for whicj speed controlled volume should be adapted
     *  @param[in] mode   the mode of speed controlled volume that shall be applied
     *  @return    error  the error code
     *  @retval    eIasAudioProcOK everything ok
     */
    IasAudioProcessingResult setSpeedControlledVolume(int32_t sinkId, bool mode);

    /*!
     *  @brief set a new speed value
     *
     *  This function inform the volume module about a new speed value
     *
     *  @param[in] speed  the new speed value in km/h
     */
    void setSpeed(uint32_t speed);

    /*!
     *  @brief set the SDV gain table
     *
     *  This function sets the new SDV gain table
     *
     *  @param[in] table          pointer to a structure containing the speed and gain values
     *  @param[in] directInit     Directly initialize and don't send command via queue
     *  @return    error  the error code
     *  @retval    eIasAudioProcOK everything ok
     */
    IasAudioProcessingResult setSDVTable(IasVolumeSDVTable* table, bool directInit = false);

    /*!
     * @brief function to update min volume
     *
     * @param[in] minVol minimum volume
     *
     */
    void updateMinVol(uint32_t minVol);

    /*!
     * @brief function to update max volume
     *
     * @param[in] maxVol maximum volume
     *
     */
    void updateMaxVol(uint32_t maxVol);

    /*!
     * @brief function to get min volume
     *
     * @param[in] minVol minimum volume
     *
     */
    int32_t getMinVol();

    /*!
     * @brief function to get max volume
     *
     * @param[in] maxVol minimum volume
     *
     */
    int32_t getMaxVol();

    /*!
     *  @brief Reset method.
     *
     *  Reset the internal component states.
     *  Derived from base class.
     */
    virtual IasAudioProcessingResult reset();

    /*!
     *  @brief Initialize method.
     *
     *  Pass audio component specific initialization parameter.
     *  Derived from base class.
     */
    virtual IasAudioProcessingResult init();

  private:

    typedef enum{
      eSpeedInc_HysOff = 0,
      eSpeedDec_HysOff,
      eSpeedInc_HysOn,
      eSpeedDec_HysOn
    }IasVolumeSDV_Hysteresis;


    /*!
     *  @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasVolumeLoudnessCore(IasVolumeLoudnessCore const &other); //lint !e1704

    /*!
     *  @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasVolumeLoudnessCore& operator=(IasVolumeLoudnessCore const &other); //lint !e1704

    /*!
     *  @brief Process method.
     *
     *  This method is periodically called by the AudioServer Core.
     *  Derived from base class.
     */
    virtual IasAudioProcessingResult processChild();

    /*!
     * @brief internal function that updates the loudness filter of a current stream due to a volume change
     *
     * @param streamId the stream idnetifier
     * @param volume the current volume value
     */
    void updateLoudness(int32_t streamId, float volume);

    /*!
     * @brief internal function to read the current loudness gain from a loudness table that fits to input parameter
     *        volume
     * @param volume the volume as linear value
     * @param table the table from where the loudness gain should be read
     * @return gain the loudness gain as linear value
     */
    float getLoudnessGain(float volume, IasVolumeLoudnessTable* table);

    /*!
     * @brief internal function to read the current sdv gain from the table that fits to input parameter
     *        speed
     * @param speed the speed as linear value in km/h
     * @return gain the sdv gain as linear value
     */
    float getSDV_Gain(uint32_t speed,std::vector<uint32_t> *gainVec);

    /*!
     * @brief internally used to calculate new sdv gain
     *
     * @params[in] speed the new speed value
     * @params[in] params struct containg the important parameters
     */
    float calculateSDV_Gain(uint32_t speed,IasVolumeParams *params);

    /*!
     * @brief function to trigger the new mute ramp
     *
     * @param[in] streamId the id of the stream
     * @param[in] mute the mute state
     * @param[in] rampTime the length of the ramp
     * @param[in] rampShape the shape of the ramp
     *
     */
    void updateMute(int32_t streamId, bool mute, uint32_t rampTime, IasRampShapes rampShape);

    /*!
     * @brief function to trigger the new volume ramp
     *
     * @param[in] streamId the id of the stream
     * @param[in] volume the new desired volume
     * @param[in] rampTime the length of the ramp
     * @param[in] rampShape the shape of the ramp
     *
     */
    void updateVolume( int32_t streamId, float volume, uint32_t rampTime, IasRampShapes rampShape);

    /*!
     * @brief function to trigger the new loudness state
     *
     * @param[in] streamId the id of the stream
     * @param[in] active the new loudness state
     */
    void updateLoudnessParams(int32_t streamId, bool active);

    /*!
     * @brief function to trigger the new sdv state
     *
     * @param[in] streamId the id of the stream
     * @param[in] active the new sdv state
     */
    void updateSDV_State(int32_t streamId, bool active);

    /*!
     * @brief function to update sdv for new speed
     *
     * @param[in] speed the new speed value
     */
    void updateSpeed(uint32_t speed);

    /*!
     * @brief function to update sdv table
     *
     * @param[in] table the new sdv value
     */
    void updateSDVTable(IasVolumeSDVTable table);

    /*!
     * @brief function to update loudness table
     *
     * @param[in] streamId the id of the stream
     * @param[in] table the new loudness value
     *
     */
    void updateLoudnessTable(uint32_t band, IasVolumeLoudnessTable table);

    /*!
     * @brief function to update loudness filter
     *
     * @param[in] band the filter band
     * @param[in] params the new filter params for this and
     *
     */
    void updateLoudnessFilter(uint32_t band, IasAudioFilterConfigParams params);

    /*!
     * @brief function to check if there are entries in the command queues
     */
    void checkQueues();

    /**
     * @brief this function checks if a stream is filtered for the filter band
     *
     * @param[in] streamId  the id of the stream
     * @param[in] band      the filter band
     *
     * @return         bool value
     * @retval true    stream is filtered
     * @retvla falses  tream is not filtered
     */
    bool checkStreamActiveForBand(int32_t streamId,uint32_t band) const;

    /**
     * @breif this function marks a stream to be loudness filtered in a defined filter band
     *
     * @param[in] band the filter band which should ba applied for the stream
     * @param[in] streamId the stream id to be filtered
     *
     * @return                            error code
     * @retval eIasAudioProcOK            success
     * @retval eIasAudioProcInvalidParam  null pointer passed as input
     * @retval eIasAudioProcInvalidStream stream not found in module list
     */
    IasAudioProcessingResult setStreamActiveForFilterband(uint32_t band, int32_t streamId);

    IasAudioProcessingResult initFromConfiguration();
    void getStreamActiveForFilterBand(uint32_t band, IasInt32Vector &streamIds);
    void sendVolumeEvent(int32_t streamId, float currentVolume);
    void sendMuteStateEvent(int32_t streamId, bool muteState);
    void sendLoudnessStateEvent(int32_t streamId, bool loudnessState);
    void sendSpeedControlledVolumeEvent(int32_t streamId, bool sdvState);


    ////////////////////
    // Member variables
    ////////////////////
    /**
     * @brief multimap for assignments of filter bands to the streams.key is filter band, value is streamId
     */
    using IasVolumeFilterAssignmentMap = std::multimap<uint32_t, int32_t>;

    int32_t                                            mMinVol;                 //!< min volume
    int32_t                                            mMaxVol;                 //!< max volume
    uint32_t                                           mNumStreams;             //!< number of streams to be processed
    uint32_t                                           mNumFilterBands;         //!< Number of filter bands
    uint32_t                                           mNumFilterBandsMax;      //!< max number of filter bands supported
    uint32_t                                           mLoudnessTableLengthMax; //!< max. supported length for loudness table
    std::size_t                                        mSDVTableLength;         //!< length of sdv table
    uint32_t                                           mSDVTableLengthMax;      //!< max. supported length for sdv table
    uint32_t                                           mNumBundles;             //!< number of bundles
    uint32_t                                           mCurrentSpeed;           //!< current speed value
    uint32_t                                           mNewSpeed;               //!< new speed value to process
    bool                                               mSpeedChanged;           //!< flag to indicate new sdv state
    IasVolumeSDV_Hysteresis                            mSDV_HysteresisState;    //!< state of sdv hysteresis
    IasAudioFilter                                   **mFilters;                //!< pointer to the memory with the pointers to the filter objects
    IasVolumeFilterVector                              mBandFilters;            //!< vector with pointers to the addresses of filter objects belonging to the same filter band
    IasFilterMap                                       mFilterMap;              //!< map containing filter object pointers
    uint32_t                                           mNumFilters;             //!< the number of filters, needed for allocating memory
    IasVolumeRampMap                                   mVolumeParamsMap;        //!< map containing all ramping parameters
    std::vector<float*>                                mGains;                  //!< bundle to store the ramp values for the volumes
    std::vector<float*>                                mGainsSDV;               //!< bundle to store the ramp values for the sdv
    std::vector<float*>                                mGainsMute;              //!< bundle to store the ramp values for the mute
    IasAudioStreamVector                               mStreams;                //!< vector carrying the pointers to the announced streams;
    IasBundleIndexMap                                  mBundleIndexMap;         //!< map used for addressing correct bundle
    std::vector<IasVolumeLoudnessTable>                mLoudnessTableVector;    //!< the loudness tables for the different filter bands
    IasVolumeCallbackMap                               mCallbackMap;            //!< parameter map used for sending ufipc events
    IasVolumeSDVTable                                  mSDVTable;               //!< the sdv table structure
    float                                              mMaxVolume;              //!< maximum user volume currently set
    float                                              mVolumeSDV_Critical;     //!< user volume, where sdv gain MUST be 1.0 to avoid overflow
    std::vector<IasAudioFilterConfigParams>            mLoudnessFilterParams;   //!< vector carrying the filter parameters for the loudness bands
    tbb::concurrent_queue<IasVolumeQueueEntry>         mVolumeQueue;            //!< queue for volume commands>
    tbb::concurrent_queue<IasMuteQueueEntry>           mMuteQueue;              //!< queue for mute commands>
    tbb::concurrent_queue<IasLoudnessStateQueueEntry>  mLoudnessStateQueue;     //!< queue for loudness on/off commands>
    tbb::concurrent_queue<uint32_t>                    mSpeedQueue;             //!< queue for speed commands>
    tbb::concurrent_queue<IasSDVStateQueueEntry>       mSDVStateQueue;          //!< queue for sdv state change commands>
    tbb::concurrent_queue<IasVolumeSDVTable>           mSDVTableQueue;          //!< queue for sdv table commands>
    tbb::concurrent_queue<IasLoudnessTableQueueEntry>  mLoudnessTableQueue;     //!< queue for loudness table commands>
    tbb::concurrent_queue<IasLoudnessFilterQueueEntry> mLoudnessFilterQueue;    //!< queue for loudness filter commands>
    std::string                                        mTypeName;               //!< Name of the module type
    std::string                                        mInstanceName;           //!< Name of the module instance
    DltContext                                        *mLogContext;             //!< The log context for the volume component.
    IasVolumeFilterAssignmentMap                       mStreamFilterMap;        //!< the map that relates the streams to the filter bands

};

} //namespace IasAudio

#endif /* IASVOLUMELOUDNESS_HPP_ */
