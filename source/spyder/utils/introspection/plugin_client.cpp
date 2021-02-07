#include "plugin_client.h"

AsyncClient::AsyncClient(QString target, QString executable, QString name,
            QStringList extra_args, QStringList libs, QString cwd, QStringList env)
{
    if (!executable.isEmpty())
        this->executable = executable;
    else // sys.executable
        this->executable = "D:/Anaconda3/pythonw.exe";
    this->target = target;
    this->name = name;
    this->libs = libs;
    this->cwd = cwd;
    this->env = env;

    this->is_initialized = false;
    this->closing = false;
    this->notifier = nullptr;
    this->process = nullptr;
    //self.context = zmq.Context()
    connect(qApp, SIGNAL(aboutToQuit()), SLOT(close()));

    this->timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), SLOT(_heartbeat()));
}

void AsyncClient::run()
{
    this->process = new QProcess(this);
    if (!this->cwd.isEmpty())
        this->process->setWorkingDirectory(this->cwd);
    QStringList p_args;
//    p_args << "-u" << this->target << QString::number(this->port);
    if (!this->extra_args.isEmpty())
        p_args.append(this->extra_args);

    QProcessEnvironment processEnvironment;
    QStringList env = this->process->systemEnvironment();
    if ((!this->env.isEmpty() && !this->env.contains("PYTHONPATH"))
            || this->env.isEmpty()) {
        //python_path = osp.dirname(get_module_path('spyder'))
        QString python_path = "D:/Anaconda3/lib/site-packages";
    }
    this->process->setProcessEnvironment(processEnvironment);

    this->process->start(this->executable, p_args);
    connect(process, SIGNAL(finished(int, QProcess::ExitStatus)),
            SLOT(_on_finished()));
    bool running = this->process->waitForStarted();
    if (!running)
        qDebug() << "Could not start " << this;

//    this->notifier = new QSocketNotifier(socket_, QSocketNotifier::Read, this);
//    connect(notifier, SIGNAL(activated(int)), SLOT(_on_msg_received()));
}

void AsyncClient::request(const QString &func_name)
{
    if (!this->is_initialized)
        return;
}

void AsyncClient::close()
{
    this->closing = true;
    this->is_initialized = false;
    this->timer->stop();

    if (this->notifier) {
        disconnect(notifier, SIGNAL(activated(int)),
                   this, SLOT(_on_msg_received()));
        this->notifier->setEnabled(false);
        delete this->notifier;
        this->notifier = nullptr;
    }
    this->request("server_quit");

    if (this->process) {
        this->process->waitForFinished(1000);
        this->process->close();
    }
}

void AsyncClient::_on_finished()
{
    if (this->closing)
        return;
    if (this->is_initialized) {
        qDebug() << "Restarting " << this->name;
        qDebug() << this->process->readAllStandardOutput();
        qDebug() << this->process->readAllStandardError();
        this->is_initialized = false;
        this->notifier->setEnabled(false);
        this->run();
    }
    else {
        qDebug() << "Errored " << this->name;
        qDebug() << this->process->readAllStandardOutput();
        qDebug() << this->process->readAllStandardError();
        emit errored();
    }
}

plugin_client::plugin_client()
{

}
