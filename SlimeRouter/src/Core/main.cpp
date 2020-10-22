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
    std::unique_ptr<SlimeRouter> slimeRouter = nullptr;

    slimeRouter = std::make_unique<SlimeRouter>();
    slimeRouter->Init();
    slimeRouter->Start();
}
