#include "arrayeditor.h"
#include <QDebug>

const int LARGE_SIZE = 5e5;
const int LARGE_NROWS = 1e5;
const int LARGE_COLS = 60;

static QRect get_idx_rect(const QModelIndexList& index_list)
{
    int min_row,max_row,min_col,max_col;
    min_row=max_row=index_list[0].row();
    min_col=max_col=index_list[0].column();

    for (int i = 1; i < index_list.size(); ++i) {
        min_row = qMin(min_row, index_list[i].row());
        max_row = qMax(max_row, index_list[i].row());

        min_col = qMin(min_col, index_list[i].column());
        max_col = qMax(max_col, index_list[i].column());
    }
    return QRect(min_row,max_row,min_col,max_col);
}

int ArrayModel::ROWS_TO_LOAD = 500;
int ArrayModel::COLS_TO_LOAD = 40;

ArrayModel::ArrayModel(const QVector<QVector<QVariant>>& data,
           const QString& format,
           const QStringList& xlabels,
           const QStringList& ylabels,
           bool readonly,
           QWidget* parent)
    : QAbstractTableModel (parent)
{
    this->dialog = parent;
    this->xlabels = xlabels;
    this->ylabels = ylabels;
    this->readonly = readonly;
    switch (data[0][0].type()) {
    case QVariant::Bool:
        test_array = false;
        break;
    case QVariant::Int:
        test_array = 0;
        break;
    case QVariant::Double:
        test_array = 0.0;
        break;
    default:
        break;
    }

    double huerange[2] = {0.66, 0.99};
    sat = 0.7;
    val = 1.0;
    alp = 0.6;

    _data = data;
    _format = format;

    total_rows = _data.size();
    total_cols = _data[0].size();
    int size = total_rows * total_cols;

    vmin = INT_MAX;
    vmax = INT_MIN;
    if (data[0][0].type() == QVariant::Bool ||
        data[0][0].type() == QVariant::Int ||
        data[0][0].type() == QVariant::Double) {
        for (int i = 0; i < total_rows; ++i) {
            for (int j = 0; j < total_cols; ++j) {
                vmin = qMin(vmin, data[i][j].toDouble());
                vmax = qMax(vmax, data[i][j].toDouble());
            }
        }
        if (vmax - vmin < 1e-5)
            vmin -= 1;
        hue0 = huerange[0];
        dhue = huerange[1] - huerange[0];
        bgcolor_enabled = true;
    }
    else {
        hue0 = -1.0;
        dhue = -1.0;
        bgcolor_enabled = false;
    }

    if (size > LARGE_SIZE) {
        rows_loaded = ROWS_TO_LOAD;
        cols_loaded = COLS_TO_LOAD;
    }
    else {
        if (total_rows > LARGE_NROWS)
            rows_loaded = ROWS_TO_LOAD;
        else
            rows_loaded = total_rows;
        if (total_cols > LARGE_COLS)
            cols_loaded = COLS_TO_LOAD;
        else
            cols_loaded = total_cols;
    }
}

QString ArrayModel::get_format() const
{
    return _format;
}

QVector<QVector<QVariant>> ArrayModel::get_data() const
{
    return _data;
}

void ArrayModel::set_format(const QString &format)
{
    _format = format;
    reset();
}

int ArrayModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    if (total_cols <= cols_loaded)
        return total_cols;
    else
        return cols_loaded;
}

int ArrayModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    if (total_rows <= rows_loaded)
        return total_rows;
    else
        return rows_loaded;
}

bool ArrayModel::can_fetch_more(bool rows, bool columns) const
{
    if (rows) {
        if (total_rows > rows_loaded)
            return true;
        else
            return false;
    }
    if (columns) {
        if (total_cols > cols_loaded)
            return true;
        else
            return false;
    }
    return false;
}

void ArrayModel::fetch_more(bool rows, bool columns)
{
    if (can_fetch_more(rows)) {
        int reminder = total_rows - rows_loaded;
        int items_to_fetch = qMin(reminder, ROWS_TO_LOAD);
        beginInsertRows(QModelIndex(), rows_loaded,
                        rows_loaded + items_to_fetch - 1);
        rows_loaded += items_to_fetch;
        endInsertRows();
    }
    if (can_fetch_more(false, columns)) {
        int reminder = total_cols - cols_loaded;
        int items_to_fetch = qMin(reminder, COLS_TO_LOAD);
        beginInsertRows(QModelIndex(), cols_loaded,
                        cols_loaded + items_to_fetch - 1);
        cols_loaded += items_to_fetch;
        endInsertRows();
    }
}

