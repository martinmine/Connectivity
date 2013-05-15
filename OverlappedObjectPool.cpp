#include "ConnectionManager.h"
#include "Stdafx.h"


void Connectivity::OverlappedObjectPool::IntPool(const int size)
{
	if (initialized)
		return;

	initialized = true;
	storage = new std::queue<LPWSAOVERLAPPED>();
	syncroot = gcnew Object();

	for (int i = 0; i < size; i++)
	{
		storage->push(new _OVERLAPPED());
	}

	//Console::WriteLine("Initialized size with {0} items", size);
};

LPWSAOVERLAPPED Connectivity::OverlappedObjectPool::GetOverlapped()
{
	if (storage->size() == 0)
	{
		objectsInCycling++;
		balance--;
		//Console::WriteLine("Out of overlapped objects, items in use: {0} pool: {1}", objectsInCycling, System::Threading::Thread::CurrentThread->IsThreadPoolThread);
		return new _OVERLAPPED();
	}

	Monitor::Enter(syncroot);

	__try
	{
		return storage->front();
	}
	__finally
	{
		storage->pop();
		balance--;
		//Console::WriteLine("Successfully taken item, balance: {0} pool: {1}", balance, System::Threading::Thread::CurrentThread->IsThreadPoolThread);
		Monitor::Exit(syncroot);
	}
}

void Connectivity::OverlappedObjectPool::GiveObject(LPWSAOVERLAPPED overlapped)
{
	Monitor::Enter(syncroot);

	__try
	{
		storage->push(overlapped);
		balance++;
	}
	__finally
	{
		//Console::WriteLine("Added object, balance: {0}", balance);
		Monitor::Exit(syncroot);
	}
}