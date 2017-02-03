/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "browsermainwindow.h"

#include "autosaver.h"
#include "bookmarks.h"
#include "browserapplication.h"
#include "chasewidget.h"
#include "downloadmanager.h"
#include "history.h"
#include "settings.h"
#include "tabwidget.h"
#include "toolbarsearch.h"
#include "ui_passworddialog.h"
#include "webview.h"

#include <QtCore/QSettings>

#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QPlainTextEdit>
#include <QtPrintSupport/QPrintDialog>
#include <QtPrintSupport/QPrintPreviewDialog>
#include <QtPrintSupport/QPrinter>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QInputDialog>
#include <QtGui/QBitmap>
#include <QWebEngineHistory>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QtCore/QDebug>
#include <QMimeData>
template<typename Arg, typename R, typename C>
struct InvokeWrapper {
    R *receiver;
    void (C::*memberFun)(Arg);
    void operator()(Arg result) {
        (receiver->*memberFun)(result);
    }

};

template<typename Arg, typename R, typename C>
InvokeWrapper<Arg, R, C> invoke(R *receiver, void (C::*memberFun)(Arg))
{
    InvokeWrapper<Arg, R, C> wrapper = {receiver, memberFun};
    return wrapper;
}

const char *BrowserMainWindow::defaultHome = "http://www.tifprint.com/";

QPushButton *custButton(QString str,QString str1)
{
    QPushButton *pushButton= new QPushButton;

    pushButton->setGeometry(10,10,200,200); //按钮的位置及大小
    pushButton->clearMask();
    pushButton->setBackgroundRole( QPalette::Base);

    QPixmap mypixmap;
    mypixmap.load(str);
    pushButton->setFlat(true);

    pushButton->setFixedSize( mypixmap.width(), mypixmap.height() );

    pushButton->setIcon(mypixmap);
    pushButton->setIconSize(QSize(mypixmap.width(),mypixmap.height()));
    pushButton->setToolTip(str1);
    return pushButton;
}

BrowserMainWindow::BrowserMainWindow(QWidget *parent, Qt::WindowFlags flags)
    : QMainWindow(parent, flags)
    , m_tabWidget(new TabWidget(this))
    , m_autoSaver(new AutoSaver(this))
    , m_historyBack(0)
    , m_historyForward(0)
    , m_stop(0)
    , m_reload(0)
    ,m_sysMenu(0)
{
    setToolButtonStyle(Qt::ToolButtonFollowStyle);
    setAttribute(Qt::WA_DeleteOnClose, true);
    QWebEngineSettings::globalSettings()->setAttribute(QWebEngineSettings::PluginsEnabled, true);
    QWebEngineSettings::globalSettings()->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    QWebEngineSettings::globalSettings()->setAttribute(QWebEngineSettings::XSSAuditingEnabled, true);

    statusBar()->setSizeGripEnabled(true);
    setupMenu();
    setupToolBar();
   #if __USE_TOOBAR__
    m_navigationBar->setVisible(true);
#else
     m_navigationBar->setVisible(false);
#endif

#if __USE_MENUEBAR__
     menuBar()->setVisible(true);
#else
     menuBar()->setVisible(false);
#endif


    QWidget *centralWidget = new QWidget(this);
#if __USE_BOOKMARK__
    BookmarksModel *bookmarksModel = BrowserApplication::bookmarksManager()->bookmarksModel();
    m_bookmarksToolbar = new BookmarksToolBar(bookmarksModel, this);
    connect(m_bookmarksToolbar, SIGNAL(openUrl(QUrl)),
            m_tabWidget, SLOT(loadUrlInCurrentTab(QUrl)));
    connect(m_bookmarksToolbar->toggleViewAction(), SIGNAL(toggled(bool)),
            this, SLOT(updateBookmarksToolbarActionText(bool)));
#endif
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setSpacing(0);
    layout->setMargin(0);
#if defined(Q_OS_OSX)
    layout->addWidget(m_bookmarksToolbar);
    layout->addWidget(new QWidget); // <- OS X tab widget style bug
#else
    addToolBarBreak();
#if __USE_BOOKMARK__
    addToolBar(m_bookmarksToolbar);
#endif

#endif
    /********
     *自定义工具栏
     */
    m_topWidget = setupToolBarExt();
    m_topWidget->setVisible(false);
    layout->addWidget(m_topWidget);


    /////////////////////////////////////////////////////////////
    layout->addWidget(m_tabWidget);
    centralWidget->setLayout(layout);
    setCentralWidget(centralWidget);

    connect(m_tabWidget, SIGNAL(loadPage(QString)),
        this, SLOT(loadPage(QString)));
    connect(m_tabWidget, SIGNAL(setCurrentTitle(QString)),
        this, SLOT(slotUpdateWindowTitle(QString)));
    connect(m_tabWidget, SIGNAL(showStatusBarMessage(QString)),
            statusBar(), SLOT(showMessage(QString)));
    connect(m_tabWidget, SIGNAL(linkHovered(QString)),
            statusBar(), SLOT(showMessage(QString)));
    connect(m_tabWidget, SIGNAL(loadProgress(int)),
            this, SLOT(slotLoadProgress(int)));
    connect(m_tabWidget, SIGNAL(tabsChanged()),
            m_autoSaver, SLOT(changeOccurred()));
    connect(m_tabWidget, SIGNAL(geometryChangeRequested(QRect)),
            this, SLOT(geometryChangeRequested(QRect)));
#if defined(QWEBENGINEPAGE_PRINTREQUESTED)
    connect(m_tabWidget, SIGNAL(printRequested(QWebEngineFrame*)),
            this, SLOT(printRequested(QWebEngineFrame*)));
#endif
    connect(m_tabWidget, SIGNAL(menuBarVisibilityChangeRequested(bool)),
            menuBar(), SLOT(setVisible(bool)));
    connect(m_tabWidget, SIGNAL(statusBarVisibilityChangeRequested(bool)),
            statusBar(), SLOT(setVisible(bool)));
#if __USE_TOOBAR__
    connect(m_tabWidget, SIGNAL(toolBarVisibilityChangeRequested(bool)),
            m_navigationBar, SLOT(setVisible(bool)));
#endif
#if    __USE_BOOKMARK__
    connect(m_tabWidget, SIGNAL(toolBarVisibilityChangeRequested(bool)),
            m_bookmarksToolbar, SLOT(setVisible(bool)));
#endif


#if defined(Q_OS_OSX)
    connect(m_tabWidget, SIGNAL(lastTabClosed()),
            this, SLOT(close()));
#else
    connect(m_tabWidget, SIGNAL(lastTabClosed()),
            m_tabWidget, SLOT(newTab()));
#endif

    slotUpdateWindowTitle();
    loadDefaultState();
    m_tabWidget->newTab();
 setAcceptDrops(true);

}
QWidget * BrowserMainWindow::setupToolBarExt()
{

    QWidget *topWidget = new QWidget(this);
    QString strylx="QWidget{ background: white; spacing:3px;}" ;
    topWidget->setStyleSheet(strylx);

    topWidget->setFixedHeight(50);

//    layout->addWidget(topWidget);

    QGridLayout *hayout = new QGridLayout;
  //  hayout->setSpacing(3);
   // hayout->setMargin(10);

    m_homeBtn=custButton(":home.png","主页");
    connect(m_homeBtn, SIGNAL(clicked()), this, SLOT(slotPushHomeActionUrl()));
    m_homeBtn->addAction(m_historyBack);
    hayout->addWidget(m_homeBtn,0,0,1,1);


    //m_historyBackBtn->setFlat(true);
    m_historyBackBtn=custButton(":historyback.png","后退");
    connect(m_historyBackBtn, SIGNAL(clicked()), this, SLOT(slotPushHistroyBackActionUrl()));
    m_historyBackBtn->addAction(m_historyBack);
    hayout->addWidget(m_historyBackBtn,0,1,1,1);


    m_historyForwardBtn=custButton(":history_forward.png","前进");
    connect(m_historyForwardBtn, SIGNAL(clicked()), this, SLOT(slotPushHistroyForwardActionUrl()));
    hayout->addWidget(m_historyForwardBtn,0,2,1,1);

     m_reloadPixmap.load(":refresh.png");

    m_refreshBtn=custButton(":refresh.png","刷新");
    connect(m_refreshBtn, SIGNAL(clicked()), this, SLOT(slotPushRefreshActionUrl()));
    hayout->addWidget(m_refreshBtn,0,3,1,1);

    m_tabWidget->lineEditStack()->setFixedHeight(40);

    hayout->addWidget(m_tabWidget->lineEditStack(),0,4,1,2);

     m_lineBtn=custButton(":line_icon.png","自定义及控制");
     m_sysMenu = createSysMenu();

     m_lineBtn->setMenu(m_sysMenu);
     m_lineBtn->setStyleSheet("QPushButton::menu-indicator{image:None;}");
    // connect(m_refreshBtn, SIGNAL(clicked()), this, SLOT(slotPushRefreshActionUrl()));
     hayout->addWidget(m_lineBtn,0,6,1,1);


     m_dotBtn=custButton(":dot_icon.png","");
     BookmarksMenu *bookmarksMenu = new BookmarksMenu(this);
     connect(bookmarksMenu, SIGNAL(openUrl(QUrl)), m_tabWidget, SLOT(loadUrlInCurrentTab(QUrl)));
     connect(bookmarksMenu, SIGNAL(hovered(QString)),this, SLOT(slotUpdateStatusbar(QString)));
     bookmarksMenu->setTitle(tr("&Bookmarks"));
     m_dotBtn->setMenu(bookmarksMenu);

     QList<QAction*> bookmarksActions;

     QAction *showAllBookmarksAction = new QAction(tr("Show All Bookmarks"), this);
     connect(showAllBookmarksAction, SIGNAL(triggered()), this, SLOT(slotShowBookmarksDialog()));
     m_addBookmark = new QAction(QIcon(QLatin1String(":addbookmark.png")), tr("Add Bookmark..."), this);
     m_addBookmark->setIconVisibleInMenu(false);

     connect(m_addBookmark, SIGNAL(triggered()), this, SLOT(slotAddBookmark()));
     m_addBookmark->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_D));

     bookmarksActions.append(showAllBookmarksAction);
     bookmarksActions.append(m_addBookmark);
     bookmarksMenu->setInitialActions(bookmarksActions);
      m_dotBtn->setStyleSheet("QPushButton::menu-indicator{image:None;}");
    // connect(m_refreshBtn, SIGNAL(clicked()), this, SLOT(slotPushRefreshActionUrl()));
     hayout->addWidget(m_dotBtn,0,7,1,1);
    topWidget->setLayout(hayout);

    return topWidget;

}
 bool BrowserMainWindow::isTopbarVisible()
 {
     return m_topWidget->isVisible();
 }

