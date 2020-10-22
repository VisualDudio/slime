/*++

Module Name:

    IClient.h

Abstract:

    Interface definition for a client.

--*/

#pragma once

//
// ---------------------------------------------------------------------- Includes
//

#include "Error.h"

//
// ---------------------------------------------------------------------- Definitions
//



//
// ---------------------------------------------------------------------- Classes
//


class IClient
{
public:
    // Constructor
    IClient();

    // Destructor
    virtual
    ~IClient();

    //
    // Public Methods
    //
    
    virtual
    ERROR_CODE
    Init() = 0;

    virtual
    ERROR_CODE
    Connect(const std::string& Hostname,
            const uint16_t Port) = 0;

    virtual
    ERROR_CODE
    Disconnect() = 0;
    
private:
    // The socket connected to the server
    int m_Socket;
};
