#include "reporterror.h"

// 在spyder\__init__.py
QString __project_url__ = "https://github.com/spyder-ide/spyder";
QString __forum_url__   = "https://groups.google.com/group/spyderlib";
QString __trouble_url__ = __project_url__ + "/wiki/Troubleshooting-Guide-and-FAQ";
QString __trouble_url_short__ = "https://tinyurl.com/SpyderHelp";
QString __website_url__ = "https://www.spyder-ide.org/";

static int TITLE_MIN_CHARS = 15;
static int DESC_MIN_CHARS = 50;

DescriptionWidget::DescriptionWidget(QWidget* parent)
    : CodeEditor (parent)
{
    QHash<QString, QVariant> kwargs;
    kwargs["language"] = "md";

    QHash<QString,ColorBoolBool> color_scheme;
    color_scheme["background"] = ColorBoolBool("#ffffff");
    color_scheme["currentline"] = ColorBoolBool("#e1f0d1");
    color_scheme["currentcell"] = ColorBoolBool("#edfcdc");
    color_scheme["occurrence"] = ColorBoolBool("#ffff99");
    color_scheme["ctrlclick"] = ColorBoolBool("#0000ff");
    color_scheme["sideareas"] = ColorBoolBool("#efefef");
    color_scheme["matched_p"] = ColorBoolBool("#99ff99");
    color_scheme["unmatched_p"] = ColorBoolBool("#ff9999");
    color_scheme["normal"] = ColorBoolBool("#000000");
    color_scheme["keyword"] = ColorBoolBool("#00007f", true);
    color_scheme["builtin"] = ColorBoolBool("#000000");
    color_scheme["definition"] = ColorBoolBool("#007f7f", true);
    color_scheme["comment"] = ColorBoolBool("#007f00");
    color_scheme["string"] = ColorBoolBool("#7f007f");
    color_scheme["number"] = ColorBoolBool("#007f7f");
    color_scheme["instance"] = ColorBoolBool("#000000", false, true);
    QHash<QString, QVariant> dict;
    for (auto it=color_scheme.begin();it!=color_scheme.end();it++) {
        QString key = it.key();
        QVariant val = QVariant::fromValue(it.value());
        dict[key] = val;
    }
    kwargs["color_scheme"] = dict;

    kwargs["linenumbers"] = false;
    kwargs["scrollflagarea"] = false;
    kwargs["wrap"] = true;
    kwargs["edge_line"] = false;
    kwargs["highlight_current_line"] = false;
    kwargs["highlight_current_cell"] = false;
    kwargs["occurrence_highlighting"] = false;
    kwargs["auto_unindent"] = false;
    this->setup_editor(kwargs);

    this->set_font(gui::get_font());

    this->header = "### What steps will reproduce the problem?\n\n"
                   "<!--- You can use Markdown here --->\n\n";
    this->set_text(this->header);
    this->move_cursor(this->header.size());
    this->header_end_pos = this->get_position("eof");
}

void DescriptionWidget::remove_text()
{
    this->truncate_selection(this->header_end_pos);
    this->remove_selected_text();
}

void DescriptionWidget::cut()
{
    this->truncate_selection(this->header_end_pos);
    if (this->has_selected_text())
        CodeEditor::cut();
}

void DescriptionWidget::keyPressEvent(QKeyEvent *event)
{
    QString text = event->text();
    int key = event->key();
    Qt::KeyboardModifiers ctrl = event->modifiers() & Qt::ControlModifier;

    int cursor_position = this->get_position("cursor");
    if (cursor_position < this->header_end_pos)
        this->restrict_cursor_position(this->header_end_pos, "eof");
    else if (key == Qt::Key_Delete) {
        if (this->has_selected_text())
            this->remove_text();
        else
            this->stdkey_clear();
    }
    else if (key == Qt::Key_Backspace) {
        if (this->has_selected_text())
            this->remove_text();
        else if (this->header_end_pos == cursor_position)
            return;
        else
            this->stdkey_backspace();
    }
    else if (key == Qt::Key_X && ctrl)
        this->cut();
    else
        CodeEditor::keyPressEvent(event);
}

