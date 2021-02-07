#ifndef SYSTEMTRAY_H
#define SYSTEMTRAY_H

#include <QtWidgets>

class Window;
class ConfigSystemTray;

class SystemTray : public QSystemTrayIcon
{
public:
    ConfigSystemTray* config;

    Window *parent;
    QMenu* menu;
    SystemTray(const QString& iconPath, Window *parent);
    void addAction(QAction* action);
};

#endif // SYSTEMTRAY_H
