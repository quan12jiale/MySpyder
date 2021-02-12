#include "codeeditor.h"

GoToLineDialog::GoToLineDialog(CodeEditor* editor)
    : QDialog (editor, Qt::WindowTitleHint
               | Qt::WindowCloseButtonHint)
{
    setAttribute(Qt::WA_DeleteOnClose);
    lineno = -1;
    this->editor = editor;

    setWindowTitle("Editor");
    setModal(true);

    QLabel *label = new QLabel("Go to line:");
    lineedit = new QLineEdit;
    QIntValidator* validator = new QIntValidator(lineedit);
    validator->setRange(1, editor->get_line_count());
    lineedit->setValidator(validator);
    connect(lineedit,SIGNAL(textChanged(const QString &)),
            this,SLOT(text_has_changed(const QString &)));
    QLabel *cl_label = new QLabel("Current line:");
    QLabel *cl_label_v = new QLabel(QString("<b>%1</b>").arg(editor->get_cursor_line_number()));
    QLabel *last_label = new QLabel("Line count:");
    QLabel *last_label_v = new QLabel(QString("%1").arg(editor->get_line_count()));

    QGridLayout* glayout = new QGridLayout();
    glayout->addWidget(label, 0, 0, Qt::AlignVCenter|Qt::AlignRight);
    glayout->addWidget(lineedit, 0, 1, Qt::AlignVCenter);
    glayout->addWidget(cl_label, 1, 0, Qt::AlignVCenter|Qt::AlignRight);
    glayout->addWidget(cl_label_v, 1, 1, Qt::AlignVCenter);
    glayout->addWidget(last_label, 2, 0, Qt::AlignVCenter|Qt::AlignRight);
    glayout->addWidget(last_label_v, 2, 1, Qt::AlignVCenter);

    QDialogButtonBox* bbox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,
                            Qt::Vertical, this);
    connect(bbox,SIGNAL(accepted()),this,SLOT(accept()));
    connect(bbox,SIGNAL(rejected()),this,SLOT(reject()));
    QVBoxLayout* btnlayout = new QVBoxLayout;
    btnlayout->addWidget(bbox);
    btnlayout->addStretch(1);

    QPushButton* ok_button = bbox->button(QDialogButtonBox::Ok);
    ok_button->setEnabled(false);
    connect(lineedit,&QLineEdit::textChanged,
            [=](const QString& text) { ok_button->setEnabled(text.size()>0); });

    QHBoxLayout* layout = new QHBoxLayout;
    layout->addLayout(glayout);
    layout->addLayout(btnlayout);
    setLayout(layout);

    lineedit->setFocus();
}

void GoToLineDialog::text_has_changed(const QString &text)
{
    if (!text.isEmpty())
        lineno = text.toInt();
    else
        lineno = -1;
}

int GoToLineDialog::get_line_number()
{
    return lineno;
}


//***********Viewport widgets

LineNumberArea::LineNumberArea(CodeEditor* editor)
    : QWidget (editor)
{
    code_editor = editor;
    setMouseTracking(true);
}

QSize LineNumberArea::sizeHint() const
{
    return QSize(code_editor->compute_linenumberarea_width(), 0);
}

void LineNumberArea::paintEvent(QPaintEvent *event)
{
    code_editor->linenumberarea_paint_event(event);
}

void LineNumberArea::mouseMoveEvent(QMouseEvent *event)
{
    code_editor->linenumberarea_mousemove_event(event);
}

void LineNumberArea::mouseDoubleClickEvent(QMouseEvent *event)
{
    code_editor->linenumberarea_mousedoubleclick_event(event);
}

void LineNumberArea::mousePressEvent(QMouseEvent *event)
{
    code_editor->linenumberarea_mousepress_event(event);
}

void LineNumberArea::mouseReleaseEvent(QMouseEvent *event)
{
    code_editor->linenumberarea_mouserelease_event(event);
}

void LineNumberArea::wheelEvent(QWheelEvent *event)
{
    code_editor->wheelEvent(event);
}

int ScrollFlagArea::WIDTH = 12;
int ScrollFlagArea::FLAGS_DX = 4;
int ScrollFlagArea::FLAGS_DY = 2;

ScrollFlagArea::ScrollFlagArea(CodeEditor* editor)
    : QWidget (editor)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    code_editor = editor;
    connect(editor->verticalScrollBar(),&QAbstractSlider::valueChanged,
            [=](int) { this->repaint(); });
}

QSize ScrollFlagArea::sizeHint() const
{
    return QSize(WIDTH, 0);
}

void ScrollFlagArea::paintEvent(QPaintEvent *event)
{
    code_editor->scrollflagarea_paint_event(event);
}

void ScrollFlagArea::mousePressEvent(QMouseEvent *event)
{
    QScrollBar* vsb = code_editor->verticalScrollBar();
    double value = this->position_to_value(event->pos().y()-1);
    vsb->setValue(static_cast<int>(value-.5*vsb->pageStep()));
}

double ScrollFlagArea::get_scale_factor(bool slider) const
{
    int delta = slider ? 0 : 2;
    QScrollBar* vsb = code_editor->verticalScrollBar();
    int position_height = vsb->height() - delta - 1;
    int value_height = vsb->maximum()-vsb->minimum()+vsb->pageStep();
    return static_cast<double>(position_height) / value_height;
}

double ScrollFlagArea::value_to_position(int y, bool slider) const
{
    int offset = slider ? 0 : 1;
    QScrollBar* vsb = code_editor->verticalScrollBar();
    return (y-vsb->minimum())*this->get_scale_factor(slider)+offset;
}

double ScrollFlagArea::position_to_value(int y, bool slider) const
{
    int offset = slider ? 0 : 1;
    QScrollBar* vsb = code_editor->verticalScrollBar();
    return vsb->minimum()+qMax(0.0, (y-offset)/this->get_scale_factor(slider));
}

QRect ScrollFlagArea::make_flag_qrect(int position) const
{
    return QRect(FLAGS_DX/2, position-FLAGS_DY/2,
                 WIDTH-FLAGS_DX, FLAGS_DY);
}

QRect ScrollFlagArea::make_slider_range(int value) const
{
    QScrollBar* vsb = code_editor->verticalScrollBar();
    double pos1 = value_to_position(value, true);
    double pos2 = value_to_position(value + vsb->pageStep(), true);
    return QRect(1, static_cast<int>(pos1), WIDTH-2, static_cast<int>(pos2-pos1+1));
}

void ScrollFlagArea::wheelEvent(QWheelEvent *event)
{
    code_editor->wheelEvent(event);
}


//============EdgeLine
EdgeLine::EdgeLine(QWidget* editor)
    : QWidget (editor)
{
    code_editor = editor;
    column = 79;
    setAttribute(Qt::WA_TransparentForMouseEvents);
}

void EdgeLine::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    QColor color(Qt::darkGray);
    color.setAlphaF(0.5);
    painter.fillRect(event->rect(), color);
}


//============BlockUserData
BlockUserData::BlockUserData(CodeEditor* editor)
    : QTextBlockUserData ()
{
    this->editor = editor;
    this->breakpoint = false;
    this->breakpoint_condition = QString();
    this->code_analysis = QList<QPair<QString,bool>>();
    this->todo = "";
    this->editor->blockuserdata_list.append(this);
}

BlockUserData::~BlockUserData()
{
	editor->blockuserdata_list.removeOne(this);
}

bool BlockUserData::is_empty()
{
    return (!breakpoint) && (code_analysis.isEmpty()) && (todo.isEmpty());
}


static void set_scrollflagarea_painter(QPainter* painter,const QString& light_color)
{
    painter->setPen(QColor(light_color).darker(120));
    painter->setBrush(QBrush(QColor(light_color)));
}
//提供该函数对QColor的重载是为了scrollflagarea_paint_event()函数
static void set_scrollflagarea_painter(QPainter* painter,const QColor& light_color)
{
    painter->setPen(QColor(light_color).darker(120));
    painter->setBrush(QBrush(QColor(light_color)));
}

QString get_file_language(const QString& filename,QString text)
{
    QFileInfo info(filename);
    QString ext = info.suffix();
    QString language = ext;
    if (ext.isEmpty()) {
        if (text.isEmpty()) {
            text = encoding::read(filename);
        }
        foreach (QString line, splitlines(text)) {
            if (line.trimmed().isEmpty())
                continue;
            if (line.startsWith("#!")) {
                QString shebang = line.mid(2);
                if (shebang.contains("python"))
                    language = "python";
            }
            else
                break;
        }
    }
    return language;
}


/********** CodeEditor **********/
CodeEditor::CodeEditor(QWidget* parent)
    : TextEditBaseWidget (parent)
{
    this->LANGUAGES = {{"Python", {"PythonSH", "#"}},
                       {"Cpp", {"CppSH", "//"}},
                       {"Markdown", {"MarkdownSH", "#"}}};
    this->TAB_ALWAYS_INDENTS = QStringList({"py", "pyw", "python", "c", "cpp", "cl", "h"});

    setFocusPolicy(Qt::StrongFocus);

    setCursorWidth(CONF_get("main", "cursor/width").toInt());

    edge_line_enabled = true;
    edge_line = new EdgeLine(this);

    blanks_enabled = false;

    markers_margin = true;
    markers_margin_width = 15;
    error_pixmap = ima::icon("error").pixmap(QSize(14, 14));
    warning_pixmap = ima::icon("warning").pixmap(QSize(14, 14));
    todo_pixmap = ima::icon("todo").pixmap(QSize(14, 14));
    bp_pixmap = ima::icon("breakpoint_big").pixmap(QSize(14, 14));
    bpc_pixmap = ima::icon("breakpoint_cond_big").pixmap(QSize(14, 14));

    linenumbers_margin = true;
    linenumberarea_enabled = false;
    linenumberarea = new LineNumberArea(this);
    connect(this,SIGNAL(blockCountChanged(int)),
            this,SLOT(update_linenumberarea_width(int)));
    connect(this,SIGNAL(updateRequest(QRect,int)),
            this,SLOT(update_linenumberarea(QRect,int)));
    linenumberarea_pressed = -1;
    linenumberarea_released = -1;

    occurrence_color = QColor();
    ctrl_click_color = QColor();
    sideareas_color = QColor();
    matched_p_color = QColor();
    unmatched_p_color = QColor();
    normal_color = QColor();
    comment_color = QColor();

    linenumbers_color = QColor(Qt::darkGray);

    highlighter_class = "TextSH";
    highlighter = nullptr;
    //QString ccs = "Spyder";
    //if (!sh::COLOR_SCHEME_NAMES.contains(ccs))
    //    ccs = sh::COLOR_SCHEME_NAMES[0];
    this->color_scheme = sh::get_color_scheme();

    highlight_current_line_enabled = false;

    scrollflagarea_enabled = false;
    scrollflagarea = new ScrollFlagArea(this);
    scrollflagarea->hide();
    warning_color = "#FFAD07";
    error_color = "#EA2B0E";
    todo_color = "#B4D4F3";
    breakpoint_color = "#30E62E";

    this->update_linenumberarea_width();

    document_id = reinterpret_cast<size_t>(this);
    connect(this, SIGNAL(cursorPositionChanged()),
            this, SLOT(__cursor_position_changed()));
    __find_first_pos = -1;
    __find_flags = -1;//本代码并没有用到

    supported_language = false;
    supported_cell_language = false;
    // classfunc_match = None
    comment_string = QString();
    _kill_ring = new QtKillRing(this);

    blockuserdata_list = QList<BlockUserData*>();
    connect(this, SIGNAL(blockCountChanged(int)),
            this, SLOT(update_breakpoints()));

    timer_syntax_highlight = new QTimer(this);
    timer_syntax_highlight->setSingleShot(true);
    timer_syntax_highlight->setInterval(300);
    connect(timer_syntax_highlight,SIGNAL(timeout()),
            this,SLOT(run_pygments_highlighter()));

    occurrence_highlighting = false;
    occurrence_timer = new QTimer(this);
    occurrence_timer->setSingleShot(true);
    occurrence_timer->setInterval(1500);
    connect(occurrence_timer,SIGNAL(timeout()),
            this,SLOT(__mark_occurrences()));
    occurrences = QList<int>();
    occurrence_color = QColor(Qt::yellow).lighter(160);

    connect(this,SIGNAL(textChanged()),this,SLOT(__text_has_changed()));
    found_results = QList<int>();
    found_results_color = QColor(Qt::magenta).lighter(180);

    gotodef_action = nullptr;
    this->setup_context_menu();

    tab_indents = false;
    tab_mode = true;

    intelligent_backspace = true;

    go_to_definition_enabled = false;
    close_parentheses_enabled = true;
    close_quotes_enabled = false;
    add_colons_enabled = true;
    auto_unindent_enabled = true;

    this->setMouseTracking(true);
    __cursor_changed = false;
    ctrl_click_color = QColor(Qt::blue);

    this->breakpoints = this->get_breakpoints();
    shortcuts = this->create_shortcuts();
    __visible_blocks = QList<IntIntTextblock>();

    connect(this,SIGNAL(painted(QPaintEvent*)),SLOT(_draw_editor_cell_divider()));
    connect(verticalScrollBar(),&QAbstractSlider::valueChanged,
            [=](int){ this->rehighlight_cells(); });
}

void CodeEditor::cb_maker(int attr)
{
    QTextCursor cursor = this->textCursor();
    auto move_type = static_cast<QTextCursor::MoveOperation>(attr);
    cursor.movePosition(move_type);
    this->setTextCursor(cursor);
}

