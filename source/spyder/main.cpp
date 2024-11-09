#ifdef ENABLE_VLD
#include "vld.h"
#endif // ENABLE_VLD
#include "config/config_main.h"
#include <QApplication>
#include <QDebug>
#include <QIcon>

#include "plugins/ipythonconsole.h"
#include "app/mainwindow.h"
#include "plugins/editor/utils/editor.h"
#include "utils/external/lockfile.h"

void registe_meta_type()
{
    QCoreApplication::setOrganizationName("Quan");
    QCoreApplication::setApplicationName("spyder");
    qRegisterMetaType<ColorBoolBool>();
    qRegisterMetaTypeStreamOperators<ColorBoolBool>("ColorBoolBool");

    qRegisterMetaType<SplitSettings>();
    qRegisterMetaTypeStreamOperators<SplitSettings>("SplitSettings");

    qRegisterMetaType<LayoutSettings>();
    qRegisterMetaTypeStreamOperators<LayoutSettings>("LayoutSettings");

    // This attibute must be set before creating the application.
    bool high_dpi_scaling = CONF_get("main", "high_dpi_scaling").toBool();
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling, high_dpi_scaling);
}

void read_default_to_settings()
{
    QSettings settings(getIniPath(), QSettings::Format::IniFormat);
    foreach (auto pair, DEFAULTS) {
        QString section = pair.first;
        auto dict = pair.second;
        foreach (QString key, dict.keys()) {
            QString tmp = section + '/' + key;
            QVariant val = dict[key];
            settings.setValue(tmp, val);
        }
    }
}

int main(int argc, char *argv[])
{
#ifdef ENABLE_VLD
	VLDSetReportOptions(VLD_OPT_REPORT_TO_FILE, L"myspyder_memory_leak_report.txt");
#endif // ENABLE_VLD
    registe_meta_type();
    QApplication a(argc, argv);
	QString applicationDirPath = QCoreApplication::applicationDirPath();
	QDir::setCurrent(applicationDirPath);

// 	# Minimal delay(0.1 - 0.2 secs) to avoid that several
// 	# instances started at the same time step in their
// 	# own foots while trying to create the lock file
	int nTimeDelay = std::rand() % 1000 + 1000;
	QThread::msleep(nTimeDelay / 10);

//  # Lock file creation
	QString lock_file = get_conf_path("spyder.lock");
	MyLockFile locker(lock_file);
	bool lock_created = locker.tryLock();
	if (!lock_created)
	{
		std::printf("Spyder is already running. If you want to open a new \n"
			"instance, please pass to it the --new-instance option");
		return 0;
	}

    QSettings settings(getIniPath(), QSettings::Format::IniFormat);
    QString first_run = "first_run";
    if (settings.value(first_run, true).toBool()) {
        read_default_to_settings();// 这里的QSettings代码不能注释，因为要调用read_default_to_settings()函数
        settings.setValue(first_run, false);
    }
    initialize();
	//testDelayJobRunner();

    MainWindow* win = new MainWindow;
    win->setup();
    win->show();
    win->post_visible_setup();

    //if main.console:
    QObject::connect(qApp, SIGNAL(focusChanged(QWidget *, QWidget *)),
            win, SLOT(change_last_focused_widget(QWidget *, QWidget *)));

	QStringList add_kw = QStringList();
	add_kw << "async" << "await";
	QString pattern = sh::make_python_patterns(add_kw);
	Q_UNUSED(pattern);

    const int res = a.exec();
    delete win;
    return res;
}
