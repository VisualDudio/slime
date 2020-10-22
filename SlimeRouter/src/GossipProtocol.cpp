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

#include "GossipProtocol.h"
#include "UdpClient.h"

//
// ---------------------------------------------------------------------- Definitions
//

#define GOSSIP_FANOUT 2
#define GOSSIP_PERIOD std::chrono::milliseconds(1000)

//
// ---------------------------------------------------------------------- Functions
//

GossipProtocol::GossipProtocol(std::unique_ptr<UdpClient> UdpClient)
/*++

Routine Description:

    Constructor for GossipProtocol.

Arguments:

    None.

Return Value:

    None.

--*/
    :
    m_UdpClient(std::move(UdpClient))
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
GossipProtocol::AddToMulticastGroup()
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

//  m_EventQueue.push(std::make_unique<NewMember>(IpAddress, Port));
    
//Cleanup:
    return ec;
}

ERROR_CODE
GossipProtocol::RemoveFromMulticastGroup()
/*++

Routine Description:

    Removes a node with the specified address from the multicast group.

Arguments:

    IpAddress - The IP address of the node to add.

    Port - The port of the node to add.

Return Value:

    S_OK on success, error otherwise.

--*/
{
    ERROR_CODE ec = S_OK;

//  m_EventQueue.push(std::make_unique<NewMember>(IpAddress, Port));
    
//Cleanup:
    return ec;
}

ERROR_CODE
GossipProtocol::Multicast(const Message& Message)
/*++

Routine Description:

    Initiates a new gossip containing the provided message.

Arguments:

    Payload - The contents of the message to multicast.

Return Value:

    S_OK on success, error otherwise.

--*/
{
    ERROR_CODE ec = S_OK;
    
    UNREFERENCED_PARAMETER(Message);
    
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
    
    UNREFERENCED_PARAMETER(Message);

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
    
    // while (m_UdpServer->GetNextDeliveredMessage(&message))
    // {
        
    // }
    
    return ec;
}
