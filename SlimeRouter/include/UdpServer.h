/*++

Module Name:

    UdpServer.h

Abstract:

    Class definition of a UDP server.

--*/

#pragma once

//
// ---------------------------------------------------------------------- Includes
//

#include <thread>

#include "BlockingQueue.h"

//
// ---------------------------------------------------------------------- Definitions
//



//
// ---------------------------------------------------------------------- Classes
//

class Message;

class UdpServer
{
public:
    // Constructor
    UdpServer(uint16_t Port);

    // Destructor
    ~UdpServer();

    //
    // Public Methods
    //
    
    ERROR_CODE
    Init();

    ERROR_CODE
    Start();

    ERROR_CODE
    Stop();
    
    ERROR_CODE
    GetNextIncomingMessage(Message* Message);

private:

    ERROR_CODE
    Listen();

    int m_ServerSocket;
    uint16_t m_Port;
    std::thread m_ServerThread;
    BlockingQueue<Message> m_IncomingMessages;
};
