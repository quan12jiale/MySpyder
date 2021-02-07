#pragma once

#include <QHash>
#include <QStringList>

QString get_filter(const QList<QPair<QString, QStringList>>& filetypes,const QString& ext);
QList<QPair<QString, QStringList>> get_edit_filetypes();
QString get_edit_filters();

bool is_gtk_desktop();

// 该函数位于spyder/__init__.py
QHash<QString,QString> get_versions(bool reporev=true);

struct Last_Ec_Exec // 中间的ec指external console
{
    QString fname;
    QString wdir;
    QString args;
    bool interact;
    bool debug;
    bool python;
    QString python_args;
    bool current;
    bool systerm;
    bool post_mortem;
    bool clear_namespace;
    Last_Ec_Exec() { fname = QString(); }
    Last_Ec_Exec(QString _fname,
                 QString _wdir,
                 QString _args,
                 bool _interact,
                 bool _debug,
                 bool _python,
                 QString _python_args,
                 bool _current,
                 bool _systerm,
                 bool _post_mortem,
                 bool _clear_namespace)
    {
        fname = _fname;
        wdir = _wdir;
        args = _args;
        interact = _interact;
        debug = _debug;
        python = _python;
        python_args = _python_args;
        current = _current;
        systerm = _systerm;
        post_mortem = _post_mortem;
        clear_namespace = _clear_namespace;
    }
};
