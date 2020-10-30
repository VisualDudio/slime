#pragma once

#include <stdio.h>
#include <cstdint>

#define S_FALSE 1
#define S_OK 0
#define E_FAIL -1
#define E_INVALIDARG -2
#define E_OUTOFMEMORY -3

#define UNREFERENCED_PARAMETER(p) ((void)(p));
#define SUCCEEDED(e) ((e) >= S_OK)
#define FAILED(e) (!SUCCEEDED(e))

#define EXIT_IF_FAILED(e, l)                    \
    do                                          \
    {                                           \
        if (FAILED((ec = (e))))                 \
        {                                       \
            goto l;                             \
        }                                       \
    }                                           \
    while (0)

#define EXIT_IF_NULL(p, r, l)                   \
    do                                          \
    {                                           \
        if ((p) == NULL)                        \
        {                                       \
            ec = (r);                           \
            goto l;                             \
        }                                       \
    }                                           \
    while (0)

#define EXIT_IF_TRUE(e, r, l)                   \
    do                                          \
    {                                           \
        if ((e) == true)                        \
        {                                       \
            ec = (r);                           \
            goto l;                             \
        }                                       \
    }                                           \
    while (0)

#define EXIT_IF_FALSE(e, r, l) EXIT_IF_TRUE(!(e), (r), l)

#define TRACE_IF_FAILED(e, l, fmt, ...)         \
    do                                          \
    {                                           \
        if (FAILED((ec = (e))))                 \
        {                                       \
            fprintf(stderr, fmt, __VA_ARGS__);  \
            goto l;                             \
        }                                       \
    }                                           \
    while (0)

typedef int32_t ERROR_CODE;