QList<ShortCutStrStr> CodeEditor::create_shortcuts()
{
    QString keystr = "Ctrl+Space";
    QShortcut* qsc = new QShortcut(QKeySequence(keystr), this);
    connect(qsc,&QShortcut::activated, [=](){ this->do_completion(false); });
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr codecomp(qsc,"Editor","Code Completion");

    keystr = "Ctrl+Alt+Up";
    qsc = new QShortcut(QKeySequence(keystr), this, SLOT(duplicate_line()));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr duplicate_line(qsc,"Editor","Duplicate line");

    keystr = "Ctrl+Alt+Down";
    qsc = new QShortcut(QKeySequence(keystr), this, SLOT(copy_line()));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr copyline(qsc,"Editor","Copy line");

    keystr = "Ctrl+D";
    qsc = new QShortcut(QKeySequence(keystr), this, SLOT(delete_line()));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr deleteline(qsc,"Editor","Delete line");

    keystr = "Alt+Up";
    qsc = new QShortcut(QKeySequence(keystr), this, SLOT(move_line_up()));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr movelineup(qsc,"Editor","Move line up");

    keystr = "Alt+Down";
    qsc = new QShortcut(QKeySequence(keystr), this, SLOT(move_line_down()));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr movelinedown(qsc,"Editor","Move line down");

    keystr = "Ctrl+Shift+Return";
    qsc = new QShortcut(QKeySequence(keystr), this, SLOT(go_to_new_line()));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr gotonewline(qsc,"Editor","Go to new line");

    keystr = "Ctrl+G";
    qsc = new QShortcut(QKeySequence(keystr), this, SLOT(do_go_to_definition()));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr gotodef(qsc,"Editor","Go to definition");

    keystr = "Ctrl+1";
    qsc = new QShortcut(QKeySequence(keystr), this, SLOT(toggle_comment()));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr toggle_comment(qsc,"Editor","Toggle comment");

    keystr = "Ctrl+4";
    qsc = new QShortcut(QKeySequence(keystr), this, SLOT(blockcomment()));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr blockcomment(qsc,"Editor","Blockcomment");

    keystr = "Ctrl+5";
    qsc = new QShortcut(QKeySequence(keystr), this, SLOT(unblockcomment()));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr unblockcomment(qsc,"Editor","Unblockcomment");

    keystr = "Ctrl+Shift+U";
    qsc = new QShortcut(QKeySequence(keystr), this, SLOT(transform_to_uppercase()));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr transform_uppercase(qsc,"Editor","Transform to uppercase");

    keystr = "Ctrl+U";
    qsc = new QShortcut(QKeySequence(keystr), this, SLOT(transform_to_lowercase()));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr transform_lowercase(qsc,"Editor","Transform to lowercase");

    // lambda表达式
    keystr = "Ctrl+]";
    qsc = new QShortcut(QKeySequence(keystr), this);
    connect(qsc,&QShortcut::activated,[=]() { this->indent(true); });
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr indent(qsc,"Editor","Indent");

    keystr = "Ctrl+[";
    qsc = new QShortcut(QKeySequence(keystr), this);
    connect(qsc,&QShortcut::activated,[=]() { this->unindent(true); });
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr unindent(qsc,"Editor","Unindent");

    QSignalMapper* signalMapper = new QSignalMapper(this);

    keystr = "Meta+A";//
    qsc = new QShortcut(QKeySequence(keystr), this);
    connect(qsc,SIGNAL(activated()),signalMapper,SLOT(map()));
    signalMapper->setMapping(qsc, static_cast<int>(QTextCursor::StartOfLine));//
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr line_start(qsc,"Editor","Start of line");//

    keystr = "Meta+E";
    qsc = new QShortcut(QKeySequence(keystr), this);
    connect(qsc,SIGNAL(activated()),signalMapper,SLOT(map()));
    signalMapper->setMapping(qsc, static_cast<int>(QTextCursor::EndOfLine));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr line_end(qsc,"Editor","End of line");

    keystr = "Meta+P";
    qsc = new QShortcut(QKeySequence(keystr), this);
    connect(qsc,SIGNAL(activated()),signalMapper,SLOT(map()));
    signalMapper->setMapping(qsc, static_cast<int>(QTextCursor::Up));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr prev_line(qsc,"Editor","Previous line");

    keystr = "Meta+N";
    qsc = new QShortcut(QKeySequence(keystr), this);
    connect(qsc,SIGNAL(activated()),signalMapper,SLOT(map()));
    signalMapper->setMapping(qsc, static_cast<int>(QTextCursor::Down));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr next_line(qsc,"Editor","Next line");

    keystr = "Meta+B";
    qsc = new QShortcut(QKeySequence(keystr), this);
    connect(qsc,SIGNAL(activated()),signalMapper,SLOT(map()));
    signalMapper->setMapping(qsc, static_cast<int>(QTextCursor::Left));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr prev_char(qsc,"Editor","Previous char");

    keystr = "Meta+F";
    qsc = new QShortcut(QKeySequence(keystr), this);
    connect(qsc,SIGNAL(activated()),signalMapper,SLOT(map()));
    signalMapper->setMapping(qsc, static_cast<int>(QTextCursor::Right));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr next_char(qsc,"Editor","Next char");

    keystr = "Meta+Left";
    qsc = new QShortcut(QKeySequence(keystr), this);
    connect(qsc,SIGNAL(activated()),signalMapper,SLOT(map()));
    signalMapper->setMapping(qsc, static_cast<int>(QTextCursor::PreviousWord));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr prev_word(qsc,"Editor","Previous word");

    keystr = "Meta+Right";
    qsc = new QShortcut(QKeySequence(keystr), this);
    connect(qsc,SIGNAL(activated()),signalMapper,SLOT(map()));
    signalMapper->setMapping(qsc, static_cast<int>(QTextCursor::NextWord));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr next_word(qsc,"Editor","Next word");


    keystr = "Meta+K";
    qsc = new QShortcut(QKeySequence(keystr), this, SLOT(kill_line_end()));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr kill_line_end(qsc,"Editor","Kill to line end");

    keystr = "Meta+U";
    qsc = new QShortcut(QKeySequence(keystr), this, SLOT(kill_line_start()));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr kill_line_start(qsc,"Editor","Kill to line start");

    keystr = "Meta+Y";
    qsc = new QShortcut(QKeySequence(keystr), this);
    connect(qsc,SIGNAL(activated()),_kill_ring,SLOT(yank()));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr yank(qsc,"Editor","Yank");

    keystr = "Shift+Meta+Y";
    qsc = new QShortcut(QKeySequence(keystr), this);
    connect(qsc,SIGNAL(activated()),_kill_ring,SLOT(rotate()));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr kill_ring_rotate(qsc,"Editor","Rotate kill ring");


    keystr = "Meta+Backspace";
    qsc = new QShortcut(QKeySequence(keystr), this, SLOT(kill_prev_word()));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr kill_prev_word(qsc,"Editor","Kill previous word");

    keystr = "Meta+D";
    qsc = new QShortcut(QKeySequence(keystr), this, SLOT(kill_next_word()));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr kill_next_word(qsc,"Editor","Kill next word");

    //
    keystr = "Ctrl+Home";
    qsc = new QShortcut(QKeySequence(keystr), this);
    connect(qsc,SIGNAL(activated()),signalMapper,SLOT(map()));
    signalMapper->setMapping(qsc, static_cast<int>(QTextCursor::Start));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr start_doc(qsc,"Editor","Start of Document");

    keystr = "Ctrl+End";
    qsc = new QShortcut(QKeySequence(keystr), this);
    connect(qsc,SIGNAL(activated()),signalMapper,SLOT(map()));
    signalMapper->setMapping(qsc, static_cast<int>(QTextCursor::End));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr end_doc(qsc,"Editor","End of Document");

    //
    connect(signalMapper, SIGNAL(mapped(int)),
                  this, SLOT(cb_maker(int)));

    keystr = "Ctrl+Z";
    qsc = new QShortcut(QKeySequence(keystr), this, SLOT(undo()));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr undo(qsc,"Editor","undo");

    keystr = "Ctrl+Shift+Z";
    qsc = new QShortcut(QKeySequence(keystr), this, SLOT(redo()));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr redo(qsc,"Editor","redo");

    keystr = "Ctrl+X";
    qsc = new QShortcut(QKeySequence(keystr), this, SLOT(cut()));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr cut(qsc,"Editor","cut");

    keystr = "Ctrl+C";
    qsc = new QShortcut(QKeySequence(keystr), this, SLOT(copy()));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr copy(qsc,"Editor","undo");

    keystr = "Ctrl+V";
    qsc = new QShortcut(QKeySequence(keystr), this, SLOT(paste()));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr paste(qsc,"Editor","paste");

    keystr = "Delete";
    qsc = new QShortcut(QKeySequence(keystr), this, SLOT(_delete()));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr _delete(qsc,"Editor","delete");

    keystr = "Ctrl+A";
    qsc = new QShortcut(QKeySequence(keystr), this, SLOT(selectAll()));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr select_all(qsc,"Editor","Select All");

    // lambda表达式
    keystr = "Ctrl+Alt+M";
    qsc = new QShortcut(QKeySequence(keystr), this);
    connect(qsc,&QShortcut::activated,[=]() { this->enter_array_inline(); });
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr array_inline(qsc,"array_builder","enter array inline");

    keystr = "Ctrl+M";
    qsc = new QShortcut(QKeySequence(keystr), this);
    connect(qsc,&QShortcut::activated,[=]() { this->enter_array_table(); });
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr array_table(qsc,"array_builder","enter array table");

    QList<ShortCutStrStr> res;
    res << codecomp<< duplicate_line<< copyline<< deleteline<< movelineup
        << movelinedown<< gotonewline<< gotodef<< toggle_comment<< blockcomment
        << unblockcomment<< transform_uppercase<< transform_lowercase
        << line_start<< line_end<< prev_line<< next_line
        << prev_char<< next_char<< prev_word<< next_word<< kill_line_end
        << kill_line_start<< yank<< kill_ring_rotate<< kill_prev_word
        << kill_next_word<< start_doc<< end_doc<< undo<< redo<< cut<< copy
        << paste<< _delete<< select_all<< array_inline<< array_table<< indent
        << unindent;
    return res;
}

QList<ShortCutStrStr> CodeEditor::get_shortcut_data()
{
    return shortcuts;
}

void CodeEditor::closeEvent(QCloseEvent *event)
{
    TextEditBaseWidget::closeEvent(event);
}

size_t CodeEditor::get_document_id()
{
    return document_id;
}

void CodeEditor::set_as_clone(CodeEditor *editor)
{
    // Set as clone editor
    this->setDocument(editor->document());
    document_id = editor->get_document_id();
    highlighter = editor->highlighter;
    eol_chars = editor->eol_chars;
    this->_apply_highlighter_color_scheme();
}


//-----Widget setup and options
void CodeEditor::toggle_wrap_mode(bool enable)
{
    this->set_wrap_mode(enable ? "word" : QString());
}

void CodeEditor::setup_editor(const QHash<QString, QVariant> &kwargs)
{
    bool linenumbers = kwargs.value("linenumbers", true).toBool();
    QString language = kwargs.value("language", QString()).toString();
    bool markers = kwargs.value("markers", false).toBool();

    // font
    QHash<QString, QVariant> dict = kwargs.value("color_scheme", QHash<QString, QVariant>()).toHash();
    QHash<QString, ColorBoolBool> color_scheme;
    for (auto it=dict.begin();it!=dict.end();it++) {
        QString key = it.key();
        ColorBoolBool val = it.value().value<ColorBoolBool>();
        color_scheme[key] = val;
    }
    bool wrap = kwargs.value("wrap", false).toBool();
    bool tab_mode = kwargs.value("tab_mode", true).toBool();

    bool intelligent_backspace = kwargs.value("intelligent_backspace", true).toBool();
    bool highlight_current_line = kwargs.value("highlight_current_line", true).toBool();
    bool highlight_current_cell = kwargs.value("highlight_current_cell", true).toBool();
    bool occurrence_highlighting= kwargs.value("occurrence_highlighting", true).toBool();

    bool scrollflagarea = kwargs.value("scrollflagarea", true).toBool();
    bool edge_line = kwargs.value("edge_line", true).toBool();
    int edge_line_column = kwargs.value("edge_line_column", 79).toInt();

    bool codecompletion_auto = kwargs.value("codecompletion_auto", false).toBool();
    bool codecompletion_case = kwargs.value("codecompletion_case", true).toBool();
    bool codecompletion_enter = kwargs.value("codecompletion_enter", false).toBool();
    bool show_blanks = kwargs.value("show_blanks", false).toBool();

    // calltips
    bool go_to_definition = kwargs.value("go_to_definition", false).toBool();
    bool close_parentheses = kwargs.value("close_parentheses", true).toBool();
    bool close_quotes = kwargs.value("close_quotes", false).toBool();

    bool add_colons = kwargs.value("add_colons", true).toBool();
    bool auto_unindent = kwargs.value("auto_unindent", true).toBool();
    QString indent_chars = kwargs.value("indent_chars", QString(4,' ')).toString();

    int tab_stop_width_spaces = kwargs.value("tab_stop_width_spaces", 4).toInt();
    // cloned_from
    QString filename = kwargs.value("filename", QString()).toString();
    int occurrence_timeout = kwargs.value("occurrence_timeout", 1500).toInt();

    this->set_codecompletion_auto(codecompletion_auto);
    this->set_codecompletion_case(codecompletion_case);
    this->set_codecompletion_enter(codecompletion_enter);
    if (kwargs.contains("calltips"))
        this->set_calltips(kwargs.value("calltips").toBool());
    this->set_go_to_definition_enabled(go_to_definition);
    this->set_close_parentheses_enabled(close_parentheses);
    this->set_close_quotes_enabled(close_quotes);
    this->set_add_colons_enabled(add_colons);
    this->set_auto_unindent_enabled(auto_unindent);
    this->set_indent_chars(indent_chars);

    this->set_scrollflagarea_enabled(scrollflagarea);

    this->set_edge_line_enabled(edge_line);
    this->set_edge_line_column(edge_line_column);

    this->set_blanks_enabled(show_blanks);

    if (kwargs.contains("cloned_from") && kwargs.contains("font"))
        this->setFont(kwargs.value("font").value<QFont>());
    this->setup_margins(linenumbers, markers);

    this->set_language(language, filename);

    this->set_highlight_current_cell(highlight_current_cell);

    this->set_highlight_current_line(highlight_current_line);

    this->set_occurrence_highlighting(occurrence_highlighting);
    this->set_occurrence_timeout(occurrence_timeout);

    this->set_tab_mode(tab_mode);

    this->toggle_intelligent_backspace(intelligent_backspace);

    if (kwargs.contains("cloned_from")) {
        size_t id = kwargs.value("cloned_from").toULongLong();
        CodeEditor* cloned_from = reinterpret_cast<CodeEditor*>(id);
        this->set_as_clone(cloned_from);
        this->update_linenumberarea_width();
    }
    else if (kwargs.contains("font")) {
        QFont font = kwargs.value("font").value<QFont>();
        this->set_font(font, color_scheme);
    }
    else if (!color_scheme.isEmpty())
        this->set_color_scheme(color_scheme);

    this->set_tab_stop_width_spaces(tab_stop_width_spaces);
    this->toggle_wrap_mode(wrap);
}

void CodeEditor::setup_editor(bool linenumbers, const QString& language, bool markers,
                  const QFont& font, const QHash<QString, ColorBoolBool> &color_scheme, bool wrap, bool tab_mode,
                  bool intelligent_backspace, bool highlight_current_line,
                  bool highlight_current_cell, bool occurrence_highlighting,
                  bool scrollflagarea, bool edge_line, int edge_line_column,
                  bool codecompletion_auto, bool codecompletion_case,
                  bool codecompletion_enter, bool show_blanks,
                  bool calltips, bool go_to_definition,
                  bool close_parentheses, bool close_quotes,
                  bool add_colons, bool auto_unindent, const QString& indent_chars,
                  int tab_stop_width_spaces, CodeEditor* cloned_from, const QString& filename,
                  int occurrence_timeout)
{
    //# Code completion and calltips
    set_codecompletion_auto(codecompletion_auto);
    set_codecompletion_case(codecompletion_case);
    set_codecompletion_enter(codecompletion_enter);
    set_calltips(calltips);
    set_go_to_definition_enabled(go_to_definition);
    set_close_parentheses_enabled(close_parentheses);
    set_close_quotes_enabled(close_quotes);
    set_add_colons_enabled(add_colons);
    set_auto_unindent_enabled(auto_unindent);
    set_indent_chars(indent_chars);

    set_scrollflagarea_enabled(scrollflagarea);

    set_edge_line_enabled(edge_line);
    set_edge_line_column(edge_line_column);

    set_blanks_enabled(show_blanks);

    if (cloned_from)
        setFont(font);
    setup_margins(linenumbers, markers);

    set_language(language, filename);

    set_highlight_current_cell(highlight_current_cell);
    set_highlight_current_line(highlight_current_line);
    set_occurrence_highlighting(occurrence_highlighting);
    set_occurrence_timeout(occurrence_timeout);

    set_tab_mode(tab_mode);

    toggle_intelligent_backspace(intelligent_backspace);

    if (cloned_from) {
        set_as_clone(cloned_from);
        update_linenumberarea_width();
    }
    else if (font != this->font())
        set_font(font, color_scheme);
    else if (!color_scheme.isEmpty())
        set_color_scheme(color_scheme);

    set_tab_stop_width_spaces(tab_stop_width_spaces);

    toggle_wrap_mode(wrap);
}

void CodeEditor::set_tab_mode(bool enable)
{
    this->tab_mode = enable;
}

void CodeEditor::toggle_intelligent_backspace(bool state)
{
    this->intelligent_backspace = state;
}

void CodeEditor::set_go_to_definition_enabled(bool enable)
{
    this->go_to_definition_enabled = enable;
}

void CodeEditor::set_close_parentheses_enabled(bool enable)
{
    this->close_parentheses_enabled = enable;
}

