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
    EXIT_IF_FAILED(slimeRouter->Init(), Cleanup);
    EXIT_IF_FAILED(slimeRouter->Start(), Cleanup);

Cleanup:
    return ec;
}
