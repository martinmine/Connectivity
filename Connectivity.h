// Connectivity.h

#pragma once

using namespace System;

namespace Connectivity {

	public ref class Class1
	{
		// TODO: Add your methods for this class here.
  public:
    void DoSomething();

	};

  class Test {
  private:
    int a, 
      b,
      c;
  public:
    Test();
  };


  void Class1 :: DoSomething()  {
    Test * k = new Test();

    delete k;

  }
}
