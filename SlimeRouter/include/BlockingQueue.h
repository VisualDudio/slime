/*++

Module Name:

    BlockingQueue.h

Abstract:

    Class definition for a thread-safe queue.

--*/

#pragma once

//
// ---------------------------------------------------------------------- Includes
//

#include <thread>
#include <queue>
#include <condition_variable>

#include "Error.h"

//
// ---------------------------------------------------------------------- Definitions
//


//
// ---------------------------------------------------------------------- Classes
//

template<typename T>
class BlockingQueue
{
public:
    // Constructor
    BlockingQueue();

    // Destructor
    ~BlockingQueue();

    //
    // Public Methods
    //
    
    ERROR_CODE
    Push(const T& Item);

    T
    Pop();
    
    ERROR_CODE
    RequestShutdown();
    
private:
	std::mutex m_Mutex;
	std::condition_variable m_CanPop;
	std::queue<T> m_Queue;
	bool m_ShouldShutdown;
};

#include "../src/BlockingQueue.tpp"
