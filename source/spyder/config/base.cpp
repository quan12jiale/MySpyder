#include "base.h"
#include <QCoreApplication>

bool DEV = false;
bool SAFE_MODE = false;

bool running_under_pytest()
{
    // return bool(os.environ.get('SPYDER_PYTEST'))
    return false;
}

int DEBUG = 0;

//源码中是.spyder-py3,这里故意不同,所以别的配置文件名字都应该一样
static QString SUBFOLDER = ".myspyder";

QString get_home_dir()
{
    return QDir::homePath();//返回用户home目录

    // 本以为os.chdir()函数对应QDir::setCurrent(directory)函数,
    // 其实setCurrent是设置Qt应用程序工作目录，没有意义，应该cmd cd到directory
    // QDir::setCurrent在plugins/console中run_script()函数，
    // plugins/WorkingDirectory::chdir()函数，
    // widgets/ExplorerTreeWidget::chdir函数有使用

    // 因此os.getcwd也不等价于QDir::currentPath(),
    // 不过暂时可以用QDir::currentPath()代替
}

QString get_conf_path(const QString& filename)
{
    QString conf_dir = get_home_dir() + '/' + SUBFOLDER;
    QFileInfo info(conf_dir);

    if (!info.isDir()) {
        QDir dir(get_home_dir());
        bool res = dir.mkdir(SUBFOLDER);
        if (res == false)
            qDebug() << __FILE__ << __FUNCTION__ << "创建目录失败";
    }
    if (filename.isEmpty())
        return conf_dir;
    else
        return conf_dir + '/' + filename;
}

const QStringList IMG_PATH = {"images/",
                              "images/actions/",
                              "images/console/",
                              "images/editor/",
                              "images/file/",
                              "images/projects/"};

QString get_image_path(const QString& name,const QString& _default)
{
    foreach (const QString& img_path, IMG_PATH) {
        QString full_path;
#ifdef _MSC_VER
        full_path = QCoreApplication::applicationDirPath() + "/";
#endif // MSVC
        full_path += img_path + name;
        QFileInfo info(full_path);
        if (info.isFile())
            return info.absoluteFilePath();
    }
    if (!_default.isEmpty()) {
        QString img_path = "images/" + _default;
        QFileInfo info(img_path);
        return info.absoluteFilePath();
    }
    return QString();
}

static QString LANG_FILE = get_conf_path("langconfig");
static QString DEFAULT_LANGUAGE = "en";

QHash<QString, QString> LANGUAGE_CODES = {{"en", "English"},
                                          {"de", "Deutsch"},
                                          {"fr", "Français"},
                                          {"es", "Español"},
                                          {"hu", "Magyar"},
                                          {"pt_BR", "Português"},
                                          {"ru", "Русский"},
                                          {"ja", "日本語"}};
static QStringList DISABLED_LANGUAGES;

void save_lang_conf(const QString& value)
{
    QFile file(LANG_FILE);
    bool ok = file.open(QIODevice::WriteOnly | QIODevice::Text);
    if (ok) {
        file.write(value.toUtf8());
        file.close();
    }
    else {
        qDebug() << __FILE__ << __FUNCTION__ << "打开语言文件失败";
    }
}

QString load_lang_conf()
{
    QFileInfo info(LANG_FILE);
    QString lang;
    if (info.isFile()) {
        QFile file(LANG_FILE);
        bool ok = file.open(QIODevice::ReadOnly | QIODevice::Text);
        if (ok) {
            lang = file.readAll();
            file.close();
        }
        else {
            qDebug() << __FILE__ << __FUNCTION__ << "打开语言文件失败";
        }
    }
    else
        qDebug() << __FILE__ << __FUNCTION__ << "语言文件不存在";
    //源码是if lang.strip('\n') in DISABLED_LANGUAGES:
    if (DISABLED_LANGUAGES.contains(lang.trimmed())) {
        lang = DEFAULT_LANGUAGE;
        save_lang_conf(lang);
    }
    return lang;
}


