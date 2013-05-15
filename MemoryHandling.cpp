#include "ConnectionManager.h"


//void Connectivity::AsyncOperationRefferance::Destroy()  {
//  try {
//    GCHandle handle = GCHandle::Alloc(result);
//  
//    IntPtr inptr = GCHandle::ToIntPtr(handle);
//    void * ptr = (void*)inptr;
//
//    delete ptr;
//    delete &handle; //HAHAHAHAHAHAHAHAHAHA
//  } catch (Exception ^ex)
//  {
//    Console::WriteLine("Unable to remove memory: " + ex->ToString());
//  }
//}
//
//void Connectivity::AsyncOperationRefferance::SetRefferance(IAsyncResult ^ result)  {
//  if (result == nullptr)  
//    Console::WriteLine("argnull");
//  this->result = result; 
//}

/*

  private:
    IAsyncResult ^ result;
  internal:
    void Destroy() { delete result; }
    void SetRefferance(IAsyncResult ^ result) { this->result = result; };
*/