QMenu* BrowserMainWindow::getSysMenu()
{
    return m_sysMenu;
}

QMenu * BrowserMainWindow::createSysMenu()
{
    // File
    QMenu *sysMenu = new QMenu(this);

     sysMenu->addAction(tr("&New Window"), this, SLOT(slotFileNew()), QKeySequence::New);
     sysMenu->addAction(m_tabWidget->newTabAction());
      sysMenu->addAction(m_tabWidget->closeTabAction());
     sysMenu->addAction(tr("&Open File..."), this, SLOT(slotFileOpen()), QKeySequence::Open);
     sysMenu->addAction(tr("Open &Location..."), this,
                 SLOT(slotSelectLineEdit()), QKeySequence(Qt::ControlModifier + Qt::Key_L));

     sysMenu->addSeparator();
#if defined(Q_OS_OSX)
    sysMenu->addAction(tr("&Quit"), BrowserApplication::instance(), SLOT(quitBrowser()), QKeySequence(Qt::CTRL | Qt::Key_Q));
#else
    sysMenu->addAction(tr("&Quit"), this, SLOT(close()), QKeySequence(Qt::CTRL | Qt::Key_Q));
#endif
    sysMenu->addSeparator();
    // Edit
    QAction *m_undo = sysMenu->addAction(tr("&Undo"));
    m_undo->setShortcuts(QKeySequence::Undo);
    m_tabWidget->addWebAction(m_undo, QWebEnginePage::Undo);
    QAction *m_redo = sysMenu->addAction(tr("&Redo"));
   // m_redo->setShortcuts(QKeySequence::Redo);
    m_tabWidget->addWebAction(m_redo, QWebEnginePage::Redo);
    sysMenu->addSeparator();
    QAction *m_cut = sysMenu->addAction(tr("Cu&t"));
    m_cut->setShortcuts(QKeySequence::Cut);
    m_tabWidget->addWebAction(m_cut, QWebEnginePage::Cut);
    QAction *m_copy = sysMenu->addAction(tr("&Copy"));
    m_copy->setShortcuts(QKeySequence::Copy);
    m_tabWidget->addWebAction(m_copy, QWebEnginePage::Copy);
    QAction *m_paste = sysMenu->addAction(tr("&Paste"));
    m_paste->setShortcuts(QKeySequence::Paste);
    m_tabWidget->addWebAction(m_paste, QWebEnginePage::Paste);
    sysMenu->addSeparator();

    QAction *m_find = sysMenu->addAction(tr("&Find"));
    m_find->setShortcuts(QKeySequence::Find);
    connect(m_find, SIGNAL(triggered()), this, SLOT(slotEditFind()));
    new QShortcut(QKeySequence(Qt::Key_Slash), this, SLOT(slotEditFind()));

    QAction *m_findNext = sysMenu->addAction(tr("&Find Next"));
    m_findNext->setShortcuts(QKeySequence::FindNext);
    connect(m_findNext, SIGNAL(triggered()), this, SLOT(slotEditFindNext()));

    QAction *m_findPrevious = sysMenu->addAction(tr("&Find Previous"));
    m_findPrevious->setShortcuts(QKeySequence::FindPrevious);
    connect(m_findPrevious, SIGNAL(triggered()), this, SLOT(slotEditFindPrevious()));
    sysMenu->addSeparator();

    sysMenu->addAction(tr("&Preferences"), this, SLOT(slotPreferences()), tr("Ctrl+,"));

    sysMenu->addSeparator();

    sysMenu->addAction(tr("删除缓存"), this, SLOT(slotClearPrivacy()), tr("Ctrl+p"));
 return sysMenu;
 /*
     m_lineBtn->setMenu(sysMenu);
     m_lineBtn->setStyleSheet("QPushButton::menu-indicator{image:None;}");
    // connect(m_refreshBtn, SIGNAL(clicked()), this, SLOT(slotPushRefreshActionUrl()));
     hayout->addWidget(m_lineBtn,0,6,1,1);


     m_dotBtn=custButton(":dot_icon.png","");
     BookmarksMenu *bookmarksMenu = new BookmarksMenu(this);
     connect(bookmarksMenu, SIGNAL(openUrl(QUrl)), m_tabWidget, SLOT(loadUrlInCurrentTab(QUrl)));
     connect(bookmarksMenu, SIGNAL(hovered(QString)),this, SLOT(slotUpdateStatusbar(QString)));
     bookmarksMenu->setTitle(tr("&Bookmarks"));
     m_dotBtn->setMenu(bookmarksMenu);

     QList<QAction*> bookmarksActions;

     QAction *showAllBookmarksAction = new QAction(tr("Show All Bookmarks"), this);
     connect(showAllBookmarksAction, SIGNAL(triggered()), this, SLOT(slotShowBookmarksDialog()));
     m_addBookmark = new QAction(QIcon(QLatin1String(":addbookmark.png")), tr("Add Bookmark..."), this);
     m_addBookmark->setIconVisibleInMenu(false);

     connect(m_addBookmark, SIGNAL(triggered()), this, SLOT(slotAddBookmark()));
     m_addBookmark->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_D));

     bookmarksActions.append(showAllBookmarksAction);
     bookmarksActions.append(m_addBookmark);
     bookmarksMenu->setInitialActions(bookmarksActions);
      m_dotBtn->setStyleSheet("QPushButton::menu-indicator{image:None;}");
    // connect(m_refreshBtn, SIGNAL(clicked()), this, SLOT(slotPushRefreshActionUrl()));
     hayout->addWidget(m_dotBtn,0,7,1,1);
    topWidget->setLayout(hayout);

    return topWidget;
*/
}

