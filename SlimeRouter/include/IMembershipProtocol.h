/*++

Module Name:

    IMembershipProtocol.h

Abstract:

    Interface definition for a membership protocol.

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

class IMembershipProtocol
{
public:
    // Destructor
    virtual
    ~IMembershipProtocol() = default;

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
    
    // virtual
    // ERROR_CODE
    // OnJoin(uint32_t IpAddress, uint16_t Port) = 0;

    // virtual
    // ERROR_CODE
    // OnLeave(uint32_t IpAddress, uint16_t Port) = 0;
    
private:
    
};
