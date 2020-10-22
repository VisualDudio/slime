/*++

Module Name:

    ISocket.h

Abstract:

    Interface definition of a socket.

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

class ISocket
{
public:
    // Virtual Destructor
    virtual
    ~ISocket();

    //
    // Public Methods
    //
    
    virtual
    ERROR_CODE
    Init() = 0;

    virtual
    ERROR_CODE
    Bind() = 0;
    
private:
    
};
