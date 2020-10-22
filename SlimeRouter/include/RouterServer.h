/*++

Module Name:

    RouterServer.h

Abstract:

    Class definition of a RouterServer.

--*/

#pragma once

//
// ---------------------------------------------------------------------- Includes
//

#include <memory>

#include "UnixServer.h"
#include "Message.h"

//
// ---------------------------------------------------------------------- Definitions
//



//
// ---------------------------------------------------------------------- Classes
//


class MappingManager;

class RouterServer final : public UnixServer
{
public:
    
    // Constructor
    RouterServer(std::string Path,
                 std::unique_ptr<MappingManager> MappingManager);

    // Destructor
    ~RouterServer();

    //
    // Public Methods
    //
    
    ERROR_CODE
    Init();

    ERROR_CODE
    Start();

    ERROR_CODE
    Stop();
    
protected:
    
    ERROR_CODE
    OnMessage(int ClientSocket,
              const Message& Message) override;
private:
    
    ERROR_CODE
    ProcessSocketRequest(int ClientSocket,
                         const SocketRequest& SocketRequest);

    ERROR_CODE
    ProcessBindRequest(int ClientSocket,
                       const BindRequest& BindRequest);

    ERROR_CODE
    ProcessAcceptRequest(int ClientSocket,
                         const AcceptRequest& AcceptRequest);

    ERROR_CODE
    ProcessConnectRequest(int ClientSocket,
                          const ConnectRequest& ConnectRequest);
    
    // Owning pointer to a MappingManager
    std::unique_ptr<MappingManager> m_MappingManager;
};
