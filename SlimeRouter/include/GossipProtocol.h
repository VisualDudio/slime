/*++

Module Name:

    GossipProtocol.h

Abstract:

    Class declaration of a gossip protocol.

--*/

#pragma once

//
// ---------------------------------------------------------------------- Includes
//

#include <memory>
#include <unordered_set>

#include "IMulticastProtocol.h"
#include "BlockingQueue.h"
#include "Message.h"

//
// ---------------------------------------------------------------------- Definitions
//



//
// ---------------------------------------------------------------------- Classes
//

class UdpClient;
class UdpServer;

class GossipProtocol : public IMulticastProtocol
{
public:
    
    // Constructor
    GossipProtocol(std::unique_ptr<UdpClient> UdpClient,
                   std::unique_ptr<UdpServer> UdpServer);
    
    // Destructor
    ~GossipProtocol();
    
    //
    // Public Methods
    //
    
    ERROR_CODE
    Init() override;

    ERROR_CODE
    Start() override;

    ERROR_CODE
    Stop() override;
    
    ERROR_CODE
    AddToMulticastGroup(const std::string& IpAddress, uint16_t Port) override;
    
    ERROR_CODE
    RemoveFromMulticastGroup(const std::string& IpAddress, uint16_t Port) override;
    
    ERROR_CODE
    Multicast(const Message& Message) override;
    
    ERROR_CODE
    GetNextDeliveredMessage(Message* Message) override;
    
private:
    ERROR_CODE
    HandleIncomingMessages();
    
    ERROR_CODE
    Deliver(const Message& Message);
    
    ERROR_CODE
    OnReceive(const Message& Message);
    
    ERROR_CODE
    InitiateGossip(const std::string& Payload);
    
    ERROR_CODE
    PeriodicallyGossip();

    ERROR_CODE
    HandleEvents();
    
    // An owning pointer to a UDP client
    std::unique_ptr<UdpClient> m_UdpClient;

    // An owning pointer to a UDP server
    std::unique_ptr<UdpServer> m_UdpServer;
    
    // The set of members to multicast to
    std::unordered_set<std::string> m_MulticastGroup;
    
    BlockingQueue<Message> m_DeliveredMessages;
};