void CodeEditor::set_close_quotes_enabled(bool enable)
{
    this->close_quotes_enabled = enable;
}

void CodeEditor::set_add_colons_enabled(bool enable)
{
    this->add_colons_enabled = enable;
}

void CodeEditor::set_auto_unindent_enabled(bool enable)
{
    this->auto_unindent_enabled = enable;
}

void CodeEditor::set_occurrence_highlighting(bool enable)
{
    this->occurrence_highlighting = enable;
    if (!enable)
        this->__clear_occurrences();
}

void CodeEditor::set_occurrence_timeout(int timeout)
{
    this->occurrence_timer->setInterval(timeout);
}

void CodeEditor::set_highlight_current_line(bool enable)
{
    this->highlight_current_line_enabled = enable;
    if (this->highlight_current_line_enabled)
        this->highlight_current_line();
    else
        this->unhighlight_current_line();
}

void CodeEditor::set_highlight_current_cell(bool enable)
{
    bool hl_cell_enable = enable && this->supported_cell_language;
    this->highlight_current_cell_enabled = hl_cell_enable;
    if (this->highlight_current_cell_enabled)
        this->highlight_current_cell();
    else
        this->unhighlight_current_cell();
}

void CodeEditor::set_language(const QString &language, const QString &filename)
{
    this->tab_indents = this->TAB_ALWAYS_INDENTS.contains(language);
    this->comment_string = "";
    QString sh_class = "TextSH";
    if (!language.isEmpty()) {
        //以后在这里添加别的语言，记得在构造函数更新LANGUAGES成员
        QStringList list({"Python", "Cpp", "Markdown"});
        foreach (QString key, list) {
            if (sourcecode::ALL_LANGUAGES[key].contains(language.toLower())) {
                this->supported_language = true;
                sh_class = LANGUAGES[key][0];
                QString comment_string = LANGUAGES[key][1];
                this->comment_string = comment_string;

                //key in CELL_LANGUAGES等价于key == "Python"
                if (key == "Python") {
                    this->supported_cell_language = true;
                    this->cell_separators = sourcecode::CELL_LANGUAGES[key];
                }
                break;
            }
        }
    }

    this->_set_highlighter(sh_class);
}

void CodeEditor::_set_highlighter(const QString& sh_class)
{
    this->highlighter_class = sh_class;
    if (this->highlighter) {
        this->highlighter->setParent(nullptr);
        this->highlighter->setDocument(nullptr);
    }
    if (highlighter_class == "PythonSH") {
        highlighter = new sh::PythonSH(this->document(),
                                       this->font(),
                                       this->color_scheme);
    }
    else if (highlighter_class == "CppSH")
        highlighter = new sh::CppSH(this->document(),
                                         this->font(),
                                         this->color_scheme);
    else if (highlighter_class == "MarkdownSH")
        highlighter = new sh::MarkdownSH(this->document(),
                                         this->font(),
                                         this->color_scheme);
    else if (highlighter_class == "TextSH")
        highlighter = new sh::TextSH(this->document(),
                                     this->font(),
                                     this->color_scheme);
    this->_apply_highlighter_color_scheme();
}


bool CodeEditor::is_json()
{
    //需要pygments第三方库
    return false;
}

bool CodeEditor::is_python()
{
    return this->highlighter_class == "PythonSH";
}

bool CodeEditor::is_cpp()
{
    return this->highlighter_class == "CppSH";
}

bool CodeEditor::is_cython()
{
    return this->highlighter_class == "CythonSH";
}

bool CodeEditor::is_enmal()
{
    return this->highlighter_class == "EnamlSH";
}

bool CodeEditor::is_python_like()
{
    return this->is_python() || this->is_cython() || this->is_enmal();
}

void CodeEditor::intelligent_tab()
{
    QString leading_text = this->get_text("sol","cursor");
    if (leading_text.trimmed().isEmpty() || leading_text.endsWith('#'))
        this->indent_or_replace();
    else if (this->in_comment_or_string() && !leading_text.endsWith(" "))
        this->do_completion();
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
    else if (leading_text.endsWith("import ") || leading_text.back()=='.')
#else
	else if (leading_text.endsWith("import ") || leading_text.at(leading_text.size() - 1) == '.')
#endif
        this->do_completion();
    else if ((leading_text.split(QRegularExpression("\\s+"))[0]=="from" ||
              leading_text.split(QRegularExpression("\\s+"))[0]=="import") &&
             !leading_text.contains(';'))
        this->do_completion();
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
    else if (leading_text.back()=='(' || leading_text.back()==',' || leading_text.endsWith(", "))
#else
	else if (leading_text.at(leading_text.size() - 1) == '(' || leading_text.at(leading_text.size() - 1) == ',' || leading_text.endsWith(", "))
#endif
        this->indent_or_replace();
    else if (leading_text.endsWith(' '))
        this->indent_or_replace();
    // \W匹配非数字字母下划线;\Z匹配字符串结束，如果是存在换行，只匹配到换行前的结束字符串
    else if (leading_text.indexOf(QRegularExpression("[^\\d\\W]\\w*\\Z",
                                                     QRegularExpression::UseUnicodePropertiesOption)) > -1)
        this->do_completion();
    else
        this->indent_or_replace();
}

void CodeEditor::intelligent_backtab()
{
    QString leading_text = this->get_text("sol","cursor");
    if (leading_text.trimmed().isEmpty())
        this->unindent();
    else if (this->in_comment_or_string())
        this->unindent();
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
    else if (leading_text.back()=='(' || leading_text.back()==',' || leading_text.endsWith(", "))
#else
	else if (leading_text.at(leading_text.size() - 1) == '(' || leading_text.at(leading_text.size() - 1) == ',' || leading_text.endsWith(", "))
#endif
	{
        int position = this->get_position("cursor");
        this->show_object_info(position);
    }
    else
        this->unindent();
}


//******************************
void CodeEditor::rehighlight()
{
    if (this->highlighter)
        this->highlighter->rehighlight();
    if (this->highlight_current_cell_enabled)
        this->highlight_current_cell();
    else
        this->unhighlight_current_cell();

    if (this->highlight_current_line_enabled)
        this->highlight_current_line();
    else
        this->unhighlight_current_line();
}

void CodeEditor::rehighlight_cells()
{
    if (this->highlight_current_cell_enabled)
        this->highlight_current_cell();
}

void CodeEditor::setup_margins(bool linenumbers, bool markers)
{
    this->linenumbers_margin = linenumbers;
    this->markers_margin = markers;
    this->set_linenumberarea_enabled(linenumbers || markers);
}

void CodeEditor::remove_trailing_spaces()
{
    // 删除尾随空格
    QTextCursor cursor = this->textCursor();
    cursor.beginEditBlock();
    cursor.movePosition(QTextCursor::Start);
    while (true) {
        cursor.movePosition(QTextCursor::EndOfBlock);
        QString text = cursor.block().text();
        int length = text.size() - rstrip(text).size();
        if (length > 0) {
            cursor.movePosition(QTextCursor::Left,QTextCursor::KeepAnchor,
                                length);
            cursor.removeSelectedText();
        }
        if (cursor.atEnd())
            break;
        cursor.movePosition(QTextCursor::NextBlock);
    }
    cursor.endEditBlock();
}

void CodeEditor::fix_indentation()
{
    QString text_before = this->toPlainText();
    QString text_after  = sourcecode::fix_indentation(text_before);
    if (text_before != text_after) {
        this->setPlainText(text_before);
        this->document()->setModified(true);
    }
}

QString CodeEditor::get_current_object()
{
    QString source_code = this->toPlainText();
    int offset = this->get_position("cursor");
    return sourcecode::get_primary_at(source_code, offset);
}

//@Slot()
void CodeEditor::_delete()
{
    if (!this->has_selected_text()) {
        QTextCursor cursor = this->textCursor();
        int position = cursor.position();
        if (!cursor.atEnd())
            cursor.setPosition(position+1, QTextCursor::KeepAnchor);
        this->setTextCursor(cursor); // 这里调用了setTextCursor更新可见光标
    }
    this->remove_selected_text();
}

//------Find occurrences
QTextCursor CodeEditor::__find_first(const QString &text)
{
    QTextDocument::FindFlags flags = QTextDocument::FindCaseSensitively | QTextDocument::FindWholeWords;
    QTextCursor cursor = this->textCursor();
    cursor.movePosition(QTextCursor::Start);
    QRegExp regexp(QString("\\b%1\\b").arg(QRegExp::escape(text)), Qt::CaseSensitive);
    cursor = this->document()->find(regexp, cursor, flags);
    this->__find_first_pos = cursor.position();
    return cursor;
}

QTextCursor CodeEditor::__find_next(const QString &text, QTextCursor cursor)
{
    QTextDocument::FindFlags flags = QTextDocument::FindCaseSensitively | QTextDocument::FindWholeWords;
    QRegExp regexp(QString("\\b%1\\b").arg(QRegExp::escape(text)), Qt::CaseSensitive);
    cursor = this->document()->find(regexp, cursor, flags);
    if (cursor.position() != this->__find_first_pos)
        return cursor;
    return QTextCursor();
}

void CodeEditor::__cursor_position_changed()
{
    auto pair = this->get_cursor_line_column();
    int line = pair.first, column=pair.second;
    emit sig_cursor_position_changed(line,column);
    if (this->highlight_current_cell_enabled)
        this->highlight_current_cell();
    else
        this->unhighlight_current_cell();

    if (this->highlight_current_line_enabled)
        this->highlight_current_line();
    else
        this->unhighlight_current_line();

    if (this->occurrence_highlighting) {
        this->occurrence_timer->stop();
        this->occurrence_timer->start();
    }
}

void CodeEditor::__clear_occurrences()
{
    occurrences.clear();
    clear_extra_selections("occurrences");
    scrollflagarea->update();
}

void CodeEditor::__highlight_selection(const QString &key, const QTextCursor &cursor, const QColor &foreground_color,
                                       const QColor &background_color, const QColor &underline_color,
                                       QTextCharFormat::UnderlineStyle underline_style,
                                       bool update)
{
    QList<QTextEdit::ExtraSelection> extra_selections = get_extra_selections(key);
    QTextEdit::ExtraSelection selection;
    if (foreground_color.isValid())
        selection.format.setForeground(foreground_color);
    if (background_color.isValid())
        selection.format.setBackground(background_color);
    if (underline_color.isValid()) {
        selection.format.setProperty(QTextFormat::TextUnderlineStyle,
                                     underline_style);
        selection.format.setProperty(QTextFormat::TextUnderlineColor,
                                     underline_color);
    }
    selection.format.setProperty(QTextFormat::FullWidthSelection,
                                 true);
    selection.cursor = cursor;
    extra_selections.append(selection);
    set_extra_selections(key,extra_selections);
    if (update)
        update_extra_selections();
}

//@Slot()
void CodeEditor::__mark_occurrences()
{
    __clear_occurrences();
    if (!supported_language)
        return;

    QString text = get_current_word();
    if (text.isEmpty())
        return;
    if (has_selected_text() && get_selected_text()!=text)
        return;

    if (is_python_like() &&
            (sourcecode::is_keyword(text) ||
             text == "self"))
        return;

    QTextCursor cursor = this->__find_first(text);
    occurrences.clear();
    while (!cursor.isNull()) {
        occurrences.append(cursor.blockNumber());
        __highlight_selection("occurrences",cursor,
                                    QColor(),occurrence_color);
        cursor = __find_next(text, cursor);
    }
    update_extra_selections();
    if (occurrences.size() > 1 && occurrences.last()==0)
        occurrences.pop_back();
    scrollflagarea->update();
}

//-----highlight found results (find/replace widget)
void CodeEditor::highlight_found_results(QString pattern, bool words, bool regexp)
{
    if (pattern.isEmpty())
        return;
    if (!regexp)
        pattern = QRegularExpression::escape(pattern);
    pattern = words ? QString("\\b%1\\b").arg(pattern) : pattern;
    QString text = this->toPlainText();
    QRegularExpression regobj(pattern);
    if (!regobj.isValid())
        return;

    QList<QTextEdit::ExtraSelection> extra_selections;
    found_results.clear();
    QRegularExpressionMatchIterator iterator = regobj.globalMatch(text);
    while (iterator.hasNext()) {
        QRegularExpressionMatch match = iterator.next();
        int pos1 = match.capturedStart();
        int pos2 = match.capturedEnd();
        QTextEdit::ExtraSelection selection;
        selection.format.setBackground(found_results_color);
        selection.cursor = this->textCursor();
        selection.cursor.setPosition(pos1);
        found_results.append(selection.cursor.blockNumber());
        selection.cursor.setPosition(pos2,QTextCursor::KeepAnchor);
        extra_selections.append(selection);
    }
    set_extra_selections("find", extra_selections);
    update_extra_selections();
}

void CodeEditor::clear_found_results()
{
    found_results.clear();
    clear_extra_selections("find");
    scrollflagarea->update();
}

void CodeEditor::__text_has_changed()
{
    if (!found_results.isEmpty())
        clear_found_results();
}

//-----markers
int CodeEditor::get_markers_margin()
{
    if (markers_margin)
        return markers_margin_width;
    else
        return 0;
}

//-----linenumberarea
void CodeEditor::set_linenumberarea_enabled(bool state)
{
    linenumberarea_enabled = state;
    linenumberarea->setVisible(state);
    update_linenumberarea_width();
}

int CodeEditor::get_linenumberarea_width()
{
    return linenumberarea->contentsRect().width();
}

int CodeEditor::compute_linenumberarea_width()
{
    if (!linenumberarea_enabled)
        return 0;
    int digits = 1;
    int maxb = qMax(1, blockCount());
    while (maxb >= 10) {
        maxb /= 10;
        digits++;
    }
    int linenumbers_margin;
    if (this->linenumbers_margin)
        linenumbers_margin  = 3+this->fontMetrics().width(QString(digits,'9'));
    else
        linenumbers_margin = 0;
    return linenumbers_margin + this->get_markers_margin();
}

void CodeEditor::update_linenumberarea_width(int new_block_count)
{
    Q_UNUSED(new_block_count);
    this->setViewportMargins(this->compute_linenumberarea_width(),0,
                             this->get_scrollflagarea_width(),0);
}

void CodeEditor::update_linenumberarea(const QRect &qrect, int dy)
{
    if (dy)
        this->linenumberarea->scroll(0, dy);
    else
        this->linenumberarea->update(0, qrect.y(),
                                     linenumberarea->width(),
                                     qrect.height());
    if (qrect.contains(this->viewport()->rect()))
        this->update_linenumberarea_width();
}


void CodeEditor::linenumberarea_paint_event(QPaintEvent *event)
{
    QPainter painter(this->linenumberarea);
    painter.fillRect(event->rect(), this->sideareas_color);

    QFont font = this->font();
    int font_height = this->fontMetrics().height();

    QTextBlock active_block = this->textCursor().block();
    int active_line_number = active_block.blockNumber()+1;

    auto draw_pixmap = [&](int ytop, const QPixmap &pixmap)
    {
        int pixmap_height;
#if QT_VERSION < QT_VERSION_CHECK(5, 5, 0)
        pixmap_height = pixmap.height();
#else
        pixmap_height = static_cast<int>(pixmap.height() / pixmap.devicePixelRatio());
#endif
        painter.drawPixmap(0, ytop + (font_height-pixmap_height) / 2, pixmap);
    };

    foreach (auto pair, this->__visible_blocks) {
        int top = pair.top;
        int line_number = pair.line_number;
        QTextBlock block = pair.block;
        if (this->linenumbers_margin) {
            if (line_number == active_line_number) {
                font.setWeight(font.Bold);
                painter.setFont(font);
                painter.setPen(this->normal_color);
            }
            else {
                font.setWeight(font.Normal);
                painter.setFont(font);
                painter.setPen(this->linenumbers_color);
            }

            painter.drawText(0, top, linenumberarea->width(),
                             font_height,
                             Qt::AlignRight | Qt::AlignBottom,
                             QString::number(line_number));
        }

        BlockUserData* data = dynamic_cast<BlockUserData*>(block.userData());
        if (this->markers_margin && data) {
            if (!data->code_analysis.isEmpty()) {
                bool error = false;
                foreach (auto pair, data->code_analysis) {
                    error = pair.second;
                    if (error)
                        break;
                }
                if (error)
                    draw_pixmap(top,error_pixmap);
                else
                    draw_pixmap(top,warning_pixmap);
            }
            if (!data->todo.isEmpty())
                draw_pixmap(top,todo_pixmap);
            if (data->breakpoint) {
                if (data->breakpoint_condition.isEmpty())
                    draw_pixmap(top,this->bp_pixmap);
                else
                    draw_pixmap(top,this->bpc_pixmap);
            }
        }
    }
}