BrowserMainWindow::~BrowserMainWindow()
{
    m_autoSaver->changeOccurred();
    m_autoSaver->saveIfNeccessary();
}

void BrowserMainWindow::loadDefaultState()
{
    QSettings settings;
    settings.beginGroup(QLatin1String("BrowserMainWindow"));
    QByteArray data = settings.value(QLatin1String("defaultState")).toByteArray();
    restoreState(data);
    settings.endGroup();
}

QSize BrowserMainWindow::sizeHint() const
{
    QRect desktopRect = QApplication::desktop()->screenGeometry();
    QSize size = desktopRect.size() * qreal(0.9);
    return size;
}

void BrowserMainWindow::save()
{
    BrowserApplication::instance()->saveSession();

    QSettings settings;
    settings.beginGroup(QLatin1String("BrowserMainWindow"));
    QByteArray data = saveState(false);
    settings.setValue(QLatin1String("defaultState"), data);
    settings.endGroup();
}

static const qint32 BrowserMainWindowMagic = 0xba;

QByteArray BrowserMainWindow::saveState(bool withTabs) const
{
    int version = 2;
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);

    stream << qint32(BrowserMainWindowMagic);
    stream << qint32(version);

    stream << size();
    stream << !m_navigationBar->isHidden();
#if __USE_BOOKMARK__
    stream << !m_bookmarksToolbar->isHidden();
#endif
    stream << !statusBar()->isHidden();
    if (withTabs)
        stream << tabWidget()->saveState();
    else
        stream << QByteArray();
    return data;
}

bool BrowserMainWindow::restoreState(const QByteArray &state)
{
    int version = 2;
    QByteArray sd = state;
    QDataStream stream(&sd, QIODevice::ReadOnly);
    if (stream.atEnd())
        return false;

    qint32 marker;
    qint32 v;
    stream >> marker;
    stream >> v;
    if (marker != BrowserMainWindowMagic || v != version)
        return false;

    QSize size;
    bool showToolbar;
#if __USE_BOOKMARK__
    bool showBookmarksBar;
#endif
    bool showStatusbar;
    QByteArray tabState;

    stream >> size;
    stream >> showToolbar;
#if __USE_BOOKMARK__
    stream >> showBookmarksBar;
#endif
    stream >> showStatusbar;
    stream >> tabState;

    resize(size);
#if __USE_TOOBAR__
    m_navigationBar->setVisible(showToolbar);
    updateToolbarActionText(showToolbar);
#endif

#if __USE_BOOKMARK__
    m_bookmarksToolbar->setVisible(showBookmarksBar);

    updateBookmarksToolbarActionText(showBookmarksBar);
#endif

    statusBar()->setVisible(showStatusbar);
    updateStatusbarActionText(showStatusbar);

    if (!tabWidget()->restoreState(tabState))
        return false;

    return true;
}

void BrowserMainWindow::runScriptOnOpenViews(const QString &source)
{
    for (int i =0; i < tabWidget()->count(); ++i)
        tabWidget()->webView(i)->page()->runJavaScript(source);
}

void BrowserMainWindow::setupMenu()
{
    new QShortcut(QKeySequence(Qt::Key_F6), this, SLOT(slotSwapFocus()));

    // File
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));

    fileMenu->addAction(tr("&New Window"), this, SLOT(slotFileNew()), QKeySequence::New);
    fileMenu->addAction(m_tabWidget->newTabAction());
    fileMenu->addAction(tr("&Open File..."), this, SLOT(slotFileOpen()), QKeySequence::Open);
    fileMenu->addAction(tr("Open &Location..."), this,
                SLOT(slotSelectLineEdit()), QKeySequence(Qt::ControlModifier + Qt::Key_L));
    fileMenu->addSeparator();
    fileMenu->addAction(m_tabWidget->closeTabAction());
    fileMenu->addSeparator();
