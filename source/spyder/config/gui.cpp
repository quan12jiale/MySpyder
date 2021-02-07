#include "gui.h"


namespace gui {

bool font_is_installed(const QString& font)
{
    QFontDatabase db;
    foreach (QString fam, db.families()) {
        if (fam == font)
            return true;
    }
    return false;
}

QString get_family(const QString& family)
{
    if (font_is_installed(family))
        return family;
    //qDebug() << "Warning: None of the following fonts is installed: " << family;
    return QFont().family();
}

static QHash<QPair<QString,QString>, QFont> FONT_CACHE;

QFont get_font(const QString& section, const QString& option, int font_size_delta)
{
    QFont font = FONT_CACHE.value(QPair<QString,QString>(section, option), QFont());

    if (!FONT_CACHE.contains(QPair<QString,QString>(section, option))) {
        QVariant variant = CONF_get(section, option+"/family");
        if (variant.isNull())
            return QFont();
        QString families = variant.toString();

        QString family = get_family(families);
        QFont::Weight weight = QFont::Normal;
        bool italic = CONF_get(section, option+"/italic", false).toBool();

        if (CONF_get(section, option+"/bold",false).toBool())
            weight = QFont::Bold;

        int size = CONF_get(section, option+"/size",9).toInt() + font_size_delta;
        font = QFont(family, size, weight);
        font.setItalic(italic);
        FONT_CACHE[QPair<QString,QString>(section, option)] = font;
    }
    int size = CONF_get(section, option+"/size",9).toInt() + font_size_delta;
    font.setPointSize(size);
    return font;
}

void set_font(const QFont& font, const QString& section, const QString& option)
{
    CONF_set(section, option+"/family", font.family());
    CONF_set(section, option+"/size", font.pointSize());
    CONF_set(section, option+"/italic", font.italic());
    CONF_set(section, option+"/bold", font.bold());
    FONT_CACHE[QPair<QString,QString>(section, option)] = font;
}

QString get_shortcut(const QString& context, const QString& name)
{
    return CONF_get("shortcuts", QString("%1/%2").arg(context).arg(name)).toString();
}

void set_shortcut(const QString& context, const QString& name, const QString& keystr)
{
    CONF_set("shortcuts", QString("%1/%2").arg(context).arg(name), keystr);
}

QHash<QString, ColorBoolBool> get_color_scheme()
{
    QHash<QString, ColorBoolBool> color_scheme;
    foreach (QString key, sh::COLOR_SCHEME_KEYS.keys()) {
        QVariant variant = CONF_get("color_schemes", key);
        color_scheme[key] = variant.value<ColorBoolBool>();
    }
    return color_scheme;
}

} // namespace gui
