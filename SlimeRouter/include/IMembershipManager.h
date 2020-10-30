/*++

Module Name:

    IMembershipManager.h

Abstract:

    Interface definition for a membership manager.

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

class IMembershipManager
{
public:
    // Destructor
    virtual
    ~IMembershipManager() = default;

    //
    // Public Methods
    //
    
    virtual
    ERROR_CODE
    OnJoin(const std::string& IpAddress) = 0;

    virtual
    ERROR_CODE
    OnLeave(const std::string& IpAddress) = 0;
};
