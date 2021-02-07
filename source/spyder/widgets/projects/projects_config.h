#ifndef PROJECTS_CONFIG_H
#define PROJECTS_CONFIG_H

#include "os.h"
#include "configparser.h"

#include <QDir>
#include <QString>
#include <QVariant>
#include <QFileInfo>

extern QString PROJECT_FILENAME;
extern QString PROJECT_FOLDER;

extern QString WORKSPACE;
extern QString WORKSPACE_VERSION;

extern QString CODESTYLE;
extern QString CODESTYLE_VERSION;

extern QString ENCODING;
extern QString ENCODING_VERSION;

extern QString VCS;
extern QString VCS_VERSION;


class ProjectConfig : public UserConfig
{
public:
    QString project_root_path;

    ProjectConfig(const QString& name, const QString& root_path, const QString& filename,
                  QList<QPair<QString, QHash<QString,QVariant>>> defaults,
                  bool load=true, QString version=QString());
};

#endif // PROJECTS_CONFIG_H