int CodeEditor::__get_linenumber_from_mouse_event(QMouseEvent *event)
{
    QTextBlock block = firstVisibleBlock();
    int line_number = block.blockNumber();
    double top = blockBoundingGeometry(block).
                  translated(this->contentOffset()).top();
    double bottom = top + blockBoundingRect(block).height();

    while (block.isValid() && top < event->pos().y()) {
        block = block.next();
        top = bottom;
        bottom = top + blockBoundingRect(block).height();
        line_number++;
    }
    return line_number;
}

void CodeEditor::linenumberarea_mousemove_event(QMouseEvent *event)
{
    int line_number = __get_linenumber_from_mouse_event(event);
    QTextBlock block = document()->findBlockByNumber(line_number-1);
    BlockUserData* data = dynamic_cast<BlockUserData*>(block.userData());

    bool check = this->linenumberarea_released == -1;
    if (data && !data->code_analysis.isEmpty() && check)
        this->__show_code_analysis_results(line_number,data->code_analysis);

    if (event->buttons() == Qt::LeftButton) {
        this->linenumberarea_released = line_number;
        this->linenumberarea_select_lines(this->linenumberarea_pressed,
                                          this->linenumberarea_released);
    }
}

void CodeEditor::linenumberarea_mousedoubleclick_event(QMouseEvent *event)
{
    int line_number = this->__get_linenumber_from_mouse_event(event);
    bool shift = event->modifiers() & Qt::ShiftModifier;
    this->add_remove_breakpoint(line_number,QString(),shift);
}

void CodeEditor::linenumberarea_mousepress_event(QMouseEvent *event)
{
    int line_number = this->__get_linenumber_from_mouse_event(event);
    this->linenumberarea_pressed = line_number;
    this->linenumberarea_released = line_number;
    this->linenumberarea_select_lines(this->linenumberarea_pressed,
                                      this->linenumberarea_released);
}

void CodeEditor::linenumberarea_mouserelease_event(QMouseEvent *event)
{
    Q_UNUSED(event);
    this->linenumberarea_pressed = -1;
    this->linenumberarea_released = -1;
}

void CodeEditor::linenumberarea_select_lines(int linenumber_pressed, int linenumber_released)
{
    int move_n_blocks = linenumber_released - linenumber_pressed;
    int start_line = linenumber_pressed;
    QTextBlock start_block = this->document()->findBlockByLineNumber(start_line-1);

    QTextCursor cursor = this->textCursor();
    cursor.setPosition(start_block.position());

    if (move_n_blocks > 0) {
        for (int n = 0; n < qAbs(move_n_blocks)+1; ++n)
            cursor.movePosition(cursor.NextBlock, cursor.KeepAnchor);
    }
    else {
        cursor.movePosition(cursor.NextBlock);
        for (int n = 0; n < qAbs(move_n_blocks)+1; ++n)
            cursor.movePosition(cursor.PreviousBlock, cursor.KeepAnchor);
    }

    if (linenumber_released == this->blockCount())
        cursor.movePosition(cursor.EndOfBlock,cursor.KeepAnchor);
    else
        cursor.movePosition(cursor.StartOfBlock,cursor.KeepAnchor);

    this->setTextCursor(cursor);
}


//------Breakpoints
void CodeEditor::add_remove_breakpoint(int line_number, QString condition, bool edit_condition)
{
    if (!this->is_python_like())
        return;
    QTextBlock block;
    if (line_number == -1)
        block = this->textCursor().block();
    else
        block = this->document()->findBlockByNumber(line_number-1);

	bool newly_created = false;
    BlockUserData* data = dynamic_cast<BlockUserData*>(block.userData());
    if (data == nullptr) {
        data = new BlockUserData(this);
        data->breakpoint = true;
		newly_created = true;
    }
    else if (!edit_condition) {
        data->breakpoint = !data->breakpoint;
        data->breakpoint_condition = QString();
    }
    if (!condition.isEmpty()) {
        data->breakpoint_condition = condition;
    }
    if (edit_condition) {
        condition = data->breakpoint_condition;
        bool valid;
        condition = QInputDialog::getText(this,
                                          "Breakpoint",
                                          "Condition",
                                          QLineEdit::Normal,condition,&valid);
		if (valid == false)
		{
			if (newly_created)
			{
				delete data;
			}
			return;
		} 
        data->breakpoint = true;
        data->breakpoint_condition = (!condition.isEmpty()) ? condition : QString();
    }
    if (data->breakpoint) {
        QString text = block.text().trimmed();
        // 下一行设置注释以及双引号，单引号不能设置断点
        if (text.size()==0 || startswith(text,QStringList({"#","\"","'"})))
            data->breakpoint = false;
    }
    block.setUserData(data); // 在这里往QTextBlock上设置BlockUserData类型的数据
    this->linenumberarea->update();
    this->scrollflagarea->update();
    emit this->breakpoints_changed();
}

QList<QList<QVariant> > CodeEditor::get_breakpoints()
{
    QList<QList<QVariant>> breakpoints;
    QTextBlock block = this->document()->firstBlock();
    for (int line_number = 1; line_number < this->document()->blockCount()+1; ++line_number) {
        BlockUserData* data = dynamic_cast<BlockUserData*>(block.userData());
        if (data && data->breakpoint) {
            QList<QVariant> tmp;
            tmp.append(line_number);
            tmp.append(data->breakpoint_condition);

            breakpoints.append(tmp);
        }
        block = block.next();
    }
    return breakpoints;
}

void CodeEditor::clear_breakpoints()
{
	this->breakpoints.clear();
	for (int i = 0; i < this->blockuserdata_list.size(); ++i) {
		this->blockuserdata_list[i]->breakpoint = false;
		if (this->blockuserdata_list[i]->is_empty()) {
			delete this->blockuserdata_list[i];
			this->blockuserdata_list[i] = nullptr;
		}
	}
	this->blockuserdata_list.removeAll(nullptr);

    /*QList<BlockUserData*> tmp = this->blockuserdata_list;
    for (BlockUserData* data : tmp) {
        data->breakpoint = false;
        if (data->is_empty())
            delete data;
    }*/
}

void CodeEditor::set_breakpoints(const QList<QVariant> &breakpoints)
{
    this->clear_breakpoints();
    for (const auto& breakpoint : breakpoints) {
        QList<QVariant> pair = breakpoint.toList();
		if (pair.size() < 2)
		{
			continue;
		}
        int line_number = pair[0].toInt();
        QString condition = pair[1].toString();
        this->add_remove_breakpoint(line_number, condition);
    }
}

void CodeEditor::update_breakpoints()
{
    emit this->breakpoints_changed();
}


//-----Code introspection
void CodeEditor::do_completion(bool automatic)
{
    if (!this->is_completion_widget_visible())
        emit this->get_completions(automatic);
}

void CodeEditor::do_go_to_definition()
{
    if (!this->in_comment_or_string())
        emit go_to_definition(this->textCursor().position());
}

void CodeEditor::show_object_info(int position)
{
    emit this->sig_show_object_info(position);
}


//-----edge line
void CodeEditor::set_edge_line_enabled(bool state)
{
    this->edge_line_enabled = state;
    this->edge_line->setVisible(state);
}

void CodeEditor::set_edge_line_column(int column)
{
    this->edge_line->column = column;
    this->edge_line->update();
}


//-----blank spaces
void CodeEditor::set_blanks_enabled(bool state)
{
    this->blanks_enabled = state;
    QTextOption option = this->document()->defaultTextOption();
    option.setFlags(option.flags() | QTextOption::AddSpaceForLineAndParagraphSeparators);
    if (this->blanks_enabled)
        option.setFlags(option.flags() | QTextOption::ShowTabsAndSpaces);
    else
        option.setFlags(option.flags() & ~QTextOption::ShowTabsAndSpaces);
    this->document()->setDefaultTextOption(option);
    this->rehighlight();
}


//-----scrollflagarea
void CodeEditor::set_scrollflagarea_enabled(bool state)
{
    this->scrollflagarea_enabled = state;
    this->scrollflagarea->setVisible(state);
    this->update_linenumberarea_width();
}

int CodeEditor::get_scrollflagarea_width()
{
    if (this->scrollflagarea_enabled)
        return ScrollFlagArea::WIDTH;
    else
        return 0;
}

void CodeEditor::scrollflagarea_paint_event(QPaintEvent *event)
{   
    QPainter painter(this->scrollflagarea);
    painter.fillRect(event->rect(), this->sideareas_color);
    QTextBlock block = this->document()->firstBlock();

    for (int line_number = 1; line_number < document()->blockCount()+1; ++line_number) {
        BlockUserData* data = dynamic_cast<BlockUserData*>(block.userData());
        if (data) {
            int position = static_cast<int>(this->scrollflagarea->value_to_position(line_number));
            if (!data->code_analysis.isEmpty()) {
                QString color = this->warning_color;
                foreach (auto pair, data->code_analysis) {
                    bool error = pair.second;
                    if (error) {
                        color = this->error_color;
                        break;
                    }
                }
                set_scrollflagarea_painter(&painter,color);
                painter.drawRect(scrollflagarea->make_flag_qrect(position));
            }
            if (!data->todo.isEmpty()) {
                set_scrollflagarea_painter(&painter, this->todo_color);
                painter.drawRect(scrollflagarea->make_flag_qrect(position));
            }
            if (data->breakpoint) {
                set_scrollflagarea_painter(&painter,this->breakpoint_color);
                painter.drawRect(scrollflagarea->make_flag_qrect(position));
            }
        }
        block = block.next();
    }

    if (!this->occurrences.isEmpty()) {
        set_scrollflagarea_painter(&painter, this->occurrence_color);
        foreach (int line_number, this->occurrences) {
            int position = static_cast<int>(this->scrollflagarea->value_to_position(line_number));
            painter.drawRect(scrollflagarea->make_flag_qrect(position));
        }
    }

    if (!this->found_results.isEmpty()) {
        set_scrollflagarea_painter(&painter,this->found_results_color);
        foreach (int line_number, this->found_results) {
            int position = static_cast<int>(this->scrollflagarea->value_to_position(line_number));
            painter.drawRect(scrollflagarea->make_flag_qrect(position));
        }
    }

    QColor pen_color = QColor(Qt::white);
    pen_color.setAlphaF(0.8);
    painter.setPen(pen_color);
    QColor brush_color = QColor(Qt::white);
    brush_color.setAlphaF(0.5);
    painter.setBrush(QBrush(brush_color));
    painter.drawRect(scrollflagarea->make_slider_range(firstVisibleBlock().blockNumber()));
}

void CodeEditor::resizeEvent(QResizeEvent *event)
{
    TextEditBaseWidget::resizeEvent(event);
    QRect cr = this->contentsRect();
    this->linenumberarea->setGeometry(QRect(cr.left(),cr.top(),
                                            compute_linenumberarea_width(),
                                            cr.height()));
    this->__set_scrollflagarea_geometry(cr);
}

void CodeEditor::__set_scrollflagarea_geometry(const QRect &contentrect)
{
    QRect cr = contentrect;
    int vsbw;
    if (this->verticalScrollBar()->isVisible())
        vsbw = this->verticalScrollBar()->contentsRect().width();
    else
        vsbw = 0;
    int _left, _top, right, _bottom;
    this->getContentsMargins(&_left, &_top, &right, &_bottom);
    if (right > vsbw)
        vsbw = 0;
    this->scrollflagarea->setGeometry(QRect(cr.right()-ScrollFlagArea::WIDTH-vsbw,
                                            cr.top(),
                                            this->scrollflagarea->WIDTH,cr.height()));
}


//-----edgeline
bool CodeEditor::viewportEvent(QEvent *event)
{
    QPointF offset = this->contentOffset();
    double x = this->blockBoundingGeometry(this->firstVisibleBlock())
        .translated(offset.x(), offset.y()).left() \
        +this->get_linenumberarea_width() \
        +this->fontMetrics().width(QString(this->edge_line->column, '9'))+5;
    QRect cr = this->contentsRect();
    this->edge_line->setGeometry(QRect(static_cast<int>(x), cr.top(), 1, cr.bottom()));
    this->__set_scrollflagarea_geometry(cr);
    return TextEditBaseWidget::viewportEvent(event);
}

void CodeEditor::_apply_highlighter_color_scheme()
{
    sh::BaseSH* hl = highlighter;
    if (hl) {
        this->set_palette(hl->get_background_color(),
                          hl->get_foreground_color());
        currentline_color = hl->get_currentline_color();
        currentcell_color = hl->get_currentcell_color();
        occurrence_color = hl->get_occurrence_color();
        ctrl_click_color = hl->get_ctrlclick_color();
        sideareas_color = hl->get_sideareas_color();
        comment_color = hl->get_comment_color();
        normal_color = hl->get_foreground_color();
        matched_p_color = hl->get_matched_p_color();
        unmatched_p_color = hl->get_unmatched_p_color();
    }
}


void CodeEditor::apply_highlighter_settings(const QHash<QString, ColorBoolBool> &color_scheme)
{
    if (this->highlighter) {
        this->highlighter->setup_formats(this->font());
        if (!color_scheme.isEmpty())
            this->set_color_scheme(color_scheme);
        else
            this->highlighter->rehighlight();
    }
}


QHash<int,sh::OutlineExplorerData> CodeEditor::get_outlineexplorer_data()
{
    return highlighter->get_outlineexplorer_data();
}


void CodeEditor::set_font(const QFont &font, const QHash<QString,ColorBoolBool> &color_scheme)
{
    if (!color_scheme.isEmpty())
        this->color_scheme = color_scheme;
    this->setFont(font);
    this->update_linenumberarea_width();
    this->apply_highlighter_settings(color_scheme);
}

void CodeEditor::set_color_scheme(const QHash<QString,ColorBoolBool> &color_scheme)
{
    this->color_scheme = color_scheme;
    if (this->highlighter) {
        this->highlighter->set_color_scheme(color_scheme);
        this->_apply_highlighter_color_scheme();
    }
    if (this->highlight_current_cell_enabled)
        this->highlight_current_cell();
    else
        this->unhighlight_current_cell();

    if (this->highlight_current_line_enabled)
        this->highlight_current_line();
    else
        this->unhighlight_current_line();
}

void CodeEditor::set_text(const QString &text)
{
    this->setPlainText(text);
    //禁用下面这行，不然会导致调用下面的函数，使未发生改变的文档状态变为isModified，文件名后出现星号
    //this->set_eol_chars(text);
}

void CodeEditor::set_text_from_file(const QString &filename, QString language)
{
    QString text = encoding::read(filename);
    if (language.isEmpty()) {
        language = get_file_language(filename, text);
    }
    this->set_language(language, filename);
    this->set_text(text);
}

void CodeEditor::append(const QString &text)
{
    QTextCursor cursor = this->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(text);
}


