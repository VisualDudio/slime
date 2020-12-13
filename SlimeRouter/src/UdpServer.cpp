/*++

Module Name:

    UdpServer.cpp

Abstract:

    Class implementation of a UDP server.

--*/

//
// ---------------------------------------------------------------------- Includes
//

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <string>

#include "UdpServer.h"
#include "Message.h"

//
// ---------------------------------------------------------------------- Definitions
//

#define MESSAGE_BUFFER_SIZE 1024

//
// ---------------------------------------------------------------------- Functions
//

UdpServer::UdpServer(uint16_t Port)
/*++

Routine Description:

    Constructor for the UDP server.

Arguments:

    Port - The port this server will listen on.

Return Value:

    None.

--*/
    :
    m_Port(Port)
{
}

UdpServer::~UdpServer()
/*++

Routine Description:

    Destructor for the UDP server.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Stop();
}

ERROR_CODE
UdpServer::Init()
/*++

Routine Description:

    Initializes the UDP server.

Arguments:

    None.

Return Value:

    S_OK on success, error otherwise.

--*/
{
    ERROR_CODE ec = S_OK;
    struct sockaddr_in serverAddress;
    
    m_ServerSocket = socket(AF_INET, SOCK_DGRAM, 0);
    EXIT_IF_FAILED(m_ServerSocket,
                   Cleanup);
    LOG("UDP server socket created\n");

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(m_Port);
    
    EXIT_IF_FAILED(bind(m_ServerSocket,
                        (const struct sockaddr*)&serverAddress,
                        sizeof(serverAddress)),
                   Cleanup);
    LOG("UDP server socket bound\n");

Cleanup:
    return ec;
}


ERROR_CODE
UdpServer::Start()
/*++

Routine Description:

    Starts the UDP server.

Arguments:

    None.

Return Value:

    S_OK on success, error otherwise.

--*/
{
    ERROR_CODE ec = 0;

    m_ServerThread = std::thread([=] { Listen(); });

    return ec;
}

ERROR_CODE
UdpServer::Stop()
/*++

Routine Description:

    Stops the UDP server.

Arguments:

    None.

Return Value:

    None.

--*/
{
    ERROR_CODE ec = 0;
    
    if (m_ServerThread.joinable())
    {
        m_ServerThread.join();
    }
    
    return ec;
}

ERROR_CODE
UdpServer::GetNextIncomingMessage(Message* Message)
/*++

Routine Description:

    Gets the next incoming message from the message queue.

Arguments:

    Message - The message to populate.

Return Value:

    S_OK on success, error otherwise.

--*/
{
    ERROR_CODE ec = S_OK;
    
    *Message = m_IncomingMessages.Pop();
    
    return ec;
}


ERROR_CODE
UdpServer::Listen()
/*++

Routine Description:

    Starts listening for incoming UDP packets.

Arguments:

    None.

Return Value:

    S_OK on success, error otherwise.

--*/
{
    ERROR_CODE ec = S_OK;
    struct sockaddr_in clientAddress;
    char rawMessage[MESSAGE_BUFFER_SIZE] = {0};
    socklen_t addressLength = 0;
    ssize_t bytesRead = 0;
    Message message;
    
    addressLength = sizeof(clientAddress);
    
    while (1)
    {
        bytesRead = recvfrom(m_ServerSocket,
                             rawMessage,
                             MESSAGE_BUFFER_SIZE,
                             MSG_WAITALL,
                             (struct sockaddr*)&clientAddress,
                             &addressLength);
        TRACE_IF_FAILED(bytesRead, Cleanup, "Failed to receive datagram from client! 0x%x\n", errno);
        
        std::memcpy(&message.Header, rawMessage, sizeof(message.Header));
        message.Body.resize(message.Header.Size);
        std::memcpy(message.Body.data(),
                    rawMessage + sizeof(message.Header),
                    message.Header.Size);
        
        m_IncomingMessages.Push(message);
    }

Cleanup:
    return ec;
}

