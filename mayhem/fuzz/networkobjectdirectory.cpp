// The builtin network-object directory as fed by an imported configuration file (`veyon-cli
// config import`): Configuration::JsonStore -> BuiltinDirectoryConfiguration::networkObjects ->
// BuiltinDirectory::update() (plugins/builtindirectory), then the lookups the master and access
// control run over it: NetworkObjectDirectory::queryObjects() and queryParents() (parent-UID chains,
// possibly cyclic) for every object, as AccessControlProvider::locationsOfComputer() does.
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include "BuiltinDirectory.h"
#include "BuiltinDirectoryConfiguration.h"
#include "Configuration/JsonStore.h"
#include "VeyonConfiguration.h"

// defined in BuiltinDirectoryPlugin.cpp, which is not compiled into this harness
IMPLEMENT_CONFIG_PROXY(BuiltinDirectoryConfiguration)

extern "C" int LLVMFuzzerTestOneInput(const char* data, size_t size)
{
	static QTemporaryDir tmpDir(QDir::tempPath() + QStringLiteral("/veyon-networkobjectdirectory-XXXXXX"));
	if (tmpDir.isValid() == false)
	{
		return -1;
	}

	const auto path = tmpDir.filePath(QStringLiteral("config.json"));
	{
		QFile f(path);
		if (f.open(QFile::WriteOnly | QFile::Truncate) == false)
		{
			return -1;
		}
		f.write(data, qint64(size));
	}

	{
		Configuration::JsonStore store(Configuration::Store::Scope::System, path);
		VeyonConfiguration config(&store);
		BuiltinDirectoryConfiguration directoryConfig(&config);
		BuiltinDirectory directory(directoryConfig, nullptr);

		directory.update();

		const auto hosts = directory.queryObjects(NetworkObject::Type::Host, NetworkObject::Attribute::None, {});
		const auto locations = directory.queryObjects(NetworkObject::Type::Location, NetworkObject::Attribute::None, {});
		for (const auto& list : {hosts, locations})
		{
			for (const auto& object : list)
			{
				(void) directory.queryParents(object);
				(void) directory.queryObjects(NetworkObject::Type::Host, NetworkObject::Attribute::Name, object.name());
				(void) directory.queryObjects(NetworkObject::Type::Host, NetworkObject::Attribute::HostAddress, object.hostAddress());
			}
		}
	}

	QFile::remove(path);

	return 0;
}
