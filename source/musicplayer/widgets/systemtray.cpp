#include "systemtray.h"
#include "music.h"

SystemTray::SystemTray(const QString &iconPath, Window *parent)
    : QSystemTrayIcon (QIcon(iconPath))
{
    this->config = nullptr;

    this->parent = parent;
    this->menu = new QMenu(this->parent);
    this->setContextMenu(this->menu);

    this->show();
}

void SystemTray::addAction(QAction *action)
{
    this->menu->addAction(action);
}

static void test()
{
    SystemTray* tray = new SystemTray("resource/logo.png", nullptr);
}
