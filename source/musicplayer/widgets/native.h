#ifndef NATIVE_H
#define NATIVE_H

#include "widgets/base.h"

class Window;
class ConfigNative;

class NativeMusic : public ScrollArea
{
    Q_OBJECT
public:
    ConfigNative* config;

    Window *parent;
    QVBoxLayout* mainLayout;
    QLabel* showLabel;
    QFrame* spaceLine;
    QPushButton* selectButton;
    QHBoxLayout* topShowLayout;
    QTableWidget* singsTable;

    NativeMusic(Window* parent);
    void setTopShow();
    void musicTable();
};

#endif // NATIVE_H