//@Slot()
void CodeEditor::paste()
{
    QClipboard* clipboard = QApplication::clipboard();
    QString text = clipboard->text();
    if (splitlines(text).size() > 1) {
        QString eol_chars = this->get_line_separator();
        clipboard->setText((splitlines(text+eol_chars)).join(eol_chars));
    }
    TextEditBaseWidget::paste();
}

void CodeEditor::get_block_data(const QTextBlock &block)
{
    // TODO highlighter.block_data是什么
}

void CodeEditor::get_fold_level(int block_nb)
{
    QTextBlock block = this->document()->findBlockByNumber(block_nb);
    //QTextBlockUserData* data = block.userData();
    // TODO
}


//==============High-level editor features
//@Slot()
void CodeEditor::center_cursor_on_next_focus()
{
    this->centerCursor();
    disconnect(this,SIGNAL(focus_in()),this,SLOT(center_cursor_on_next_focus()));
}

void CodeEditor::go_to_line(int line, const QString &word)
{
    line = qMin(line, this->get_line_count());
    QTextBlock block = this->document()->findBlockByNumber(line-1);
    this->setTextCursor(QTextCursor(block));
    if (this->isVisible())
        this->centerCursor();
    else
        connect(this,SIGNAL(focus_in()),this,SLOT(center_cursor_on_next_focus()));
    this->horizontalScrollBar()->setValue(0);
    if (!word.isEmpty() && block.text().contains(word))
        this->find(word, QTextDocument::FindCaseSensitively);
}

void CodeEditor::exec_gotolinedialog()
{
    GoToLineDialog* dlg = new GoToLineDialog(this);
    if (dlg->exec())
        this->go_to_line(dlg->get_line_number());
}

void CodeEditor::cleanup_code_analysis()
{
	this->setUpdatesEnabled(false);
	this->clear_extra_selections("code_analysis");
	for (int i = 0; i < this->blockuserdata_list.size(); ++i) {
		this->blockuserdata_list[i]->code_analysis.clear();
		if (this->blockuserdata_list[i]->is_empty()) {
			delete this->blockuserdata_list[i];
			this->blockuserdata_list[i] = nullptr;
		}
	}
	this->blockuserdata_list.removeAll(nullptr);
    this->setUpdatesEnabled(true);

	this->scrollflagarea->update();
	this->linenumberarea->update();
}


void CodeEditor::process_code_analysis(const QList<QList<QVariant> > &check_results)
{
    cleanup_code_analysis();
    if (check_results.isEmpty())
        return;
    setUpdatesEnabled(false);
    QTextCursor cursor = textCursor();
    QTextDocument* document = this->document();
    QTextDocument::FindFlags flags = QTextDocument::FindCaseSensitively | QTextDocument::FindWholeWords;
    foreach (auto pair, check_results) {
        QString message = pair[0].toString();
        int line_number = pair[1].toInt();
        bool error = message.contains("syntax");
        QTextBlock block = this->document()->findBlockByNumber(line_number-1);
        BlockUserData* data = dynamic_cast<BlockUserData*>(block.userData());
        if (data == nullptr)
            data = new BlockUserData(this);
        data->code_analysis.append(qMakePair(message, error));
        block.setUserData(data);
        QRegularExpression re("\\'[a-zA-Z0-9_]*\\'");
        QRegularExpressionMatchIterator iterator = re.globalMatch(message);
        while (iterator.hasNext()) {
            QRegularExpressionMatch match = iterator.next();
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
            QString text = match.captured().chopped(1);
#else
			QString text = match.captured().right(1);
#endif
            text = text.mid(1);

            auto is_line_splitted = [=](int line_no)
            {
                QString text = document->findBlockByNumber(line_no).text();
                QString stripped = text.trimmed();
                return stripped.endsWith("\\") || stripped.endsWith(',')
                        || stripped.size() == 0;
            };

            int line2 = line_number-1;
            while (line2<this->blockCount()-1 && is_line_splitted(line2))
                line2++;
            cursor.setPosition(block.position());
            cursor.movePosition(QTextCursor::StartOfBlock);
            QRegExp regexp(QString("\\b%1\\b").arg(QRegExp::escape(text)),
                           Qt::CaseSensitive);
            QString color = error ? this->error_color : this->warning_color;
            cursor = document->find(regexp, cursor, flags);
            if (!cursor.isNull()) {
                while (!cursor.isNull() && cursor.blockNumber() <= line2
                       && cursor.blockNumber() >= line_number-1
                       && cursor.position() > 0) {
                    this->__highlight_selection("code_analysis",cursor,QColor(),
                                                QColor(),QColor(color));
                    cursor = document->find(text, cursor, flags);
                }
            }
        }
    }
    update_extra_selections();
    setUpdatesEnabled(true);
    linenumberarea->update();
}

void CodeEditor::__show_code_analysis_results(int line_number, const QList<QPair<QString, bool> > &code_analysis)
{
    QStringList msglist;
    foreach (auto pair, code_analysis) {
        msglist.append(pair.first);
    }
    this->show_calltip("Code analysis",msglist,
                       false,"#129625",line_number);
    // 这里调用了show_calltip(text = QtringList)
    // show_calltip本代码总共两处调用，这里第一次
}

int CodeEditor::go_to_next_warning()
{
    QTextBlock block = this->textCursor().block();
    int line_count = this->document()->blockCount();
    BlockUserData* data;
    while (true) {
        if (block.blockNumber()+1 < line_count)
            block = block.next();
        else
            block = this->document()->firstBlock();
        data = dynamic_cast<BlockUserData*>(block.userData());
        if (data && !data->code_analysis.isEmpty())
            break;
    }
    int line_number = block.blockNumber()+1;
    this->go_to_line(line_number);
    this->__show_code_analysis_results(line_number, data->code_analysis);
    return this->get_position("cursor");
}

int CodeEditor::go_to_previous_warning()
{
    QTextBlock block = this->textCursor().block();
    BlockUserData* data;
    while (true) {
        if (block.blockNumber() > 0)
            block = block.previous();
        else
            block = this->document()->lastBlock();
        data = dynamic_cast<BlockUserData*>(block.userData());
        if (data && !data->code_analysis.isEmpty())
            break;
    }
    int line_number = block.blockNumber()+1;
    this->go_to_line(line_number);
    this->__show_code_analysis_results(line_number, data->code_analysis);
    return this->get_position("cursor");
}


//------Tasks management
int CodeEditor::go_to_next_todo()
{
    QTextBlock block = this->textCursor().block();
    int line_count = this->document()->blockCount();
    BlockUserData* data;
    while (true) {
        if (block.blockNumber()+1 < line_count)
            block = block.next();
        else
            block = this->document()->firstBlock();
        data = dynamic_cast<BlockUserData*>(block.userData());
        if (data && !data->todo.isEmpty())
            break;
    }
    int line_number = block.blockNumber()+1;
    this->go_to_line(line_number);
    this->show_calltip("To do",data->todo,
                       false,"#3096FC",line_number);
    // 这里调用了show_calltip(text = Qtring)
    // show_calltip本代码总共两处调用，这里第二次
    return this->get_position("cursor");
}

void CodeEditor::process_todo(const QList<QList<QVariant> > &todo_results)
{
	for (int i = 0; i < this->blockuserdata_list.size(); ++i) {
		this->blockuserdata_list[i]->todo.clear();
		if (this->blockuserdata_list[i]->is_empty()) {
			delete this->blockuserdata_list[i];
			this->blockuserdata_list[i] = nullptr;
		}
	}
	this->blockuserdata_list.removeAll(nullptr);

    for (const auto& pair : todo_results) {
        QString message = pair[0].toString();
        int line_number = pair[1].toInt();
        QTextBlock block = this->document()->findBlockByNumber(line_number-1);
        BlockUserData* data = dynamic_cast<BlockUserData*>(block.userData());
        if (data == nullptr)
            data = new BlockUserData(this);
        data->todo = message;
        block.setUserData(data);
    }
    this->scrollflagarea->update();
}


//------Comments/Indentation
void CodeEditor::add_prefix(const QString& prefix)
{
    QTextCursor cursor = this->textCursor();
    if (this->has_selected_text()) {
        int start_pos=cursor.selectionStart(), end_pos=cursor.selectionEnd();
        int first_pos = qMin(start_pos, end_pos);
        QTextCursor first_cursor = this->textCursor();
        first_cursor.setPosition(first_pos);
        bool begins_at_block_start = first_cursor.atBlockStart();

        cursor.beginEditBlock();
        cursor.setPosition(end_pos);
        if (cursor.atBlockStart()) {
            cursor.movePosition(QTextCursor::PreviousBlock);
            if (cursor.position() <start_pos)
                cursor.setPosition(start_pos);
        }

        while (cursor.position() >= start_pos) {
            cursor.movePosition(QTextCursor::StartOfBlock);
            cursor.insertText(prefix);
            if (start_pos==0 && cursor.blockNumber()==0)
                break;
            cursor.movePosition(QTextCursor::PreviousBlock);
            cursor.movePosition(QTextCursor::EndOfBlock);
        }
        cursor.endEditBlock();
        if (begins_at_block_start) {
            cursor = this->textCursor();
            start_pos = cursor.selectionStart();
            end_pos = cursor.selectionEnd();
            if (start_pos < end_pos)
                start_pos -= prefix.size();
            else
                end_pos -= prefix.size();
            cursor.setPosition(start_pos, QTextCursor::MoveAnchor);
            cursor.setPosition(end_pos, QTextCursor::KeepAnchor);
            this->setTextCursor(cursor);
        }
    }
    else {
        cursor.beginEditBlock();
        cursor.movePosition(QTextCursor::StartOfBlock);
        cursor.insertText(prefix);
        cursor.endEditBlock();
    }
}

void CodeEditor::__is_cursor_at_start_of_block(QTextCursor *cursor)
{
    cursor->movePosition(QTextCursor::StartOfBlock);
}

void CodeEditor::remove_suffix(const QString &suffix)
{
    QTextCursor cursor = this->textCursor();
    cursor.setPosition(cursor.position()-suffix.size(),
                       QTextCursor::KeepAnchor);
    if (cursor.selectedText() == suffix)
        cursor.removeSelectedText();
}

void CodeEditor::remove_prefix(const QString &prefix)
{
    QTextCursor cursor = this->textCursor();
    if (this->has_selected_text()) {
        int start_pos = qMin(cursor.selectionStart(), cursor.selectionEnd());
        int end_pos = qMax(cursor.selectionStart(), cursor.selectionEnd());
        cursor.setPosition(start_pos);
        if (!cursor.atBlockStart()) {
            cursor.movePosition(QTextCursor::StartOfBlock);
            start_pos = cursor.position();
        }
        cursor.beginEditBlock();
        cursor.setPosition(end_pos);

        if (cursor.atBlockStart()) {
            cursor.movePosition(QTextCursor::PreviousBlock);
            if (cursor.position() <start_pos)
                cursor.setPosition(start_pos);
        }

        cursor.movePosition(QTextCursor::StartOfBlock);
        int old_pos = INT_MIN;
        while (cursor.position() >= start_pos) {
            int new_pos = cursor.position();
            if (old_pos == new_pos)
                break;
            else
                old_pos = new_pos;
            QString line_text = cursor.block().text();
            if ((!prefix.trimmed().isEmpty() && lstrip(line_text).startsWith(prefix))
                    || line_text.startsWith(prefix)) {
                cursor.movePosition(QTextCursor::Right,
                                    QTextCursor::MoveAnchor,
                                    line_text.indexOf(prefix));
                cursor.movePosition(QTextCursor::Right,
                                    QTextCursor::KeepAnchor,prefix.size());
                cursor.removeSelectedText();
            }
            cursor.movePosition(QTextCursor::PreviousBlock);
        }
        cursor.endEditBlock();
    }
    else {
        cursor.movePosition(QTextCursor::StartOfBlock);
        QString line_text = cursor.block().text();
        if ((!prefix.trimmed().isEmpty() && lstrip(line_text).startsWith(prefix))
                || line_text.startsWith(prefix)) {
            cursor.movePosition(QTextCursor::Right,
                                QTextCursor::MoveAnchor,
                                line_text.indexOf(prefix));
            cursor.movePosition(QTextCursor::Right,
                                QTextCursor::KeepAnchor,prefix.size());
            cursor.removeSelectedText();
        }
    }
}


bool CodeEditor::fix_indent(bool forward, bool comment_or_string)
{
    if (this->is_python_like())
        return this->fix_indent_smart(forward, comment_or_string);
    else
        return this->simple_indentation(forward, comment_or_string);
}

bool CodeEditor::simple_indentation(bool forward, bool comment_or_string)
{
    Q_UNUSED(comment_or_string);
    QTextCursor cursor = this->textCursor();
    int block_nb = cursor.blockNumber();
    QTextBlock prev_block = this->document()->findBlockByLineNumber(block_nb-1);
    QString prevline = prev_block.text();

    QRegularExpression re("\\s*");
    QRegularExpressionMatch match = re.match(prevline);
    QString indentation;
    if (match.capturedStart() == 0)
        indentation = match.captured();
    if (!forward)
        indentation = indentation.mid(this->indent_chars.size());
    cursor.insertText(indentation);
    return false;
}

