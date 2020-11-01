#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <iostream>
#include <arpa/inet.h>

#include "Error.h"

namespace NetworkUtils
{
    int
    WriteAllToSocket(int Socket,
                     const void* Data,
                     size_t Count);

    int
    ReadAllFromSocket(int Socket,
                      void* Data,
                      size_t Count);

    ERROR_CODE
    ReadFileDescriptorFromUnixSocket(int Socket,
                                     int* FileDescriptor);

    ERROR_CODE
    WriteFileDescriptorToUnixSocket(int Socket,
                                    int FileDescriptor);

    ERROR_CODE
    GetIpAddress(uint32_t* IpAddress);
}