void ArrayModel::bgcolor(int state)
{
    bgcolor_enabled = state > 0;
    reset();
}

QVariant ArrayModel::get_value(const QModelIndex &index) const
{
    int i = index.row();
    int j = index.column();
    QVariant value = _data[i][j];
    return changes.value(qMakePair(i, j), value);
}

QVariant ArrayModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    QVariant value = get_value(index);
    if (role == Qt::DisplayRole) {
        switch (value.type()) {
        case QVariant::Bool:
            return QString::asprintf(_format.toLatin1(), value.toBool());
        case QVariant::Int:
            return QString::asprintf(_format.toLatin1(), value.toInt());
        case QVariant::Double:
            return QString::asprintf(_format.toLatin1(), value.toDouble());
        default:
            break;
        }
        return value.toString();
    }
    else if (role == Qt::TextAlignmentRole)
        return int(Qt::AlignCenter|Qt::AlignVCenter);
    else if (role == Qt::BackgroundColorRole && bgcolor_enabled) {
        if (value.type() == QVariant::Bool || // True和False颜色不同
            value.type() == QVariant::Int ||
            value.type() == QVariant::Double) {
            double hue = (hue0 +
                          dhue * (double(vmax) - value.toDouble())
                          / (double(vmax) - vmin));
            hue = qAbs(hue);
            QColor color = QColor::fromHsvF(hue, sat, val, alp);
            return color;
        }
    }
    else if (role == Qt::FontRole) {
        QFont font("Consolas", 9, 50);
        font.setItalic(false);
        return font;
    }
    return QVariant();
}

bool ArrayModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_UNUSED(role);
    if (!index.isValid() || readonly)
        return false;
    int i = index.row();
    int j = index.column();
    QVariant val;
    if (_data[0][0].type() == QVariant::Bool) {
        val = value.toBool();
        if (value.toBool() > vmax)
            vmax = value.toBool();
        if (value.toBool() < vmin)
            vmin = value.toBool();
    }
    else if (_data[0][0].type() == QVariant::Int) {
        val = value.toInt();
        if (value.toInt() > vmax)
            vmax = value.toInt();
        if (value.toInt() < vmin)
            vmin = value.toInt();
    }
    else if (_data[0][0].type() == QVariant::Double) {
        val = value.toDouble();
        if (value.toDouble() > vmax)
            vmax = value.toDouble();
        if (value.toDouble() < vmin)
            vmin = value.toDouble();
    }
    else {
        QMessageBox::critical(dialog, "Error",
                              QString("Value error: %1").arg(value.typeName()));
        return false;
    }
    test_array = val;

    // Add change to self.changes
    changes[qMakePair(i,j)] = val;
    emit dataChanged(index, index);
    return true;
}

Qt::ItemFlags ArrayModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;
    return Qt::ItemFlags(QAbstractTableModel::flags(index) |
                         Qt::ItemIsEditable);
}

QVariant ArrayModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();
    QStringList labels;
    if (orientation == Qt::Horizontal)
        labels = xlabels;
    else
        labels = ylabels;
    if (labels.empty())
        return section;
    else
        return labels[section];
}

void ArrayModel::reset()
{
    beginResetModel();
    endResetModel();
}


/********** ArrayDelegate **********/
ArrayDelegate::ArrayDelegate(const QString& dtype,QWidget* parnet)
    : QItemDelegate (parnet)
{
    this->dtype = dtype;
}

QWidget* ArrayDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);
    const ArrayModel* mdl = qobject_cast<const ArrayModel*>(index.model());
    ArrayModel* model = const_cast<ArrayModel*>(mdl);
    QVariant value = model->get_value(index);
    if (model->_data[0][0].type() == QVariant::Bool) {
        bool val = !value.toBool();
        model->setData(index, val);
        return nullptr;
    }
    QLineEdit* editor = new QLineEdit(parent);
    QFont font("Consolas", 9, 50);
    font.setItalic(false);
    editor->setFont(font);
    editor->setAlignment(Qt::AlignCenter);
    if (dtype == "int" || dtype == "double") {
        QDoubleValidator* validator = new QDoubleValidator(editor);
        validator->setLocale(QLocale("C"));
        editor->setValidator(validator);
    }
    connect(editor,SIGNAL(returnPressed()),this,SLOT(commitAndCloseEditor()));
    return editor;
}

void ArrayDelegate::commitAndCloseEditor()
{
    QWidget* editor = qobject_cast<QWidget*>(sender());
    emit commitData(editor);
    emit closeEditor(editor, QAbstractItemDelegate::NoHint);
}

void ArrayDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QVariant data = index.model()->data(index, Qt::DisplayRole);
    QLineEdit* lineedit = qobject_cast<QLineEdit*>(editor);
    lineedit->setText(data.toString());
}


/********** ArrayView **********/
ArrayView::ArrayView(QWidget* parent,QAbstractItemModel *model,
          const QString& dtype,const QPair<int,int>& shape)
    : QTableView (parent)
{
    setModel(model);
    setItemDelegate(new ArrayDelegate(dtype, this));
    int total_width = 0;
    for (int k=0; k<shape.second;k++)
        total_width += columnWidth(k);
    viewport()->resize(qMin(total_width, 1024), height());
    this->shape = shape;
    menu = setup_menu();

    QString keystr = "Ctrl+C";
    QShortcut* qsc = new QShortcut(QKeySequence(keystr), this, SLOT(copy()));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    connect(horizontalScrollBar(), &QScrollBar::valueChanged,
            [=](int val){this->load_more_data(val, false, true);});
    connect(verticalScrollBar(), &QScrollBar::valueChanged,
            [=](int val){this->load_more_data(val, true);});
}

void ArrayView::load_more_data(int value, bool rows, bool columns)
{
    QItemSelection old_selection = selectionModel()->selection();
    int old_rows_loaded = -1;
    int old_cols_loaded = -1;

    ArrayModel* model = qobject_cast<ArrayModel*>(this->model());
    if (rows && value == verticalScrollBar()->maximum()) {
        old_rows_loaded = model->rows_loaded;
        model->fetch_more(rows);
    }

    if (columns && value == horizontalScrollBar()->maximum()) {
        old_cols_loaded = model->cols_loaded;
        model->fetch_more(false,columns);
    }

    if (old_rows_loaded != -1 || old_cols_loaded != -1) {
        QItemSelection new_selection;
        foreach (auto part, old_selection) {
            int top = part.top();
            int bottom = part.bottom();
            if (old_rows_loaded != -1 &&
                    top == 0 && bottom == (old_rows_loaded-1))
                bottom = model->rows_loaded-1;
            int left = part.left();
            int right = part.right();
            if (old_cols_loaded != -1 &&
                    left == 0 && right == (old_cols_loaded-1))
                right = model->cols_loaded-1;
            QModelIndex top_left = model->index(top, left);
            QModelIndex bottom_right = model->index(bottom, right);
            QItemSelectionRange part2(top_left, bottom_right);
            new_selection.append(part2);
        }
        selectionModel()->select(new_selection, selectionModel()->ClearAndSelect);
    }
}

void ArrayView::resize_to_contents()
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    resizeColumnsToContents();
    ArrayModel* model = qobject_cast<ArrayModel*>(this->model());
    model->fetch_more(false, true);
    resizeColumnsToContents();
    QApplication::restoreOverrideCursor();
}

QMenu* ArrayView::setup_menu()
{
    copy_action = new QAction("Copy", this);
    connect(copy_action,SIGNAL(triggered(bool)),
            this,SLOT(copy()));
    copy_action->setIcon(ima::icon("editcopy"));
    copy_action->setShortcut(QKeySequence(QKeySequence::Copy));
    copy_action->setShortcutContext(Qt::WidgetShortcut);
    menu = new QMenu();
    QList<QAction*> actions;
    actions << copy_action;
    add_actions(menu, actions);
    return menu;
}

void ArrayView::contextMenuEvent(QContextMenuEvent *event)
{
    menu->popup(event->globalPos());
    event->accept();
}

void ArrayView::keyPressEvent(QKeyEvent *event)
{
    if (event->matches(QKeySequence::Copy)) // 记得这里，别的地方也可以这样写
        copy();
    else
        QTableView::keyReleaseEvent(event);
}