#if defined(QWEBENGINE_SAVE_AS_FILE)
    fileMenu->addAction(tr("&Save As..."), this,
                SLOT(slotFileSaveAs()), QKeySequence(QKeySequence::Save));
    fileMenu->addSeparator();
#endif

#if __USE_BOOKMARK__
    BookmarksManager *bookmarksManager = BrowserApplication::bookmarksManager();
    fileMenu->addAction(tr("&Import Bookmarks..."), bookmarksManager, SLOT(importBookmarks()));
    fileMenu->addAction(tr("&Export Bookmarks..."), bookmarksManager, SLOT(exportBookmarks()));

    fileMenu->addSeparator();
#endif

#if defined(QWEBENGINEPAGE_PRINT)
    fileMenu->addAction(tr("P&rint Preview..."), this, SLOT(slotFilePrintPreview()));
    fileMenu->addAction(tr("&Print..."), this, SLOT(slotFilePrint()), QKeySequence::Print);
    fileMenu->addSeparator();
#endif
 fileMenu->addSeparator();

#if defined(Q_OS_OSX)
    fileMenu->addAction(tr("&Quit"), BrowserApplication::instance(), SLOT(quitBrowser()), QKeySequence(Qt::CTRL | Qt::Key_Q));
#else
    fileMenu->addAction(tr("&Quit"), this, SLOT(close()), QKeySequence(Qt::CTRL | Qt::Key_Q));
#endif

    // Edit
    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));
    QAction *m_undo = editMenu->addAction(tr("&Undo"));
  //  m_undo->setShortcuts(QKeySequence::Undo);
    m_tabWidget->addWebAction(m_undo, QWebEnginePage::Undo);
    QAction *m_redo = editMenu->addAction(tr("&Redo"));
    m_redo->setShortcuts(QKeySequence::Redo);
    m_tabWidget->addWebAction(m_redo, QWebEnginePage::Redo);
    editMenu->addSeparator();
    QAction *m_cut = editMenu->addAction(tr("Cu&t"));
    m_cut->setShortcuts(QKeySequence::Cut);
    m_tabWidget->addWebAction(m_cut, QWebEnginePage::Cut);
    QAction *m_copy = editMenu->addAction(tr("&Copy"));
    m_copy->setShortcuts(QKeySequence::Copy);
    m_tabWidget->addWebAction(m_copy, QWebEnginePage::Copy);
    QAction *m_paste = editMenu->addAction(tr("&Paste"));
    m_paste->setShortcuts(QKeySequence::Paste);
    m_tabWidget->addWebAction(m_paste, QWebEnginePage::Paste);
    editMenu->addSeparator();

    QAction *m_find = editMenu->addAction(tr("&Find"));
    m_find->setShortcuts(QKeySequence::Find);
    connect(m_find, SIGNAL(triggered()), this, SLOT(slotEditFind()));
    new QShortcut(QKeySequence(Qt::Key_Slash), this, SLOT(slotEditFind()));

    QAction *m_findNext = editMenu->addAction(tr("&Find Next"));
    m_findNext->setShortcuts(QKeySequence::FindNext);
    connect(m_findNext, SIGNAL(triggered()), this, SLOT(slotEditFindNext()));

    QAction *m_findPrevious = editMenu->addAction(tr("&Find Previous"));
    m_findPrevious->setShortcuts(QKeySequence::FindPrevious);
    connect(m_findPrevious, SIGNAL(triggered()), this, SLOT(slotEditFindPrevious()));
    editMenu->addSeparator();

    editMenu->addAction(tr("&Preferences"), this, SLOT(slotPreferences()), tr("Ctrl+,"));

    // View
    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
#if __USE_BOOKMARK__
    m_viewBookmarkBar = new QAction(this);
    updateBookmarksToolbarActionText(true);
    m_viewBookmarkBar->setShortcut(tr("Shift+Ctrl+B"));
    connect(m_viewBookmarkBar, SIGNAL(triggered()), this, SLOT(slotViewBookmarksBar()));
    viewMenu->addAction(m_viewBookmarkBar);
#endif
    m_viewToolbar = new QAction(this);
    updateToolbarActionText(true);
    m_viewToolbar->setShortcut(tr("Ctrl+|"));
    connect(m_viewToolbar, SIGNAL(triggered()), this, SLOT(slotViewToolbar()));
    viewMenu->addAction(m_viewToolbar);

    m_viewStatusbar = new QAction(this);
    updateStatusbarActionText(true);
    m_viewStatusbar->setShortcut(tr("Ctrl+/"));
    connect(m_viewStatusbar, SIGNAL(triggered()), this, SLOT(slotViewStatusbar()));
    viewMenu->addAction(m_viewStatusbar);

    viewMenu->addSeparator();

    m_stop = viewMenu->addAction(tr("&Stop"));
    QList<QKeySequence> shortcuts;
    shortcuts.append(QKeySequence(Qt::CTRL | Qt::Key_Period));
    shortcuts.append(Qt::Key_Escape);
    m_stop->setShortcuts(shortcuts);
    m_tabWidget->addWebAction(m_stop, QWebEnginePage::Stop);

    m_reload = viewMenu->addAction(tr("Reload Page"));
    m_reload->setShortcuts(QKeySequence::Refresh);
    m_tabWidget->addWebAction(m_reload, QWebEnginePage::Reload);

    viewMenu->addAction(tr("Zoom &In"), this, SLOT(slotViewZoomIn()), QKeySequence(Qt::CTRL | Qt::Key_Plus));
    viewMenu->addAction(tr("Zoom &Out"), this, SLOT(slotViewZoomOut()), QKeySequence(Qt::CTRL | Qt::Key_Minus));
    viewMenu->addAction(tr("Reset &Zoom"), this, SLOT(slotViewResetZoom()), QKeySequence(Qt::CTRL | Qt::Key_0));

    viewMenu->addSeparator();
    viewMenu->addAction(tr("Page S&ource"), this, SLOT(slotViewPageSource()), tr("Ctrl+Alt+U"));
    QAction *a = viewMenu->addAction(tr("&Full Screen"), this, SLOT(slotViewFullScreen(bool)),  Qt::Key_F11);
    a->setCheckable(true);

    // History
    HistoryMenu *historyMenu = new HistoryMenu(this);
    connect(historyMenu, SIGNAL(openUrl(QUrl)),
            m_tabWidget, SLOT(loadUrlInCurrentTab(QUrl)));
    connect(historyMenu, SIGNAL(hovered(QString)), this,
            SLOT(slotUpdateStatusbar(QString)));
    historyMenu->setTitle(tr("Hi&story"));
    menuBar()->addMenu(historyMenu);
    QList<QAction*> historyActions;

    m_historyBack = new QAction(tr("Back"), this);
    m_tabWidget->addWebAction(m_historyBack, QWebEnginePage::Back);
 //   m_historyBack->setShortcuts(QKeySequence::Back);
    m_historyBack->setIconVisibleInMenu(false);
    historyActions.append(m_historyBack);

    m_historyForward = new QAction(tr("Forward"), this);
    m_tabWidget->addWebAction(m_historyForward, QWebEnginePage::Forward);
    m_historyForward->setShortcuts(QKeySequence::Forward);
    m_historyForward->setIconVisibleInMenu(false);
    historyActions.append(m_historyForward);

    QAction *m_historyHome = new QAction(tr("Home"), this);
    connect(m_historyHome, SIGNAL(triggered()), this, SLOT(slotHome()));
    m_historyHome->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_H));
    historyActions.append(m_historyHome);