bool CodeEditor::fix_indent_smart(bool forward, bool comment_or_string)
{
    QTextCursor cursor = this->textCursor();
    int block_nb = cursor.blockNumber();

    int diff_paren = 0;
    int diff_brack = 0;
    int diff_curly = 0;
    bool add_indent = false;
    int prevline = 0;
    QString prevtext = "";
    for (prevline = block_nb-1; prevline >= 0; --prevline) {
        cursor.movePosition(QTextCursor::PreviousBlock);
        prevtext = rstrip(cursor.block().text());

        int inline_comment = prevtext.indexOf('#');
        if (inline_comment != -1)
            prevtext = prevtext.left(inline_comment);

        if ((this->is_python_like() &&
              !prevtext.trimmed().startsWith('#') && !prevtext.isEmpty()) ||
             !prevtext.isEmpty()) {

            if (!prevtext.trimmed().split(QRegularExpression("\\s")).mid(0,1).contains("return") &&
                    (prevtext.trimmed().endsWith(')') ||
                     prevtext.trimmed().endsWith(']') ||
                     prevtext.trimmed().endsWith('}')))
                comment_or_string = true;
            else if (prevtext.trimmed().endsWith(':') && this->is_python_like()) {
                add_indent = true;
                comment_or_string = true;
            }
            if (prevtext.count(')') > prevtext.count('('))
                diff_paren = prevtext.count(')') - prevtext.count('(');
            else if (prevtext.count(']') > prevtext.count('['))
                diff_brack = prevtext.count(']') - prevtext.count('[');
            else if (prevtext.count('}') > prevtext.count('{'))
                diff_curly = prevtext.count('}') - prevtext.count('{');
            else if (diff_paren || diff_brack || diff_curly) {
                diff_paren += prevtext.count(')') - prevtext.count('(');
                diff_brack += prevtext.count(']') - prevtext.count('[');
                diff_curly += prevtext.count('}') - prevtext.count('{');
                if (!(diff_paren || diff_brack || diff_curly))
                    break;
            }
            else
                break;
        }
    }

    int correct_indent;
    if (prevline)
        correct_indent = this->get_block_indentation(prevline);
    else
        correct_indent = 0;

    int indent = this->get_block_indentation(block_nb);

    if (add_indent) {
        if (this->indent_chars == "\t")
            correct_indent += this->tab_stop_width_spaces;
        else
            correct_indent += this->indent_chars.size();
    }

    if (!comment_or_string) {
        if (prevtext.endsWith(':') && this->is_python_like()) {
            if (this->indent_chars == "\t")
                correct_indent += this->tab_stop_width_spaces;
            else
                correct_indent += this->indent_chars.size();
        }
        else if(this->is_python_like() &&
                (prevtext.endsWith("continue") ||
                 prevtext.endsWith("break") ||
                 prevtext.endsWith("pass") ||
                 (prevtext.trimmed().split(QRegularExpression("\\t")).mid(0,1).contains("return") &&
                  prevtext.split(QRegularExpression("\\(|\\{|\\[")).size() ==
                  prevtext.split(QRegularExpression("\\)|\\}|\\]")).size()))) {
            if (this->indent_chars == "\t")
                correct_indent -= this->tab_stop_width_spaces;
            else
                correct_indent -= this->indent_chars.size();
        }
        else if (prevtext.split(QRegularExpression("\\(|\\{|\\[")).size() > 1) {
            // Dummy elemet to avoid index errors
            QStringList stack = {"dummy"};
            QChar deactivate = QChar(0);
            foreach (QChar c, prevtext) {
                if (deactivate != QChar(0)) {
                    if (c == deactivate)
                        deactivate = QChar(0);
                }
                else if (c=='\'' || c=='\"')
                    deactivate = c;
                else if (c=='(' || c=='[' || c=='{')
                    stack.append(c);
                else if (c==')' && stack.last()=="(")
                    stack.removeLast();
                else if (c==']' && stack.last()=="[")
                    stack.removeLast();
                else if (c=='}' && stack.last()=="{")
                    stack.removeLast();
            }
            if (stack.size() == 1) {

            }
            else if (prevtext.indexOf(QRegularExpression("[\\(|\\{|\\[]\\s*$"))>-1 &&
                     ((this->indent_chars=="\t" &&
                       this->tab_stop_width_spaces*2 < prevtext.size()) ||
                      (this->indent_chars.startsWith(' ') &&
                       this->indent_chars.size()*2 < prevtext.size()))) {
                if (this->indent_chars == "\t")
                    correct_indent += this->tab_stop_width_spaces * 2;
                else
                    correct_indent += this->indent_chars.size() * 2;
            }
            else {
                QHash<QChar,QChar> rlmap = {{')','('}, {']','['}, {'}','{'}};
                bool break_flag = true;
                for (auto it=rlmap.begin();it!=rlmap.end();it++) {
                    QChar par = it.key();
                    int i_right = prevtext.lastIndexOf(par);
                    if (i_right != -1) {
                        prevtext = prevtext.left(i_right);
                        for (int _i=0;_i<prevtext.split(par).size();_i++) {
                            int i_left = prevtext.lastIndexOf(rlmap[par]);
                            if (i_left != -1)
                                prevtext = prevtext.left(i_left);
                            else {
                                break_flag = false;
                                break;
                            }
                        }
                    }
                }
                if (break_flag) {
                    if (!prevtext.trimmed().isEmpty()) {
                        if (prevtext.split(QRegularExpression("\\(|\\{|\\[")).size() > 1) {
                            QString prevexpr = prevtext.split(QRegularExpression("\\(|\\{|\\[")).last();
                            correct_indent = prevtext.size()-prevexpr.size();
                        }
                        else
                            correct_indent = prevtext.size();
                    }
                }
            }
        }
    }

    if (!(diff_paren || diff_brack || diff_curly) &&
            !prevtext.endsWith(':') && prevline) {
        int cur_indent = this->get_block_indentation(block_nb-1);
        bool is_blank = this->get_text_line(block_nb-1).trimmed().isEmpty();
        int prevline_indent = this->get_block_indentation(prevline);
        QString trailing_text = this->get_text_line(block_nb).trimmed();

        if (cur_indent < prevline_indent &&
                (!trailing_text.isEmpty() || is_blank)) {
            if (cur_indent % this->indent_chars.size() == 0)
                correct_indent = cur_indent;
            else
                correct_indent = cur_indent + (this->indent_chars.size() - cur_indent % this->indent_chars.size());
        }
    }

    if ((forward && indent >= correct_indent) ||
            (!forward && indent <= correct_indent))
        return false;

    if (correct_indent >= 0) {
        QTextCursor cursor = this->textCursor();
        cursor.movePosition(QTextCursor::StartOfBlock);
        if (this->indent_chars == "\t")
            indent = indent / this->tab_stop_width_spaces;
        cursor.setPosition(cursor.position()+indent,QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
        QString indent_text;
        if (this->indent_chars == "\t")
            indent_text = QString(correct_indent / this->tab_stop_width_spaces,'\t')
                    + QString(correct_indent % this->tab_stop_width_spaces,' ');
        else
            indent_text = QString(correct_indent,' ');
        cursor.insertText(indent_text);
        return true;
    }
    return false;
}

//@Slot()
void CodeEditor::clear_all_output()
{
    try {
        qDebug()<<"clear_all_output"<<__FILE__<<__LINE__;
        // TODO该函数依赖于第三方库nbformat

    } catch (const std::exception &e) {
        QMessageBox::critical(this,"Removal error",
                              QString("It was not possible to remove outputs from this notebook. The error is:\n\n")+e.what());

        return;
    }
}

//@Slot()
void CodeEditor::convert_notebook()
{
    try {
        qDebug()<<"convert_notebook"<<__FILE__<<__LINE__;
        // TODO该函数依赖于第三方库nbformat

    } catch (const std::exception &e) {
        QMessageBox::critical(this,"Conversion error",
                              QString("It was not possible to convert this notebook. The error is:\n\n")+e.what());
        return;
    }
    //TODO emit sig_new_file();
}


void CodeEditor::indent(bool force)
{
    QString leading_text = this->get_text("sol","cursor");
    if (this->has_selected_text())
        this->add_prefix(this->indent_chars);
    else if (force || leading_text.trimmed().isEmpty()
             || (this->tab_indents && this->tab_mode)) {
        if (this->is_python_like()) {
            if (!this->fix_indent(true))
                this->add_prefix(this->indent_chars);
        }
        else
            this->add_prefix(this->indent_chars);
    }
    else {
        if (this->indent_chars.size() > 1) {
            int length = this->indent_chars.size();
            this->insert_text(QString(length-(leading_text.size()%length),' '));
        }
        else
            this->insert_text(this->indent_chars);
    }
}

void CodeEditor::indent_or_replace()
{
    if ((this->tab_indents && this->tab_mode) || !this->has_selected_text())
        this->indent();
    else {
        QTextCursor cursor = this->textCursor();
        if (this->get_selected_text() == cursor.block().text())
            this->indent();
        else {
            QTextCursor cursor1 = this->textCursor();
            cursor1.setPosition(cursor.selectionStart());
            QTextCursor cursor2 = this->textCursor();
            cursor2.setPosition(cursor.selectionEnd());
            if (cursor1.blockNumber() != cursor2.blockNumber())
                this->indent();
            else
                this->replace(this->indent_chars);
        }
    }
}

void CodeEditor::unindent(bool force)
{
    if (this->has_selected_text())
        this->remove_prefix(this->indent_chars);
    else {
        QString leading_text = this->get_text("sol","cursor");
        if (force || leading_text.trimmed().isEmpty()
                || (this->tab_indents && this->tab_mode)) {
            if (this->is_python_like()) {
                if (!this->fix_indent(false))
                    this->remove_prefix(this->indent_chars);
            }
            else if (leading_text.endsWith("\t"))
                this->remove_prefix("\t");
            else
                this->remove_prefix(this->indent_chars);
        }
    }
}


// @Slot()
void CodeEditor::toggle_comment()
{
    QTextCursor cursor = this->textCursor();
    int start_pos = qMin(cursor.selectionStart(), cursor.selectionEnd());
    int end_pos = qMax(cursor.selectionStart(), cursor.selectionEnd());
    cursor.setPosition(end_pos);
    int last_line = cursor.block().blockNumber();
    if (cursor.atBlockStart() && start_pos!=end_pos)
        last_line -= 1;
    cursor.setPosition(start_pos);
    int first_line = cursor.block().blockNumber();

    bool is_comment_or_whitespace = true;
    bool at_least_one_comment = false;
    for (int _line_nb = first_line; _line_nb < last_line+1; ++_line_nb) {
        QString text = lstrip(cursor.block().text());
        bool is_comment = text.startsWith(this->comment_string);
        bool is_whitespace = text == "";
        is_comment_or_whitespace *= (is_comment || is_whitespace);
        if (is_comment)
            at_least_one_comment = true;
        cursor.movePosition(QTextCursor::NextBlock);
    }
    if (is_comment_or_whitespace && at_least_one_comment)
        this->uncomment();
    else
        this->comment();
}

void CodeEditor::comment()
{
    this->add_prefix(this->comment_string);
}

void CodeEditor::uncomment()
{
    this->remove_prefix(this->comment_string);
}


QString CodeEditor::__blockcomment_bar()
{
    QString res = this->comment_string + " " + QString(78-this->comment_string.size(), '=');
    return res;
}

void CodeEditor::transform_to_uppercase()
{
    QTextCursor cursor = this->textCursor();
    int prev_pos = cursor.position();
    QString selected_text = cursor.selectedText();

    if (selected_text.size() == 0) {
        prev_pos = cursor.position();
        cursor.select(QTextCursor::WordUnderCursor);
        selected_text = cursor.selectedText();
    }

    QString s = selected_text.toUpper();
    cursor.insertText(s);
    this->set_cursor_position(prev_pos);
}

void CodeEditor::transform_to_lowercase()
{
    QTextCursor cursor = this->textCursor();
    int prev_pos = cursor.position();
    QString selected_text = cursor.selectedText();

    if (selected_text.size() == 0) {
        prev_pos = cursor.position();
        cursor.select(QTextCursor::WordUnderCursor);
        selected_text = cursor.selectedText();
    }

    QString s = selected_text.toLower();
    cursor.insertText(s);
    this->set_cursor_position(prev_pos);
}

void CodeEditor::blockcomment()
{
    QString comline = this->__blockcomment_bar() + this->get_line_separator();
    QTextCursor cursor = this->textCursor();
    int start_pos, end_pos;
    if (this->has_selected_text()) {
        this->extend_selection_to_complete_lines();
        start_pos = cursor.selectionStart();
        end_pos = cursor.selectionEnd();
    }
    else {
        start_pos = cursor.position();
        end_pos = cursor.position();
    }
    cursor.beginEditBlock();
    cursor.setPosition(start_pos);
    cursor.movePosition(QTextCursor::StartOfBlock);
    while (cursor.position() <= end_pos) {
        cursor.insertText(this->comment_string + " ");
        cursor.movePosition(QTextCursor::EndOfBlock);
        if (cursor.atEnd())
            break;
        cursor.movePosition(QTextCursor::NextBlock);
        end_pos += this->comment_string.size() + 1;
    }
    cursor.setPosition(end_pos);
    cursor.movePosition(QTextCursor::EndOfBlock);
    if (cursor.atEnd())
        cursor.insertText(this->get_line_separator());
    else
        cursor.movePosition(QTextCursor::NextBlock);
    cursor.insertText(comline);
    cursor.setPosition(start_pos);
    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.insertText(comline);
    cursor.endEditBlock();
}


void CodeEditor::unblockcomment()
{
    auto __is_comment_bar = [this](const QTextCursor &cursor)
    {
        return cursor.block().text().startsWith(this->__blockcomment_bar());
    };

    QTextCursor cursor1 = this->textCursor();
    if (__is_comment_bar(cursor1))
        return;
    while (!__is_comment_bar(cursor1)) {
        cursor1.movePosition(QTextCursor::PreviousBlock);
        if (cursor1.atStart())
            break;
    }
    if (!__is_comment_bar(cursor1))
        return;

    auto __in_block_comment = [this](const QTextCursor &cursor)
    {
        QString cs = this->comment_string;
        return cursor.block().text().startsWith(cs);

    };

    QTextCursor cursor2 = QTextCursor(cursor1);
    cursor2.movePosition(QTextCursor::NextBlock);
    while (!__is_comment_bar(cursor2) && __in_block_comment(cursor2)) {
        cursor2.movePosition(QTextCursor::NextBlock);
        if (cursor2.block() == this->document()->lastBlock())
            break;
    }
    if (!__is_comment_bar(cursor2))
        return;

    QTextCursor cursor3 = this->textCursor();
    cursor3.beginEditBlock();
    cursor3.setPosition(cursor1.position());
    cursor3.movePosition(QTextCursor::NextBlock);
    while (cursor3.position() < cursor2.position()) {
        cursor3.movePosition(QTextCursor::NextCharacter,
                             QTextCursor::KeepAnchor);
        if (!cursor3.atBlockEnd())
            cursor3.movePosition(QTextCursor::NextCharacter,
                                 QTextCursor::KeepAnchor);
        cursor3.removeSelectedText();
        cursor3.movePosition(QTextCursor::NextBlock);
    }
    QList<QTextCursor> tmp;
    tmp << cursor2 << cursor1;
    foreach (QTextCursor cursor, tmp) {
        cursor3.setPosition(cursor.position());
        cursor3.select(QTextCursor::BlockUnderCursor);
        cursor3.removeSelectedText();
    }
    cursor3.endEditBlock();
}


//------Kill ring handlers
void CodeEditor::kill_line_end()
{
    QTextCursor cursor = this->textCursor();
    cursor.clearSelection();
    cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
    if (!cursor.hasSelection())
        cursor.movePosition(QTextCursor::NextBlock,
                            QTextCursor::KeepAnchor);
    this->_kill_ring->kill_cursor(&cursor);
    this->setTextCursor(cursor);
}

void CodeEditor::kill_line_start()
{
    QTextCursor cursor = this->textCursor();
    cursor.clearSelection();
    cursor.movePosition(QTextCursor::StartOfBlock,
                        QTextCursor::KeepAnchor);
    this->_kill_ring->kill_cursor(&cursor);
    this->setTextCursor(cursor);
}

QTextCursor CodeEditor::_get_word_start_cursor(int position)
{
    QTextDocument* document = this->document();
    position--;
    while (position && !document->characterAt(position).isLetterOrNumber())
        position--;
    while (position && document->characterAt(position).isLetterOrNumber())
        position--;
    QTextCursor cursor = this->textCursor();
    cursor.setPosition(position+1);
    return cursor;
}

QTextCursor CodeEditor::_get_word_end_cursor(int position)
{
    QTextDocument* document = this->document();
    QTextCursor cursor = this->textCursor();
    position = cursor.position();
    cursor.movePosition(QTextCursor::End);
    int end = cursor.position();
    while (position < end && !document->characterAt(position).isLetterOrNumber())
        position++;
    while (position < end && document->characterAt(position).isLetterOrNumber())
        position++;
    cursor.setPosition(position);
    return cursor;
}

void CodeEditor::kill_prev_word()
{
    int position = this->textCursor().position();
    QTextCursor cursor = this->_get_word_start_cursor(position);
    cursor.setPosition(position, QTextCursor::KeepAnchor);
    this->_kill_ring->kill_cursor(&cursor);
    this->setTextCursor(cursor);
}

void CodeEditor::kill_next_word()
{
    int position = this->textCursor().position();
    QTextCursor cursor = this->_get_word_end_cursor(position);
    cursor.setPosition(position, QTextCursor::KeepAnchor);
    this->_kill_ring->kill_cursor(&cursor);
    this->setTextCursor(cursor);
}


//------Autoinsertion of quotes/colons
QString CodeEditor::__get_current_color(QTextCursor cursor)
{
    if (cursor.isNull())
        cursor = this->textCursor();
    QTextBlock block = cursor.block();
    int pos = cursor.position() - block.position();
    QTextLayout* layout = block.layout();
    QVector<QTextLayout::FormatRange> block_formats = layout->formats();

    if (!block_formats.isEmpty()) {
        QTextCharFormat current_format;
        if (cursor.atBlockEnd())
            current_format = block_formats.last().format;
        else {
            int flag = 1;
            foreach (auto fmt, block_formats) {
                if ((pos>=fmt.start) && (pos<fmt.start+fmt.length)) {
                    current_format = fmt.format;
                    flag = 0;
                }
            }
            if (flag)
                return QString();
        }
        QString color = current_format.foreground().color().name();
        return color;
    }
    else
        return QString();
}

bool CodeEditor::in_comment_or_string(QTextCursor cursor)
{
    if (this->highlighter) {
        QString current_color;
        if (cursor.isNull())
            current_color = this->__get_current_color();
        else
            current_color = this->__get_current_color(cursor);

        QString comment_color = this->highlighter->get_color_name("comment");
        QString string_color = this->highlighter->get_color_name("string");
        if ((current_color==comment_color) || (current_color==string_color))
            return true;
        else
            return false;
    }
    else
        return false;
}

bool CodeEditor::__colon_keyword(QString text)
{
    QStringList stmt_kws = {"def", "for", "if", "while", "with", "class", "elif",
                            "except"};
    QStringList whole_kws = {"else", "try", "except", "finally"};
    text = lstrip(text);
    QStringList words = text.split(QRegularExpression("\\s+"));
    foreach (const QString& wk, whole_kws) {
        if (text == wk)
            return true;
    }
    if (words.size() < 2)
        return false;
    foreach (const QString& sk, stmt_kws) {
        if (words[0] == sk)
            return true;
    }
    return false;
}

bool CodeEditor::__forbidden_colon_end_char(QString text)
{
    QList<QChar> end_chars = {':', '\\', '[', '{', '(', ','};
    text = rstrip(text);
    foreach (const QChar& c, end_chars) {
        if (text.endsWith(c))
            return true;
    }
    return false;
}

bool CodeEditor::__unmatched_braces_in_line(const QString &text,const QChar& closing_braces_type)
{
    QList<QChar> opening_braces, closing_braces;
    if (closing_braces_type == QChar(0)) {
        opening_braces = {'(', '[', '{'};
        closing_braces = {')', ']', '}'};
    }
    else {
        closing_braces = {closing_braces_type};
        QHash<QChar,QChar> dict = {{')','('}, {'}','{'}, {']','['}};
        opening_braces = {dict[closing_braces_type]};
    }
    QTextBlock block = this->textCursor().block();
    int line_pos = block.position();
    for (int pos = 0; pos < text.size(); ++pos) {
        QChar _char = text[pos];
        if (opening_braces.contains(_char)) {
            int match = this->find_brace_match(line_pos+pos, _char, true);
            if ((match == -1) || (match > line_pos+text.size()))
                return true;
        }
        if (closing_braces.contains(_char)) {
            int match = this->find_brace_match(line_pos+pos, _char, false);
            if ((match == -1) || (match < line_pos))
                return true;
        }
    }
    return false;
}

bool CodeEditor::__has_colon_not_in_brackets(const QString &text)
{
    for (int pos = 0; pos < text.size(); ++pos) {
        QChar _char = text[pos];
        if (_char == ':' && !this->__unmatched_braces_in_line(text.left(pos)))
            return true;
    }
    return false;
}

bool CodeEditor::autoinsert_colons()
{
    QString line_text = this->get_text("sol","cursor");
    if (!this->textCursor().atBlockEnd())
        return false;
    else if (this->in_comment_or_string())
        return false;
    else if (!this->__colon_keyword(line_text))
        return false;
    else if (this->__forbidden_colon_end_char(line_text))
        return false;
    else if (this->__unmatched_braces_in_line(line_text))
        return false;
    else if (this->__has_colon_not_in_brackets(line_text))
        return false;
    else
        return true;
}

QString CodeEditor::__unmatched_quotes_in_line(QString text)
{
    text.replace("\\'","");
    text.replace("\\\"","");
    if (text.count("\"") % 2)
        return "\"";
    else if (text.count("'") % 2)
        return "'";
    else
        return "";
}

QString CodeEditor::__next_char()
{
    QTextCursor cursor = this->textCursor();
    cursor.movePosition(QTextCursor::NextCharacter,
                        QTextCursor::KeepAnchor);
    QString next_char = cursor.selectedText();
    return next_char;
}

bool CodeEditor::__in_comment()
{
    if (this->highlighter) {
        QString current_color = this->__get_current_color();
        QString comment_color = this->highlighter->get_color_name("comment");
        if (current_color == comment_color)
            return true;
        else
            return false;
    }
    else
        return false;
}

void CodeEditor::autoinsert_quotes(int key)
{
    QHash<int,QChar> dict = {{Qt::Key_QuoteDbl, '"'}, {Qt::Key_Apostrophe, '\''}};
    QChar _char = dict[key];

    QString line_text = this->get_text("sol","eol");
    QString line_to_cursor = this->get_text("sol","cursor");
    QTextCursor cursor = this->textCursor();
    QString last_three = this->get_text("sol","cursor").right(3);
    QString last_two = this->get_text("sol","cursor").right(2);
    QString trailing_text = this->get_text("cursor","eol").trimmed();

    if (this->has_selected_text()) {
        QString text = _char + this->get_selected_text() + _char;
        this->insert_text(text);
    }
    else if (this->__in_comment())
        this->insert_text(_char);
    else if (trailing_text.size() > 0 &&
             !(this->__unmatched_quotes_in_line(line_to_cursor) == _char))
        this->insert_text(_char);
    else if (!this->__unmatched_quotes_in_line(line_text).isEmpty() &&
             !(last_three == QString(3,_char)))
        this->insert_text(_char);
    else if (this->__next_char() == _char) {
        cursor.movePosition(QTextCursor::NextCharacter,
                            QTextCursor::KeepAnchor, 1);
        cursor.clearSelection();
        this->setTextCursor(cursor);
    }
    else if (last_three == QString(3,_char)) {
        this->insert_text(QString(3,_char));
        cursor = this->textCursor();
        cursor.movePosition(QTextCursor::PreviousCharacter,
                            QTextCursor::KeepAnchor, 3);
        cursor.clearSelection();
        this->setTextCursor(cursor);
    }
    else if (last_two == QString(2,_char)) {
        this->insert_text(_char);
    }
    else {
        this->insert_text(QString(2,_char));
        cursor = this->textCursor();
        cursor.movePosition(QTextCursor::PreviousCharacter);
        this->setTextCursor(cursor);
    }
}


void CodeEditor::setup_context_menu()
{
    undo_action = new QAction("Undo",this);
    connect(undo_action,SIGNAL(triggered(bool)),this,SLOT(undo()));
    undo_action->setIcon(ima::icon("undo"));
    undo_action->setShortcut(QKeySequence("Ctrl+Z"));
    undo_action->setShortcutContext(Qt::WindowShortcut);

    redo_action = new QAction("Redo",this);
    connect(redo_action,SIGNAL(triggered(bool)),this,SLOT(redo()));
    redo_action->setIcon(ima::icon("redo"));
    redo_action->setShortcut(QKeySequence("Ctrl+Shift+Z"));
    redo_action->setShortcutContext(Qt::WindowShortcut);

    cut_action = new QAction("Cut",this);
    connect(cut_action,SIGNAL(triggered(bool)),this,SLOT(cut()));
    cut_action->setIcon(ima::icon("editcut"));
    cut_action->setShortcut(QKeySequence("Ctrl+X"));
    cut_action->setShortcutContext(Qt::WindowShortcut);

    copy_action = new QAction("Copy",this);
    connect(copy_action,SIGNAL(triggered(bool)),this,SLOT(copy()));
    copy_action->setIcon(ima::icon("editcopy"));
    copy_action->setShortcut(QKeySequence("Ctrl+C"));
    copy_action->setShortcutContext(Qt::WindowShortcut);

    paste_action = new QAction("Paste",this);
    connect(paste_action,SIGNAL(triggered(bool)),this,SLOT(paste()));
    paste_action->setIcon(ima::icon("editpaste"));
    paste_action->setShortcut(QKeySequence("Ctrl+V"));
    paste_action->setShortcutContext(Qt::WindowShortcut);

    QAction* selectall_action = new QAction("Select All",this);
    connect(selectall_action,SIGNAL(triggered(bool)),this,SLOT(selectAll()));
    selectall_action->setIcon(ima::icon("selectall"));
    selectall_action->setShortcut(QKeySequence("Ctrl+A"));
    selectall_action->setShortcutContext(Qt::WindowShortcut);

    QAction* toggle_comment_action = new QAction("Comment/Uncomment",this);
    connect(toggle_comment_action,SIGNAL(triggered(bool)),this,SLOT(toggle_comment()));
    toggle_comment_action->setIcon(ima::icon("comment"));
    toggle_comment_action->setShortcut(QKeySequence("Ctrl+1"));
    toggle_comment_action->setShortcutContext(Qt::WindowShortcut);

    clear_all_output_action = new QAction("Clear all output",this);
    connect(clear_all_output_action,SIGNAL(triggered(bool)),this,SLOT(clear_all_output()));
    clear_all_output_action->setIcon(ima::icon("ipython_console"));
    clear_all_output_action->setShortcutContext(Qt::WindowShortcut);

    ipynb_convert_action = new QAction("Convert to Python script",this);
    connect(ipynb_convert_action,SIGNAL(triggered(bool)),this,SLOT(convert_notebook()));
    ipynb_convert_action->setIcon(ima::icon("python"));
    ipynb_convert_action->setShortcutContext(Qt::WindowShortcut);

    gotodef_action = new QAction("Go to definition",this);
    connect(gotodef_action,SIGNAL(triggered(bool)),this,SLOT(go_to_definition_from_cursor()));
    gotodef_action->setIcon(ima::icon("go to definition"));
    gotodef_action->setShortcut(QKeySequence("Ctrl+G"));
    gotodef_action->setShortcutContext(Qt::WindowShortcut);

    //# Run actions
    run_cell_action = new QAction("Run cell",this);
    connect(run_cell_action,SIGNAL(triggered(bool)),this,SIGNAL(run_cell()));
    run_cell_action->setIcon(ima::icon("run_cell"));
    run_cell_action->setShortcut(QKeySequence("Ctrl+Return"));
    run_cell_action->setShortcutContext(Qt::WindowShortcut);

    run_cell_and_advance_action = new QAction("Run cell and advance",this);
    connect(run_cell_and_advance_action,SIGNAL(triggered(bool)),this,SIGNAL(run_cell_and_advance()));
    run_cell_and_advance_action->setIcon(ima::icon("run_cell"));
    run_cell_and_advance_action->setShortcut(QKeySequence("Shift+Return"));
    run_cell_and_advance_action->setShortcutContext(Qt::WindowShortcut);

    re_run_last_cell_action = new QAction("Re-run last cell",this);
    connect(re_run_last_cell_action,SIGNAL(triggered(bool)),this,SIGNAL(re_run_last_cell()));
    re_run_last_cell_action->setIcon(ima::icon("run_cell"));
    re_run_last_cell_action->setShortcut(QKeySequence("Alt+Return"));
    re_run_last_cell_action->setShortcutContext(Qt::WindowShortcut);

    run_selection_action = new QAction("Run &selection or current line",this);
    connect(run_selection_action,SIGNAL(triggered(bool)),this,SIGNAL(run_selection()));
    run_selection_action->setIcon(ima::icon("run selection"));
    run_selection_action->setShortcut(QKeySequence("F9"));
    run_selection_action->setShortcutContext(Qt::WindowShortcut);

    //# Zoom actions
    QAction* zoom_in_action = new QAction("Zoom in",this);
    connect(zoom_in_action,SIGNAL(triggered(bool)),this,SIGNAL(zoom_in()));
    zoom_in_action->setIcon(ima::icon("zoom_in"));
    zoom_in_action->setShortcut(QKeySequence("ZoomIn"));
    zoom_in_action->setShortcutContext(Qt::WindowShortcut);

    QAction* zoom_out_action = new QAction("Zoom out",this);
    connect(zoom_out_action,SIGNAL(triggered(bool)),this,SIGNAL(zoom_out()));
    zoom_out_action->setIcon(ima::icon("zoom_out"));
    zoom_out_action->setShortcut(QKeySequence("ZoomOut"));
    zoom_out_action->setShortcutContext(Qt::WindowShortcut);

    QAction* zoom_reset_action = new QAction("Zoom reset",this);
    connect(zoom_reset_action,SIGNAL(triggered(bool)),this,SIGNAL(zoom_reset()));
    zoom_reset_action->setShortcut(QKeySequence("Ctrl+0"));
    zoom_reset_action->setShortcutContext(Qt::WindowShortcut);

    this->menu = new QMenu(this);
    QList<QAction*> actions_1 = { this->run_cell_action, this->run_cell_and_advance_action,
                                  this->re_run_last_cell_action, this->run_selection_action,
                                  this->gotodef_action, nullptr, this->undo_action,
                                  this->redo_action, nullptr, this->cut_action,
                                  this->copy_action, this->paste_action, selectall_action };
    QList<QAction*> actions_2 = { nullptr, zoom_in_action, zoom_out_action, zoom_reset_action,
                                  nullptr, toggle_comment_action };

    QList<QAction*> actions = actions_1 + actions_2;
    add_actions(this->menu, actions);

    this->readonly_menu = new QMenu(this);
    QList<QAction*> tmp = { this->copy_action, nullptr, selectall_action,
                            this->gotodef_action };
    add_actions(this->readonly_menu, tmp);
}


void CodeEditor::keyReleaseEvent(QKeyEvent *event)
{
    this->timer_syntax_highlight->start();
    TextEditBaseWidget::keyReleaseEvent(event);
    event->ignore();
}

void CodeEditor::keyPressEvent(QKeyEvent *event)
{
	emit this->sig_key_pressed(event);
    int key = event->key();
    bool ctrl=  event->modifiers() & Qt::ControlModifier;
    bool shift = event->modifiers() & Qt::ShiftModifier;
    QString text = event->text();

    bool has_selection = this->has_selected_text();
    if (!text.isEmpty())
        this->__clear_occurrences();
    if (QToolTip::isVisible())
        this->hide_tooltip_if_necessary(key);

    QList<QPair<QString,QString>> checks = {{qMakePair(QString("SelectAll"), QString("Select All"))},
                                           {qMakePair(QString("Copy"), QString("Copy"))},
                                           {qMakePair(QString("Cut"), QString("Cut"))},
                                           {qMakePair(QString("Paste"), QString("Paste"))}};

    //TODO for qname, name in checks:
    //    seq = getattr(QKeySequence, qname)

    QStringList list_two = {"()", "[]", "{}", "\'\'", "\"\""};
    QList<QChar> list_one = {',', ')', ']', '}'};
    QHash<QString,QString> dict = {{"{", "{}"}, {"[", "[]"}};

    if (key == Qt::Key_Enter || key == Qt::Key_Return) {
        if (!shift && !ctrl) {
            if (this->add_colons_enabled && this->is_python_like() &&
                this->autoinsert_colons()) {
                this->textCursor().beginEditBlock();
                this->insert_text(':' + this->get_line_separator());
                this->fix_indent();
                this->textCursor().endEditBlock();
            }
            else if (this->is_completion_widget_visible()
                     && this->codecompletion_enter)
                this->select_completion_list();
            else {
                bool cmt_or_str_cursor = this->in_comment_or_string();

                QTextCursor cursor = this->textCursor();
                cursor.setPosition(cursor.block().position(),
                                   QTextCursor::KeepAnchor);
                bool cmt_or_str_line_begin = this->in_comment_or_string(
                                            cursor);

                bool cmt_or_str = cmt_or_str_cursor && cmt_or_str_line_begin;

                this->textCursor().beginEditBlock();
                TextEditBaseWidget::keyPressEvent(event);
                this->fix_indent(cmt_or_str);
                this->textCursor().endEditBlock();
            }
        }
        else if (shift)
            emit this->run_cell_and_advance();
        else if (ctrl)
            emit this->run_cell();
    }
    else if (shift && key == Qt::Key_Delete) {
        if (has_selection)
            this->cut();
        else
            this->delete_line();
    }
    else if (shift && key == Qt::Key_Insert)
        this->paste();
    else if (key == Qt::Key_Insert && !shift && !ctrl)
        this->setOverwriteMode(!this->overwriteMode());
    else if (key == Qt::Key_Backspace && !shift && !ctrl) {
        QString leading_text = this->get_text("sol", "cursor");
        int leading_length = leading_text.size();
        int trailing_spaces = leading_length - rstrip(leading_text).size();
        if (has_selection || !this->intelligent_backspace)
            TextEditBaseWidget::keyPressEvent(event);
        else {
            QString trailing_text = this->get_text("cursor", "eol");
            if (leading_text.trimmed().isEmpty()
               && leading_length > this->indent_chars.size()) {
                if (leading_length % this->indent_chars.size() == 0)
                    this->unindent();
                else
                    TextEditBaseWidget::keyPressEvent(event);
            }
            else if (trailing_spaces && trailing_text.trimmed().isEmpty())
                this->remove_suffix(leading_text.right(trailing_spaces));
            else if (!leading_text.isEmpty() && !trailing_text.isEmpty() &&
                     list_two.contains(leading_text.right(1) + trailing_text.left(1))) {
                QTextCursor cursor = this->textCursor();
                cursor.movePosition(QTextCursor::PreviousCharacter);
                cursor.movePosition(QTextCursor::NextCharacter,
                                    QTextCursor::KeepAnchor, 2);
                cursor.removeSelectedText();
            }
            else {
                TextEditBaseWidget::keyPressEvent(event);
                if (this->is_completion_widget_visible())
                    this->completion_text.chop(1);
            }
        }
    }
    else if (key == Qt::Key_Period) {
        this->insert_text(text);
        if (this->is_python_like() &&
                !this->in_comment_or_string() && this->codecompletion_auto) {
            QString last_obj = getobj(this->get_text("sol", "cursor"));
            if (!last_obj.isEmpty() && !isdigit(last_obj))
                this->do_completion(true);
        }
    }
    else if (key == Qt::Key_Home)
        this->stdkey_home(shift, ctrl);
    else if (key == Qt::Key_End)
        this->stdkey_end(shift, ctrl);
    else if (text == "(" && !has_selection) {
        this->hide_completion_widget();
        this->handle_parentheses(text);
    }
    else if ((text == "[" || text == "{") && !has_selection &&
             this->close_parentheses_enabled) {
        QString s_trailing_text = this->get_text("cursor", "eol").trimmed();
        if (s_trailing_text.size() == 0 ||
            list_one.contains(s_trailing_text[0])) {
            this->insert_text(dict[text]);
            QTextCursor cursor = this->textCursor();
            cursor.movePosition(QTextCursor::PreviousCharacter);
            this->setTextCursor(cursor);
        }
        else
            TextEditBaseWidget::keyPressEvent(event);
    }
    else if ((key == Qt::Key_QuoteDbl || key == Qt::Key_Apostrophe) &&
              this->close_quotes_enabled)
        this->autoinsert_quotes(key);
    else if ((key == Qt::Key_ParenRight || key ==  Qt::Key_BraceRight || key ==  Qt::Key_BracketRight)
              && !has_selection && this->close_parentheses_enabled
              && !this->textCursor().atBlockEnd()) {
        QTextCursor cursor = this->textCursor();
        cursor.movePosition(QTextCursor::NextCharacter,
                            QTextCursor::KeepAnchor);
        QString text = cursor.selectedText();
        QHash<int,QString> dict2 = {{Qt::Key_ParenRight, ")"},
                                  {Qt::Key_BraceRight, "}"},
                                  {Qt::Key_BracketRight, "]"}};
        bool key_matches_next_char = text == dict2[key];
        if (key_matches_next_char
                && !this->__unmatched_braces_in_line(
                    cursor.block().text(), text[0])) {
            cursor.clearSelection();
            this->setTextCursor(cursor);
        }
        else
            TextEditBaseWidget::keyPressEvent(event);
    }
    else if (key == Qt::Key_Colon && !has_selection
             && this->auto_unindent_enabled) {
        QString leading_text = this->get_text("sol", "cursor");
        if (lstrip(leading_text)=="else" ||  lstrip(leading_text)=="finally") {
            auto ind = [](const QString& txt) ->int { return txt.size()-lstrip(txt).size(); };
            QString prevtxt = this->textCursor().block().previous().text();
            if (ind(leading_text) == ind(prevtxt))
                this->unindent(true);
        }
        TextEditBaseWidget::keyPressEvent(event);
    }
    else if (key == Qt::Key_Space && !shift && !ctrl
             && !has_selection && this->auto_unindent_enabled) {
        QString leading_text = this->get_text("sol", "cursor");
        if (lstrip(leading_text)=="elif" ||  lstrip(leading_text)=="except") {
            auto ind = [](const QString& txt) ->int { return txt.size()-lstrip(txt).size(); };
            QString prevtxt = this->textCursor().block().previous().text();
            if (ind(leading_text) == ind(prevtxt))
                this->unindent(true);
        }
        TextEditBaseWidget::keyPressEvent(event);
    }
    else if (key == Qt::Key_Tab) {
        if (!has_selection && !this->tab_mode)
            this->intelligent_tab();
        else
            this->indent_or_replace();
    }
    else if (key == Qt::Key_Backtab)
        if (!has_selection && !this->tab_mode)
            this->intelligent_backtab();
        else
            this->unindent();
    else {
        TextEditBaseWidget::keyPressEvent(event);
        if (this->is_completion_widget_visible() && !text.isEmpty())
            this->completion_text += text;
    }
}


// 该函数依赖于第三方库pygments语法高亮库，因此不去实现它
void CodeEditor::run_pygments_highlighter()
{}

void CodeEditor::handle_parentheses(const QString &text)
{
    int position = this->get_position("cursor");
    QString rest = rstrip(this->get_text("cursor","eol"));
    QList<QChar> list = {',', ')', ']', '}'};
    bool valid = rest.isEmpty() || list.contains(rest[0]);
    if (this->close_parentheses_enabled && valid) {
        this->insert_text("()");
        QTextCursor cursor = this->textCursor();
        cursor.movePosition(QTextCursor::PreviousCharacter);
        this->setTextCursor(cursor);
    }
    else
        this->insert_text(text);
    if (this->is_python_like() && !this->get_text("sol","cursor").isEmpty()
            && this->calltips)
        emit this->sig_show_object_info(position);
}


void CodeEditor::mouseMoveEvent(QMouseEvent *event)
{
    if (this->has_selected_text()) {
        TextEditBaseWidget::mouseMoveEvent(event);
        return;
    }
    if (this->go_to_definition_enabled &&
        event->modifiers() & Qt::ControlModifier) {
        QString text = this->get_word_at(event->pos());
        if (!text.isEmpty() && (this->is_python_like())
           && !sourcecode::is_keyword(text)) {
            if (!this->__cursor_changed) {
                QApplication::setOverrideCursor(QCursor(Qt::PointingHandCursor));
                this->__cursor_changed = true;
            }
            QTextCursor cursor = this->cursorForPosition(event->pos());
            cursor.select(QTextCursor::WordUnderCursor);
            this->clear_extra_selections("ctrl_click");
            this->__highlight_selection("ctrl_click", cursor,
                            this->ctrl_click_color,
                            QColor(),
                            this->ctrl_click_color,
                            QTextCharFormat::SingleUnderline,
                            false);
            event->accept();
            return;
        }
    }
    if (this->__cursor_changed) {
        QApplication::restoreOverrideCursor();
        this->__cursor_changed = false;
        this->clear_extra_selections("ctrl_click");
    }
    TextEditBaseWidget::mouseMoveEvent(event);
}


void CodeEditor::setPlainText(const QString &text)
{
	__super::setPlainText(text);
	emit this->new_text_set();
}

void CodeEditor::leaveEvent(QEvent *event)
{
    if (this->__cursor_changed) {
        QApplication::restoreOverrideCursor();
        this->__cursor_changed = false;
        this->clear_extra_selections("ctrl_click");
    }
    TextEditBaseWidget::leaveEvent(event);
}


//@Slot()
void CodeEditor::go_to_definition_from_cursor(QTextCursor cursor)
{
    if (!this->go_to_definition_enabled)
        return;
    if (cursor.isNull())
        cursor = this->textCursor();
    if (this->in_comment_or_string())
        return;
    int position = cursor.position();
    QString text = cursor.selectedText();
    if (text.size() == 0) {
        cursor.select(QTextCursor::WordUnderCursor);
        text = cursor.selectedText();
    }
    if (!text.isEmpty())
        emit this->go_to_definition(position);
}

void CodeEditor::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton &&
            (event->modifiers() & Qt::ControlModifier)) {
        TextEditBaseWidget::mousePressEvent(event);
        QTextCursor cursor = this->cursorForPosition(event->pos());
        this->go_to_definition_from_cursor(cursor);
    }
    else
        TextEditBaseWidget::mousePressEvent(event);
}

