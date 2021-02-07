#ifndef PLUGIN_CLIENT_H
#define PLUGIN_CLIENT_H

#include <QtCore>

class AsyncClient : public QObject
{
    Q_OBJECT
signals:
    void initialized();
    void errored();
public:
    QString executable;
    QStringList extra_args;
    QString target;
    QString name;
    QStringList libs;
    QString cwd;
    QStringList env;

    bool is_initialized;
    bool closing;
    QSocketNotifier* notifier;
    QProcess* process;
    //context = zmq.Context()
    QTimer* timer;

    AsyncClient(QString target, QString executable=QString(), QString name=QString(),
                QStringList extra_args=QStringList(), QStringList libs=QStringList(),
                QString cwd=QString(), QStringList env=QStringList());
    void run();
    void request(const QString& func_name);
    void close();//
    void _on_finished();//
};

class plugin_client
{
public:
    plugin_client();
};

#endif // PLUGIN_CLIENT_H
