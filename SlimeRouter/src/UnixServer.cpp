/*++

Module Name:

    UnixServer.cpp

Abstract:

    Class implementation of a unix domain socket server.

--*/

//
// ---------------------------------------------------------------------- Includes
//

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>

#include "UnixServer.h"
#include "NetworkUtils.h"

//
// ---------------------------------------------------------------------- Definitions
//



//
// ---------------------------------------------------------------------- Functions
//

UnixServer::UnixServer(std::string Path)
/*++

Routine Description:

    Constructor for UnixServer.

Arguments:

    None.

Return Value:

    None.

--*/
    :
    m_Path(Path),
    m_ServerSocket(0)
{
}

UnixServer::~UnixServer()
/*++

Routine Description:

    Destructor for UnixServer.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Stop();
}

ERROR_CODE
UnixServer::Init()
/*++

Routine Description:

    Initializes the unix server.

Arguments:

    None.

Return Value:

    S_OK on success, error otherwise.

--*/
{
    ERROR_CODE ec = S_OK;
    struct sockaddr_un addr;
    
    m_ServerSocket = socket(AF_UNIX, SOCK_STREAM, 0);
    TRACE_IF_FAILED(m_ServerSocket,
                    Cleanup,
                    "Failed to create server socket! 0x%x\n", ec);

    addr.sun_family = AF_UNIX;
    std::strcpy(addr.sun_path, m_Path.c_str());
    
    unlink(m_Path.c_str());
    
    TRACE_IF_FAILED(bind(m_ServerSocket,
                         (const sockaddr*)&addr,
                         sizeof(addr.sun_family) + strlen(addr.sun_path)),
                    Cleanup,
                    "Failed to bind server socket! 0x%x\n", ec);

    TRACE_IF_FAILED(listen(m_ServerSocket, 128),
                    Cleanup,
                    "Failed to listen server socket! 0x%x\n", ec);
    
Cleanup:
    return ec;
}

ERROR_CODE
UnixServer::Start()
/*++

Routine Description:

    Starts the unix server.

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
UnixServer::Stop()
/*++

Routine Description:

    Stops the unix server.

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
UnixServer::Listen()
/*++

Routine Description:

    Starts listening for incoming connections.

Arguments:

    None.

Return Value:

    S_OK on success, error otherwise.

--*/
{
    ERROR_CODE ec = 0;
    int clientSocket = 0;
 
    while (1)
    {
        clientSocket = accept(m_ServerSocket, NULL, NULL);

        TRACE_IF_FAILED(clientSocket,
                        Cleanup,
                        "Failed to accept new connection! 0x%x\n", errno);
        
        LOG("new connection!\n");

        std::thread incomingMessageThread([=]{ HandleIncomingConnection(clientSocket); });
        incomingMessageThread.detach();
    }

Cleanup:
    return ec;
}

ERROR_CODE
UnixServer::HandleIncomingConnection(int ClientSocket)
/*++

Routine Description:

    Handles an incoming connection.

Arguments:

    ClientSocket - The socket used to communicate with the SlimeSocket client.

Return Value:

    S_OK on success, error otherwise.

--*/
{
    ERROR_CODE ec = S_OK;
    Message message;
    
    while (1)
	{
		EXIT_IF_TRUE(NetworkUtils::ReadAllFromSocket(ClientSocket,
                                                     &message.Header,
                                                     sizeof(MessageHeader)) == 0,
                     S_OK,
                     Cleanup);

        message.Body.resize(message.Header.Size);
        
        EXIT_IF_TRUE(NetworkUtils::ReadAllFromSocket(ClientSocket,
                                                     message.Body.data(),
                                                     message.Header.Size) == 0,
                     S_OK,
                     Cleanup);
        
        OnMessage(ClientSocket, message);
	}
    
    
Cleanup:
    return ec;
}
