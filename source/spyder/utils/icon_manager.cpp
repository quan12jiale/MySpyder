#include "icon_manager.h"

namespace ima {

// int QMetaEnum::keysToValue(const char *keys)函数是用于根据'|'分割的字符串，获得枚举的异或值。
// QStyle::StandardPixmap枚举值是连续的，因此改用QMetaEnum::keyToValue接口
QIcon get_std_icon(QString name, int size)
{
    if (!name.startsWith("SP_"))
        name = "SP_" + name;

    // 根据字符串获取某个类的枚举
    QMetaObject metaObject = QStyle::staticMetaObject;
    int idx = metaObject.indexOfEnumerator("StandardPixmap");
    QMetaEnum metaEnum = metaObject.enumerator(idx);
    int val = metaEnum.keyToValue(name.toUtf8());
    QStyle::StandardPixmap stdIcon = static_cast<QStyle::StandardPixmap>(val);

    QIcon icon = QWidget().style()->standardIcon(stdIcon);
    if (size <= 0)
        return icon;
    else
        return QIcon(icon.pixmap(size,size));
}

QIcon get_icon(const QString& name,const QIcon& _default,bool resample)
{
    QString icon_path = get_image_path(name, QString());
    QIcon icon;
    if (!icon_path.isEmpty())
        icon = QIcon(QPixmap(icon_path));
    else
        icon = _default;
    if (resample) {
        QIcon icon0;
        QList<int> list = { 16, 24, 32, 48, 96, 128, 256, 512 };
        foreach (int size, list) {
            icon0.addPixmap(icon.pixmap(size, size));
        }
        return icon0;
    }
    else
        return icon;
}

QIcon get_icon(const QString& name,const QString& _default,bool resample)
{
    QString icon_path = get_image_path(name, QString());
    QIcon icon;
    if (!icon_path.isEmpty())
        icon = QIcon(QPixmap(icon_path));
    else if (_default.isEmpty()) {
        try {
            icon = get_std_icon(name.left(name.size()-4));
        }
        catch (...) {
            icon = QIcon(QPixmap(get_image_path(name, _default)));
        }
    }
    else
        icon = QIcon(QPixmap(get_image_path(name, _default)));
    if (resample) {
        QIcon icon0;
        QList<int> list = { 16, 24, 32, 48, 96, 128, 256, 512 };
        foreach (int size, list) {
            icon0.addPixmap(icon.pixmap(size, size));
        }
        return icon0;
    }
    else
        return icon;
}

QIcon icon(const QString& name,bool resample,const QString& icon_path)
{
    // TODO实现qta第三方库
    QIcon icon = get_icon(name+".png",QString(),resample);
    if (!icon_path.isEmpty()) {
        QString path = icon_path + "/" + name + ".png";
        QFileInfo info(path);
        if (info.isFile())
            icon = QIcon(QPixmap(path));
    }
    if (!icon.isNull())
        return icon;
    else
        return QIcon();
}

} // namespace ima
