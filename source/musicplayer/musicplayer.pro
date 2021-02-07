#-------------------------------------------------
#
# Project created by QtCreator 2020-03-08T09:12:33
#
#-------------------------------------------------

QT       += core gui network multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = musicplayer
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11

SOURCES += \
        main.cpp \
    widgets/addition.cpp \
    widgets/desktoplyricbuttons.cpp \
    widgets/base.cpp \
    music.cpp \
    widgets/downloadframe.cpp \
    widgets/loginframes.cpp \
    widgets/native.cpp \
    widgets/recommendframe.cpp \
    widgets/singsframebase.cpp \
    os.cpp \
    apis/apirequestsbase.cpp \
    apis/neteaseapi.cpp \
    apis/neteaseencode.cpp \
    widgets/systemtray.cpp \
    widgets/player.cpp \
    widgets/neteasesingsframes.cpp \
    features/configneteasefeatures.cpp \
    features/configrecommendframefeatures.cpp \
    features/configmainfeatures.cpp \
    features/configdownloadframefeatures.cpp \
    widgets/changeskinwidget.cpp \
    widgets/skinwidget.cpp

HEADERS += \
    widgets/addition.h \
    widgets/desktoplyricbuttons.h \
    widgets/base.h \
    music.h \
    widgets/downloadframe.h \
    widgets/loginframes.h \
    widgets/native.h \
    widgets/recommendframe.h \
    widgets/singsframebase.h \
    os.h \
    apis/apirequestsbase.h \
    apis/neteaseapi.h \
    apis/neteaseencode.h \
    widgets/systemtray.h \
    widgets/player.h \
    widgets/neteasesingsframes.h \
    features/configneteasefeatures.h \
    features/configrecommendframefeatures.h \
    features/configmainfeatures.h \
    features/configdownloadframefeatures.h \
    widgets/changeskinwidget.h \
    widgets/skinwidget.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

