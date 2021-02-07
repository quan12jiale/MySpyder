#ifndef NETEASESINGSFRAMES_H
#define NETEASESINGSFRAMES_H

#include "widgets/singsframebase.h"

class ConfigNetEase;

class NetEaseSingsArea : public SingsFrameBase
{
    Q_OBJECT
public:
    ConfigNetEase* config;

    NetEaseSingsArea(MainContent* parent);
};


class NetEaseSearchResultFrame : public SingsSearchResultFrameBase
{
    Q_OBJECT
public:
    NetEaseSearchResultFrame(QWidget* parent);
};

#endif // NETEASESINGSFRAMES_H
