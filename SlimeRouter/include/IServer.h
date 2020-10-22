/*++

Module Name:

    IServer.h

Abstract:

    Interface definition for a server.

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

class IServer
{
public:
    // Destructor
    virtual
    ~IServer();

    //
    // Public Methods
    //
    
    virtual
    ERROR_CODE
    Init() = 0;
    
    virtual
    ERROR_CODE
    Start() = 0;

    virtual
    ERROR_CODE
    Stop() = 0;
};