#if defined(QWEBENGINEHISTORY_RESTORESESSION)
    m_restoreLastSession = new QAction(tr("Restore Last Session"), this);
    connect(m_restoreLastSession, SIGNAL(triggered()), BrowserApplication::instance(), SLOT(restoreLastSession()));
    m_restoreLastSession->setEnabled(BrowserApplication::instance()->canRestoreSession());
    historyActions.append(m_tabWidget->recentlyClosedTabsAction());
    historyActions.append(m_restoreLastSession);
#endif

    historyMenu->setInitialActions(historyActions);

    // Bookmarks
#if __USE_BOOKMARK__
    BookmarksMenu *bookmarksMenu = new BookmarksMenu(this);
    connect(bookmarksMenu, SIGNAL(openUrl(QUrl)),
            m_tabWidget, SLOT(loadUrlInCurrentTab(QUrl)));
    connect(bookmarksMenu, SIGNAL(hovered(QString)),
            this, SLOT(slotUpdateStatusbar(QString)));
    bookmarksMenu->setTitle(tr("&Bookmarks"));
    menuBar()->addMenu(bookmarksMenu);

    QList<QAction*> bookmarksActions;

    QAction *showAllBookmarksAction = new QAction(tr("Show All Bookmarks"), this);
    connect(showAllBookmarksAction, SIGNAL(triggered()), this, SLOT(slotShowBookmarksDialog()));
    m_addBookmark = new QAction(QIcon(QLatin1String(":addbookmark.png")), tr("Add Bookmark..."), this);
    m_addBookmark->setIconVisibleInMenu(false);

    connect(m_addBookmark, SIGNAL(triggered()), this, SLOT(slotAddBookmark()));
    m_addBookmark->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_D));

    bookmarksActions.append(showAllBookmarksAction);
    bookmarksActions.append(m_addBookmark);
    bookmarksMenu->setInitialActions(bookmarksActions);
#endif

    // Window
    m_windowMenu = menuBar()->addMenu(tr("&Window"));
    connect(m_windowMenu, SIGNAL(aboutToShow()),
            this, SLOT(slotAboutToShowWindowMenu()));
    slotAboutToShowWindowMenu();

    QMenu *toolsMenu = menuBar()->addMenu(tr("&Tools"));
    toolsMenu->addAction(tr("Web &Search"), this, SLOT(slotWebSearch()), QKeySequence(tr("Ctrl+K", "Web Search")));
#if defined(QWEBENGINEINSPECTOR)
    a = toolsMenu->addAction(tr("Enable Web &Inspector"), this, SLOT(slotToggleInspector(bool)));
    a->setCheckable(true);
#endif

    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(tr("About &Qt"), qApp, SLOT(aboutQt()));
    helpMenu->addAction(tr("About &Demo Browser"), this, SLOT(slotAboutApplication()));
}

void BrowserMainWindow::setupToolBar()
{
    m_navigationBar = addToolBar(tr("Navigation"));

#if __USE_TOOBAR__
    connect(m_navigationBar->toggleViewAction(), SIGNAL(toggled(bool)),
            this, SLOT(updateToolbarActionText(bool)));
#endif
    m_historyBack->setIcon(style()->standardIcon(QStyle::SP_ArrowBack, 0, this));
    m_historyBackMenu = new QMenu(this);
    m_historyBack->setMenu(m_historyBackMenu);
    connect(m_historyBackMenu, SIGNAL(aboutToShow()),
            this, SLOT(slotAboutToShowBackMenu()));
    connect(m_historyBackMenu, SIGNAL(triggered(QAction*)),
            this, SLOT(slotOpenActionUrl(QAction*)));
#if __USE_TOOBAR__
    m_navigationBar->addAction(m_historyBack);
#endif

    m_historyForward->setIcon(style()->standardIcon(QStyle::SP_ArrowForward, 0, this));
    m_historyForwardMenu = new QMenu(this);
    connect(m_historyForwardMenu, SIGNAL(aboutToShow()),
            this, SLOT(slotAboutToShowForwardMenu()));
    connect(m_historyForwardMenu, SIGNAL(triggered(QAction*)),
            this, SLOT(slotOpenActionUrl(QAction*)));
    m_historyForward->setMenu(m_historyForwardMenu);
#if __USE_TOOBAR__
    m_navigationBar->addAction(m_historyForward);
#endif
    m_stopReload = new QAction(this);
    m_reloadIcon = style()->standardIcon(QStyle::SP_BrowserReload);
    m_stopReload->setIcon(m_reloadIcon);
#if __USE_TOOBAR__
    m_navigationBar->addAction(m_stopReload);
#endif
 //   m_navigationBar->addWidget(m_tabWidget->lineEditStack());

    m_toolbarSearch = new ToolbarSearch(m_navigationBar);
#if __USE_TOOBAR__
    m_navigationBar->addWidget(m_toolbarSearch);
#endif
    connect(m_toolbarSearch, SIGNAL(search(QUrl)), SLOT(loadUrl(QUrl)));

    m_chaseWidget = new ChaseWidget(this);
#if __USE_TOOBAR__
    m_navigationBar->addWidget(m_chaseWidget);
#endif

}

void BrowserMainWindow::slotShowBookmarksDialog()
{
    BookmarksDialog *dialog = new BookmarksDialog(this);
    connect(dialog, SIGNAL(openUrl(QUrl)),
            m_tabWidget, SLOT(loadUrlInCurrentTab(QUrl)));
    dialog->show();
}

