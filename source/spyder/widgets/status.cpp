#include "status.h"
#include <memory>
#if defined(Q_OS_WIN)
#include <Windows.h>
#endif

StatusBarWidget::StatusBarWidget(QWidget* parent, QStatusBar* statusbar)
    : QWidget (parent)
{
    label_font = QFont("Calibri",10,75);
    label_font.setPointSize(this->font().pointSize());
    label_font.setBold(true);

    QHBoxLayout* layout = new QHBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);

    statusbar->addPermanentWidget(this);
}


BaseTimerStatus::BaseTimerStatus(QWidget* parent, QStatusBar* statusbar)
    : StatusBarWidget (parent, statusbar)
{
    label = new QLabel(TITLE);
    value = new QLabel;

    setToolTip(TIP);
    value->setAlignment(Qt::AlignRight);
    value->setFont(label_font);
    QFontMetrics fm = value->fontMetrics();
    value->setMinimumWidth(fm.width("000%"));

    QHBoxLayout* layout = qobject_cast<QHBoxLayout*>(this->layout());
    layout->addWidget(label);
    layout->addWidget(value);
    layout->addSpacing(20);

    timer = new QTimer;
    connect(timer,SIGNAL(timeout()),this,SLOT(update_label()));
    timer->start(2000);
}

void BaseTimerStatus::set_interval(int interval)
{
    if (timer)
        timer->setInterval(interval);
}

void BaseTimerStatus::update_label()
{
    if (isVisible())
        value->setText(QString::asprintf("%d %%", get_value()));
}

/********** MemoryStatus **********/
MemoryStatus::MemoryStatus(QWidget* parent, QStatusBar* statusbar)
    : BaseTimerStatus (parent, statusbar)
{
    TITLE = "Memory:";
    TIP = "Memory usage status: "
          "requires the `psutil` (>=v0.3) library on non-Windows platforms";
    label->setText(TITLE);
    setToolTip(TIP);
}

void close_file(FILE* fp) 
{
	if (fp)
	{
		fclose(fp);
	}
}

/***************************************************************************************************
功能： 获取Linux系统内存使用率
返回： 内存使用率百分比
proc/meminfo文件前三行数据格式如下：
MemTotal:       16413744 kB
MemFree:        13392420 kB
MemAvailable:   14592932 kB
***************************************************************************************************/
quint32 getMemoryLoadLinux()
{
	quint32 dMemUsedRate;

	char cBuff[128];
	char cNameTotal[128];
	unsigned long ulMemTotal = 1L; // 初始化，防止除0

	char name11[128];
	unsigned long ulMemFree;

	std::unique_ptr<FILE, void(*)(FILE*) > fd{ fopen("/proc/meminfo", "r"), close_file };
	if (fd == NULL)
	{
		return 0;
	}

	if (NULL == fgets(cBuff, sizeof(cBuff), fd.get()))
	{
		return 0;
	}
	sscanf(cBuff, "%s %lu", cNameTotal, &ulMemTotal);

	if (NULL == fgets(cBuff, sizeof(cBuff), fd.get()))
	{
		return 0;
	}
	sscanf(cBuff, "%s %lu", name11, &ulMemFree);

	dMemUsedRate = (1.0 - (double)ulMemFree / (double)ulMemTotal) * 100;
	return dMemUsedRate;
}

quint32 memory_usage()
{
#if defined(Q_OS_WIN)
    MEMORYSTATUSEX memorystatus;
	memorystatus.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memorystatus);
    return memorystatus.dwMemoryLoad;
#else
	return getMemoryLoadLinux();
#endif
}

quint32 MemoryStatus::get_value()
{
    return memory_usage();
}

/********** ReadWriteStatus **********/
ReadWriteStatus::ReadWriteStatus(QWidget* parent, QStatusBar* statusbar)
    : StatusBarWidget (parent, statusbar)
{
    label = new QLabel("Permissions:");
    readwrite = new QLabel;

    label->setAlignment(Qt::AlignRight);
    readwrite->setFont(label_font);

    QHBoxLayout* layout = qobject_cast<QHBoxLayout*>(this->layout());
    layout->addWidget(label);
    layout->addWidget(readwrite);
    layout->setSpacing(20);
}

void ReadWriteStatus::readonly_changed(bool readonly)
{
    QString readwrite = readonly ? "R" : "RW";
    this->readwrite->setText(readwrite.leftJustified(3));
}


/********** EOLStatus **********/
EOLStatus::EOLStatus(QWidget* parent, QStatusBar* statusbar)
    : StatusBarWidget (parent, statusbar)
{
    label = new QLabel("End-of-lines:");
    eol = new QLabel;

    label->setAlignment(Qt::AlignRight);
    eol->setFont(label_font);

    QHBoxLayout* layout = qobject_cast<QHBoxLayout*>(this->layout());
    layout->addWidget(label);
    layout->addWidget(eol);
    layout->setSpacing(20);
}

void EOLStatus::eol_changed(const QString &os_name)
{
    QHash<QString,QString> dict;
    dict["nt"] = "CRLF";
    dict["posix"] = "LF";
    eol->setText(dict.value(os_name, "CR"));
}


/********** EncodingStatus **********/
EncodingStatus::EncodingStatus(QWidget* parent, QStatusBar* statusbar)
    : StatusBarWidget (parent, statusbar)
{
    label = new QLabel("Encoding:");
    encoding = new QLabel;

    label->setAlignment(Qt::AlignRight);
    encoding->setFont(label_font);

    QHBoxLayout* layout = qobject_cast<QHBoxLayout*>(this->layout());
    layout->addWidget(label);
    layout->addWidget(encoding);
    layout->setSpacing(20);
}

void EncodingStatus::encoding_changed(const QString &encoding)
{
    this->encoding->setText(encoding.toUpper().leftJustified(15));
}



CursorPositionStatus::CursorPositionStatus(QWidget* parent, QStatusBar* statusbar)
    : StatusBarWidget (parent, statusbar)
{
    label_line = new QLabel("Line:");
    label_column = new QLabel("Column:");
    column = new QLabel;
    line = new QLabel;

    line->setFont(label_font);
    column->setFont(label_font);

    QHBoxLayout* layout = qobject_cast<QHBoxLayout*>(this->layout());
    layout->addWidget(label_line);
    layout->addWidget(line);
    layout->addWidget(label_column);
    layout->addWidget(column);
    setLayout(layout);
}

void CursorPositionStatus::cursor_position_changed(int line, int index)
{
    this->line->setText(QString::asprintf("%-6d", line+1));
    this->column->setText(QString::asprintf("%-4d", index+1));
}

static void test()
{
    QMainWindow* win = new QMainWindow;
    win->setWindowTitle("Status widgets test");
    win->resize(900, 300);
    QStatusBar* statusbar = win->statusBar();
    new ReadWriteStatus(win, statusbar);
    new EOLStatus(win, statusbar);
    new EncodingStatus(win, statusbar);
    new CursorPositionStatus(win, statusbar);
    new MemoryStatus(win, statusbar);
    win->show();
}
