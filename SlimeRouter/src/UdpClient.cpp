/*++

Module Name:

    UdpClient.cpp

Abstract:

    Class implementation for a UDP client.

--*/

//
// ---------------------------------------------------------------------- Includes
//

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "UdpClient.h"

//
// ---------------------------------------------------------------------- Definitions
//



//
// ---------------------------------------------------------------------- Functions
//


UdpClient::UdpClient()
/*++

Routine Description:

    Constructor for UdpClient.

Arguments:

    None.

Return Value:

    None.

--*/
    :
    m_ClientSocket(0)
{

}

UdpClient::~UdpClient()
/*++

Routine Description:

    Destructor for UdpClient.

Arguments:

    None.

Return Value:

    None.

--*/
{
}

ERROR_CODE
UdpClient::Init()
/*++

Routine Description:

    Initializes the UDP client.

Arguments:

    None.

Return Value:

    S_OK on success, error otherwise.

--*/
{
    ERROR_CODE ec = S_OK;
    
    EXIT_IF_TRUE((m_ClientSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0,
                 E_FAIL,
                 Cleanup);
    
Cleanup:
    return ec;
}

ERROR_CODE
UdpClient::Send(const Message& Message,
                const std::string& IpAddress,
                const uint16_t Port)
/*++

Routine Description:

    Sends a message to the UDP server at the given IP address and port.

Arguments:

    Message - The message to send.

    IpAddress - The IP address to send to.

    Port - The port to send to.

Return Value:

    S_OK on success, error otherwise.

--*/
{
    ERROR_CODE ec = S_OK;
    struct sockaddr_in serverAddress;
    
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(Port);
    
    inet_pton(AF_INET,
              IpAddress.c_str(),
              &serverAddress.sin_addr);

    EXIT_IF_FAILED(sendto(m_ClientSocket,
                          &Message,
                          sizeof(Message),
                          MSG_CONFIRM,
                          (struct sockaddr*)&serverAddress,
                          sizeof(serverAddress)),
                   Cleanup);
Cleanup:
    return ec;
}
