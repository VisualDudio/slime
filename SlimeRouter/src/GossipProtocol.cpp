/*++

Module Name:

    GossipProtocol.cpp

Abstract:

    Class implementation of a gossip protocol.

--*/

//
// ---------------------------------------------------------------------- Includes
//

#include <chrono>
#include <cstring>

#include "GossipProtocol.h"
#include "UdpClient.h"
#include "UdpServer.h"

//
// ---------------------------------------------------------------------- Definitions
//

#define GOSSIP_FANOUT 2
#define GOSSIP_PERIOD std::chrono::milliseconds(1000)
#define UDP_PORT 8080

//
// ---------------------------------------------------------------------- Functions
//

GossipProtocol::GossipProtocol(std::unique_ptr<UdpClient> UdpClient,
                               std::unique_ptr<UdpServer> UdpServer)
/*++

Routine Description:

    Constructor for GossipProtocol.

Arguments:

    None.

Return Value:

    None.

--*/
    :
    m_UdpClient(std::move(UdpClient)),
    m_UdpServer(std::move(UdpServer))
{
}

GossipProtocol::~GossipProtocol()
/*++

Routine Description:

    Destructor for GossipProtocol.

Arguments:

    None.

Return Value:

    None.

--*/
{
}

ERROR_CODE
GossipProtocol::Init()
/*++

Routine Description:

    Initializes the GossipProtocol instance.

Arguments:

    None.

Return Value:

    S_OK on success, error otherwise.

--*/
{
    ERROR_CODE ec = S_OK;
    
    // TODO: remove hardcode
    m_MulticastGroup.insert("172.22.152.5");
    m_MulticastGroup.insert("172.22.152.6");
    EXIT_IF_FAILED(m_UdpClient->Init(),
                   Cleanup);
    
Cleanup:
    return ec;
}

ERROR_CODE
GossipProtocol::Start()
/*++

Routine Description:

    Starts the gossip protocol.

Arguments:

    None.

Return Value:

    S_OK on success, error otherwise.

--*/
{
    ERROR_CODE ec = S_OK;
    

    return ec;
}

ERROR_CODE
GossipProtocol::Stop()
/*++

Routine Description:

    Stops the gossip protocol.

Arguments:

    None.

Return Value:

    S_OK on success, error otherwise.

--*/
{
    ERROR_CODE ec = S_OK;

     
    return ec;
}

ERROR_CODE
GossipProtocol::AddToMulticastGroup(const std::string& IpAddress, uint16_t Port)
/*++

Routine Description:

    Adds a node with the specified address to the multicast group.

Arguments:

    IpAddress - The IP address of the node to add.

    Port - The port of the node to add.

Return Value:

    S_OK on success, error otherwise.

--*/
{
    ERROR_CODE ec = S_OK;
    
    UNREFERENCED_PARAMETER(Port);
    
    m_MulticastGroup.insert(std::move(IpAddress));

    return ec;
}

ERROR_CODE
GossipProtocol::RemoveFromMulticastGroup(const std::string& IpAddress, uint16_t Port)
/*++

Routine Description:

    Removes a node with the specified address from the multicast group.

Arguments:

    IpAddress - The IP address of the node to remove.

    Port - The port of the node to remove.

Return Value:

    S_OK on success, error otherwise.

--*/
{
    ERROR_CODE ec = S_OK;

    UNREFERENCED_PARAMETER(Port);
    
    m_MulticastGroup.erase(IpAddress);
    
    return ec;
}

ERROR_CODE
GossipProtocol::Multicast(const Message& Message)
/*++

Routine Description:

    Initiates a new gossip containing the provided message.

Arguments:

    Message - The contents of the message to multicast.

Return Value:

    S_OK on success, error otherwise.

--*/
{
    ERROR_CODE ec = S_OK;
    
    for (const std::string& ipAddress : m_MulticastGroup)
    {
        m_UdpClient->Send(Message, ipAddress, UDP_PORT);
    }
    
    return ec;
}

ERROR_CODE
GossipProtocol::PeriodicallyGossip()
/*++

Routine Description:

    Gossips to GOSSIP_FANOUT random neighbors every GOSSIP_PERIOD time units.

Arguments:

    None.

Return Value:

    S_OK on success, error otherwise.

--*/
{
    ERROR_CODE ec = S_OK;
    
    while (1)
    {
        std::this_thread::sleep_for(GOSSIP_PERIOD);
    }
    
//Cleanup:
    return ec;
}

ERROR_CODE
GossipProtocol::HandleEvents()
/*++

Routine Description:

    Handles events.

Arguments:

    None.

Return Value:

    S_OK on success, error otherwise.

--*/
{
    ERROR_CODE ec = S_OK;
    // std::vector<std::string> membershipList;
    
//     while (m_EventQueue.Pop())
//     {
//         std::vector<int> destinations;
 
//         std::sample(membershipList.begin(),
//                     membershipList.end(),
//                     std::back_inserter(destinations),
//                     GOSSIP_FANOUT,
//                     std::mt19937{ std::random_device{}() });

//         for (const int destination : destinations)
//         {
//             m_UdpClient->Send(destination, message);
//         }
//     }
    
// Cleanup:
     return ec;
}

ERROR_CODE
GossipProtocol::GetNextDeliveredMessage(Message* Message)
/*++

Routine Description:

    Gets the next delivered message from the queue.

Arguments:

    Message - The next delivered message.

Return Value:

    S_OK on success, error otherwise.

--*/
{
    ERROR_CODE ec = S_OK;
    
    *Message = m_DeliveredMessages.Pop();
    
    return ec;
}

ERROR_CODE
GossipProtocol::Deliver(const Message& Message)
/*++

Routine Description:

    Delivers the provided message to the above layer.

Arguments:

    Message - The message to deliver.

Return Value:

    S_OK on success, error otherwise.

--*/
{
    ERROR_CODE ec = S_OK;
    
    m_DeliveredMessages.Push(Message);
    
    return ec;
}


ERROR_CODE
GossipProtocol::OnReceive(const Message& Message)
/*++

Routine Description:

    Handles the OnReceive event.

Arguments:

    Message - The received message.

Return Value:

    S_OK on success, error otherwise.

--*/
{
    ERROR_CODE ec = S_OK;
    
    EXIT_IF_FAILED(Deliver(Message),
                   Cleanup);
    
Cleanup:
    return ec;
}


ERROR_CODE
GossipProtocol::HandleIncomingMessages()
/*++

Routine Description:

    Handles incoming external messages.

Arguments:

    None.

Return Value:

    S_OK on success, error otherwise.

--*/
{
    ERROR_CODE ec = S_OK;
    std::string rawMessage;
    Message message;
    
    while (m_UdpServer->GetNextIncomingMessage(&rawMessage))
    {
        std::memcpy(&message.Header, rawMessage.c_str(), sizeof(message.Header));
        message.Body.resize(message.Header.Size);
        std::memcpy(message.Body.data(),
                    rawMessage.c_str() + sizeof(message.Header),
                    sizeof(message.Header.Size));
        OnReceive(message);
    }
    
    return ec;
}
