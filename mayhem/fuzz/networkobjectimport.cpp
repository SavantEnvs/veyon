// `veyon-cli networkobjects import FILE format|regex SPEC`: BuiltinDirectoryPlugin::handle_import()
// (plugins/builtindirectory) reads an operator- or third-party-supplied text/CSV file line by line and
// matches each line against a regular expression built from the user's format string (or a raw
// regex with %placeholder% captures), turning matches into network objects that are merged into the
// configuration (importFile -> toNetworkObject -> ObjectManager).
//
// Input layout: the first line is the import spec, the rest is the file content. A spec starting with
// '~' is passed as `regex` (the rest of the line), otherwise the whole line is passed as `format`.
// The plugin is invoked exactly the way cli/src/main.cpp does it (looked up in the plugin manager by
// its command-line module name, handle_import via QMetaObject::invokeMethod).
//
// State: the imported objects land in the process-global VeyonCore::config(); its
// BuiltinDirectory/NetworkObjects key is reset to empty before every input. saveConfiguration() then
// tries the system LocalStore, which is not writable for the unprivileged fuzzing user, so nothing is
// persisted. The input file lives in a per-process QTemporaryDir and is removed after each input.
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include "BuiltinDirectoryConfiguration.h"
#include "CommandLinePluginInterface.h"
#include "PluginManager.h"
#include "VeyonConfiguration.h"

// defined in BuiltinDirectoryPlugin.cpp (inside the plugin module); needed here to reset the key
IMPLEMENT_CONFIG_PROXY(BuiltinDirectoryConfiguration)

static QObject* findImportPlugin()
{
	const auto pluginObjects = VeyonCore::pluginManager().pluginObjects();
	for (auto pluginObject : pluginObjects)
	{
		auto commandLinePluginInterface = qobject_cast<CommandLinePluginInterface*>(pluginObject);
		if (commandLinePluginInterface &&
			commandLinePluginInterface->commandLineModuleName() == QLatin1String("networkobjects"))
		{
			return pluginObject;
		}
	}
	qFatal("networkobjectimport: BuiltinDirectory plugin not loaded");
	return nullptr;
}

extern "C" int LLVMFuzzerTestOneInput(const char* data, size_t size)
{
	static QObject* plugin = findImportPlugin();
	static QTemporaryDir tmpDir(QDir::tempPath() + QStringLiteral("/veyon-networkobjectimport-XXXXXX"));
	if (tmpDir.isValid() == false)
	{
		return -1;
	}

	const QByteArray input(data, qsizetype(size));
	const auto newline = input.indexOf('\n');
	if (newline < 0)
	{
		return -1;
	}
	const auto spec = QString::fromUtf8(input.left(newline));
	const auto content = input.mid(newline + 1);

	const auto path = tmpDir.filePath(QStringLiteral("import.txt"));
	{
		QFile f(path);
		if (f.open(QFile::WriteOnly | QFile::Truncate) == false)
		{
			return -1;
		}
		f.write(content);
	}

	BuiltinDirectoryConfiguration(&VeyonCore::config()).setNetworkObjects(QJsonArray{});

	const QStringList arguments = spec.startsWith(QLatin1Char('~'))
		? QStringList{path, QStringLiteral("regex"), spec.mid(1)}
		: QStringList{path, QStringLiteral("format"), spec};

	auto runResult = CommandLinePluginInterface::Unknown;
	QMetaObject::invokeMethod(plugin, "handle_import", Qt::DirectConnection,
							  Q_RETURN_ARG(CommandLinePluginInterface::RunResult, runResult),
							  Q_ARG(QStringList, arguments));

	QFile::remove(path);

	return 0;
}
