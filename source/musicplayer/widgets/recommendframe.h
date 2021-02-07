#ifndef RECOMMENDFRAME_H
#define RECOMMENDFRAME_H

#include "widgets/base.h"

class Window;
class ConfigRecommendFrame;

class RecommendFrame : public ScrollArea
{
    Q_OBJECT
public:
    ConfigRecommendFrame* config;

    Window* parent;
    QVBoxLayout* mainLayout;
    QFrame* spaceLine;
    QLabel* recommendLabel;
    QHBoxLayout* topShowLayout;
    QTableWidget* singsTable;

    RecommendFrame(Window* parent);
    void setHeader();
    void setMusicTable();
};

#endif // RECOMMENDFRAME_H
