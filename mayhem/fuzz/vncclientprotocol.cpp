// Port of upstream tests/libfuzzer/core/vncclientprotocol/main.cpp; the state byte is reduced
// into the State range (an out-of-range enum load is UB in the harness, not in Veyon).
#include <QBuffer>

#include "VncClientProtocol.h"

class VncClientProtocolTest : public VncClientProtocol
{
public:
	VncClientProtocolTest(QIODevice* socket) :
		VncClientProtocol(socket, {})
	{
	}

	void init(char state)
	{
		setState(State(uint8_t(state) % StateCount));
	}
};


extern "C" int LLVMFuzzerTestOneInput(const char* data, size_t size)
{
	if (size < 3)
	{
		return 0;
	}

	QBuffer buffer;
	buffer.open(QIODevice::ReadWrite);

	VncClientProtocolTest protocol(&buffer);

	const auto mode = data[0];
	const auto state = data[1];

	protocol.init(state);

	buffer.write(QByteArray::fromRawData(data + 2, size - 2));
	buffer.seek(0);

	if (mode)
	{
		protocol.read();
	}
	else
	{
		protocol.receiveMessage();
	}

	return 0;
}
