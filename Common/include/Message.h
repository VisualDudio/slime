#pragma once

#include <cstdint>
#include <vector>

struct MessageHeader
{
    int Id;
    uint32_t Size;
};

struct Message
{
    MessageHeader Header;
    std::vector<uint8_t> Body;
};

// Application Messages

struct SocketRequest
{
    int Domain;
    int Type;
    int Protocol;
};

struct SocketResponse
{
    int Status;
};

struct BindRequest
{
    uint32_t VirtualIpAddress;
    uint16_t VirtualPort;
};

struct BindResponse
{
    int Status;
};

struct AcceptRequest
{
    int Flags;
};

struct AcceptResponse
{
    int Status;
};

struct ConnectRequest
{
    uint32_t VirtualIpAddress;
    uint16_t VirtualPort;
};

struct ConnectResponse
{
    int Status;
};

enum MessageType
{
    MESSAGE_TYPE_NEW_MAPPING = 0,
    MESSAGE_TYPE_DELETE_MAPPING = 1
};

enum RequestType
{
    REQUEST_TYPE_SOCKET = 0,
    REQUEST_TYPE_BIND = 1,
    REQUEST_TYPE_ACCEPT = 2,
    REQUEST_TYPE_CONNECT = 3,
    REQUEST_TYPE_MAX
};

struct AddressMapping
{
    uint32_t VirtualIpAddress;
    uint32_t HostIpAddress;
    uint16_t VirtualPort;
    uint16_t HostPort;
};

struct Address
{
    uint32_t IpAddress;
    uint16_t Port;
};

// Gossip Messages

/* struct GossipHeader */
/* { */
/*     uint16_t PushGossipCount; */
/*     std::vector<PushGossip> PushGossips; */
/*     uint16_t PullGossipCount; */
/*     std::vector<PullGossip> PullGossips; */
/* }; */

enum class EventType
{
    NewMember,
    DeleteMember,
    NewMessage,
    Send
};

struct NewMemberEvent
{
    int32_t IpAddress;
    uint16_t Port;
};

struct DeleteMemberEvent
{
    uint32_t IpAddress;
    uint16_t Port;
};

struct NewMessageEvent
{
    std::vector<uint8_t> Payload;
};

struct SendEvent
{
    int Fanout;
};
