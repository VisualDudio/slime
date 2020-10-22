/*++

Module Name:

    BlockingQueue.tpp

Abstract:

    Class implementation of a thread-safe queue.

--*/

//
// ---------------------------------------------------------------------- Includes
//

#include "BlockingQueue.h"

//
// ---------------------------------------------------------------------- Definitions
//



//
// ---------------------------------------------------------------------- Functions
//

template<typename T>
BlockingQueue<T>::BlockingQueue()
/*++

Routine Description:

    Constructor for BlockingQueue.

Arguments:

    None.

Return Value:

    None.

--*/
    :
    m_ShouldShutdown(false)
{
}

template<typename T>
BlockingQueue<T>::~BlockingQueue()
/*++

Routine Description:

    Destructor for BlockingQueue.

Arguments:

    None.

Return Value:

    None.

--*/
{
}

template<typename T>
ERROR_CODE
BlockingQueue<T>::Push(const T& Item)
/*++

Routine Description:

    Pushes an element of type T onto the queue.

Arguments:

    Item - The item to push onto the queue.

Return Value:

    S_OK on success, error otherwise.

--*/
{
    ERROR_CODE ec = S_OK;

    {
		std::unique_lock<std::mutex> lck(m_Mutex);
		m_Queue.push(Item);
	}

	m_CanPop.notify_one();
    
    return ec;
}


template<typename T>
T
BlockingQueue<T>::Pop()
/*++

Routine Description:

    Pops an item from the queue. Blocks if the queue is empty.

Arguments:

    None.

Return Value:

    The element that was popped from the queue.

--*/
{
	std::unique_lock<std::mutex> lck(m_Mutex);

	m_CanPop.wait(lck, [=] { return !m_Queue.empty(); });

	T item = std::move(m_Queue.front());
	
	m_Queue.pop();

	return item;
}


template<typename T>
ERROR_CODE
BlockingQueue<T>::RequestShutdown()
/*++

Routine Description:

    Unblocks all currently blocked threads and performs cleanup.

Arguments:

    None.

Return Value:

    S_OK on success, error otherwise.

--*/
{
    ERROR_CODE ec = 0;
    
    {
		std::unique_lock<std::mutex> lock(m_Mutex);
		m_ShouldShutdown = true;
	}

	m_CanPop.notify_all();
    
Cleanup:
    return ec;
}
