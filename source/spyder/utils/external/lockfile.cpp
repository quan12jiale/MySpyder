#include "utils/external/lockfile.h"
#include <QtCore/QFile>
#include <QtCore/QThread>
#include <QtCore/QElapsedtimer>
#include <QtCore/QDatetime>
#include <QtCore/QDebug>
#include <QtCore/QFileinfo>
#include <QtCore/QCoreapplication>
#if defined(Q_OS_WIN)
#include <Windows.h>
#endif

MyLockFile::MyLockFile(const QString &fn)
	: fileName(fn),
	fileHandle(INVALID_HANDLE_VALUE),
	m_nStaleLockTime(30 * 1000), // 30 seconds
	lockError(MyLockFile::NoError),
	m_bIsLocked(false)
{

}

MyLockFile::~MyLockFile()
{
	unlock();
}

void MyLockFile::setStaleLockTime(int staleLockTime)
{
	this->m_nStaleLockTime = staleLockTime;
}

int MyLockFile::staleLockTime() const
{
	return this->m_nStaleLockTime;
}

bool MyLockFile::isLocked() const
{
	return this->m_bIsLocked;
}

MyLockFile::LockError MyLockFile::error() const
{
	return this->lockError;
}

bool MyLockFile::lock()
{
	return tryLock(-1);
}

bool MyLockFile::tryLock(int timeout /*= 0*/)
{
	QElapsedTimer timer;
	if (timeout > 0)
		timer.start();
	int sleepTime = 100;
	forever {
		this->lockError = this->tryLock_sys();
		switch (this->lockError) {
		case NoError:
			this->m_bIsLocked = true;
			return true;
		case PermissionError:
		case UnknownError:
			return false;
		case LockFailedError:
			if (!this->m_bIsLocked && this->isApparentlyStale()) {
				// Stale lock from another thread/process
				// Ensure two processes don't remove it at the same time
				MyLockFile rmlock(this->fileName + QStringLiteral(".rmlock"));
				if (rmlock.tryLock()) {
					if (this->isApparentlyStale() && this->removeStaleLock())
						continue;
				}
			}
			break;
		}
		if (timeout == 0 || (timeout > 0 && timer.hasExpired(timeout)))
			return false;
		QThread::msleep(sleepTime);
		if (sleepTime < 5 * 1000)
			sleepTime *= 2;
	}
		// not reached
	return false;
}

void MyLockFile::unlock()
{
	if (!this->m_bIsLocked)
		return;
	CloseHandle(this->fileHandle);
	int attempts = 0;
	static const int maxAttempts = 500; // 500ms
	while (!QFile::remove(this->fileName) && ++attempts < maxAttempts) {
		// Someone is reading the lock file right now (on Windows this prevents deleting it).
		QThread::msleep(1);
	}
	if (attempts == maxAttempts) {
		qWarning() << "Could not remove our own lock file" << this->fileName << ". Either other users of the lock file are reading it constantly for 500 ms, or we (no longer) have permissions to delete the file";
		// This is bad because other users of this lock file will now have to wait for the stale-lock-timeout...
	}
	this->lockError = MyLockFile::NoError;
	this->m_bIsLocked = false;
}

bool MyLockFile::getLockInfo(qint64 *pid, QString *hostname, QString *appname) const
{
	QFile reader(fileName);
	if (!reader.open(QIODevice::ReadOnly))
		return false;

	QByteArray pidLine = reader.readLine();
	pidLine.chop(1);
	QByteArray appNameLine = reader.readLine();
	appNameLine.chop(1);
	QByteArray hostNameLine = reader.readLine();
	hostNameLine.chop(1);
	if (pidLine.isEmpty())
		return false;

	qint64 thePid = pidLine.toLongLong();
	if (pid)
		*pid = thePid;
	if (appname)
		*appname = QString::fromUtf8(appNameLine);
	if (hostname)
		*hostname = QString::fromUtf8(hostNameLine);
	return thePid > 0;
}

bool MyLockFile::removeStaleLockFile()
{
	if (this->m_bIsLocked) {
		qWarning("removeStaleLockFile can only be called when not holding the lock");
		return false;
	}
	return this->removeStaleLock();
}

/****************** LockFile ********************/
static inline QByteArray localHostName()
{
	return qgetenv("COMPUTERNAME");
}

static inline bool fileExists(const wchar_t *fileName)
{
	WIN32_FILE_ATTRIBUTE_DATA  data;
	return GetFileAttributesEx(fileName, GetFileExInfoStandard, &data);
}

