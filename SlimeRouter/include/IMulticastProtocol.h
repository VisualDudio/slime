/*++

Module Name:

    IMulticastProtocol.h

Abstract:

    Interface definition of a multicast protocol.

--*/

#pragma once

//
// ---------------------------------------------------------------------- Includes
//

#include "Error.h"
#include "Message.h"

//
// ---------------------------------------------------------------------- Definitions
//



//
// ---------------------------------------------------------------------- Classes
//

class IMulticastProtocol
{
public:
    
    // Virtual Destructor
    virtual
    ~IMulticastProtocol() = default;
    
    virtual
    ERROR_CODE
    Init() = 0;

    virtual
    ERROR_CODE
    Start() = 0;
    
    virtual
    ERROR_CODE
    Stop() = 0;
    
    virtual
    ERROR_CODE
    AddToMulticastGroup() = 0;
    
    virtual
    ERROR_CODE
    RemoveFromMulticastGroup() = 0;
    
    virtual
    ERROR_CODE
    Multicast(const Message& Message) = 0;
    
    virtual
    ERROR_CODE
    GetNextDeliveredMessage(Message* Message) = 0;
};
