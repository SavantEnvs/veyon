// `veyon-cli config import <file>` (cli/src/ConfigCommands.cpp handle_import): the file is loaded
// through Configuration::JsonStore into a VeyonConfiguration. Here the configuration is a fresh
// object per input (not merged into VeyonCore::config(), which would carry state across inputs)
// and is then exported again with JsonStore::flush() as `config export` does.
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include "Configuration/JsonStore.h"
#include "VeyonConfiguration.h"

extern "C" int LLVMFuzzerTestOneInput(const char* data, size_t size)
{
	static QTemporaryDir tmpDir(QDir::tempPath() + QStringLiteral("/veyon-configimport-XXXXXX"));
	if (tmpDir.isValid() == false)
	{
		return -1;
	}

	const auto inPath = tmpDir.filePath(QStringLiteral("import.json"));
	const auto outPath = tmpDir.filePath(QStringLiteral("export.json"));
	{
		QFile f(inPath);
		if (f.open(QFile::WriteOnly | QFile::Truncate) == false)
		{
			return -1;
		}
		f.write(data, qint64(size));
	}

	{
		Configuration::JsonStore store(Configuration::Store::Scope::System, inPath);
		VeyonConfiguration config(&store);
		(void) config.data();

		Configuration::JsonStore(Configuration::Store::Scope::System, outPath).flush(&config);
	}

	QFile::remove(inPath);
	QFile::remove(outPath);

	return 0;
}
