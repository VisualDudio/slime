#include "NetworkUtils.h"

int NetworkUtils::WriteAllToSocket(int Socket, const void* Data, size_t Count)
{
    ssize_t totalBytes = 0;
    ssize_t bytesWritten = 0;
    
    while (totalBytes < (ssize_t)Count)
    {
        bytesWritten = send(Socket, (char*)Data + totalBytes, Count - totalBytes, MSG_NOSIGNAL);

        if (bytesWritten > 0)
        {
            totalBytes += bytesWritten;
        }
        else if (bytesWritten == -1 && errno == EINTR)
        {
            continue;
        }
        else
        {
            LOG("Failed to write to socket! 0x%x\n", errno);
            break;
        }
    }

    return totalBytes;
}

int NetworkUtils::ReadAllFromSocket(int Socket, void* Data, size_t Count)
{
    ssize_t totalBytes = 0;
    ssize_t bytesRead = 0;
    
    while (totalBytes < (ssize_t)Count)
    {
        bytesRead = read(Socket, (char*)Data + totalBytes, Count - totalBytes);

        if (bytesRead > 0)
        {
            totalBytes += bytesRead;
        }
        else if (bytesRead == -1 && errno == EINTR)
        {
            continue;
        }
        else
        {
            LOG("Failed to read from socket! 0x%x\n", errno);
            break;
        }
    }

    return totalBytes;
}

ERROR_CODE
NetworkUtils::GetIpAddress(uint32_t* IpAddress)
{
    int ec = 0;
    char hostname_buffer[256];
    struct hostent* host_entry;

    TRACE_IF_FAILED(gethostname(hostname_buffer, sizeof(hostname_buffer)),
                    Cleanup,
                    "Failed to get hostname! 0x%x\n", errno);
    
    host_entry = gethostbyname(hostname_buffer);
    EXIT_IF_NULL(host_entry,
                 E_FAIL,
                 Cleanup);
    
    *IpAddress = ((struct in_addr*)host_entry->h_addr_list[0])->s_addr;
    
Cleanup:
    return ec;
}

ERROR_CODE
NetworkUtils::ReadFileDescriptorFromUnixSocket(int Socket, int* FileDescriptor)
{
    ERROR_CODE ec = S_OK;
    struct msghdr msg;
    struct iovec iov;
    union
    {
        struct cmsghdr cmsghdr;
        char control[CMSG_SPACE(sizeof (int))];
    } cmsgu;
    struct cmsghdr* cmsg = {0};
    char buf[2] = {0};

    iov.iov_base = buf;
    iov.iov_len = 2;

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = cmsgu.control;
    msg.msg_controllen = sizeof(cmsgu.control);
    
    TRACE_IF_FAILED(recvmsg(Socket, &msg, 0),
                    Cleanup,
                    "Failed to receive control information! 0x%x\n", errno);
    
    cmsg = CMSG_FIRSTHDR(&msg);
    
    if (cmsg && cmsg->cmsg_len == CMSG_LEN(sizeof(int)))
    {
        EXIT_IF_TRUE(cmsg->cmsg_level != SOL_SOCKET,
                     E_FAIL,
                     Cleanup);
        
        EXIT_IF_TRUE(cmsg->cmsg_type != SCM_RIGHTS,
                     E_FAIL,
                     Cleanup);
        
        *FileDescriptor = *(int*)CMSG_DATA(cmsg);
    }
    else
    {
        *FileDescriptor = -1;
    }

Cleanup:
    return ec;
}

ERROR_CODE
NetworkUtils::WriteFileDescriptorToUnixSocket(int Socket, int FileDescriptor)
{
    ERROR_CODE ec = S_OK;
    struct msghdr msg;
    struct iovec iov;
    union
    {
        struct cmsghdr cmsghdr;
        char control[CMSG_SPACE(sizeof(int))];
    } cmsgu;
    struct cmsghdr* cmsg = nullptr;
    char buf[2] = {0};

    iov.iov_base = buf;
    iov.iov_len = 2;

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    if (FileDescriptor != -1)
    {
        msg.msg_control = cmsgu.control;
        msg.msg_controllen = sizeof(cmsgu.control);

        cmsg = CMSG_FIRSTHDR(&msg);
        cmsg->cmsg_len = CMSG_LEN(sizeof(int));
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;

        *((int *)CMSG_DATA(cmsg)) = FileDescriptor;
    }
    else
    {
        msg.msg_control = NULL;
        msg.msg_controllen = 0;
    }

    TRACE_IF_FAILED(sendmsg(Socket, &msg, 0),
                    Cleanup,
                    "Failed to send control message! 0x%x\n", errno);
Cleanup:    
    return ec;
}