QString ArrayView::_sel_to_text(const QModelIndexList &cell_range)
{
    if (cell_range.empty())
        return QString();
    QRect tmp = get_idx_rect(cell_range);
    int row_min = tmp.x();
    int row_max = tmp.y();
    int col_min = tmp.width();
    int col_max = tmp.height();
    ArrayModel* model = qobject_cast<ArrayModel*>(this->model());
    if (col_min == 0 && col_max == (model->cols_loaded-1))
        col_max = model->total_cols-1;
    if (row_min == 0 && row_max == (model->rows_loaded-1))
        row_max = model->total_rows-1;

    QVector<QVector<QVariant>> _data = model->get_data();
    QString contents;
    if (_data[0][0].type() == QVariant::Bool) {
        for (int i = row_min; i < row_max+1; ++i) {
            for (int j = col_min; j < col_max; ++j) {
                contents += QString::asprintf(model->get_format().toLatin1(),
                                              _data[i][j].toBool());
                contents += "\t";
            }
            contents += QString::asprintf(model->get_format().toLatin1(),
                                          _data[i][col_max].toBool());
            contents += "\n";
        }
    }
    else if (_data[0][0].type() == QVariant::Int) {
        for (int i = row_min; i < row_max+1; ++i) {
            for (int j = col_min; j < col_max; ++j) {
                contents += QString::asprintf(model->get_format().toLatin1(),
                                              _data[i][j].toInt());
                contents += "\t";
            }
            contents += QString::asprintf(model->get_format().toLatin1(),
                                          _data[i][col_max].toInt());
            contents += "\n";
        }
    }
    else if (_data[0][0].type() == QVariant::Double) {
        for (int i = row_min; i < row_max+1; ++i) {
            for (int j = col_min; j < col_max; ++j) {
                contents += QString::asprintf(model->get_format().toLatin1(),
                                              _data[i][j].toDouble());
                contents += "\t";
            }
            contents += QString::asprintf(model->get_format().toLatin1(),
                                          _data[i][col_max].toDouble());
            contents += "\n";
        }
    }
    else {
        QMessageBox::warning(this, "Warning",
                             "It was not possible to copy values for this array");
        return QString();

    }
    return contents;
}

void ArrayView::copy()
{
    QString cliptext = _sel_to_text(selectedIndexes());
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(cliptext);
}


/********** ArrayEditorWidget **********/
ArrayEditorWidget::ArrayEditorWidget(QWidget* parent,
                  const QVector<QVector<QVariant>>& data,
                  bool readonly,
                  const QStringList& xlabels,
                  const QStringList& ylabels)
    : QWidget (parent)
{
    this->data = data;

    QString dtype;
    QString format = "%s";
    switch (data[0][0].type()) {
    case QVariant::Bool:
        dtype = "bool";
        format = "%r";
        break;
    case QVariant::Int:
        dtype = "int";
        format = "%d";
        break;
    case QVariant::Double:
        dtype = "double";
        format = "%.6g";
        break;
    default:
        break;
    }
    model = new ArrayModel(data, format, xlabels, ylabels, readonly, this);
    QPair<int,int> shape = qMakePair(data.size(), data[0].size());
    view = new ArrayView(this, model, dtype, shape);

    QHBoxLayout* btn_layout = new QHBoxLayout;
    btn_layout->setAlignment(Qt::AlignLeft);
    QPushButton* btn = new QPushButton("Format");
    if (dtype == "double")
        btn->setEnabled(true);
    else
        btn->setEnabled(false);
    btn_layout->addWidget(btn);
    connect(btn,SIGNAL(clicked(bool)),this,SLOT(change_format()));
    btn = new QPushButton("Resize");
    btn_layout->addWidget(btn);
    connect(btn,SIGNAL(clicked(bool)),view,SLOT(resize_to_contents()));
    QCheckBox* bgcolor = new QCheckBox("Background color");
    bgcolor->setChecked(model->bgcolor_enabled);
    bgcolor->setEnabled(model->bgcolor_enabled);
    connect(bgcolor,SIGNAL(stateChanged(int)),model,SLOT(bgcolor(int)));
    btn_layout->addWidget(bgcolor);

    QVBoxLayout* layout = new QVBoxLayout;
    layout->addWidget(view);
    layout->addLayout(btn_layout);
    setLayout(layout);
}

void ArrayEditorWidget::accept_changes()
{
    for (auto it=model->changes.begin();it!=model->changes.end();it++) {
        int i = it.key().first;
        int j = it.key().second;
        QVariant value = it.value();
        this->data[i][j] = value;
    }
    //old_data_shape is not None说明构造函数传入的data是一维或0维的，不是二维的
}

void ArrayEditorWidget::reject_changes()
{}

void ArrayEditorWidget::change_format()
{
    bool valid;
    QString format = QInputDialog::getText(this, "Format",
                                           "Float formatting",
                                           QLineEdit::Normal,
                                           model->get_format(),
                                           &valid);
    if (valid) {
        try {
            // 这里是用1.1检测format合不合法
            QString::asprintf(format.toLatin1(), 1.1);
        } catch (...) {
            QMessageBox::critical(this, "Error",
                                  QString("Format (%1) is incorrect").arg(format));
            return;
        }
        model->set_format(format);
    }
}


/********** ArrayEditor **********/
ArrayEditor::ArrayEditor(QWidget* parent)
    : QDialog (parent)
{
    setAttribute(Qt::WA_DeleteOnClose);
}

