#include "projects_config.h"

QString PROJECT_FILENAME = ".spyproj";
QString PROJECT_FOLDER = ".spyproject";

QString WORKSPACE = "workspace";
QString WORKSPACE_VERSION = "0.1.0";

QString CODESTYLE = "codestyle";
QString CODESTYLE_VERSION = "0.1.0";

QString ENCODING = "encoding";
QString ENCODING_VERSION = "0.1.0";

QString VCS = "vcs";
QString VCS_VERSION = "0.1.0";


ProjectConfig::ProjectConfig(const QString& name, const QString& root_path,
                             const QString& filename,
                             QList<QPair<QString, QHash<QString, QVariant> > > defaults,
                             bool load, QString version)
    : UserConfig (name, defaults, load, version,
                  QString(), false, true, true)
{
    this->project_root_path = root_path;

    this->_root_path = root_path + '/' + PROJECT_FOLDER;
    this->_filename = filename;

    QFileInfo info(this->_root_path);
    if (!info.isDir()) {
        os::makedirs(this->_root_path);
    }
    this->_save();
    //之所以放在这里创建文件，是因为调用父类构造函数时，还没有生成相应目录
}