void BrowserMainWindow::slotAddBookmark()
{
    WebView *webView = currentTab();
    QString url = webView->url().toString();
    QString title = webView->title();
    AddBookmarkDialog dialog(url, title);
    dialog.exec();
}
void BrowserMainWindow::slotAddBookmark2(void *pobj)
{
   BrowserMainWindow* pthis= (BrowserMainWindow*)pobj;
   pthis->slotAddBookmark();
}

void BrowserMainWindow::slotViewToolbar()
{
#if __USE_TOOBAR__
    if (m_navigationBar->isVisible()) {
        updateToolbarActionText(false);
        m_navigationBar->close();
    } else {
        updateToolbarActionText(true);
        m_navigationBar->show();
    }
    m_autoSaver->changeOccurred();
#endif

}
#if __USE_BOOKMARK__
void BrowserMainWindow::slotViewBookmarksBar()
{

    if (m_bookmarksToolbar->isVisible()) {
        updateBookmarksToolbarActionText(false);
        m_bookmarksToolbar->close();
    } else {
        updateBookmarksToolbarActionText(true);
        m_bookmarksToolbar->show();
    }
    m_autoSaver->changeOccurred();

}
 #endif

void BrowserMainWindow::updateStatusbarActionText(bool visible)
{
    m_viewStatusbar->setText(!visible ? tr("Show Status Bar") : tr("Hide Status Bar"));
}

void BrowserMainWindow::handleFindTextResult(bool found)
{
    if (!found)
        slotUpdateStatusbar(tr("\"%1\" not found.").arg(m_lastSearch));
}

void BrowserMainWindow::updateToolbarActionText(bool visible)
{
    m_viewToolbar->setText(!visible ? tr("Show Toolbar") : tr("Hide Toolbar"));
}
#if __USE_BOOKMARK__
void BrowserMainWindow::updateBookmarksToolbarActionText(bool visible)
{
    m_viewBookmarkBar->setText(!visible ? tr("Show Bookmarks bar") : tr("Hide Bookmarks bar"));
}
#endif

void BrowserMainWindow::slotViewStatusbar()
{
    if (statusBar()->isVisible()) {
        updateStatusbarActionText(false);
        statusBar()->close();
    } else {
        updateStatusbarActionText(true);
        statusBar()->show();
    }
    m_autoSaver->changeOccurred();
}

void BrowserMainWindow::loadUrl(const QUrl &url)
{
    if (!currentTab() || !url.isValid())
        return;
    if(url.scheme().compare(QString("file"))==0)
    {
        QString strFilePath = url.path().mid(1);
        QFileInfo fi(strFilePath);
        if(fi.isFile() && fi.exists() && strFilePath.endsWith(QString(".pdf"),Qt::CaseInsensitive))
        {
            QFile file(strFilePath);
            if (file.open(QIODevice::ReadOnly)) {
                QByteArray pdfData = file.readAll();
                file.close();
                pdfData = pdfData.toBase64();
                QString script = QString("showPdfFile('%1')") .arg(QString::fromUtf8(pdfData));
                m_tabWidget->currentWebView()->setShowPdfScript(script);
                m_tabWidget->currentWebView()->setPdfPath(strFilePath);
                // Clear pdf data
                pdfData.clear();

                QUrl url = QUrl::fromUserInput("qrc:/web/viewer.html");
                m_tabWidget->currentWebView()->setUrl(url);
                m_tabWidget->currentLineEdit()->setText(strFilePath);

                return;
            }
        }

    }

    {

      m_tabWidget->currentLineEdit()->setText(QString::fromUtf8(url.toEncoded()));

     m_tabWidget->loadUrlInCurrentTab(url);
    }
}
void BrowserMainWindow::loadUrlEx(const QUrl &url,const QString &title)
{
    if (!currentTab() || !url.isValid())
        return;

    m_tabWidget->currentLineEdit()->setText(title);

    m_tabWidget->loadUrlInCurrentTab(url);
}

void BrowserMainWindow::slotDownloadManager()
{
    BrowserApplication::downloadManager()->show();
}

void BrowserMainWindow::slotSelectLineEdit()
{
    m_tabWidget->currentLineEdit()->selectAll();
    m_tabWidget->currentLineEdit()->setFocus();
}

void BrowserMainWindow::slotFileSaveAs()
{
    // not implemented yet.
}

void BrowserMainWindow::slotPreferences()
{
    SettingsDialog *s = new SettingsDialog(this);
    s->show();
}
void BrowserMainWindow::slotClearPrivacy()
{
    QSettings settings;
    settings.beginGroup(QLatin1String("cookies"));

   QString pdataPath = settings.value(QLatin1String("persistentDataPath")).toString();
   if(pdataPath.length()==0)
       pdataPath= QWebEngineProfile::defaultProfile()->persistentStoragePath();
   settings.endGroup();
removeFolderContent(pdataPath);

}

void BrowserMainWindow::slotUpdateStatusbar(const QString &string)
{
    statusBar()->showMessage(string, 2000);
}

void BrowserMainWindow::slotUpdateWindowTitle(const QString &title)
{
    if (title.isEmpty()) {
        setWindowTitle(tr("特赋云浏览器"));
    } else {
#if defined(Q_OS_OSX)
        setWindowTitle(title);
#else
        setWindowTitle(tr("%1 - 特赋云浏览器", "Page title and Browser name").arg(title));
#endif
    }
}

void BrowserMainWindow::slotAboutApplication()
{
    QMessageBox::about(this, tr("About"), tr(
        "Version %1"
        "<p>特赋云浏览器 "
        ).arg(QCoreApplication::applicationVersion()));
}

void BrowserMainWindow::slotFileNew()
{
    BrowserApplication::instance()->newMainWindow();
    BrowserMainWindow *mw = BrowserApplication::instance()->mainWindow();
    mw->slotHome();
}

void BrowserMainWindow::slotFileOpen()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("Open Web Resource"), QString(),
            tr("Web Resources (*.html *.htm *.svg *.png *.gif *.svgz *.pdf);;All files (*.*)"));

    if (filePath.isEmpty())
        return;

   openFile(filePath);
}
void BrowserMainWindow::createNewTab()
{
    m_tabWidget->newTab();
}

void BrowserMainWindow::openFile(QString filePath)
{
    if(filePath.endsWith(".pdf",Qt::CaseInsensitive))
    {
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray pdfData = file.readAll();
            file.close();
            pdfData = pdfData.toBase64();
            QString script = QString("showPdfFile('%1')") .arg(QString::fromUtf8(pdfData));
            m_tabWidget->currentWebView()->setShowPdfScript(script);
            m_tabWidget->currentWebView()->setPdfPath(filePath);
            // Clear pdf data
            pdfData.clear();

            QUrl url = QUrl::fromUserInput("qrc:/web/viewer.html");
            m_tabWidget->currentWebView()->setUrl(url);
            m_tabWidget->currentLineEdit()->setText(filePath);

            return;
        }
    }
    loadPage(filePath);
}

