// Ported from tests/libfuzzer/core/variantarraymessage/main.cpp. Upstream's harness stops after
// VariantArrayMessage::receive(), which only reads the 4-byte big-endian size and copies the payload.
// Every real consumer (FeatureMessage::receive, VncServerProtocol/VncClientProtocol auth handshakes)
// then decodes the payload with read() until it has its fields, so this port does the same: after a
// successful receive() it reads variants until the VariantStream decoder rejects one (bounded).
#include <QBuffer>

#include "VariantArrayMessage.h"

extern "C" int LLVMFuzzerTestOneInput(const char* data, size_t size)
{
	QBuffer buffer;
	buffer.open(QIODevice::ReadWrite);
	buffer.write(QByteArray::fromRawData(data, size));
	buffer.seek(0);

	VariantArrayMessage message(&buffer);
	if (message.receive() == false)
	{
		return 0;
	}

	for (int i = 0; i < 64; ++i)
	{
		const auto v = message.read();
		if (v.isValid() == false)
		{
			break;
		}
		(void) v.toString();
	}

	return 0;
}
