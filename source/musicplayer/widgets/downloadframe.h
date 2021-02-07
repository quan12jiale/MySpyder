#ifndef DOWNLOADFRAME_H
#define DOWNLOADFRAME_H

#include "widgets/base.h"

class Window;
class ConfigDownloadFrame;

class DownloadFrame : public ScrollArea
{
    Q_OBJECT
public:
    ConfigDownloadFrame* config;

    Window* parent;
    QVBoxLayout* mainLayout;
    QFrame* spaceLine;
    QLabel* currentStorageFolderLabel;
    QLabel* currentStorageFolder;
    QPushButton* selectButton;
    QHBoxLayout* topShowLayout;
    QTableWidget* singsTable;

    DownloadFrame(Window* parent);
    void setHeader();
    void setMusicTable();
};

#endif // DOWNLOADFRAME_H
