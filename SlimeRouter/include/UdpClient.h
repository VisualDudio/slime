/*++

Module Name:

    UdpClient.h

Abstract:

    Class declaration for a UDP client.

--*/

#pragma once

//
// ---------------------------------------------------------------------- Includes
//

#include <string>

#include "Error.h"

// #include "IClient.h"

//
// ---------------------------------------------------------------------- Definitions
//



//
// ---------------------------------------------------------------------- Classes
//

class UdpClient
{
public:
    // Constructor
    UdpClient();

    // Destructor
    ~UdpClient();

    //
    // Public Methods
    //
    
    ERROR_CODE
    Init();
    
    ERROR_CODE
    Send(const std::string& Message,
         const std::string& IpAddress,
         const uint16_t Port);
    
private:
    int m_ClientSocket;
// BlockingQueue<std::string> m_OutgoingMessages;
};