MyLockFile::LockError MyLockFile::tryLock_sys()
{
	const std::wstring fileEntry = fileName.toStdWString();
	// When writing, allow others to read.
	// When reading, QFile will allow others to read and write, all good.
	// Adding FILE_SHARE_DELETE would allow forceful deletion of stale files,
	// but Windows doesn't allow recreating it while this handle is open anyway,
	// so this would only create confusion (can't lock, but no lock file to read from).
	const DWORD dwShareMode = FILE_SHARE_READ;

	SECURITY_ATTRIBUTES securityAtts = { sizeof(SECURITY_ATTRIBUTES), NULL, FALSE };
	HANDLE fh = CreateFile(fileEntry.c_str(), // (const wchar_t*)fileEntry.nativeFilePath().utf16()
		GENERIC_WRITE,
		dwShareMode,
		&securityAtts,
		CREATE_NEW, // error if already exists
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (fh == INVALID_HANDLE_VALUE) {
		const DWORD lastError = GetLastError();
		switch (lastError) {
		case ERROR_SHARING_VIOLATION:
		case ERROR_ALREADY_EXISTS:
		case ERROR_FILE_EXISTS:
			return MyLockFile::LockFailedError;
		case ERROR_ACCESS_DENIED:
			// readonly file, or file still in use by another process.
			// Assume the latter if the file exists, since we don't create it readonly.
			return fileExists(fileEntry.c_str())
				? MyLockFile::LockFailedError
				: MyLockFile::PermissionError;
		default:
			qWarning() << "Got unexpected locking error" << lastError;
			return MyLockFile::UnknownError;
		}
	}

	// We hold the lock, continue.
	fileHandle = fh;
	// Assemble data, to write in a single call to write
	// (otherwise we'd have to check every write call)
	QByteArray fileData;
	fileData += QByteArray::number(QCoreApplication::applicationPid());
	fileData += '\n';
	fileData += QCoreApplication::applicationName().toUtf8();
	fileData += '\n';
	fileData += localHostName();
	fileData += '\n';
	DWORD bytesWritten = 0;
	MyLockFile::LockError error = MyLockFile::NoError;
	if (!WriteFile(fh, fileData.constData(), fileData.size(), &bytesWritten, NULL) || !FlushFileBuffers(fh))
		error = MyLockFile::UnknownError; // partition full
	return error;
}

bool MyLockFile::removeStaleLock()
{
	// QFile::remove fails on Windows if the other process is still using the file, so it's not stale.
	return QFile::remove(fileName);
}

bool MyLockFile::isApparentlyStale() const
{
	qint64 pid;
	QString hostname, appname;

	if (getLockInfo(&pid, &hostname, &appname)) {
		if (hostname.isEmpty() || hostname == QString::fromLocal8Bit(localHostName())) {
			HANDLE procHandle = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
			if (!procHandle)
				return true;
			// We got a handle but check if process is still alive
			DWORD exitCode = 0;
			if (!::GetExitCodeProcess(procHandle, &exitCode))
				exitCode = 0;
			::CloseHandle(procHandle);
			if (exitCode != STILL_ACTIVE)
				return true;
			const QString processName = processNameByPid(pid);
			if (!processName.isEmpty() && processName != appname)
				return true; // PID got reused by a different application.
		}
	}

	const qint64 age = QFileInfo(fileName).lastModified().msecsTo(QDateTime::currentDateTime());
	return m_nStaleLockTime > 0 && age > m_nStaleLockTime;
}

QString MyLockFile::processNameByPid(qint64 pid)
{
	typedef DWORD(WINAPI *GetModuleFileNameExFunc)(HANDLE, HMODULE, LPTSTR, DWORD);

	HMODULE hPsapi = LoadLibraryA("psapi");
	if (!hPsapi)
		return QString();

	GetModuleFileNameExFunc qGetModuleFileNameEx
		= (GetModuleFileNameExFunc)GetProcAddress(hPsapi, "GetModuleFileNameExW");
	if (!qGetModuleFileNameEx) {
		FreeLibrary(hPsapi);
		return QString();
	}

	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, DWORD(pid));
	if (!hProcess) {
		FreeLibrary(hPsapi);
		return QString();
	}
	wchar_t buf[MAX_PATH];
	const DWORD length = qGetModuleFileNameEx(hProcess, NULL, buf, sizeof(buf) / sizeof(wchar_t));
	CloseHandle(hProcess);
	FreeLibrary(hPsapi);
	if (!length)
		return QString();
	QString name = QString::fromWCharArray(buf, length);
	int i = name.lastIndexOf(QLatin1Char('\\'));
	if (i >= 0)
		name.remove(0, i + 1);
	i = name.lastIndexOf(QLatin1Char('.'));
	if (i >= 0)
		name.truncate(i);
	return name;
}
