/*++

Module Name:

    RouterServer.cpp

Abstract:

    Class implementation of a router server.

--*/

//
// ---------------------------------------------------------------------- Includes
//

#include <fcntl.h>

#include "RouterServer.h"
#include "NetworkUtils.h"
#include "MappingManager.h"

//
// ---------------------------------------------------------------------- Definitions
//



//
// ---------------------------------------------------------------------- Functions
//

RouterServer::RouterServer(std::string Path,
                           std::unique_ptr<MappingManager> MappingManager)
/*++

Routine Description:

    Constructor for RouterServer.

Arguments:

    Path - Unix domain socket path the run the server on.

    MappingManager - Mapping manager to use to store the mappings.

Return Value:

    None.

--*/
    :
    UnixServer(Path),
    m_MappingManager(std::move(MappingManager))
{
}

RouterServer::~RouterServer()
/*++

Routine Description:

    Destructor for RouterServer.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Stop();
}


ERROR_CODE
RouterServer::Init()
/*++

Routine Description:

    Initializes RouterServer.

Arguments:

    None.

Return Value:

    S_OK on success, error otherwise.

--*/
{
    ERROR_CODE ec = S_OK;
    
    TRACE_IF_FAILED(UnixServer::Init(),
                    Cleanup,
                    "Failed to initialize unix server! 0x%x\n", ec);

    TRACE_IF_FAILED(m_MappingManager->Init(),
                    Cleanup,
                    "Failed to initialize mapping manager! 0x%x\n", ec);
    
Cleanup:
    return ec;
}

ERROR_CODE
RouterServer::Start()
/*++

Routine Description:

    Starts the router server.

Arguments:

    None.

Return Value:

    S_OK on success, error otherwise.

--*/
{
    ERROR_CODE ec = S_OK;
    
    TRACE_IF_FAILED(UnixServer::Start(),
                    Cleanup,
                    "Failed to start unix server! 0x%x\n", ec);
    
    TRACE_IF_FAILED(m_MappingManager->Start(),
                    Cleanup,
                    "Failed to start mappping manager! 0x%x\n", ec);
Cleanup:
    return ec;
}

ERROR_CODE
RouterServer::Stop()
/*++

Routine Description:

    Stops the router server.

Arguments:

    None.

Return Value:

    S_OK on success, error otherwise.

--*/
{
    ERROR_CODE ec = S_OK;

    TRACE_IF_FAILED(UnixServer::Stop(),
                    Cleanup,
                    "Failed to stop unix server! 0x%x\n", ec);
    
    TRACE_IF_FAILED(m_MappingManager->Stop(),
                    Cleanup,
                    "Failed to stop mapping manager! 0x%x\n", ec);
    
Cleanup:    
    return ec;
}

ERROR_CODE
RouterServer::OnMessage(int ClientSocket,
                        const Message& Message)
/*++

Routine Description:

    Processes an incoming message.

Arguments:

    ClientSocket - The socket used to communicate with the SlimeSocket client.

    Message - The incoming message.

Return Value:

    S_OK on success, error otherwise.

--*/
{
    ERROR_CODE ec = 0;
    
    switch (Message.Header.Id)
    {
    case REQUEST_TYPE_SOCKET:
    {
        const SocketRequest* request = reinterpret_cast<const SocketRequest*>(Message.Body.data());
        EXIT_IF_FAILED(ProcessSocketRequest(ClientSocket,
                                            *request),
                       Cleanup);
        break;
    }
    case REQUEST_TYPE_BIND:
    {
        const BindRequest* request = reinterpret_cast<const BindRequest*>(Message.Body.data());
        EXIT_IF_FAILED(ProcessBindRequest(ClientSocket,
                                          *request),
                       Cleanup);
        break;
    }
    case REQUEST_TYPE_ACCEPT:
    {
        const AcceptRequest* request = reinterpret_cast<const AcceptRequest*>(Message.Body.data());
        EXIT_IF_FAILED(ProcessAcceptRequest(ClientSocket,
                                            *request),
                       Cleanup);
        break;
    }
    case REQUEST_TYPE_CONNECT:
    {
        const ConnectRequest* request = reinterpret_cast<const ConnectRequest*>(Message.Body.data());
        EXIT_IF_FAILED(ProcessConnectRequest(ClientSocket,
                                             *request),
                       Cleanup);
        break;
    }
    }
    
Cleanup:
    return ec;
}

ERROR_CODE
RouterServer::ProcessSocketRequest(int ClientSocket,
                                   const SocketRequest& SocketRequest)
/*++

Routine Description:

    Processes an incoming socket request. Creates a socket on the host namespace and
responds with the file descriptor.

Arguments:

    ClientSocket - The socket used to communicate with the SlimeSocket client.

    SocketRequest - The request sent by the SlimeSocket client.

Return Value:

    S_OK on success, error otherwise.

--*/
{
    ERROR_CODE ec = S_OK;
    SocketResponse response = {0};
    
    response.Status = socket(AF_INET,
                             SocketRequest.Type,
                             SocketRequest.Protocol);

    if (response.Status < 0)
    {
        response.Status = -errno;
    }
    else
    {
        TRACE_IF_FAILED(NetworkUtils::WriteFileDescriptorToUnixSocket(ClientSocket,
                                                                      response.Status),
                        Cleanup,
                        "Failed to send file descriptor! 0x%x", ec);
        close(response.Status);
    }
    
    TRACE_IF_FAILED(NetworkUtils::WriteAllToSocket(ClientSocket,
                                                   &response,
                                                   sizeof(response)),
                    Cleanup,
                    "Failed to send response to client! 0x%x", ec);
    
Cleanup:
    return ec;
}

