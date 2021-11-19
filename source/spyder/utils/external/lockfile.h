#pragma once
#include <QtCore/QString>

class MyLockFile
{
public:
	MyLockFile(const QString &fn);
	~MyLockFile();

	void setStaleLockTime(int staleLockTime);
	int staleLockTime() const;

	bool isLocked() const;
	enum LockError {
		NoError = 0,
		LockFailedError = 1,
		PermissionError = 2,
		UnknownError = 3
	};
	LockError error() const;

	bool lock();
	bool tryLock(int timeout = 0);
	void unlock();

	bool getLockInfo(qint64 *pid, QString *hostname, QString *appname) const;
	bool removeStaleLockFile();

private:
	MyLockFile::LockError tryLock_sys();
	bool removeStaleLock();
	// Returns \c true if the lock belongs to dead PID, or is old.
	// The attempt to delete it will tell us if it was really stale or not, though.
	bool isApparentlyStale() const;
	// used in dbusmenu
	static QString processNameByPid(qint64 pid);

private:
	QString fileName;
	Qt::HANDLE fileHandle;
	int m_nStaleLockTime; // "int milliseconds" is big enough for 24 days
	MyLockFile::LockError lockError;
	bool m_bIsLocked;

private:
	Q_DISABLE_COPY(MyLockFile)
};