void CodeEditor::contextMenuEvent(QContextMenuEvent *event)
{
    bool nonempty_selection = this->has_selected_text();
    this->copy_action->setEnabled(nonempty_selection);
    this->cut_action->setEnabled(nonempty_selection);
    // TODO
    this->clear_all_output_action->setVisible(this->is_json() && false);
    this->ipynb_convert_action->setVisible(this->is_json() && false);
    this->run_cell_action->setVisible(this->is_python());
    this->run_cell_and_advance_action->setVisible(this->is_python());
    this->re_run_last_cell_action->setVisible(this->is_python());
    this->gotodef_action->setVisible(this->go_to_definition_enabled
                                     && this->is_python_like());

    QTextCursor cursor = this->textCursor();
    QString text = cursor.selectedText();
    if (text.size() == 0) {
        cursor.select(QTextCursor::WordUnderCursor);
        text = cursor.selectedText();
    }
    this->undo_action->setEnabled(this->document()->isUndoAvailable());
    this->redo_action->setEnabled(this->document()->isRedoAvailable());
    QMenu* menu = this->menu;
    if (this->isReadOnly())
        menu = this->readonly_menu;
    menu->popup(event->globalPos());
    event->accept();
}

void CodeEditor::dragEnterEvent(QDragEnterEvent *event)
{
    if (!mimedata2url(event->mimeData()).isEmpty())
        event->ignore();
    else
        TextEditBaseWidget::dragEnterEvent(event);
}

