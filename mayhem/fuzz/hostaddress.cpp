// HostAddress parsing of user/peer supplied host strings ("host:port", IPv4/IPv6/FQDN).
// convert()/toFQDN() are avoided because they perform DNS lookups.
#include "HostAddress.h"

extern "C" int LLVMFuzzerTestOneInput(const char* data, size_t size)
{
	const auto s = QString::fromUtf8(data, static_cast<qsizetype>(size));

	HostAddress address(s);
	(void) address.type();
	(void) HostAddress::parseHost(s);
	(void) HostAddress::parsePortNumber(s);

	return 0;
}