void BrowserMainWindow::slotFilePrintPreview()
{
#ifndef QT_NO_PRINTPREVIEWDIALOG
    if (!currentTab())
        return;
    QPrintPreviewDialog *dialog = new QPrintPreviewDialog(this);
    connect(dialog, SIGNAL(paintRequested(QPrinter*)),
            currentTab(), SLOT(print(QPrinter*)));
    dialog->exec();
#endif
}

void BrowserMainWindow::slotFilePrint()
{
#if defined(QWEBENGINEPAGE_PRINT)
    if (!currentTab())
        return;
    printRequested(currentTab()->page()->mainFrame());
#endif
}

#if defined(QWEBENGINEPAGE_PRINT)
void BrowserMainWindow::printRequested(QWebEngineFrame *frame)
{
#ifndef QT_NO_PRINTDIALOG
    QPrinter printer;
    QPrintDialog *dialog = new QPrintDialog(&printer, this);
    dialog->setWindowTitle(tr("Print Document"));
    if (dialog->exec() != QDialog::Accepted)
        return;
    frame->print(&printer);
#endif
}
#endif



void BrowserMainWindow::closeEvent(QCloseEvent *event)
{
    if (m_tabWidget->count() > 1) {
        int ret = QMessageBox::warning(this, QString(),
                           tr("Are you sure you want to close the window?"
                              "  There are %1 tabs open").arg(m_tabWidget->count()),
                           QMessageBox::Yes | QMessageBox::No,
                           QMessageBox::No);
        if (ret == QMessageBox::No) {
            event->ignore();
            return;
        }
    }
    event->accept();
    deleteLater();
}

bool BrowserMainWindow::event(QEvent *event)
{
    if(event->type() == QEvent::Paint)
    {
        QWebEngineHistory *history = currentTab()->history();
        if(history!=NULL)
        {
            int historyCount = history->count();
            if(history->backItems(historyCount).count()>0)
                m_historyBackBtn->setEnabled(true);
             else
                m_historyBackBtn->setEnabled(false);

            if(history->forwardItems(historyCount).count()>0)
                m_historyForwardBtn->setEnabled(true);
             else
                m_historyForwardBtn->setEnabled(false);
        }

    }
    return QMainWindow::event(event);
}

void BrowserMainWindow::slotEditFind()
{
    if (!currentTab())
        return;
    bool ok;
    QString search = QInputDialog::getText(this, tr("Find"),
                                          tr("Text:"), QLineEdit::Normal,
                                          m_lastSearch, &ok);
    if (ok && !search.isEmpty()) {
        m_lastSearch = search;
        currentTab()->findText(m_lastSearch, 0, invoke(this, &BrowserMainWindow::handleFindTextResult));
    }
}

void BrowserMainWindow::slotEditFindNext()
{
    if (!currentTab() && !m_lastSearch.isEmpty())
        return;
    currentTab()->findText(m_lastSearch);
}

void BrowserMainWindow::slotEditFindPrevious()
{
    if (!currentTab() && !m_lastSearch.isEmpty())
        return;
    currentTab()->findText(m_lastSearch, QWebEnginePage::FindBackward);
}

void BrowserMainWindow::slotViewZoomIn()
{
    if (!currentTab())
        return;
    currentTab()->setZoomFactor(currentTab()->zoomFactor() + 0.1);
}

void BrowserMainWindow::slotViewZoomOut()
{
    if (!currentTab())
        return;
    currentTab()->setZoomFactor(currentTab()->zoomFactor() - 0.1);
}

void BrowserMainWindow::slotViewResetZoom()
{
    if (!currentTab())
        return;
    currentTab()->setZoomFactor(1.0);
}

void BrowserMainWindow::slotViewFullScreen(bool makeFullScreen)
{
    if (makeFullScreen) {
        showFullScreen();
    } else {
        if (isMinimized())
            showMinimized();
        else if (isMaximized())
            showMaximized();
        else showNormal();
    }
}

void BrowserMainWindow::slotViewPageSource()
{
    if (!currentTab())
        return;

    QPlainTextEdit *view = new QPlainTextEdit;
    view->setWindowTitle(tr("Page Source of %1").arg(currentTab()->title()));
    view->setMinimumWidth(640);
    view->setAttribute(Qt::WA_DeleteOnClose);
    view->show();

    currentTab()->page()->toHtml(invoke(view, &QPlainTextEdit::setPlainText));
}

void BrowserMainWindow::slotHome()
{
    QSettings settings;
    settings.beginGroup(QLatin1String("MainWindow"));
    QString home = settings.value(QLatin1String("home"), QLatin1String(defaultHome)).toString();
    loadPage(home);
}

void BrowserMainWindow::slotWebSearch()
{
    m_toolbarSearch->lineEdit()->selectAll();
    m_toolbarSearch->lineEdit()->setFocus();
}

void BrowserMainWindow::slotToggleInspector(bool enable)
{
#if defined(QWEBENGINEINSPECTOR)
    QWebEngineSettings::globalSettings()->setAttribute(QWebEngineSettings::DeveloperExtrasEnabled, enable);
    if (enable) {
        int result = QMessageBox::question(this, tr("Web Inspector"),
                                           tr("The web inspector will only work correctly for pages that were loaded after enabling.\n"
                                           "Do you want to reload all pages?"),
                                           QMessageBox::Yes | QMessageBox::No);
        if (result == QMessageBox::Yes) {
            m_tabWidget->reloadAllTabs();
        }
    }
#else
    Q_UNUSED(enable);
#endif
}

void BrowserMainWindow::slotSwapFocus()
{
    if (currentTab()->hasFocus())
        m_tabWidget->currentLineEdit()->setFocus();
    else
        currentTab()->setFocus();
}
void BrowserMainWindow::slotPushHistroyBackActionUrl()
{
   m_historyBack->trigger();
}
void BrowserMainWindow::slotPushHistroyForwardActionUrl()
{
   m_historyForward->trigger();
}
void BrowserMainWindow::slotPushHomeActionUrl()
{
   emit slotHome();
}

void BrowserMainWindow::slotPushRefreshActionUrl()
{
  m_stopReload->trigger();
}

void BrowserMainWindow::loadPage(const QString &page)
{
    QUrl url = QUrl::fromUserInput(page);
    loadUrl(url);
}

TabWidget *BrowserMainWindow::tabWidget() const
{
    return m_tabWidget;


}

WebView *BrowserMainWindow::currentTab() const
{
    return m_tabWidget->currentWebView();
}

