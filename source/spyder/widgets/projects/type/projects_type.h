#ifndef PROJECTS_TYPE_H
#define PROJECTS_TYPE_H

#include "widgets/projects/projects_config.h"
#include <QDebug>

class BaseProject
{
public:
    QList<QVariant> WORKSPACE_DEFAULTS;
    QList<QVariant> CODESTYLE_DEFAULTS;
    QList<QVariant> ENCODING_DEFAULTS;
    QList<QVariant> VCS_DEFAULTS;

    QString PROJECT_FOLDER;
    QString PROJECT_TYPE_NAME;
    QString IGNORE_FILE;
    QHash<QString,QHash<QString,QVariant>> CONFIG_SETUP;

    QString name;
    QString root_path;
    QStringList open_project_files;
    QStringList open_non_project_files;
    QStringList config_files;
    QHash<QString,ProjectConfig*> CONF;

    QStringList related_projects;
    bool opened;
    bool ioerror_flag;
public:
    BaseProject(const QString& root_path);
    void set_recent_files(QStringList recent_files);
    QStringList get_recent_files() const;
    void create_project_config_files();
    QHash<QString,ProjectConfig*> get_conf_files() const;
    void add_ignore_lines(const QStringList& lines);
    void set_root_path(const QString& root_path);
    void rename(const QString& new_name);
    QString __get_project_config_folder() const;
    QString __get_project_config_path() const;
    virtual void load(){}
    virtual void save(){}
};

class EmptyProject : public BaseProject
{
public:
    EmptyProject(const QString& root_path);
};

#endif // PROJECTS_TYPE_H
