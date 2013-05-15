using namespace System;

namespace Connectivity  {
  public interface class IDataParser : IDisposable, ICloneable  {
    void handlePacketData(array<unsigned char> ^ packet, int bytesReceived);
  };
}