void BrowserMainWindow::slotLoadProgress(int progress)
{
    if (progress < 100 && progress > 0) {
        m_chaseWidget->setAnimated(true);
        disconnect(m_stopReload, SIGNAL(triggered()), m_reload, SLOT(trigger()));
        if (m_stopIcon.isNull())
            m_stopIcon = style()->standardIcon(QStyle::SP_BrowserStop);
        m_stopReload->setIcon(m_stopIcon);
        connect(m_stopReload, SIGNAL(triggered()), m_stop, SLOT(trigger()));
        m_stopReload->setToolTip(tr("Stop loading the current page"));
        m_refreshBtn->setIcon(m_stopIcon);
        m_refreshBtn->setToolTip("停止网页加载");
    } else {
        m_chaseWidget->setAnimated(false);
        disconnect(m_stopReload, SIGNAL(triggered()), m_stop, SLOT(trigger()));
        m_stopReload->setIcon(m_reloadIcon);
        connect(m_stopReload, SIGNAL(triggered()), m_reload, SLOT(trigger()));
        m_stopReload->setToolTip(tr("Reload the current page"));
        m_refreshBtn->setIcon(m_reloadPixmap);
        m_refreshBtn->setToolTip("刷新");

    }
}

void BrowserMainWindow::slotAboutToShowBackMenu()
{

    m_historyBackMenu->clear();
    if (!currentTab())
        return;
    QWebEngineHistory *history = currentTab()->history();
    int historyCount = history->count();
    for (int i = history->backItems(historyCount).count() - 1; i >= 0; --i) {
        QWebEngineHistoryItem item = history->backItems(history->count()).at(i);
        QAction *action = new QAction(this);
        action->setData(-1*(historyCount-i-1));
        QIcon icon = BrowserApplication::instance()->icon(item.url());
        action->setIcon(icon);
        action->setText(item.title());
        m_historyBackMenu->addAction(action);
    }

}

void BrowserMainWindow::slotAboutToShowForwardMenu()
{
    m_historyForwardMenu->clear();
    if (!currentTab())
        return;

    QWebEngineHistory *history = currentTab()->history();
    int historyCount = history->count();
    for (int i = 0; i < history->forwardItems(history->count()).count(); ++i) {
        QWebEngineHistoryItem item = history->forwardItems(historyCount).at(i);
        QAction *action = new QAction(this);
        action->setData(historyCount-i);
        QIcon icon = BrowserApplication::instance()->icon(item.url());
        action->setIcon(icon);
        action->setText(item.title());
        m_historyForwardMenu->addAction(action);
    }
}

void BrowserMainWindow::slotAboutToShowWindowMenu()
{
    m_windowMenu->clear();
    m_windowMenu->addAction(m_tabWidget->nextTabAction());
    m_windowMenu->addAction(m_tabWidget->previousTabAction());
    m_windowMenu->addSeparator();
    m_windowMenu->addAction(tr("Downloads"), this, SLOT(slotDownloadManager()), QKeySequence(tr("Alt+Ctrl+L", "Download Manager")));
    m_windowMenu->addSeparator();

    QList<BrowserMainWindow*> windows = BrowserApplication::instance()->mainWindows();
    for (int i = 0; i < windows.count(); ++i) {
        BrowserMainWindow *window = windows.at(i);
        QAction *action = m_windowMenu->addAction(window->windowTitle(), this, SLOT(slotShowWindow()));
        action->setData(i);
        action->setCheckable(true);
        if (window == this)
            action->setChecked(true);
    }
}

void BrowserMainWindow::slotShowWindow()
{
    if (QAction *action = qobject_cast<QAction*>(sender())) {
        QVariant v = action->data();
        if (v.canConvert<int>()) {
            int offset = qvariant_cast<int>(v);
            QList<BrowserMainWindow*> windows = BrowserApplication::instance()->mainWindows();
            windows.at(offset)->activateWindow();
            windows.at(offset)->currentTab()->setFocus();
        }
    }
}

void BrowserMainWindow::slotOpenActionUrl(QAction *action)
{
    int offset = action->data().toInt();
    QWebEngineHistory *history = currentTab()->history();
    if (offset < 0)
        history->goToItem(history->backItems(-1*offset).first()); // back
    else if (offset > 0)
        history->goToItem(history->forwardItems(history->count() - offset + 1).back()); // forward
}

void BrowserMainWindow::geometryChangeRequested(const QRect &geometry)
{
    setGeometry(geometry);
}
//返回值：true----删除成功；false----文件夹不存在
//备注：无*/
bool BrowserMainWindow::removeFolderContent(const QString &folderDir)
{
    QDir dir(folderDir);
    QFileInfoList fileList;
    QFileInfo curFile;
    if(!dir.exists())  {return false;}//文件不存，则返回false
    fileList=dir.entryInfoList(QDir::Dirs|QDir::Files
                               |QDir::Readable|QDir::Writable
                               |QDir::Hidden|QDir::NoDotAndDotDot
                               ,QDir::Name);
    if(fileList.size()>0)//跳出条件
    {
        int infoNum=fileList.size();
        for(int i=infoNum-1;i>=0;i--)
        {
            curFile=fileList[i];
            if(curFile.isFile())//如果是文件，删除文件
            {
                QFile fileTemp(curFile.filePath());
                fileTemp.remove();
                fileList.removeAt(i);
            }
            if(curFile.isDir())//如果是文件夹
            {
                QDir dirTemp(curFile.filePath());
                QFileInfoList fileList1=dirTemp.entryInfoList(QDir::Dirs|QDir::Files
                                                              |QDir::Readable|QDir::Writable
                                                              |QDir::Hidden|QDir::NoDotAndDotDot
                                                              ,QDir::Name);
                if(fileList1.size()==0)//下层没有文件或文件夹
                {
                    dirTemp.rmdir(".");
                    fileList.removeAt(i);
                }
                else//下层有文件夹或文件
                {
                    for(int j=0;j<fileList1.size();j++)
                    {
                        if(!(fileList.contains(fileList1[j])))
                            fileList.append(fileList1[j]);
                    }
                }
            }
        }
    }
   return true;
}


void BrowserMainWindow::dragEnterEvent(QDragEnterEvent *event)
{
   if(event->mimeData()->hasFormat("text/uri-list"))
    {
         event->acceptProposedAction();
    }
}

void BrowserMainWindow::dropEvent(QDropEvent *event)
{
    QList<QUrl> urls = event->mimeData()->urls();
    if (urls.isEmpty()) {
        return;
    }
    foreach (QUrl ul, urls) {
        QString fileName =ul.toLocalFile();
        if (fileName.isEmpty()) {
            return;
        }
        QFileInfo fi(fileName);
        if(fi.isFile() && fi.exists())
        {

                createNewTab();
                openFile(fileName);

        }
    }


}