void DescriptionWidget::contextMenuEvent(QContextMenuEvent *event)
{
    // Reimplemented Qt Method to not show the context menu.
    Q_UNUSED(event);
}


/********** ShowErrorWidget **********/
ShowErrorWidget::ShowErrorWidget(QWidget* parent)
    : ConsoleBaseWidget (parent)
{
    // 下面2行是TracebackLinksMixin构造函数中的全部代码
    this->__cursor_changed = false;
    this->setMouseTracking(true);

    this->setReadOnly(true);
}

void ShowErrorWidget::mouseReleaseEvent(QMouseEvent *event)
{
    QPlainTextEdit::mouseReleaseEvent(event);
    QString text = this->get_line_at(event->pos());
    if (misc::get_error_match(text) && !this->has_selected_text()) {
        emit go_to_error(text);
    }
}

void ShowErrorWidget::mouseMoveEvent(QMouseEvent *event)
{
    QString text = this->get_line_at(event->pos());
    if (misc::get_error_match(text)) {
        if (this->__cursor_changed) {
            QApplication::setOverrideCursor(QCursor(Qt::PointingHandCursor));
            this->__cursor_changed = true;
        }
        event->accept();
        return;
    }
    if (this->__cursor_changed) {
        QApplication::restoreOverrideCursor();
        this->__cursor_changed = false;
    }
    QPlainTextEdit::mouseMoveEvent(event);
}

void ShowErrorWidget::leaveEvent(QEvent *event)
{
    if (this->__cursor_changed) {
        QApplication::restoreOverrideCursor();
        this->__cursor_changed = false;
    }
    QPlainTextEdit::leaveEvent(event);
}


/********** SpyderErrorDialog **********/
SpyderErrorDialog::SpyderErrorDialog(QWidget* parent, bool is_report)
    : QDialog (parent)
{
    this->is_report = is_report;

    this->setWindowTitle("Issue reporter");
    this->setModal(true);

    this->error_traceback = "";

    QString title;
    if (this->is_report)
        title = "Please fill the following information";
    else
        title = "Spyder has encountered an internal problem!";
    QLabel* main_label = new QLabel(QString("<h3>%1</h3>"
                                            "Before reporting this problem, <i>please</i> consult our "
                                            "comprehensive "
                                            "<b><a href=\"%2\">Troubleshooting Guide</a></b> "
                                            "which should help solve most issues, and search for "
                                            "<b><a href=\"%3\">known bugs</a></b> "
                                            "matching your error message or problem description for a "
                                            "quicker solution.").arg(title).arg(__trouble_url__)
                                    .arg(__project_url__));
    main_label->setOpenExternalLinks(true);
    main_label->setWordWrap(true);
    main_label->setAlignment(Qt::AlignJustify);
    main_label->setStyleSheet("font-size: 12px;");

    this->title = new QLineEdit;
    connect(this->title, SIGNAL(textChanged(const QString &)), this, SLOT(_contents_changed()));
    this->title_chars_label = new QLabel(QString("%1 more characters "
                                                 "to go...").arg(TITLE_MIN_CHARS));
    QFormLayout* form_layout = new QFormLayout;
    QString red_asterisk = "<font color=\"Red\">*</font>";
    QLabel* title_label = new QLabel(QString("<b>Title</b>: %1").arg(red_asterisk));
    form_layout->setWidget(0, QFormLayout::LabelRole, title_label);
    form_layout->setWidget(0, QFormLayout::FieldRole, this->title);

    QLabel* steps_header = new QLabel(QString("<b>Steps to reproduce:</b> %1").arg(red_asterisk));
    QLabel* steps_text = new QLabel("Please enter a detailed step-by-step "
                                    "description (in English) of what led up to "
                                    "the problem below. Issue reports without a "
                                    "clear way to reproduce them will be closed.");
    steps_text->setWordWrap(true);
    steps_text->setAlignment(Qt::AlignJustify);
    steps_text->setStyleSheet("font-size: 12px;");

    this->input_description = new DescriptionWidget(this);
    connect(input_description, SIGNAL(textChanged()), this, SLOT(_contents_changed()));

    this->details = new ShowErrorWidget(this);
    this->details->set_pythonshell_font(gui::get_font());
    this->details->hide();

    this->initial_chars = this->input_description->toPlainText().size();
    this->desc_chars_label = new QLabel(QString("%1 more characters "
                                                "to go...").arg(DESC_MIN_CHARS));

    this->dismiss_box = new QCheckBox("Hide all future errors during this session");
    if (this->is_report)
        this->dismiss_box->hide();

    QIcon gh_icon = ima::icon("github");
    this->submit_btn = new QPushButton(gh_icon, "Submit to Github");
    this->submit_btn->setEnabled(false);
    connect(submit_btn, SIGNAL(clicked(bool)), this, SLOT(_submit_to_github()));

    this->details_btn = new QPushButton("Show details");
    connect(details_btn, SIGNAL(clicked(bool)), this, SLOT(_show_details()));
    if (this->is_report)
        this->details_btn->hide();

    this->close_btn = new QPushButton("Close");
    if (this->is_report)
        connect(close_btn, SIGNAL(clicked(bool)), this, SLOT(reject()));

    QHBoxLayout* buttons_layout = new QHBoxLayout;
    buttons_layout->addWidget(this->submit_btn);
    buttons_layout->addWidget(this->details_btn);
    buttons_layout->addWidget(this->close_btn);

    QVBoxLayout* layout = new QVBoxLayout;
    layout->addWidget(main_label);
    layout->addSpacing(20);
    layout->addLayout(form_layout);
    layout->addWidget(this->title_chars_label);
    layout->addSpacing(12);
    layout->addWidget(steps_header);
    layout->addSpacing(-1);
    layout->addWidget(steps_text);
    layout->addSpacing(1);
    layout->addWidget(this->input_description);
    layout->addWidget(this->details);
    layout->addWidget(this->desc_chars_label);
    layout->addSpacing(15);
    layout->addWidget(this->dismiss_box);
    layout->addSpacing(15);
    layout->addLayout(buttons_layout);
    layout->setContentsMargins(25, 20, 25, 10);
    this->setLayout(layout);

    this->resize(570, 600);
    this->title->setFocus();

    this->setTabOrder(this->title, this->input_description);
}

