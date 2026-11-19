// Veyon server authentication (server/src/ServerAuthenticationManager.cpp): the messages a
// connecting client sends after choosing a Veyon auth type. Byte 0 selects the auth type
// (KeyFile/Logon/Token), byte 1 the client's current auth state; the rest is the client's
// VariantArrayMessage as read from the socket.
#include "CryptoCore.h"
#include "FuzzSocket.h"
#include "ServerAuthenticationManager.h"
#include "VariantArrayMessage.h"
#include "VncServerClient.h"

static const CryptoCore::PrivateKey& serverPrivateKey()
{
	// what performLogonAuthentication() stores in the client on Init; generated once, read-only after
	static const auto key = CryptoCore::KeyGenerator().createRSA(CryptoCore::RsaKeySize);
	return key;
}

extern "C" int LLVMFuzzerTestOneInput(const char* data, size_t size)
{
	if (size < 2)
	{
		return -1;
	}

	static const RfbVeyonAuth::Type authTypes[] = { RfbVeyonAuth::KeyFile, RfbVeyonAuth::Logon, RfbVeyonAuth::Token };
	const auto authType = authTypes[uint8_t(data[0]) % 3];
	const auto authState = VncServerClient::AuthState(uint8_t(data[1]) % 6);

	FuzzSocket socket(QByteArray(data + 2, qsizetype(size - 2)));

	VncServerClient client;
	client.setAuthType(authType);
	client.setAuthState(authState);
	client.setUsername(QStringLiteral("fuzz"));
	client.setChallenge(QByteArray(CryptoCore::ChallengeSize, 'c'));
	if (authType == RfbVeyonAuth::Logon && authState == VncServerClient::AuthState::Password)
	{
		client.setPrivateKey(serverPrivateKey());
	}

	ServerAuthenticationManager manager(nullptr);

	// as VncServerProtocol::receiveAuthenticationMessage() does
	VariantArrayMessage message(&socket);
	if (message.receive())
	{
		manager.processAuthenticationMessage(&client, message);
	}

	return 0;
}
