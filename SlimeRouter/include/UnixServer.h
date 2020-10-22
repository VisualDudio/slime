/*++

Module Name:

    UnixServer.h

Abstract:

    Class declaration of a unix domain socket server.

--*/

#pragma once

//
// ---------------------------------------------------------------------- Includes
//

#include <thread>
#include <string>

#include "Error.h"
#include "Message.h"

//
// ---------------------------------------------------------------------- Definitions
//



//
// ---------------------------------------------------------------------- Classes
//

class UnixServer
{
public:
    // Constructor
    UnixServer(std::string Path);

    // Destructor
    ~UnixServer();

    //
    // Public Methods
    //
    
    ERROR_CODE
    Init();
    
    ERROR_CODE
    Start();

    ERROR_CODE
    Stop();

protected:
    
    virtual
    ERROR_CODE
    OnMessage(int ClientSocket,
              const Message& Message) = 0;
    
private:
    
    ERROR_CODE
    Listen();

    ERROR_CODE
    HandleIncomingConnection(int ClientSocket);
    
    std::string m_Path;
    int m_ServerSocket;
    std::thread m_ServerThread;
};