bool ArrayEditor::setup_and_check(const QVector<QVector<QVariant> > &data,
                                  QString title,
                                  bool readonly,
                                  const QStringList &xlabels,
                                  const QStringList &ylabels)
{
    this->data = data;
    bool is_record_array = false;
    // dt = np.dtype([('name', np.str_, 16), ('grades', np.float64, (2,))])
    // dt.names：('name', 'grades')
    bool is_masked_array = false;

    if (!xlabels.empty() && xlabels.size() != data[0].size()) {
        error("The 'xlabels' argument length do no match array "
              "column number");
        return false;
    }
    if (!ylabels.empty() && ylabels.size() != data.size()) {
        error("The 'ylabels' argument length do no match array "
              "row number");
        return false;
    }
    if (is_record_array == false) {
        QString dtn;
        switch (data[0][0].type()) {
        case QVariant::Bool:
            dtn = "bool";
            break;
        case QVariant::Int:
            dtn = "int";
            break;
        case QVariant::Double:
            dtn = "double";
            break;
        default:
            break;
        }
        if (dtn.isEmpty()) {
            QString arr = QString("%1 arrays").arg(data[0][0].typeName());
            error(QString("%1 are currently not supported").arg(arr));
            return false;
        }
    }

    layout = new QGridLayout;
    setLayout(layout);
    setWindowIcon(ima::icon("arredit"));
    if (!title.isEmpty())
        title = title + " - NumPy array";
    else
        title = "Array editor";
    if (readonly)
        title += " (read only)";
    setWindowTitle(title);
    resize(600, 500);

    stack = new QStackedWidget(this);
    if (is_record_array)
        ;
    else if (is_masked_array)
        ;
    else
        stack->addWidget(new ArrayEditorWidget(this, data, readonly,
                                               xlabels, ylabels));
    arraywidget = qobject_cast<ArrayEditorWidget*>(stack->currentWidget());
    if (arraywidget)
        connect(arraywidget->model, SIGNAL(dataChanged(QModelIndex, QModelIndex)),
                this,SLOT(save_and_close_enable(QModelIndex, QModelIndex)));
    connect(stack, SIGNAL(currentChanged(int)),
            this, SLOT(current_widget_changed(int)));
    layout->addWidget(stack ,1, 0);

    QHBoxLayout* btn_layout = new QHBoxLayout;
    btn_layout->addStretch();

    if (!readonly) {
        btn_save_and_close = new QPushButton("Save and Close");
        btn_save_and_close->setDisabled(true);
        connect(btn_save_and_close, SIGNAL(clicked(bool)),
                this, SLOT(accept()));
        btn_layout->addWidget(btn_save_and_close);
    }
    btn_close = new QPushButton("Close");
    btn_close->setAutoDefault(true);
    btn_close->setDefault(true);
    connect(btn_close, SIGNAL(clicked(bool)),this,SLOT(reject()));
    btn_layout->addWidget(btn_close);
    layout->addLayout(btn_layout, 2, 0);

    setMinimumSize(400, 300);
    setWindowFlags(Qt::Window);
    return true;
}

void ArrayEditor::save_and_close_enable(const QModelIndex& left_top,
                                        const QModelIndex& bottom_right)
{
    Q_UNUSED(left_top);
    Q_UNUSED(bottom_right);
    if (btn_save_and_close) {
        btn_save_and_close->setEnabled(true);
        btn_save_and_close->setAutoDefault(true);
        btn_save_and_close->setDefault(true);
    }
}

void ArrayEditor::current_widget_changed(int index)
{
    arraywidget = qobject_cast<ArrayEditorWidget*>(stack->widget(index));
    connect(arraywidget->model, SIGNAL(dataChanged(QModelIndex, QModelIndex)),
            this,SLOT(save_and_close_enable(QModelIndex, QModelIndex)));
}

void ArrayEditor::accept()
{
    for (int index = 0; index < stack->count(); ++index) {
        qobject_cast<ArrayEditorWidget*>(stack->widget(index))->accept_changes();
    }
    QDialog::accept();
}

QVector<QVector<QVariant>> ArrayEditor::get_value() const
{
    return data;
}

void ArrayEditor::error(const QString &message)
{
    QMessageBox::critical(this, "Array editor", message);
    setAttribute(Qt::WA_DeleteOnClose);
    reject();
}

void ArrayEditor::reject()
{
    if (arraywidget) {
        for (int index = 0; index < stack->count(); ++index) {
            qobject_cast<ArrayEditorWidget*>(stack->widget(index))->reject_changes();
        }
    }
    QDialog::reject();
}
