#include "config/config_main.h"
#include <QApplication>
#include <QDebug>
#include <QIcon>

#include "plugins/ipythonconsole.h"
#include "app/mainwindow.h"
#include "plugins\editor\utils\editor.h"

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
    QSettings settings;
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

void onButtonClicked()
{

}

// connect、disconnect函数模板用法的例子，包含了所有场景，除信号、槽参数是字符串类型
void connectTemplateExample()
{
	QToolButton* button = new QToolButton;
	QAction* action = new QAction(nullptr);

	// 1、槽是成员函数。qobject.h文件213行函数模板
	QObject::connect(button, &QToolButton::clicked, action, &QAction::trigger);
	// 测试QSlotObject类impl方法枚举为Compare时的情形。调试时可以看出QAction::trigger的地址和void **a解引用相同
	QObject::connect(button, &QToolButton::clicked, action, &QAction::trigger, Qt::UniqueConnection);

	// 2、槽是普通函数、或类的静态成员函数。2.1、qobject.h文件245行函数模板
	QObject::connect(button, &QToolButton::clicked, onButtonClicked);

	// 2.2、qobject.h文件254行函数模板
	QObject::connect(button, &QToolButton::clicked, qApp, onButtonClicked);

	// 3、槽是lambda、或std::function。3.1、qobject.h文件285行函数模板
	auto onButtonClickedLambda = []() {};
	std::function<void ()> onButtonClickedFunction = []() {};
	QObject::connect(button, &QToolButton::clicked, onButtonClickedLambda);
	QObject::connect(button, &QToolButton::clicked, onButtonClickedFunction);

	// 3.2、qobject.h文件293行函数模板
	QObject::connect(button, &QToolButton::clicked, qApp, onButtonClickedLambda);
	QObject::connect(button, &QToolButton::clicked, qApp, onButtonClickedFunction);

	// 小插曲：信号、槽有重载时的连接
	QComboBox* combobox = new QComboBox;
	typedef void (QComboBox::*_t)(int);
	QObject::connect(combobox, static_cast<_t>(&QComboBox::currentIndexChanged), onButtonClicked);
	
	// 测试这两个重载函数地址是否相同，好像是一样的。。。
	typedef void (QComboBox::*_s)(const QString &);
	QObject::connect(combobox, static_cast<_s>(&QComboBox::currentIndexChanged), onButtonClicked);


	// 1、槽是成员函数。qobject.h文件368行函数模板
	bool disconnectSuccess = QObject::disconnect(button, &QToolButton::clicked, action, &QAction::trigger);

	/*
	2、
	3、
	It is not possible to use this overload to diconnect signals
	connected to functors or lambda expressions. That is because it is not
	possible to compare them. Instead, use the overload that takes a
	QMetaObject::Connection
	上面这段话来自qobject.cpp文件4824行。
	成员函数指针连接时创建的是QSlotObject，实现了当枚举为Compare时的函数指针比较。
	普通函数、或类的静态成员函数连接时创建的是QStaticSlotObject; lambda、或std::function连接时创建的是QFunctorSlotObject。都没有实现当枚举为Compare时的比较。
	QFunctorSlotObject无法实现枚举为Compare时的比较，因为lambda、或std::function都是仿函数对象，所以无法像成员函数指针那样比较。可是为什么普通函数不可以比较???
	*/

	// 2、disconnect sender a signal to receiver any slot
	disconnectSuccess = QObject::disconnect(button, &QToolButton::clicked, qApp, nullptr);

	delete combobox;
	delete action;
	delete button;
}

int main(int argc, char *argv[])
{
    registe_meta_type();
    QApplication a(argc, argv);
	QString applicationDirPath = QCoreApplication::applicationDirPath();
	QDir::setCurrent(applicationDirPath);
    QSettings settings;
    QString first_run = "first_run";
    if (settings.value(first_run, true).toBool()) {
        read_default_to_settings();
        settings.setValue(first_run, false);
    }
    initialize();
	testDelayJobRunner();
	connectTemplateExample();

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

    return a.exec();
}
