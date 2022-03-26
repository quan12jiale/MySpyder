#include "tabs.h"

EditTabNamePopup::EditTabNamePopup(QWidget* parent,const QString& split_char,int split_index)
    : QLineEdit (nullptr)
{
    if (parent != nullptr)
        this->main = qobject_cast<TabBar*>(parent);
    else
        this->main = qobject_cast<TabBar*>(this->parent());
    this->split_char = split_char;
    this->split_index = split_index;

    tab_index = -1;

    connect(this,SIGNAL(editingFinished()),this,SLOT(edit_finished()));

    installEventFilter(this);
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint |
                   Qt::NoDropShadowWindowHint);
    setFrame(false);

    setTextMargins(9, 0, 0, 0);
}

bool EditTabNamePopup::eventFilter(QObject *widget, QEvent *event)
{
    QMouseEvent* mouse_event = dynamic_cast<QMouseEvent*>(event);
    QKeyEvent* key_event = dynamic_cast<QKeyEvent*>(event);
    if ((event->type() == QEvent::MouseButtonPress &&
         !geometry().contains(mouse_event->globalPos())) ||
            (event->type() == QEvent::KeyPress &&
             key_event->key() == Qt::Key_Escape)) {
        this->hide();
        this->clearFocus();//源码是setFocus(False)，可是没有setFocus(bool)函数
        return true;
    }
    return QLineEdit::eventFilter(widget, event);
}

void EditTabNamePopup::edit_tab(int index)
{
    setFocus();//源码是setFocus(True)，可是没有setFocus(bool)函数

    this->tab_index = index;

    QRect rect = main->tabRect(index);
    rect.adjust(1, 1, -2, -1);

    setFixedSize(rect.size());

	this->move(main->mapToGlobal(rect.topLeft()));

    QString text = main->tabText(index);
    text.replace("&", "");
    if (!split_char.isEmpty())
        text = text.split(split_char)[split_index];

    setText(text);
    selectAll();

    if (!isVisible())
        show();
}

void EditTabNamePopup::edit_finished()
{
    hide();
    if (tab_index >= 0) {
        QString tab_text = text();
        main->setTabText(tab_index, tab_text);
        emit main->sig_change_name(tab_text);
    }
}


/********** TabBar **********/
TabBar::TabBar(QWidget* parent,QWidget* ancestor,bool rename_tabs,
       const QString& split_char,int split_index)
    : QTabBar (parent)
{
    this->ancestor = ancestor;

#if defined (Q_OS_MAC)
    setObjectName("plugin-tab");
#endif

    __drag_start_pos = QPoint();
    setAcceptDrops(true);
    setUsesScrollButtons(true);
    setMovable(true);

    this->rename_tabs = rename_tabs;
    if (this->rename_tabs)
        tab_name_editor = new EditTabNamePopup(this, split_char, split_index);
    else
        tab_name_editor = nullptr;
}

void TabBar::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        __drag_start_pos = event->pos();
    QTabBar::mousePressEvent(event);
}

void TabBar::dragEnterEvent(QDragEnterEvent *event)
{
    const QMimeData* mimeData = event->mimeData();
    QStringList formats = mimeData->formats();

    if (formats.contains("parent-id")) {
        size_t res1 = mimeData->data("parent-id").toULongLong();
        size_t res2 = reinterpret_cast<size_t>(this->ancestor);
        if (res1 == res2)
            event->acceptProposedAction();
    }
    QTabBar::dragEnterEvent(event);
}

void TabBar::dropEvent(QDropEvent *event)
{
    const QMimeData* mimeData = event->mimeData();
    int index_from = mimeData->data("source-index").toInt();
    int index_to = this->tabAt(event->pos());
    if (index_to == -1)
        index_to = this->count();

    size_t res1 = mimeData->data("tabbar-id").toULongLong();
    size_t res2 = reinterpret_cast<size_t>(this);
    if (res1 != res2) {
        QString tabwidget_from = mimeData->data("tabwidget-id");
        emit sig_move_tab(tabwidget_from, index_from, index_to);
        event->acceptProposedAction();
    }
    else if (index_from != index_to)
        event->acceptProposedAction();
    QTabBar::dropEvent(event);
}

void TabBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    // 双击ipythonconsole的tabbar会出现tab_name_editor
    if (rename_tabs && event->buttons() == Qt::MouseButtons(Qt::LeftButton)) {
        int index = tabAt(event->pos());
        if (index >= 0)
            tab_name_editor->edit_tab(index);
    }
    else
        QTabBar::mouseDoubleClickEvent(event);
}


