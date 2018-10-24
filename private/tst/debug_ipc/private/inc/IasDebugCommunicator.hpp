/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file IasDebugCommunicator.hpp
 * @date 2017
 * @brief The declaration of the debug communicator.
 */

#ifndef IASDEBUGCOMMUNICATOR_HPP_
#define IASDEBUGCOMMUNICATOR_HPP_

#include <thread>

namespace IasAudio {

class IasIDebug;

/**
 * @brief The debug communicator.
 *
 * It is injected via the debugHook. After being instantiated it
 * opens an ipc path to the IasIDebug interface via a named pipe to start and stop probing.
 *
 * To use it you have to preload the debug ipc library, e.g. like this:
 *
 * LD_PRELOAD=/opt/audio/lib/libias-audio-debug_ipc.so /opt/audio/bin/smartx_interactive_example
 *
 * On success you will find the message '*** enabled debug_ipc ***' in the DLT log with context id _SMX_.
 * Then you have to create the named pipe if not already done:
 *
 * mkfifo /run/smartx/debug
 *
 * You can then echo the probing commands to this named pipe like that:
 *
 * echo rpd FILE_PREFIX PORT_NAME DURATION > /run/smartx/debug
 *
 * e.g. for the smartx_interactive_example:
 *
 * echo rpd myrec map_2_4_port 10 > /run/smartx/debug
 *
 * This will record 10 seconds of audio from the port map_2_4_port and will use the file name prefix myrec.
 * The syntax of the command itself is basically the same like in the smartx_interactive_example. The command spd to stop
 * probing is also implemented:
 *
 * echo spd PORT_NAME > /run/smartx/debug
 *
 * e.g. for the smartx_interactive_example:
 *
 * echo spd map_2_4_port
 */
class IasDebugCommunicator
{
  public:
    /**
     * @brief Constructor
     *
     * @param[in] debug Pointer to the original
     */
    IasDebugCommunicator(IasIDebug *debug);

    /**
     * @brief Destructor
     */
    virtual ~IasDebugCommunicator();

    /**
     * @brief Get the internally stored IasIDebug instance
     *
     * @returns A pointer to the IasIDebug instance
     */
    IasIDebug* getDebugDecorated();

  private:
    /**
     * Copy constructor, deleted to prevent misuse
     */
    IasDebugCommunicator(IasDebugCommunicator const &other) = delete;
    /**
     * Assignment operator, deleted to prevent misuse
     */
    IasDebugCommunicator& operator=(IasDebugCommunicator const &other) = delete;

    /**
     * @brief Thread that checks for the existence of the named pipe
     *
     * In case it finds the named pipe, it starts a new thread, that reads from the named pipe and exits.
     */
    void checkForPipeExistence();

    /**
     * @brief Thread that reads from the previously detected named pipe
     */
    void readInputFromPipe();

    IasIDebug         *mDebug;              //!< The IasIDebug instance
    std::thread       *mCheckForPipeThread; //!< The thread handle of the thread checking for the pipe existence
    std::thread       *mReadInputFromPipe;  //!< The thread handle of the thread reading from the named pipe
};

} /* namespace IasAudio */

#endif /* IASDEBUGCOMMUNICATOR_HPP_ */
