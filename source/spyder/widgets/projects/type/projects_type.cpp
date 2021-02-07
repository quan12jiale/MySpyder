#include "projects_type.h"

BaseProject::BaseProject(const QString &root_path)
{
    WORKSPACE_DEFAULTS.append(QVariant());
    QList<QVariant> tmp;
    tmp.append(WORKSPACE);
    QHash<QString,QVariant> dict;
    dict["restore_data_on_startup"] = true;
    dict["save_data_on_exit"] = true;
    dict["save_history"] = true;
    dict["save_non_project_files"] = false;
    tmp.append(dict);
    WORKSPACE_DEFAULTS[0] = tmp;

    CODESTYLE_DEFAULTS.append(QVariant());
    tmp.clear();
    tmp.append(CODESTYLE);
    dict.clear();
    dict["indentation"] = true;
    tmp.append(dict);
    CODESTYLE_DEFAULTS[0] = tmp;

    ENCODING_DEFAULTS.append(QVariant());
    tmp.clear();
    tmp.append(ENCODING);
    dict.clear();
    dict["text_encoding"] = QString("utf-8");
    tmp.append(dict);
    ENCODING_DEFAULTS[0] = tmp;

    VCS_DEFAULTS.append(QVariant());
    tmp.clear();
    tmp.append(VCS);
    dict.clear();
    dict["use_version_control"] = false;
    dict["version_control_system"] = QString("");
    tmp.append(dict);
    VCS_DEFAULTS[0] = tmp;

    PROJECT_FOLDER = ".spyproject";
    PROJECT_TYPE_NAME = QString();
    IGNORE_FILE = "";

    QHash<QString,QVariant> workspace;
    workspace["filename"] = QString("%1.ini").arg(WORKSPACE);
    workspace["defaults"] = WORKSPACE_DEFAULTS;
    workspace["version"] = WORKSPACE_VERSION;

    QHash<QString,QVariant> codestyle;
    codestyle["filename"] = QString("%1.ini").arg(CODESTYLE);
    codestyle["defaults"] = CODESTYLE_DEFAULTS;
    codestyle["version"] = CODESTYLE_VERSION;

    QHash<QString,QVariant> encoding;
    encoding["filename"] = QString("%1.ini").arg(ENCODING);
    encoding["defaults"] = ENCODING_DEFAULTS;
    encoding["version"] = ENCODING_VERSION;

    QHash<QString,QVariant> vcs;
    vcs["filename"] = QString("%1.ini").arg(VCS);
    vcs["defaults"] = VCS_DEFAULTS;
    vcs["version"] = VCS_VERSION;

    CONFIG_SETUP.clear();
    CONFIG_SETUP[WORKSPACE] = workspace;
    CONFIG_SETUP[CODESTYLE] = codestyle;
    CONFIG_SETUP[ENCODING] = encoding;
    CONFIG_SETUP[VCS] = vcs;

    //构造函数
    this->name = QString();
    this->root_path = root_path;
    this->open_project_files = QStringList();
    this->open_non_project_files = QStringList();
    this->config_files = QStringList();
    this->CONF = QHash<QString,ProjectConfig*>();

    this->related_projects = QStringList();  // storing project path, not project objects

    this->opened = true;

    this->ioerror_flag = false;
    this->create_project_config_files();
}

void BaseProject::set_recent_files(QStringList recent_files)
{
    QStringList tmp(recent_files);
    foreach (QString recent_file, tmp) {
        QFileInfo info(recent_file);
        if (!info.isFile())
            recent_files.removeOne(recent_file);
    }

    this->CONF[WORKSPACE]->set("main", "recent_files", recent_files);
}

QStringList BaseProject::get_recent_files() const
{
    QStringList recent_files = this->CONF[WORKSPACE]->get("main", "recent_files",
                                                          QStringList()).toStringList();
    QStringList tmp(recent_files);
    foreach (QString recent_file, tmp) {
        QFileInfo info(recent_file);
        if (!info.isFile())
            recent_files.removeOne(recent_file);
    }
    return recent_files;
}

void BaseProject::create_project_config_files()
{
    QHash<QString,QHash<QString,QVariant>> dic = this->CONFIG_SETUP;
    foreach (QString key, dic.keys()) {
        QString name = key;
        QString filename = dic[key]["filename"].toString();
        QList<QVariant> tmp = dic[key]["defaults"].toList();
        QList<QPair<QString, QHash<QString,QVariant>>> defaults;
        foreach (QVariant val, tmp) {
            QList<QVariant> pair = val.toList();
            Q_ASSERT(pair.size() == 2);
            QString sec = pair[0].toString();
            QHash<QString,QVariant> items = pair[1].toHash();
            defaults.append(QPair<QString, QHash<QString,QVariant>>(sec, items));
        }

        QString version = dic[key]["version"].toString();
        this->CONF[key] = new ProjectConfig(name, this->root_path, filename,
                                            defaults, true, version);
    }
}

QHash<QString,ProjectConfig*> BaseProject::get_conf_files() const
{
    return this->CONF;
}

void BaseProject::add_ignore_lines(const QStringList &lines)
{
    QString text = this->IGNORE_FILE;
    foreach (QString line, lines) {
        text += line;
    }
    this->IGNORE_FILE = text;
}

void BaseProject::set_root_path(const QString &root_path)
{
    if (this->name.isEmpty()) {
        QFileInfo info(root_path);
        this->name = info.fileName();
    }
    this->root_path = root_path;
    QString config_path = this->__get_project_config_folder();
    if (QFileInfo::exists(config_path))
        this->load();
    else {
        QFileInfo info(this->root_path);
        if (!info.isDir()) {
            os::makedirs(this->root_path);
        }
        this->save();
    }
}

void BaseProject::rename(const QString &new_name)
{
    QString old_name = this->name;
    this->name = new_name;
    //pypath = self.relative_pythonpath  # ??
    this->root_path = this->root_path.right(old_name.size())+new_name;

    this->save();
}

QString BaseProject::__get_project_config_folder() const
{
    return this->root_path + '/' + this->PROJECT_FOLDER;
}

QString BaseProject::__get_project_config_path() const
{
    return this->root_path + '/' + this->PROJECT_TYPE_NAME;
}


EmptyProject::EmptyProject(const QString& root_path)
    : BaseProject (root_path)
{
    this->PROJECT_TYPE_NAME = "Empty project";
}
