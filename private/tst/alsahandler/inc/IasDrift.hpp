/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file  IasDrift.hpp
 * @brief Simulation of time-dependent drifts.
 */

#ifndef IASDRIFT_HPP
#define IASDRIFT_HPP


/*!
 * @brief namespace Ias
 */
namespace IasAudio {

/*!
 * @brief Documentation for class IasDrift
 */
class IasDrift
{
  public:
    /**
     * @brief  Result type of the class IasAlsaHandler.
     */
    enum IasResult
    {
      eIasOk,        //!< Ok, Operation successful
      eIasFileError  //!< Error while opening the file with the drift profile.
    };

    /**
     *  @brief Constructor.
     */
    IasDrift();

    /**
     *  @brief Destructor, virtual by default.
     */
    virtual ~IasDrift();

    /*!
     *  @brief Initialization function.
     *
     *  @returns error code
     *  @retval  IasDrift::IasResult::eIasOk         On success
     *  @retval  IasDrift::IasResult::eIasFileError  Error while opening text file with drift profile.
     *
     *  @param[in] driftFileName  Name of the text file with the drift specification.
     */
    IasResult init(const std::string &driftFileName);

    /**
     *  @brief  Update and get the current drift value.
     *
     *  Based on the current time, which must be provided as an input parameter, the
     *  function calculates the current drift. This is based on the drift and on the
     *  gradient specified in the driftFileName.
     *
     *  @param[in]  time  The current time (since the begin of the simulation) [ms].
     *                    Between two calls of this function, the new time value must not
     *                    be smaller than the last one. If the simulation shall restart
     *                    at time=0, you have to call rewind().
     *  @param[out] drift The current drift [ppm]
     */
    void updateCurrentDrift(uint32_t  time,
                            int32_t  *drift);

    /**
     *  @brief  Get the current drift value.
     *
     *  This function provides the current drift value, which is the drift value that
     *  has been calculated during the last call of the method IasDrift::updateCurrentDrift().
     *  This function does not any internal states of the component.
     */
    void getCurrentDrift(int32_t  *drift) const;

    /**
     *  @brief  Check if there is another element in the DriftSpecVec.
     *
     *  @returns  true, if there is another element in the DriftSpecVec.
     */
    bool furtherDriftAvail() const;

    /**!
     *  @brief Rewind the drift simulation, so that it restarts at time=0.
     */
    void rewind();


  private:
    /*!
     *  @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasDrift(IasDrift const &other);

    /*!
     *  @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasDrift& operator=(IasDrift const &other);

    /**
     *  @brief drift simulation parameters.
     *
     *  Structure defining the drift simulation parameters.
     */
    typedef struct
    {
      uint32_t  timestamp;    //!< time [ms], at which the new drift & gradient become active
      int32_t   drift;        //!< the new drift value [ppm]
      int32_t   gradient;     //!< the new drift gradient [ppm per sec]
    } IasDriftSpecElement;

    std::vector<IasDriftSpecElement> mDriftSpecVec; //!< Vector holding all drift specifications
    uint32_t mCurrentElement;                          //!< Current element index within mDriftSpecVec
    int32_t  mCurrentDrift;
};


inline void IasDrift::getCurrentDrift(int32_t  *drift) const
{
  *drift = mCurrentDrift;
}


inline bool IasDrift::furtherDriftAvail() const
{
  // Return true, if there is another element in the DriftSpecVec.
  return (mCurrentElement+1 < mDriftSpecVec.size());
}


} // namespace Ias

#endif // IASDRIFT_HPP
