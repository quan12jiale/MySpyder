#pragma once

#include "base.h"
#include "fonts.h"

#include <QSize>
#include <QPoint>
#include <QVariant>
#include <QShortcut>
#include <QSettings>
#include <QDataStream>
#include <QStringList>

struct ColorBoolBool {
    QString color;
    bool bold;
    bool italic;
    ColorBoolBool() = default;
    explicit ColorBoolBool(const QString& _color,bool _bold=false,bool _italic=false)
        : color(_color),bold(_bold),italic(_italic){}
    friend QDataStream &operator<<(QDataStream &out, const ColorBoolBool &obj)
    {
        out << obj.color << obj.bold << obj.italic;
        return out;
    }
    friend QDataStream &operator>>(QDataStream &in, ColorBoolBool &obj)
    {
        in >> obj.color >> obj.bold >> obj.italic;
        return in;
    }
    friend QDebug &operator<<(QDebug &debug, const ColorBoolBool &obj)
    {
        debug << obj.color << obj.bold << obj.italic;
        return debug;
        //qDebug () <<key<< settings.value(key).value<ColorBoolBool>();需要这样使用，得在前面加点东西
    }
};
Q_DECLARE_METATYPE(ColorBoolBool)


struct OrientationStrIntlist
{
    bool is_vertical;
    QString cfname;
    QList<int> clines;
    OrientationStrIntlist() = default;
    OrientationStrIntlist(bool _is_vertical,
                          QString _cfname,
                          QList<int> _clines)
    {
        is_vertical = _is_vertical;
        cfname = _cfname;
        clines = _clines;
    }
    friend QDataStream &operator<<(QDataStream &out, const OrientationStrIntlist &obj)
    {
        out << obj.is_vertical << obj.cfname << obj.clines;
        return out;
    }
    friend QDataStream &operator>>(QDataStream &in, OrientationStrIntlist &obj)
    {
        in >> obj.is_vertical >> obj.cfname >> obj.clines;
        return in;
    }
};

struct SplitSettings
{
    QString hexstate;
    QList<int> sizes;
    QList<OrientationStrIntlist> splitsettings;
    SplitSettings() = default;
    SplitSettings(QString _hexstate,
                  QList<int> _sizes,
                  QList<OrientationStrIntlist> _splitsettings)
    {
        hexstate = _hexstate;
        sizes = _sizes;
        splitsettings = _splitsettings;
    }
    friend QDataStream &operator<<(QDataStream &out, const SplitSettings &obj)
    {
        out << obj.hexstate << obj.sizes << obj.splitsettings;
        return out;
    }
    friend QDataStream &operator>>(QDataStream &in, SplitSettings &obj)
    {
        in >> obj.hexstate >> obj.sizes >> obj.splitsettings;
        return in;
    }
};
Q_DECLARE_METATYPE(SplitSettings)

struct LayoutSettings
{
    QSize size;
    QPoint pos;
    bool is_maximized;
    bool is_fullscreen;
    QString hexstate;
    SplitSettings splitsettings;
    LayoutSettings() = default;
    LayoutSettings(QSize size,QPoint pos,bool is_maximized,bool is_fullscreen,
                   QString hexstate,SplitSettings splitsettings)
        :size(size), pos(pos), is_maximized(is_maximized),
          is_fullscreen(is_fullscreen),
          hexstate(hexstate),splitsettings(splitsettings) {}
    friend QDataStream &operator<<(QDataStream &out, const LayoutSettings &obj)
    {
        out << obj.size << obj.pos << obj.is_maximized << obj.is_fullscreen << obj.hexstate << obj.splitsettings;
        return out;
    }
    friend QDataStream &operator>>(QDataStream &in, LayoutSettings &obj)
    {
        in >> obj.size >> obj.pos >> obj.is_maximized >> obj.is_fullscreen >> obj.hexstate >> obj.splitsettings;
        return in;
    }
};
Q_DECLARE_METATYPE(LayoutSettings)

struct WindowSettings
{
    QString hexstate;
    QSize window_size;
    QSize prefs_dialog_size;
    QPoint pos;
    bool is_maximized;
    bool is_fullscreen;
    WindowSettings() = default;
    WindowSettings(QString _hexstate, QSize _window_size,
                   QSize _prefs_dialog_size, QPoint _pos,
                   bool _is_maximized, bool _is_fullscreen)
    {
        hexstate = _hexstate;
        window_size = _window_size;
        prefs_dialog_size = _prefs_dialog_size;
        pos = _pos;
        is_maximized = _is_maximized;
        is_fullscreen = _is_fullscreen;
    }
};


struct FocusWidgetProperties
{
    QWidget* widget;
    bool console;
    bool not_readonly;
    bool readwrite_editor;
    FocusWidgetProperties() { widget = nullptr; }
    FocusWidgetProperties(QWidget* _widget, bool _console, bool _not_readonly,
                          bool _readwrite_editor)
    {
        widget = _widget;
        console = _console;
        not_readonly = _not_readonly;
        readwrite_editor = _readwrite_editor;
    }
};

struct StrStrActions
{
    QString title;
    QString object_name;
    QList<QAction*> actions;
    StrStrActions(QString _title, QString _object_name, QList<QAction*> _actions)
    {
        title = _title;
        object_name = _object_name;
        actions = _actions;
    }
};


struct ShortCutStrStrBool
{
    QObject* qaction_or_qshortcut;
    QString context,name;
    bool add_sc_to_tip;
    ShortCutStrStrBool() = default;
    ShortCutStrStrBool(QObject* _qaction_or_qshortcut, const QString& cxt,
                       const QString& _name, bool _add_sc_to_tip)
    {
        qaction_or_qshortcut = _qaction_or_qshortcut;
        context = cxt;
        name = _name;
        add_sc_to_tip = _add_sc_to_tip;
    }
};

struct ShortCutStrStr   //用于create_shortcut
{
    QShortcut* qsc;
    QString context,name;
    ShortCutStrStr() = default;
    ShortCutStrStr(QShortcut* _qsc,const QString& cxt,const QString& _name):
        qsc(_qsc),context(cxt),name(_name){}
};

class Widget_get_shortcut_data
{
public:
    virtual QList<ShortCutStrStr> get_shortcut_data() = 0;
};


extern QStringList EXCLUDE_PATTERNS;

extern unsigned short OPEN_FILES_PORT;

extern QList<QPair<QString, QHash<QString,QVariant>>> DEFAULTS;

QString qbytearray_to_str(const QByteArray& qba);

QString getIniPath();
QVariant get_default(const QString& section, const QString& option);
QVariant CONF_get(const QString& section, const QString& option, const QVariant& _default=QVariant());

void CONF_set(const QString& section, const QString& option, const QVariant& value);

QVariant get_option(const QString& CONF_SECTION, const QString& option, const QVariant& _default=QVariant());
QHash<QString, QVariant> Factory(const QString& CONF_SECTION);
