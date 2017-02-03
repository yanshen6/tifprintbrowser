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

#ifndef BROWSERMAINWINDOW_H
#define BROWSERMAINWINDOW_H

#include <QtWidgets/QMainWindow>
#include <QtGui/QIcon>
#include <QtCore/QUrl>
#define __USE_BOOKMARK__ 0
#define __USE_TOOBAR__ 0
#define __USE_MENUEBAR__ 0
QT_BEGIN_NAMESPACE
class QWebEngineFrame;
QT_END_NAMESPACE

class AutoSaver;
class BookmarksToolBar;
class ChaseWidget;
class TabWidget;
class ToolbarSearch;
class WebView;
class QPanel;
class QPushButton;
/*!
    The MainWindow of the Browser Application.

    Handles the tab widget and all the actions
 */
class BrowserMainWindow : public QMainWindow {
    Q_OBJECT

public:
    BrowserMainWindow(QWidget *parent = 0, Qt::WindowFlags flags = 0);
    ~BrowserMainWindow();
    QSize sizeHint() const;

    static const char *defaultHome;

public:
    TabWidget *tabWidget() const;
    WebView *currentTab() const;
    QByteArray saveState(bool withTabs = true) const;
    bool restoreState(const QByteArray &state);
    Q_INVOKABLE void runScriptOnOpenViews(const QString &);
    void openFile(QString filePath);
    void createNewTab();
    QMenu* getSysMenu();
    bool isTopbarVisible();
public slots:
    void loadPage(const QString &url);
    void slotHome();

protected:
    void closeEvent(QCloseEvent *event);
    bool event(QEvent *event) ;

    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
private slots:
    void save();

    void slotLoadProgress(int);
    void slotUpdateStatusbar(const QString &string);
    void slotUpdateWindowTitle(const QString &title = QString());

    void loadUrl(const QUrl &url);
    void loadUrlEx(const QUrl &url,const QString &title);
    void slotPreferences();
    void slotClearPrivacy();
    void slotFileNew();
    void slotFileOpen();
    void slotFilePrintPreview();
    void slotFilePrint();
//    void slotPrivateBrowsing();
    void slotFileSaveAs();
    void slotEditFind();
    void slotEditFindNext();
    void slotEditFindPrevious();

    void slotShowBookmarksDialog();
    void slotAddBookmark();
    static void   slotAddBookmark2(void* pthis);
    void slotViewZoomIn();
    void slotViewZoomOut();
    void slotViewResetZoom();
    void slotViewToolbar();
#if __USE_BOOKMARK__
    void slotViewBookmarksBar();
#endif
    void slotViewStatusbar();
    void slotViewPageSource();
    void slotViewFullScreen(bool enable);

    void slotWebSearch();
    void slotToggleInspector(bool enable);
    void slotAboutApplication();
    void slotDownloadManager();
    void slotSelectLineEdit();

    void slotAboutToShowBackMenu();
    void slotAboutToShowForwardMenu();
    void slotAboutToShowWindowMenu();
    void slotOpenActionUrl(QAction *action);
    void slotShowWindow();
    void slotSwapFocus();
    void slotPushHistroyBackActionUrl();
    void slotPushHistroyForwardActionUrl();
    void slotPushHomeActionUrl();
    void slotPushRefreshActionUrl();
#if defined(QWEBENGINEPAGE_PRINT)
    void printRequested(QWebEngineFrame *frame);
#endif
    void geometryChangeRequested(const QRect &geometry);
    void updateToolbarActionText(bool visible);
#if __USE_BOOKMARK__
    void updateBookmarksToolbarActionText(bool visible);
#endif
private:
    void loadDefaultState();
    void setupMenu();
    void setupToolBar();
    QWidget * setupToolBarExt();
    void updateStatusbarActionText(bool visible);
    void handleFindTextResult(bool found);

    void setupMyToolBar(QPanel *pParent);
    bool removeFolderContent(const QString &folderDir);
    QMenu * createSysMenu();
private:
    QToolBar *m_navigationBar;

    ToolbarSearch *m_toolbarSearch;
#if __USE_BOOKMARK__
    BookmarksToolBar *m_bookmarksToolbar;
#endif
    ChaseWidget *m_chaseWidget;
    TabWidget *m_tabWidget;
    AutoSaver *m_autoSaver;

    QAction *m_historyBack;
    QMenu *m_historyBackMenu;
    QAction *m_historyForward;
    QMenu *m_historyForwardMenu;
    QMenu *m_windowMenu;

    QAction *m_stop;
    QAction *m_reload;
    QAction *m_stopReload;
    QAction *m_viewToolbar;

    QAction *m_viewStatusbar;
    QAction *m_restoreLastSession;
#if __USE_BOOKMARK__
    QAction *m_viewBookmarkBar;
#endif
    QAction *m_addBookmark;

    QIcon m_reloadIcon;
    QIcon m_stopIcon;

    QPixmap m_reloadPixmap;
   // QPixmap m_stopPixmap;
    QPushButton *m_historyBackBtn;
    QPushButton *m_historyForwardBtn;
    QPushButton *m_homeBtn;
    QPushButton *m_refreshBtn;
    QPushButton *m_lineBtn;
    QPushButton *m_dotBtn;

    QString m_lastSearch;
    friend class BrowserApplication;
    QWidget *m_topWidget ;
    QMenu *  m_sysMenu;
};

#endif // BROWSERMAINWINDOW_H
