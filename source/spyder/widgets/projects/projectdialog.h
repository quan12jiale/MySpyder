#ifndef PROJECTDIALOG_H
#define PROJECTDIALOG_H

#include "config/base.h"
#include "utils/icon_manager.h"

#include <QtWidgets>

class ProjectDialog : public QDialog
{
    Q_OBJECT
signals:
    void sig_project_creation_requested(QString, QString, QStringList);
public:
    QString project_name;
    QString location;

    QGroupBox* groupbox;
    QRadioButton* radio_new_dir;
    QRadioButton* radio_from_dir;

    QLabel* label_project_name;
    QLabel* label_location;
    QLabel* label_project_type;
    QLabel* label_python_version;

    QLineEdit* text_project_name;
    QLineEdit* text_location;
    QComboBox* combo_project_type;
    QComboBox* combo_python_version;

    QToolButton* button_select_location;
    QPushButton* button_cancel;
    QPushButton* button_create;

    QDialogButtonBox* bbox;

    ProjectDialog(QWidget* parent);
    QStringList _get_project_types() const;

public slots:
    void select_location();
    void update_location(const QString& text="");
    void create_project();
};

#endif // PROJECTDIALOG_H
