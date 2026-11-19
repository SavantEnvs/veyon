// A QTcpSocket that never touches the network: reads come from a byte array (the fuzz input),
// writes are discarded. VncServerProtocol and friends take a QTcpSocket* since upstream changed
// the constructor (tests/libfuzzer/core/vncserverprotocol still passes a QIODevice*).
#pragma once

#include <QTcpSocket>

class FuzzSocket : public QTcpSocket
{
public:
	explicit FuzzSocket(const QByteArray& data) : m_data(data)
	{
		setOpenMode(QIODevice::ReadWrite | QIODevice::Unbuffered);
	}

	qint64 bytesAvailable() const override
	{
		return (m_data.size() - m_pos) + QIODevice::bytesAvailable();
	}

	void close() override
	{
		QIODevice::close();
	}

	bool atEnd() const override
	{
		return bytesAvailable() <= 0;
	}

protected:
	qint64 readData(char* data, qint64 maxSize) override
	{
		const auto n = qMin<qint64>(maxSize, m_data.size() - m_pos);
		if (n <= 0)
		{
			return isOpen() ? 0 : -1;
		}
		memcpy(data, m_data.constData() + m_pos, size_t(n));
		m_pos += n;
		return n;
	}

	qint64 readLineData(char* data, qint64 maxSize) override
	{
		return QIODevice::readLineData(data, maxSize);
	}

	qint64 writeData(const char* data, qint64 len) override
	{
		Q_UNUSED(data)
		return len;
	}

private:
	const QByteArray m_data;
	qint64 m_pos{0};
};
