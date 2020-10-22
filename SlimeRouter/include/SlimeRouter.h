/*++

Module Name:

    SlimeRouter.h

Abstract:

    Class declaration of SlimeRouter.

--*/

#pragma once

//
// ---------------------------------------------------------------------- Includes
//

#include <memory>

#include "Error.h"

//
// ---------------------------------------------------------------------- Definitions
//



//
// ---------------------------------------------------------------------- Classes
//

class RouterServer;

class SlimeRouter
{
public:
    // Constructor
    SlimeRouter();

    // Destructor
    ~SlimeRouter();

    //
    // Public Methods
    //
    
    ERROR_CODE
    Init();

    ERROR_CODE
    Start();
    
    ERROR_CODE
    Stop();
    
private:
    std::unique_ptr<RouterServer> m_RouterServer;
};
