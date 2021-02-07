#-------------------------------------------------
#
# Project created by QtCreator 2019-12-17T08:53:09
#
#-------------------------------------------------

QT       += core gui printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = spyder
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

LIBS += -lwsock32

SOURCES += \
        main.cpp \
    utils/icon_manager.cpp \
    config/base.cpp \
    widgets/helperwidgets.cpp \
    widgets/arraybuilder.cpp \
    utils/sourcecode.cpp \
    saxutils.cpp \
    os.cpp \
    textwrap.cpp \
    str.cpp \
    widgets/mixins.cpp \
    widgets/calltip.cpp \
    widgets/sourcecode/widgets_base.cpp \
    builtins.cpp \
    keyword.cpp \
    utils/syntaxhighlighters.cpp \
    widgets/sourcecode/codeeditor.cpp \
    widgets/sourcecode/kill_ring.cpp \
    utils/qthelpers.cpp \
    widgets/editortools.cpp \
    widgets/onecolumntree.cpp \
    widgets/variableexplorer/texteditor.cpp \
    widgets/variableexplorer/arrayeditor.cpp \
    widgets/variableexplorer/objecteditor.cpp \
    widgets/variableexplorer/collectionseditor.cpp \
    nsview.cpp \
    widgets/variableexplorer/importwizard.cpp \
    widgets/explorer.cpp \
    utils/encoding.cpp \
    utils/check.cpp \
    utils/programs.cpp \
    utils/misc.cpp \
    widgets/comboboxes.cpp \
    widgets/waitingspinner.cpp \
    widgets/findinfiles.cpp \
    fnmatch.cpp \
    config/config_main.cpp \
    widgets/editor.cpp \
    widgets/tabs.cpp \
    widgets/fileswitcher.cpp \
    config/utils.cpp \
    widgets/status.cpp \
    widgets/colors.cpp \
    app/mainwindow.cpp \
    plugins/plugins.cpp \
    config/gui.cpp \
    config/fonts.cpp \
    widgets/projects/projects_explorer.cpp \
    widgets/findreplace.cpp \
    widgets/pathmanager.cpp \
    widgets/github/gh_login.cpp \
    utils/external/github.cpp \
    widgets/github/backend.cpp \
    widgets/sourcecode/terminal.cpp \
    widgets/reporterror.cpp \
    widgets/ipythonconsole/control.cpp \
    plugins/plugins_findinfiles.cpp \
    plugins/plugins_explorer.cpp \
    plugins/outlineexplorer.cpp \
    plugins/configdialog.cpp \
    plugins/plugins_editor.cpp \
    plugins/history.cpp \
    plugins/ipythonconsole.cpp \
    plugins/workingdirectory.cpp \
    widgets/projects/type/projects_type.cpp \
    widgets/projects/projects_config.cpp \
    configparser.cpp \
    widgets/projects/projectdialog.cpp \
    plugins/projects.cpp \
    plugins/maininterpreter.cpp \
    plugins/runconfig.cpp \
    utils/introspection/plugin_client.cpp

HEADERS += \
    utils/icon_manager.h \
    config/base.h \
    widgets/helperwidgets.h \
    widgets/arraybuilder.h \
    utils/sourcecode.h \
    saxutils.h \
    os.h \
    textwrap.h \
    str.h \
    widgets/mixins.h \
    widgets/calltip.h \
    widgets/tests/test_mixins.h \
    widgets/sourcecode/widgets_base.h \
    builtins.h \
    keyword.h \
    utils/syntaxhighlighters.h \
    widgets/sourcecode/codeeditor.h \
    widgets/sourcecode/kill_ring.h \
    utils/qthelpers.h \
    widgets/editortools.h \
    widgets/onecolumntree.h \
    widgets/variableexplorer/texteditor.h \
    widgets/variableexplorer/arrayeditor.h \
    widgets/variableexplorer/objecteditor.h \
    widgets/variableexplorer/collectionseditor.h \
    nsview.h \
    widgets/variableexplorer/importwizard.h \
    widgets/explorer.h \
    utils/encoding.h \
    utils/check.h \
    utils/programs.h \
    utils/misc.h \
    widgets/comboboxes.h \
    widgets/waitingspinner.h \
    widgets/findinfiles.h \
    fnmatch.h \
    config/config_main.h \
    widgets/editor.h \
    widgets/tabs.h \
    widgets/fileswitcher.h \
    config/utils.h \
    widgets/status.h \
    widgets/colors.h \
    plugins/plugins.h \
    config/gui.h \
    config/fonts.h \
    widgets/projects/projects_explorer.h \
    widgets/findreplace.h \
    widgets/pathmanager.h \
    widgets/github/gh_login.h \
    utils/external/github.h \
    widgets/github/backend.h \
    widgets/sourcecode/terminal.h \
    widgets/reporterror.h \
    widgets/ipythonconsole/control.h \
    app/mainwindow.h \
    plugins/plugins_findinfiles.h \
    plugins/plugins_explorer.h \
    plugins/outlineexplorer.h \
    plugins/configdialog.h \
    plugins/plugins_editor.h \
    plugins/history.h \
    plugins/ipythonconsole.h \
    plugins/runconfig.h \
    plugins/workingdirectory.h \
    widgets/projects/type/projects_type.h \
    widgets/projects/projects_config.h \
    configparser.h \
    widgets/projects/projectdialog.h \
    plugins/projects.h \
    plugins/maininterpreter.h \
    utils/introspection/plugin_client.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

