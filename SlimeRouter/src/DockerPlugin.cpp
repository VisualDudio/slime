/*++

Module Name:

    DockerPlugin.cpp

Abstract:

    Class implementation of a docker network driver plugin.

--*/

//
// ---------------------------------------------------------------------- Includes
//

#include <iostream>

#include "DockerPlugin.h"
#include "httplib.h"
#include "json.hpp"
#include "IMembershipManager.h"

//
// ---------------------------------------------------------------------- Definitions
//

#define DOCKER_PLUGIN_PATH "/run/docker/plugins/slime.sock"

//
// ---------------------------------------------------------------------- Functions
//

DockerPlugin::DockerPlugin(std::unique_ptr<httplib::Server> HttpServer)
/*++

Routine Description:

    Constructor for DockerPlugin.

Arguments:

    None.

Return Value:

    None.

--*/
    :
    m_HttpServer(std::move(HttpServer))
{}

DockerPlugin::~DockerPlugin()
/*++

Routine Description:

    Destructor for DockerPlugin.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Stop();
}


ERROR_CODE
DockerPlugin::Init(IMembershipManager& MembershipManager)
/*++

Routine Description:

    Initializes DockerPlugin.

Arguments:

    None.

Return Value:

    S_OK on success, error otherwise.

--*/
{
    ERROR_CODE ec = S_OK;

    m_HttpServer->Post("/Plugin.Activate", [](const httplib::Request&, httplib::Response& Response)
    {
        Response.set_content("{\"Implements\":[\"NetworkDriver\"]}", "application/json");
    });

    m_HttpServer->Post("/NetworkDriver.GetCapabilities", [](const httplib::Request&, httplib::Response& Response)
    {
        Response.set_content("{\"Scope\":\"global\",\"ConnectivityScope\":\"global\"}", "application/json");
    });

    m_HttpServer->Post("/NetworkDriver.CreateNetwork", [](const httplib::Request& Request, httplib::Response& Response)
    {
        nlohmann::json body = nlohmann::json::parse(Request.body);
        std::cout << body << std::endl;
        Response.set_content("{}", "application/json");
    });

    m_HttpServer->Post("/NetworkDriver.DeleteNetwork", [](const httplib::Request& Request, httplib::Response& Response)
    {
        nlohmann::json body = nlohmann::json::parse(Request.body);
        std::cout << body << std::endl;
        Response.set_content("{}", "application/json");
    });

    m_HttpServer->Post("/NetworkDriver.CreateEndpoint", [](const httplib::Request& Request, httplib::Response& Response)
    {
        nlohmann::json body = nlohmann::json::parse(Request.body);
        std::cout << body << std::endl;
        Response.set_content("{}", "application/json");
    });

    m_HttpServer->Post("/NetworkDriver.DeleteEndpoing", [](const httplib::Request& Request, httplib::Response& Response)
    {
        nlohmann::json body = nlohmann::json::parse(Request.body);
        std::cout << body << std::endl;
        Response.set_content("{}", "application/json");
    });

    m_HttpServer->Post("/NetworkDriver.Join", [](const httplib::Request& Request, httplib::Response& Response)
    {
        nlohmann::json body = nlohmann::json::parse(Request.body);
        std::cout << body << std::endl;
        Response.set_content("{}", "application/json");
    });

    m_HttpServer->Post("/NetworkDriver.Leave", [](const httplib::Request& Request, httplib::Response& Response)
    {
        nlohmann::json body = nlohmann::json::parse(Request.body);
        std::cout << body << std::endl;
        Response.set_content("{}", "application/json");
    });

    m_HttpServer->Post("/NetworkDriver.DiscoverNew", [&MembershipManager](const httplib::Request& Request, httplib::Response& Response)
    {
        nlohmann::json body = nlohmann::json::parse(Request.body);
        std::cerr << body << std::endl;
        MembershipManager.OnJoin(body["DiscoveryData"]["Address"]);
        Response.set_content("{}", "application/json");
    });

    m_HttpServer->Post("/NetworkDriver.DiscoverDelete", [&MembershipManager](const httplib::Request& Request, httplib::Response& Response)
    {
        nlohmann::json body = nlohmann::json::parse(Request.body);
        std::cerr << body << std::endl;
        MembershipManager.OnLeave(body["DiscoveryData"]["Address"]);
        Response.set_content("{}", "application/json");
    });
    
    return ec;
}

ERROR_CODE
DockerPlugin::Start()
/*++

Routine Description:

    Starts the docker plugin.

Arguments:

    None.

Return Value:

    S_OK on success, error otherwise.

--*/
{
    ERROR_CODE ec = S_OK;
    
    std::thread t([=] { m_HttpServer->listen(DOCKER_PLUGIN_PATH, 1); });
    t.detach();

    return ec;
}

ERROR_CODE
DockerPlugin::Stop()
/*++

Routine Description:

    Stops the docker plugin.

Arguments:

    None.

Return Value:

    S_OK on success, error otherwise.

--*/
{
    ERROR_CODE ec = S_OK;
    
    return ec;
}