ERROR_CODE
RouterServer::ProcessBindRequest(int ClientSocket,
                                 const BindRequest& BindRequest)
/*++

Routine Description:

    Processes an incoming bind request. Binds the host socket to an used port
and multicasts the new mapping.

Arguments:

    ClientSocket - The socket used to communicate with the SlimeSocket client.

    BindRequest - The request sent by the SlimeSocket client.

Return Value:

    S_OK on success, error otherwise.

--*/
{
    ERROR_CODE ec = S_OK;
    BindResponse response = {0};
    int hostSocket = 0;
    struct sockaddr_in hostAddress;
    socklen_t hostAddressLength = 0;
    AddressMapping addressMapping;
    
    // Get file descriptor
    TRACE_IF_FAILED(NetworkUtils::ReadFileDescriptorFromUnixSocket(ClientSocket,
                                                                   &hostSocket),
                    Cleanup,
                    "Failed to receive file descriptor! 0x%x", ec);

    // Perform bind
    hostAddress.sin_family = AF_INET;
    hostAddress.sin_port = 0;
    TRACE_IF_FAILED(NetworkUtils::GetIpAddress(&hostAddress.sin_addr.s_addr),
                    Cleanup,
                    "Failed to get IP address! 0x%x", ec);

    response.Status = bind(hostSocket,
                           (struct sockaddr*)&hostAddress,
                           sizeof(hostAddress));
    

    if (response.Status < 0)
    {
        response.Status = -errno;
    }
    
    // Send response to client
    TRACE_IF_FAILED(NetworkUtils::WriteAllToSocket(ClientSocket,
                                                   &response,
                                                   sizeof(response)),
                    Cleanup,
                    "Failed to send response to client! 0x%x", ec);
    
    // Find port and create mapping
    hostAddressLength = sizeof(hostAddress);
    getsockname(hostSocket,
                (struct sockaddr*)&hostAddress,
                &hostAddressLength);
    
    addressMapping.VirtualIpAddress = BindRequest.VirtualIpAddress;
    addressMapping.VirtualPort = BindRequest.VirtualPort;
    addressMapping.HostIpAddress = hostAddress.sin_addr.s_addr;
    addressMapping.HostPort = hostAddress.sin_port;
    
    m_MappingManager->AddMapping(addressMapping,
                                 true);
Cleanup:
    return ec;
}

ERROR_CODE
RouterServer::ProcessAcceptRequest(int ClientSocket,
                                   const AcceptRequest& AcceptRequest)
/*++

Routine Description:

    Processes an incoming accept request. 

Arguments:

    ClientSocket - The socket used to communicate with the SlimeSocket client.

    AcceptRequest - The request sent by the SlimeSocket client.

Return Value:

    S_OK on success, error otherwise.

--*/
{
    ERROR_CODE ec = S_OK;
    AcceptResponse response = {0};
    int hostSocket = 0;
    int originalFlags = 0;
    
    TRACE_IF_FAILED(NetworkUtils::ReadFileDescriptorFromUnixSocket(ClientSocket,
                                                                   &hostSocket),
                    Cleanup,
                    "Failed to receive file descriptor 0x%x", ec);
    
    originalFlags = fcntl(hostSocket, F_GETFL);
    fcntl(hostSocket, F_SETFL, originalFlags & ~O_NONBLOCK);

    response.Status = accept4(hostSocket,
                              NULL,
                              NULL,
                              AcceptRequest.Flags);

    fcntl(hostSocket, F_SETFL, originalFlags);
    
    if (response.Status < 0)
    {
        response.Status = -errno;
    }
    else
    {
        TRACE_IF_FAILED(NetworkUtils::WriteFileDescriptorToUnixSocket(ClientSocket,
                                                                      response.Status),
                        Cleanup,
                        "Failed to send file descriptor 0x%x!", ec);
        close(response.Status);
    }
    
    TRACE_IF_FAILED(NetworkUtils::WriteAllToSocket(ClientSocket,
                                                   &response,
                                                   sizeof(response)),
                    Cleanup,
                    "Failed to send response to client! 0x%x", ec);
    
Cleanup:
    return ec;
}

ERROR_CODE
RouterServer::ProcessConnectRequest(int ClientSocket,
                                    const ConnectRequest& ConnectRequest)
/*++

Routine Description:

    Processes an incoming connect request.

Arguments:

    ConnectRequest - The connect request to process.

Return Value:

    S_OK on success, error otherwise.

--*/
{
    ERROR_CODE ec = S_OK;
    ConnectResponse response = {0};
    int hostSocket = 0;
    struct sockaddr_in hostAddress;
    
    TRACE_IF_FAILED(NetworkUtils::ReadFileDescriptorFromUnixSocket(ClientSocket,
                                                                   &hostSocket),
                    Cleanup,
                    "Failed to receive file descriptor! 0x%x", ec);

    TRACE_IF_FAILED(m_MappingManager->PerformLookup(ConnectRequest.VirtualIpAddress,
                                                    ConnectRequest.VirtualPort,
                                                    &hostAddress.sin_addr.s_addr,
                                                    &hostAddress.sin_port),
                    Cleanup,
                    "Failed to perform mapping lookup! 0x%x", ec);
    

    hostAddress.sin_family = AF_INET;
    response.Status = connect(hostSocket,
                              (struct sockaddr*)&hostAddress,
                              sizeof(hostAddress));
    
    if (response.Status < 0)
    {
        response.Status = -errno;
    }
    
Cleanup:
    return ec;
}