void SpyderErrorDialog::_submit_to_github()
{
    qDebug()<<__FILE__<<__FUNCTION__;
    // 该槽函数需要GithubBackend类

    // 该函数调用了MainWindow::report_issue(self, body=None,
    // title=None, open_webpage=False)函数
}

void SpyderErrorDialog::append_traceback(const QString &text)
{
    this->error_traceback += text;
}

void SpyderErrorDialog::_show_details()
{
    if (this->details->isVisible()) {
        this->details->hide();
        this->details_btn->setText("Show details");
    }
    else {
        this->resize(570, 700);
        this->details->document()->setPlainText("");
        this->details->append_text_to_shell(this->error_traceback, true, false);
        this->details->show();
        this->details_btn->setText("Hide details");
    }
}

void SpyderErrorDialog::_contents_changed()
{
    int desc_chars = this->input_description->toPlainText().size() - this->initial_chars;
    if (desc_chars < DESC_MIN_CHARS) {
        this->desc_chars_label->setText(QString("%1 %2").arg(DESC_MIN_CHARS - desc_chars)
                                        .arg("more characters to go..."));
    }
    else
        this->desc_chars_label->setText("Description complete; thanks!");

    int title_chars = this->title->text().size();
    if (title_chars < TITLE_MIN_CHARS) {
        this->title_chars_label->setText(QString("%1 %2").arg(TITLE_MIN_CHARS - title_chars)
                                         .arg("more characters to go..."));
    }
    else
        this->title_chars_label->setText("Title complete; thanks!");

    bool submission_enabled = desc_chars >= DESC_MIN_CHARS && title_chars >= TITLE_MIN_CHARS;
    this->submit_btn->setEnabled(submission_enabled);
}


static void test()
{
    // 无bug
    SpyderErrorDialog dlg;
    dlg.show();
    dlg.exec();
}
