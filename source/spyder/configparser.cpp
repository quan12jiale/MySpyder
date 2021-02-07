#include "configparser.h"

UserConfig::UserConfig(const QString& name,
           QList<QPair<QString, QHash<QString,QVariant>>> defaults,
           bool load, QString version,
           QString subfolder, bool backup,
           bool raw_mode, bool remove_obsolete)
{
    this->defaults = defaults;

    //其实第一次创建配置文件是在config/user.py203行get_version()函数
    //之后调用get()函数,get()函数在377行调用set()函数,
    //set()函数在445行调用_save()函数,_save()函数创建文件

}

void UserConfig::_save()
{
    QString fname = this->filename();
    QFile file(fname);
    bool ok = file.open(QIODevice::WriteOnly);
    if (ok) {
        this->write(&file);
        file.close();
    }
}

QString UserConfig::filename()
{
    if (this->_filename.isEmpty() && this->_root_path.isEmpty())
        return QString();
    else
        return this->_filename_projects();
}

QString UserConfig::_filename_projects()
{
    return this->_root_path + '/' + this->_filename;
}

void UserConfig::write(QFile *fp, bool space_around_delimiters)
{
    QString d;
    if (space_around_delimiters)
        d = QString(" %1 ").arg("=");
    else
        d = "=";

    foreach (auto pair, this->defaults) {
        QString section = pair.first;
        QHash<QString,QVariant> section_items = pair.second;
        this->_write_section(fp, section, section_items, d);
    }
}

void UserConfig::_write_section(QFile *fp, QString section_name,
                                QHash<QString, QVariant> section_items,
                                QString delimiter)
{
    QString tmp = QString("[%1]\n").arg(section_name);
    fp->write(tmp.toUtf8());
    for (auto it = section_items.begin(); it != section_items.end(); ++it) {
        QString key = it.key();
        QVariant val = it.value();
        bool _allow_no_value = false;

        QString value = "";
        if (!val.isNull() || !_allow_no_value)
            value = delimiter + val.toString().replace("\n", "\n\t");
        tmp = QString("%1%2\n").arg(key).arg(value);
        fp->write(tmp.toUtf8());
    }
    fp->write("\n");
}

QVariant UserConfig::get(const QString &section, const QString &option,
                         const QVariant &_default)
{
    if (!this->has_section(section))
        return _default;
    for (int i = 0; i < this->defaults.size(); ++i) {
        auto pair = this->defaults[i];
        QString sec = pair.first;
        if (sec == section) {
            QHash<QString,QVariant> dict = pair.second;
            for (auto it = dict.begin(); it!= dict.end(); ++it) {
                if (it.key() == option)
                    return it.value();
            }
        }
    }
    return _default;
}

bool UserConfig::has_section(const QString &section)
{
    foreach (auto pair, this->defaults) {
        QString sec = pair.first;
        if (sec == section)
            return true;
    }
    return false;
}

void UserConfig::_set(const QString &section, const QString &option,
                      const QVariant &value)
{
    for (int i = 0; i < this->defaults.size(); ++i) {
        auto pair = this->defaults[i];
        QString sec = pair.first;
        if (sec == section) {
            QHash<QString,QVariant> dict = pair.second;
            dict[option] = value;
            this->defaults[i].second = dict;
            break;
        }
    }
}

void UserConfig::set(const QString &section, const QString &option,
                     const QVariant &value, bool save)
{
    if (!this->has_section(section)) {
        QPair<QString, QHash<QString,QVariant>> pair;
        pair.first = section;

        QHash<QString,QVariant> dict;
        dict[option] = value;
        pair.second = dict;

        this->defaults.append(pair);
    }

    this->_set(section, option, value);
    if (save)
        this->_save();
}
