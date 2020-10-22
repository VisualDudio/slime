/*++

Module Name:

    MappingManager.cpp

Abstract:

    Class implementation of a mapping manager.

--*/

//
// ---------------------------------------------------------------------- Includes
//

#include <cstring>

#include "MappingManager.h"
#include "IMembershipProtocol.h"
#include "IMulticastProtocol.h"

//
// ---------------------------------------------------------------------- Definitions
//

#define CREATE_ADDRESS(address, ip, port) address = (uint64_t)port << 32 | ip

#define GET_IP_ADDRESS(address) (uint32_t)(address & 0xFFFF)
#define GET_PORT(address) (uint16_t)(address >> 32) & 0xFFFF

//
// ---------------------------------------------------------------------- Functions
//

MappingManager::MappingManager(std::unique_ptr<IMembershipProtocol> MembershipProtocol,
                               std::unique_ptr<IMulticastProtocol> MulticastProtocol)
/*++

Routine Description:

    Constructor for MappingManager.

Arguments:

    MembershipProtocol - The membership protocol to use to maintain cluster membership.

    MulticastProtocol - The multicast protocol to use to disseminate mappings.

Return Value:

    None.

--*/
    :
    m_MembershipProtocol(std::move(MembershipProtocol)),
    m_MulticastProtocol(std::move(MulticastProtocol))
{
}

MappingManager::~MappingManager()
/*++

Routine Description:

    Destructor for MappingManager.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Stop();
}

ERROR_CODE
MappingManager::Init()
/*++

Routine Description:

    Initializes the MappingManager instance.

Arguments:

    None.

Return Value:

    S_OK on success, error otherwise.

--*/
{
    ERROR_CODE ec = S_OK;
    
    EXIT_IF_FAILED(m_MembershipProtocol->Init(),
                   Cleanup);
    
    EXIT_IF_FAILED(m_MulticastProtocol->Init(),
                   Cleanup);
    
Cleanup:
    return ec;
}

ERROR_CODE
MappingManager::Start()
/*++

Routine Description:

    Starts the MappingManager.

Arguments:

    None.

Return Value:

    S_OK on success, error otherwise.

--*/
{
    ERROR_CODE ec = S_OK;
    
    m_IncomingMessageThread = std::thread([=] { HandleIncomingMessages(); });
    
    EXIT_IF_FAILED(m_MembershipProtocol->Start(),
                   Cleanup);
    
    EXIT_IF_FAILED(m_MulticastProtocol->Start(),
                   Cleanup);
    
Cleanup:
    return ec;
}

ERROR_CODE
MappingManager::Stop()
/*++

Routine Description:

    Stops the mapping manager.

Arguments:

    None.

Return Value:

    S_OK on success, error otherwise.

--*/
{
    ERROR_CODE ec = S_OK;

    EXIT_IF_FAILED(m_MulticastProtocol->Stop(),
                   Cleanup);
    
    EXIT_IF_FAILED(m_MembershipProtocol->Stop(),
                   Cleanup);
    
    if (m_IncomingMessageThread.joinable())
    {
        m_IncomingMessageThread.join();
    }
    
Cleanup:
    return ec;
}



ERROR_CODE
MappingManager::AddMapping(const AddressMapping& AddressMapping,
                           bool ShouldMulticast)
/*++

Routine Description:

    Adds the specified mapping to the data store.

Arguments:

    AddressMapping - The address mapping to store.

    ShouldMulticast - Whether or not this mapping should be multicast to other nodes.

Return Value:

    S_OK on success, error otherwise.

--*/
{
    ERROR_CODE ec = S_OK;
    uint64_t virtualAddress = 0;
    Message multicastMessage;
    
    CREATE_ADDRESS(virtualAddress,
                   AddressMapping.VirtualIpAddress,
                   AddressMapping.VirtualPort);
    
    m_AddressLookup.insert({virtualAddress, AddressMapping});
    
    if (ShouldMulticast)
    {
        multicastMessage.Header.Id = MESSAGE_TYPE_NEW_MAPPING;
        multicastMessage.Header.Size = sizeof(AddressMapping);
        multicastMessage.Body.resize(sizeof(AddressMapping));
        std::memcpy(multicastMessage.Body.data(), &AddressMapping, sizeof(AddressMapping));
        EXIT_IF_FAILED(m_MulticastProtocol->Multicast(multicastMessage),
                       Cleanup);
    }
    
Cleanup:
    return ec;
}

