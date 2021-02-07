#include "objecteditor.h"

QWidget* create_dialog(const QVector<QVector<QVariant>>& obj,const QString& obj_name)
{
    bool readonly = false;
    ArrayEditor* dialog = new ArrayEditor;
    if (!dialog->setup_and_check(obj, obj_name, readonly))
        return nullptr;
    return dialog;
}

QVector<QVector<QVariant>> oedit(const QVector<QVector<QVariant>>& obj, bool modal)
{
    QString obj_name;
    if (modal)
        obj_name = "";

    QWidget* result = create_dialog(obj, obj_name);
    if (modal) {
        ArrayEditor* editor = qobject_cast<ArrayEditor*>(result);
        if (editor->exec())
            return editor->get_value();
    }
}

void test()
{
    QVector<QVector<QVariant>> obj;
    for (int i = 0; i < 10; ++i) {
        QVector<QVariant> tmp;
        for (int j = 0; j < 10; ++j) {
            tmp.append(double(std::rand() % 100));
        }
        obj.append(tmp);
    }

    qDebug() << oedit(obj);
}
