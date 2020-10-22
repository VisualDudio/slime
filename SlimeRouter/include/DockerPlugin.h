/*++

Module Name:

    DockerPlugin.h

Abstract:

    Class definition of a docker network driver plugin.

--*/

#pragma once

//
// ---------------------------------------------------------------------- Includes
//

#include <memory>

#include "IMembershipProtocol.h"

//
// ---------------------------------------------------------------------- Definitions
//


//
// ---------------------------------------------------------------------- Classes
//

namespace httplib
{
    class Server;
}

class DockerPlugin final : public IMembershipProtocol
{
public:
    // Constructor
    DockerPlugin(std::unique_ptr<httplib::Server> HttpServer);

    // Destructor
    ~DockerPlugin();

    //
    // Public Methods
    //

    ERROR_CODE
    Init() override;

    ERROR_CODE
    Start() override;

    ERROR_CODE
    Stop() override;
    
private:
    std::unique_ptr<httplib::Server> m_HttpServer;
};
