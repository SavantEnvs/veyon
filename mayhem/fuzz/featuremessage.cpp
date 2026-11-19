// FeatureMessage::receive(): the message every Veyon server/worker reads from a peer
// (VariantArrayMessage length prefix + VariantStream-checked QDataStream QVariants).
#include <QBuffer>

#include "FeatureMessage.h"
#include "VeyonCore.h"

extern "C" int LLVMFuzzerTestOneInput(const char* data, size_t size)
{
	QBuffer buffer;
	buffer.open(QIODevice::ReadWrite);
	buffer.write(QByteArray::fromRawData(data, size));
	buffer.seek(0);

	for (int i = 0; i < 16; ++i)
	{
		FeatureMessage message;
		if (message.receive(&buffer) == false)
		{
			break;
		}
		(void) message.featureUid().toString();
		(void) message.command();
		(void) VeyonCore::stringify(message.arguments());
	}

	return 0;
}
