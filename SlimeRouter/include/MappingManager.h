/*++

Module Name:

    MappingManager.h

Abstract:

    Class declaration of a mapping manager. This class is responsible for managing
    the <Virtual IP, Virtual Port> to <Host IP, Host Port> mappings
    and responding to lookup requests.

--*/

#pragma once

//
// ---------------------------------------------------------------------- Includes
//

#include <unordered_map>
#include <thread>
#include <memory>

#include "Message.h"
#include "Error.h"

//
// ---------------------------------------------------------------------- Definitions
//



//
// ---------------------------------------------------------------------- Classes
//

class IMembershipProtocol;
class IMulticastProtocol;

class MappingManager
{
public:
    // Constructor
    MappingManager(std::unique_ptr<IMembershipProtocol> MembershipProtocol,
                   std::unique_ptr<IMulticastProtocol> MulticastProtocol);

    // Destructor
    ~MappingManager();

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
    AddMapping(const AddressMapping& AddressMapping,
               bool ShouldMulticast);
    
    ERROR_CODE
    RemoveMapping(const AddressMapping& AddressMapping,
                  bool ShouldMulticast);    
    ERROR_CODE
    PerformLookup(uint32_t VirtualAddress,
                  uint16_t VirtualPort,
                  uint32_t* HostAddress,
                  uint16_t* HostPort) const;
    
private:

    ERROR_CODE
    HandleIncomingMessages();
    
    ERROR_CODE
    ProcessIncomingMessage(const Message& Message);
    
    // Hash table storing the mappings
    std::unordered_map<uint64_t, AddressMapping> m_AddressLookup;
    
    // Owning pointer to a MembershipProtocol
    std::unique_ptr<IMembershipProtocol> m_MembershipProtocol;

    // Owning pointer to a MulticastProtocol
    std::unique_ptr<IMulticastProtocol> m_MulticastProtocol;

    std::thread m_IncomingMessageThread;
};
