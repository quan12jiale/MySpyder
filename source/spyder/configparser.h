#ifndef CONFIGPARSER_H
#define CONFIGPARSER_H

#include <QHash>
#include <QFile>
#include <QDebug>
#include <QVariant>
#include <QStringList>

class UserConfig
{
public:
    QString _filename;
    QString _root_path;

    QList<QPair<QString, QHash<QString,QVariant>>> defaults;

    UserConfig(const QString& name,
               QList<QPair<QString, QHash<QString,QVariant>>> defaults,
               bool load=true, QString version=QString(),
               QString subfolder=QString(), bool backup=false,
               bool raw_mode=false, bool remove_obsolete=false);

    void _save();
    QString filename();
    QString _filename_projects();

    void write(QFile* fp, bool space_around_delimiters=true);
    void _write_section(QFile* fp, QString section_name,
                        QHash<QString,QVariant> section_items,
                        QString delimiter);

    QVariant get(const QString& section, const QString& option,
                 const QVariant& _default=QVariant());

    bool has_section(const QString& section);
    void _set(const QString& section, const QString& option,
             const QVariant& value);
    void set(const QString& section, const QString& option,
             const QVariant& value, bool save=true);
};

#endif // CONFIGPARSER_H
