/*++

Module Name:

    SlimeRouter.cpp

Abstract:

    Class implementation of SlimeRouter.

--*/

//
// ---------------------------------------------------------------------- Includes
//

#include "SlimeRouter.h"
#include "MappingManager.h"
#include "GossipProtocol.h"
#include "UdpClient.h"
#include "UdpServer.h"
#include "DockerPlugin.h"
#include "RouterServer.h"
#include "httplib.h"

//
// ---------------------------------------------------------------------- Definitions
//

#define UNIX_SERVER_PATH "/home/ombarki2/slime/SlimeSocket.sock"
#define UDP_PORT 8080

//
// ---------------------------------------------------------------------- Functions
//

SlimeRouter::SlimeRouter()
/*++

Routine Description:

    Constructor for SlimeRouter.

Arguments:

    None.

Return Value:

    None.

--*/
{
}


SlimeRouter::~SlimeRouter()
/*++

Routine Description:

    Destructor for SlimeRouter.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Stop();
}

ERROR_CODE
SlimeRouter::Init()
/*++

Routine Description:

    Initializes the SlimeRouter instance and all of its modules.

Arguments:

    None.

Return Value:

    S_OK on success, error otherwise.

--*/
{
    ERROR_CODE ec = S_OK;
    std::unique_ptr<GossipProtocol> gossipProtocol = nullptr;
    std::unique_ptr<MappingManager> mappingManager = nullptr;
    std::unique_ptr<UdpClient> udpClient = nullptr;
    std::unique_ptr<UdpServer> udpServer = nullptr;
    std::unique_ptr<DockerPlugin> dockerPlugin = nullptr;
    std::unique_ptr<httplib::Server> httpServer = nullptr;
    
    udpClient = std::make_unique<UdpClient>();
    EXIT_IF_NULL(udpClient,
                 E_OUTOFMEMORY,
                 Cleanup);
    
    udpServer = std::make_unique<UdpServer>(UDP_PORT);
    EXIT_IF_NULL(udpServer,
                 E_OUTOFMEMORY,
                 Cleanup);
    
    gossipProtocol = std::make_unique<GossipProtocol>(std::move(udpClient),
                                                      std::move(udpServer));
    EXIT_IF_NULL(gossipProtocol,
                 E_OUTOFMEMORY,
                 Cleanup);
    
    httpServer = std::make_unique<httplib::Server>();
    EXIT_IF_NULL(httpServer,
                 E_OUTOFMEMORY,
                 Cleanup);    
    
    dockerPlugin = std::make_unique<DockerPlugin>(std::move(httpServer));
    EXIT_IF_NULL(dockerPlugin,
                 E_OUTOFMEMORY,
                 Cleanup);
    
    mappingManager = std::make_unique<MappingManager>(std::move(dockerPlugin),
                                                      std::move(gossipProtocol));
    EXIT_IF_NULL(mappingManager,
                 E_OUTOFMEMORY,
                 Cleanup);
    
    m_RouterServer = std::make_unique<RouterServer>(UNIX_SERVER_PATH,
                                                    std::move(mappingManager));
    EXIT_IF_NULL(m_RouterServer,
                 E_OUTOFMEMORY,
                 Cleanup);
    
    EXIT_IF_FAILED(m_RouterServer->Init(),
                   Cleanup);
    
Cleanup:
    return ec;
}

ERROR_CODE
SlimeRouter::Start()
/*++

Routine Description:

    Starts the SlimeRouter.

Arguments:

    None.

Return Value:

    S_OK on success, error otherwise.

--*/
{
    ERROR_CODE ec = S_OK;
    
    EXIT_IF_FAILED(m_RouterServer->Start(),
                   Cleanup);
    
Cleanup:
    return ec;
}

ERROR_CODE
SlimeRouter::Stop()
/*++

Routine Description:

    Stops the SlimeRouter.

Arguments:

    None.

Return Value:

    S_OK on success, error otherwise.

--*/
{
    ERROR_CODE ec = S_OK;
    
    EXIT_IF_FAILED(m_RouterServer->Stop(),
                   Cleanup);
    
Cleanup:
    return ec;
}