ERROR_CODE
MappingManager::RemoveMapping(const AddressMapping& AddressMapping,
                              bool ShouldMulticast)
/*++

Routine Description:

    Removes the specified mapping from the data store.

Arguments:

    AddressMapping - The address mapping to remove.

    ShouldMulticast - Whether or not this event should be multicast to other nodes.

Return Value:

    S_OK on success, error otherwise.

--*/
{
    ERROR_CODE ec = S_OK;
    uint64_t virtualAddress = 0;
    Message multicastMessage;
    
    CREATE_ADDRESS(virtualAddress,
                   AddressMapping.VirtualIpAddress,
                   AddressMapping.VirtualPort);
    
    auto mappingIt = m_AddressLookup.find(virtualAddress);
    EXIT_IF_TRUE(mappingIt == m_AddressLookup.end(),
                 S_FALSE,
                 Cleanup);
    
    // TODO: operator ==
    // EXIT_IF_TRUE(AddressMapping != mappingIt->second,
    //              S_FALSE,
    //              Cleanup);
    
    m_AddressLookup.erase(mappingIt);
    
    if (ShouldMulticast)
    {
        multicastMessage.Header.Id = MESSAGE_TYPE_DELETE_MAPPING;
        multicastMessage.Header.Size = sizeof(AddressMapping);
        multicastMessage.Body.resize(sizeof(AddressMapping));
        std::memcpy(multicastMessage.Body.data(), &AddressMapping, sizeof(AddressMapping));
        EXIT_IF_FAILED(m_MulticastProtocol->Multicast(multicastMessage),
                       Cleanup);
    }
    
Cleanup:    
    return ec;
}

ERROR_CODE
MappingManager::PerformLookup(uint32_t VirtualIpAddress,
                              uint16_t VirtualPort,
                              uint32_t* HostIpAddress,
                              uint16_t* HostPort) const
/*++

Routine Description:

    Gets the associated host address from the specified virtual address.

Arguments:

    VirtualIpAddress - The virtual IP address.

    VirtualPort - The virtual port.

    HostIpAddress - The host IP address.

    HostPort - The host port.

Return Value:

    S_OK on success, error otherwise.

--*/
{
    ERROR_CODE ec = S_OK;
    uint64_t virtualAddress = 0;
    AddressMapping addressMapping;
    
    CREATE_ADDRESS(virtualAddress, VirtualIpAddress, VirtualPort);
    
    auto mappingIt = m_AddressLookup.find(virtualAddress);

    EXIT_IF_TRUE(mappingIt == m_AddressLookup.end(),
                 S_FALSE,
                 Cleanup);
    
    addressMapping = mappingIt->second;
    *HostIpAddress = addressMapping.HostIpAddress;
    *HostPort = addressMapping.HostPort;

Cleanup:
    return ec;
}

ERROR_CODE
MappingManager::HandleIncomingMessages()
/*++

Routine Description:

    Handles incoming messages from other nodes.

Arguments:

    None.

Return Value:

    S_OK on success, error otherwise.

--*/
{
    ERROR_CODE ec = S_OK;
    Message message;
    
    while (true)
    {
        EXIT_IF_FAILED(m_MulticastProtocol->GetNextDeliveredMessage(&message),
                       Cleanup);
        
        EXIT_IF_FAILED(ProcessIncomingMessage(message),
                       Cleanup);
    }
    
Cleanup:
    return ec;
}

ERROR_CODE
MappingManager::ProcessIncomingMessage(const Message& Message)
/*++

Routine Description:

    Process an message incoming from another node.

Arguments:

    Message - The incoming message to process.

Return Value:

    S_OK on success, error otherwise.

--*/
{
    ERROR_CODE ec = S_OK;
    const AddressMapping* addressMapping = NULL;
    
    switch (Message.Header.Id)
    {
    case MESSAGE_TYPE_NEW_MAPPING:
        addressMapping = reinterpret_cast<const AddressMapping*>(Message.Body.data());
        EXIT_IF_FAILED(AddMapping(*addressMapping, false),
                       Cleanup);
        break;
    case MESSAGE_TYPE_DELETE_MAPPING:
        addressMapping = reinterpret_cast<const AddressMapping*>(Message.Body.data());
        EXIT_IF_FAILED(RemoveMapping(*addressMapping, false),
                       Cleanup);
        break;
    default:
        EXIT_IF_FAILED(E_FAIL, Cleanup);
        break;
    }
    
Cleanup:
    return ec;
}