/********** BaseTabs **********/
BaseTabs::BaseTabs(QWidget* parent,
         const QList<QAction*>& actions,
         QMenu* menu,
         QHash<Qt::Corner, QList<QPair<int, QToolButton*>>> corner_widgets,
         bool menu_use_tooltips)
    : QTabWidget (parent)
{
    this->setUsesScrollButtons(true);

#if defined (Q_OS_MAC)
    setObjectName("plugin-tab");
#endif

    this->corner_widgets.clear();
    this->menu_use_tooltips = menu_use_tooltips;

    if (menu == nullptr) {
        this->menu = new QMenu(this);
        if (!actions.isEmpty())
            add_actions(this->menu, actions);
    }
    else
        this->menu = menu;

    if (!corner_widgets.contains(Qt::TopLeftCorner))
        corner_widgets[Qt::TopLeftCorner] = QList<QPair<int, QToolButton*>>();
    if (!corner_widgets.contains(Qt::TopRightCorner))
        corner_widgets[Qt::TopRightCorner] = QList<QPair<int, QToolButton*>>();
    browse_button = new QToolButton(this);
    browse_button->setIcon(ima::icon("browse_tab"));
    browse_button->setToolTip("Browse tabs");

    browse_tabs_menu = new QMenu(this);
    browse_button->setMenu(browse_tabs_menu);
    browse_button->setPopupMode(QToolButton::InstantPopup);
    connect(browse_tabs_menu,SIGNAL(aboutToShow()),this,SLOT(update_browse_tabs_menu()));
    corner_widgets[Qt::TopLeftCorner].append(qMakePair(-1, browse_button));

    set_corner_widgets(corner_widgets);
}

void BaseTabs::update_browse_tabs_menu()
{
    browse_tabs_menu->clear();
    QStringList names;
    QStringList dirnames;
    for (int index = 0; index < this->count(); ++index) {
        QString text;
        if (menu_use_tooltips)
            text = this->tabToolTip(index);
        else
            text = this->tabText(index);
        names.append(text);
        QFileInfo info(text);
        if (info.isFile()) {
            dirnames.append(info.absolutePath());
        }
    }
    int offset = 0;
    //这里设为0,因为text[None:] <==> text[0:]

    if (names.size() == dirnames.size()) {
        QString common = misc::get_common_path(dirnames);
        if (common.isEmpty())
            offset = 0;
        else {
            offset = common.size()+1;
            if (offset <= 3)
                offset = 0;
        }
    }

    for (int index = 0; index < names.size(); ++index) {
        QString text = names[index];
        QAction* tab_action = new QAction(text.mid(offset), this);
        tab_action->setIcon(tabIcon(index));
        connect(tab_action, &QAction::toggled,
                [=](){this->setCurrentIndex(index);});
        tab_action->setCheckable(true);
        tab_action->setToolTip(tabToolTip(index));
        tab_action->setChecked(index == currentIndex());
        browse_tabs_menu->addAction(tab_action);
    }
}

void BaseTabs::set_corner_widgets(const QHash<Qt::Corner, QList<QPair<int, QToolButton*>> > &corner_widgets)
{
    foreach (Qt::Corner key, corner_widgets.keys()) {
        Q_ASSERT(key == Qt::TopLeftCorner || key == Qt::TopRightCorner);
    }
    for (auto it=corner_widgets.begin();it!=corner_widgets.end();it++)
        this->corner_widgets[it.key()] = it.value();
    for (auto it=this->corner_widgets.begin();it!=this->corner_widgets.end();it++) {
        Qt::Corner corner = it.key();
        QList<QPair<int, QToolButton*>> widgets = it.value();
        QWidget* cwidget = new QWidget;
        cwidget->hide();
        QWidget* prev_widget = this->cornerWidget(corner);
        if (prev_widget)
            prev_widget->close();
        this->setCornerWidget(cwidget, corner);
        QHBoxLayout* clayout = new QHBoxLayout;
        clayout->setContentsMargins(0, 0, 0, 0);
        foreach (auto widget, widgets) {
            if (widget.second == nullptr)
                clayout->addSpacing(widget.first);
            else
                clayout->addWidget(widget.second);
        }
        cwidget->setLayout(clayout);
        cwidget->show();
    }
}

void BaseTabs::add_corner_widgets(const QList<QPair<int, QToolButton*>> &widgets, Qt::Corner corner)
{
   QList<QPair<int, QToolButton*>> list =
            this->corner_widgets.value(corner, QList<QPair<int, QToolButton*>>());
   list.append(widgets);
   QHash<Qt::Corner, QList<QPair<int, QToolButton*>> > corner_widgets;
   corner_widgets[corner] = list;
   set_corner_widgets(corner_widgets);
}

