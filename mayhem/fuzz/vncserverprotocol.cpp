// Port of upstream tests/libfuzzer/core/vncserverprotocol/main.cpp: VncServerProtocol now takes a
// QTcpSocket*, so the input is served through FuzzSocket. Bytes 0-3 select the auth type and the
// client's protocol/auth/access-control states, the rest is the client's RFB stream.
#include "FuzzSocket.h"
#include "VncServerClient.h"
#include "VncServerProtocol.h"

class VncServerProtocolTest : public VncServerProtocol
{
public:
	VncServerProtocolTest(RfbVeyonAuth::Type authMethod, QTcpSocket* socket, VncServerClient* client) :
		VncServerProtocol(socket, client),
		m_authMethod(authMethod)
	{
	}

	QVector<RfbVeyonAuth::Type> supportedAuthTypes() const override
	{
		return {m_authMethod};
	}

	void processAuthenticationMessage(VariantArrayMessage& message) override
	{
		Q_UNUSED(message)
	}

	void performAccessControl() override
	{
	}

private:
	const RfbVeyonAuth::Type m_authMethod;
};


extern "C" int LLVMFuzzerTestOneInput(const char* data, size_t size)
{
	if (size < 5)
	{
		return 0;
	}

	FuzzSocket socket(QByteArray(data + 4, qsizetype(size - 4)));

	VncServerClient client;
	// enum values are reduced into range: an out-of-range enum load is UB in the harness itself
	VncServerProtocolTest protocol(RfbVeyonAuth::Type(uint8_t(data[0]) % 6), &socket, &client);

	client.setProtocolState(VncServerProtocol::State(uint8_t(data[1]) % int(VncServerProtocol::State::StateCount)));
	client.setAuthState(VncServerClient::AuthState(uint8_t(data[2]) % 6));
	client.setAccessControlState(VncServerClient::AccessControlState(uint8_t(data[3]) % 5));

	// drive the state machine as the server's readyRead handler would, until it stops consuming
	for (int i = 0; i < 64 && socket.isOpen(); ++i)
	{
		const auto before = socket.bytesAvailable();
		if (protocol.read() == false && socket.bytesAvailable() == before)
		{
			break;
		}
	}

	return 0;
}
