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

#include "UdpServer.h"
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

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(m_Port);
    
    EXIT_IF_FAILED(bind(m_ServerSocket,
                        (const struct sockaddr*)&serverAddress,
                        sizeof(serverAddress)),
                   Cleanup);

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
    char buffer[MESSAGE_BUFFER_SIZE] = {0};
    socklen_t addressLength = 0;
    
    addressLength = sizeof(clientAddress);
    
    while (1)
    {
        EXIT_IF_FAILED(recvfrom(m_ServerSocket,
                                buffer,
                                sizeof(buffer),
                                MSG_WAITALL,
                                (struct sockaddr*)&clientAddress,
                                &addressLength),
                       Cleanup);
        
        m_IncomingMessages.Push(std::string(buffer));
    }

Cleanup:
    return ec;
}