void BaseTabs::contextMenuEvent(QContextMenuEvent *event)
{
    this->setCurrentIndex(this->tabBar()->tabAt(event->pos()));
    if (this->menu)
        this->menu->popup(event->globalPos());
}

void BaseTabs::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MidButton) {
        int index = this->tabBar()->tabAt(event->pos());
        if (index >= 0) {
            emit sig_close_tab(index);
            event->accept();
            return;
        }
    }
    QTabWidget::mousePressEvent(event);
}

void BaseTabs::keyPressEvent(QKeyEvent *event)
{
    Qt::KeyboardModifiers ctrl = event->modifiers() & Qt::ControlModifier;
    int key = event->key();
    bool handled = false;
    if (ctrl && this->count() > 0) {
        int index = this->currentIndex();
        if (key == Qt::Key_PageUp) {
            if (index > 0)
                setCurrentIndex(index - 1);
            else
                setCurrentIndex(this->count() - 1);
            handled = true;
        }
        else if (key == Qt::Key_PageDown) {
            if (index < this->count()-1)
                setCurrentIndex(index + 1);
            else
                setCurrentIndex(0);
            handled = true;
        }
    }
    if (!handled)
        QTabWidget::keyPressEvent(event);
}

void BaseTabs::tab_navigate(int delta)
{
    int index;
    if (delta > 0 && currentIndex() == count()-1)
        index = delta-1;
    else if (delta < 0 && currentIndex() == 0)
        index = count() + delta;
    else
        index = currentIndex()+delta;
    setCurrentIndex(index);
}

void BaseTabs::set_close_function(std::function<void(int)> func)
{
    bool state = func != nullptr;
    if (state)
        connect(this, &BaseTabs::sig_close_tab, func);

    this->setTabsClosable(state);
    connect(this, &QTabWidget::tabCloseRequested, func);
}


/********** Tabs **********/
Tabs::Tabs(QWidget* parent,
     const QList<QAction*>& actions,
     QMenu* menu,
     QHash<Qt::Corner, QList<QPair<int, QToolButton*>>> corner_widgets,
     bool menu_use_tooltips,
     bool rename_tabs,
     const QString& split_char,
     int split_index)
    : BaseTabs (parent, actions, menu, corner_widgets, menu_use_tooltips)
{
    TabBar* tab_bar = new TabBar(this, parent, rename_tabs, split_char, split_index);
    connect(tab_bar,SIGNAL(sig_move_tab(int,int)),this,SLOT(move_tab(int,int)));
    connect(tab_bar,SIGNAL(sig_move_tab(QString,int,int)),
            this,SLOT(move_tab_from_another_tabwidget(QString,int,int)));
    setTabBar(tab_bar);

    QString keystr = "Ctrl+Tab";
    QShortcut* qsc = new QShortcut(QKeySequence(keystr), this);
    connect(qsc,&QShortcut::activated, [=](){ this->tab_navigate(1); });
    qsc->setContext(Qt::WidgetWithChildrenShortcut);

    keystr = "Ctrl+Shift+Tab";
    qsc = new QShortcut(QKeySequence(keystr), this);
    connect(qsc,&QShortcut::activated, [=](){ this->tab_navigate(-1); });
    qsc->setContext(Qt::WidgetWithChildrenShortcut);

    keystr = "Ctrl+W";
    qsc = new QShortcut(QKeySequence(keystr), this);
    connect(qsc,&QShortcut::activated, [=](){ emit sig_close_tab(currentIndex()); });
    qsc->setContext(Qt::WidgetWithChildrenShortcut);

    keystr = "Ctrl+F4";
    qsc = new QShortcut(QKeySequence(keystr), this);
    connect(qsc,&QShortcut::activated, [=](){ emit sig_close_tab(currentIndex()); });
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
}

void Tabs::move_tab(int index_from, int index_to)
{
    emit move_data(index_from, index_to);

    QString tip = tabToolTip(index_from);
    QString text = tabText(index_from);
    QIcon icon = tabIcon(index_from);
    QWidget* widget = this->widget(index_from);
    QWidget* current_widget = currentWidget();

    removeTab(index_from);
    insertTab(index_to, widget, icon, text);
    setTabToolTip(index_to, tip);

    setCurrentWidget(current_widget);
    emit move_tab_finished();
}

void Tabs::move_tab_from_another_tabwidget(QString tabwidget_from,
                                           int index_from,int index_to)
{
    size_t res = reinterpret_cast<size_t>(this);
    emit sig_move_tab(tabwidget_from, QString::number(res), index_from, index_to);
}
