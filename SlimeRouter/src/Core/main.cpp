/*++

Module Name:

    main.cpp

Abstract:

    Entry point for SlimeRouter.

--*/

//
// ---------------------------------------------------------------------- Includes
//

#include <memory>
#include <thread>
#include <chrono>
#include <iostream>

#include "SlimeRouter.h"

//
// ---------------------------------------------------------------------- Definitions
//


//
// ---------------------------------------------------------------------- Functions
//

int main(void)
{
    ERROR_CODE ec = S_OK;
    std::unique_ptr<SlimeRouter> slimeRouter = nullptr;

    slimeRouter = std::make_unique<SlimeRouter>();
    TRACE_IF_FAILED(slimeRouter->Init(),
                    Cleanup,
                    "Failed to initialize SlimeRouter 0x%x.\n", ec);
    TRACE_IF_FAILED(slimeRouter->Start(),
                    Cleanup,
                    "Failed to start SlimeRouter 0x%x.\n", ec);

    while (1)
    {
        std::cout << "Running..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(60000));
    }

Cleanup:
    return ec;
}
