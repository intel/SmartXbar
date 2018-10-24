/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file IasSmartXDebugFacade.hpp
 * @date 2017
 * @brief This is the declaration of IasSmartXDebugFacade
 */

#ifndef IASSMARTXDEBUGFACADE_HPP_
#define IASSMARTXDEBUGFACADE_HPP_

#include <dlt/dlt.h>
#include "audio/smartx/IasSmartX.hpp"
#include "audio/smartx/IasIDebug.hpp"
#include "audio/smartx/IasISetup.hpp"

/**
 * Forward declaring libxml2 needed types to avoid forwarding the dependency to clients.
 */
struct _xmlNode;
using xmlNodePtr = _xmlNode*;

namespace IasAudio {

class IAS_AUDIO_PUBLIC IasSmartXDebugFacade
{
  public:
    /**
     * @brief The result type for the IasConfigSinkDevice methods
     */
    enum IasResult
    {
      eIasOk,               //!< Operation successful
      eIasFailed            //!< Operation failed
    };

    /**
     * @brief Constructor.
     */
    IasSmartXDebugFacade(IasSmartX *&smartx);

    /**
     * @brief Destructor.
     */
    virtual ~IasSmartXDebugFacade();

    /**
     * @brief Retrieves SmartX Topology from running instance.
     *
     * @param[in] xmlTopologyString String to contain the xml topology
     * @return success or fail
     */
    IasResult getSmartxTopology(std::string &xmlTopologyString);

    /**
     * @brief Returns smartx version
     *
     * @return smartx version as string
     */
    const std::string getVersion();

    /**
     * @brief Get module parameters
     *
     * @param[in] moduleInstanceName Module name
     * @param[in] parameters Module parameters
     * @return success or fail
     */
    IasResult getParameters(const std::string &moduleInstanceName, IasProperties &parameters);

    /**
     * @brief Set module parameters
     *
     * @param[in] moduleInstanceName Module name
     * @param[in] parameters Module parameters
     * @return success or fail
     */
    IasResult setParameters(const std::string &moduleInstanceName, const IasProperties &parameters);

  private:

    /**
     * @brief Add all source devices to the XML root node.
     *
     * @param[in] root_node Pointer to root XML node
     * @param[in] setup Pointer to smartx setup
     * @return success or fail
     */
    IasResult getSmartxSources(xmlNodePtr root_node, IasISetup *setup);

    /**
     * @brief Add all sink devices to the XML root node.
     *
     * @param[in] root_node Pointer to root XML node
     * @param[in] setup Pointer to smartx setup
     * @return success or fail
     */
    IasResult getSmartxSinks(xmlNodePtr root_node, IasISetup *setup);

    /**
     * @brief Add all Routing zones to the XML root node.
     *
     * @param[in] root_node Pointer to root XML node
     * @param[in] setup Pointer to smartx setup
     * @return success or fail
     */
    IasResult getSmartxRoutingZones(xmlNodePtr root_node, IasISetup *setup);

    /**
     * @brief Add pipeline to the routing node.
     *
     * @param[in] rzPipeline Pointer to the routing zone
     * @param[in] routingzone Pointer to the routingzone node
     * @param[in] setup Pointer to smartx setup
     * @return success or fail
     */
    IasResult getSmartxPipeline(IasPipelinePtr rzPipeline, xmlNodePtr routingzone, IasISetup *setup);

    /**
     * @brief Add processing modules to the processing pipeline node
     *
     * @param[in] processingPipeline Pointer to the processing pipeline
     * @param[in] rzPipeline Pointer to the routing zone
     * @param[in] setup Pointer to smartx setup
     * @return success or fail
     */
    IasResult getSmartxProcessingModules(xmlNodePtr processingPipeline, IasPipelinePtr rzPipeline, IasISetup *setup);

    /**
     * @brief Add module properties
     *
     * @param[in] propertiesNode Pointer to the processing module properties node
     * @param[in] module Pointer to the processing module
     * @param[in] setup Pointer to smartx setup
     * @return success or fail
     */
    IasResult getSmartxProcessingModuleProperties(xmlNodePtr propertiesNode, IasProcessingModulePtr module, IasISetup *setup);

    /**
     * @brief Add processing links
     *
     * @param[in] processingLinksNode Pointer to the processing links node
     * @param[in] rzPipeline Pointer to the routing zone pipeline
     * @return success or fail
     */
    IasResult getSmartxProcessingLinks(xmlNodePtr processingLinksNode, IasPipelinePtr rzPipeline);

    /**
     * @brief Add Other smartx Links
     *
     * @param[in] root_node Pointer to root XML node
     * @param[in] setup Pointer to smartx setup
     * @return success or fail
     */
    IasResult getSmartxLinks(xmlNodePtr root_node, IasISetup *setup);


    // Member variables
    IasSmartX          *mSmartx;    //!< Pointer to the smartx instance
    DltContext         *mLog;       //!< DLT context
};


} //end of IasAudio namespace

#endif /* IASSMARTXDEBUGFACADEHPP_ */