void CodeEditor::dropEvent(QDropEvent *event)
{
    if (!mimedata2url(event->mimeData()).isEmpty())
        event->ignore();
    else
        TextEditBaseWidget::dropEvent(event);
}


//------ Paint event
void CodeEditor::paintEvent(QPaintEvent *event)
{
    update_visible_blocks(event);
    TextEditBaseWidget::paintEvent(event);
    emit this->painted(event);
}

void CodeEditor::update_visible_blocks(QPaintEvent *event)
{
    Q_UNUSED(event);
    this->__visible_blocks.clear();
    QTextBlock block = this->firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = static_cast<int>(this->blockBoundingGeometry(block).translated(
                               this->contentOffset()).top());
    int bottom = top + static_cast<int>(this->blockBoundingRect(block).height());
    int ebottom_bottom = this->height();

    while (block.isValid()) {
        bool visible  = bottom <= ebottom_bottom;
        if (visible == false)
            break;
        if (block.isVisible())
            this->__visible_blocks.append(IntIntTextblock(top, blockNumber+1, block));
        block = block.next();
        top = bottom;
        bottom = top + static_cast<int>(this->blockBoundingRect(block).height());
        blockNumber = block.blockNumber();
    }
}

void CodeEditor::_draw_editor_cell_divider()
{
    if (supported_cell_language) {
        QColor cell_line_color = this->comment_color;
        QPainter painter(this->viewport());
        QPen pen = painter.pen();
        pen.setStyle(Qt::SolidLine);
        pen.setBrush(cell_line_color);
        painter.setPen(pen);

        foreach (auto pair, this->__visible_blocks) {
            int top = pair.top;
            QTextBlock block = pair.block;
            // TODO源码是if self.is_cell_separator(block):
            if (this->is_cell_separator(QTextCursor(),block))
                painter.drawLine(4,top,this->width(),top);
        }
    }
}

QList<IntIntTextblock> CodeEditor::visible_blocks()
{
    return __visible_blocks;
}

bool CodeEditor::is_editor()
{
    return true;
}


Printer::Printer(QPrinter::PrinterMode mode, const QFont& header_font)
    : QPrinter (mode)
{
    this->setColorMode(QPrinter::Color);
    this->setPageOrder(QPrinter::FirstPageFirst);
    this->date = QDateTime::currentDateTime().toString();
    if (header_font != QFont())
        this->header_font = header_font;
}


TestWidget::TestWidget(QWidget *parent)
    : QSplitter (parent)
{
    editor = new CodeEditor(this);

    QHash<QString, QVariant> kwargs;
    kwargs["linenumbers"] = true;
    kwargs["markers"] = true;
    kwargs["tab_mode"] = false;
    QFont font("Courier New", 10);
    kwargs["font"] = QVariant::fromValue(font);
    kwargs["show_blanks"] = true;

    QHash<QString,ColorBoolBool> color_scheme;
    color_scheme["background"] = ColorBoolBool("#3f3f3f");
    color_scheme["currentline"] = ColorBoolBool("#333333");
    color_scheme["currentcell"] = ColorBoolBool("#2c2c2c");
    color_scheme["occurrence"] = ColorBoolBool("#7a738f");
    color_scheme["ctrlclick"] = ColorBoolBool("#0000ff");
    color_scheme["sideareas"] = ColorBoolBool("#3f3f3f");
    color_scheme["matched_p"] = ColorBoolBool("#688060");
    color_scheme["unmatched_p"] = ColorBoolBool("#bd6e76");
    color_scheme["normal"] = ColorBoolBool("#dcdccc");
    color_scheme["keyword"] = ColorBoolBool("#dfaf8f", true);
    color_scheme["builtin"] = ColorBoolBool("#efef8f");
    color_scheme["definition"] = ColorBoolBool("#efef8f");
    color_scheme["comment"] = ColorBoolBool("#7f9f7f", false, true);
    color_scheme["string"] = ColorBoolBool("#cc9393");
    color_scheme["number"] = ColorBoolBool("#8cd0d3");
    color_scheme["instance"] = ColorBoolBool("#dcdccc", false, true);
    QHash<QString, QVariant> dict;
    for (auto it=color_scheme.begin();it!=color_scheme.end();it++) {
        QString key = it.key();
        QVariant val = QVariant::fromValue(it.value());
        dict[key] = val;
    }
    kwargs["color_scheme"] = dict;

    editor->setup_editor(kwargs);

    addWidget(editor);
    classtree = new OutlineExplorerWidget(this);
    addWidget(classtree);
    connect(classtree,&OutlineExplorerWidget::edit_goto,
            [this](const QString&,int line,const QString& word){ editor->go_to_line(line,word); });
    setStretchFactor(0, 4);
    setStretchFactor(1, 1);
    setWindowIcon(ima::icon("spyder"));
}

void TestWidget::load(const QString &filename)
{
    editor->set_text_from_file(filename);
    QFileInfo info(filename);
    setWindowTitle(QString("%1 - %2 (%3)").arg("Editor").
                   arg(info.fileName()).arg(info.absolutePath()));
    classtree->set_current_editor(editor,filename,false,false);
}

static void test()
{
    TestWidget* win = new TestWidget(nullptr);
    win->show();
    win->load("F:/MyPython/spyder/widgets/sourcecode/codeeditor.py");
    win->resize(900, 700);
}
