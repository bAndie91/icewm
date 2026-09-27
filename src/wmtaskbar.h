#ifndef TASKBAR_H
#define TASKBAR_H

#include "yaction.h"
#include "ytimer.h"
#include "wmclient.h"
#include "yxtray.h"
#include "applet.h"
#include "yarray.h"

class ObjectBar;
class ObjectButton;
class MEMStatus;
class CPUStatusControl;
class NetStatusControl;
class AddressBar;
class KeyboardStatus;
class MailBoxControl;
class MailBoxStatus;
class YButton;
class ClockSet;
class YApm;
class TaskBarMenu;
class TaskPane;
class TrayPane;
class AWorkspaces;
class WorkspacesPane;
class YXTray;
class YSMListener;
class TaskBar;
class TaskBarApp;
class TrayApp;

class EdgeTrigger: public YDndWindow, public YTimerListener {
public:
    EdgeTrigger(TaskBar *owner);
    virtual ~EdgeTrigger();

    static bool enabled();
    void show(bool enable);
    enum HideOrShow { Hide, Show };
    void startTimer(HideOrShow = Hide);
    void stopTimer();

    virtual void handleDNDEnter();
    virtual void handleDNDLeave();

    virtual void handleCrossing(const XCrossingEvent &crossing);
    virtual bool handleTimer(YTimer *t);
private:
    TaskBar *fTaskBar;
    lazy<YTimer> fAutoHideTimer;
    HideOrShow fHideOrShow;
};

class TaskBar:
    public YFrameClient,
    public YActionListener,
    public YPopDownListener,
    public YXTrayNotifier,
    public IAppletContainer
{
public:
    // screen: the Xinerama/RandR monitor index this instance belongs to
    // and positions itself on (see getScreenGeometry()).
    TaskBar(IApp *app, YWindow *aParent, YActionListener *wmActionListener,
            YSMListener *smActionListener, int screen);
    virtual ~TaskBar();

    int screen() const { return fScreen; }

    // Is `client` any of our TaskBar instances (as a plain YFrameClient*),
    // and if so, which one. Replaces the old single-taskbar `client() ==
    // taskBar` / `!= taskBar` idiom used throughout wmmgr.cc/wmframe.cc.
    static TaskBar* whichTaskBar(const YFrameClient* client);
    static bool isTaskBar(const YFrameClient* client) {
        return whichTaskBar(client) != nullptr;
    }

    // True if this instance should host a "singleton" applet configured
    // with the given monitor-descriptor preference (one of the
    // TaskBar*Screen prefs, e.g. taskBarMailboxScreen): always true when
    // there's only one bar (TaskBarShowOnAllMonitors off) so a stray
    // *Screen setting can never leave a singleton widget homeless;
    // otherwise true only on the instance matching screenFromDescriptor().
    bool hostsSingleton(const char* screenPref) const;

    // Broadcast helpers for the widgets that are replicated across every
    // TaskBar instance (workspace pager, toolbar, collapse) and for
    // whole-desktop lifecycle events (screen change, idle relayout).
    // Each is a thin loop over `taskBars`, safe to call even when empty.
    // See MULTITASKBAR-PLAN.md, Piece 2.
    static void workspacesRepaintAll(long workspace);
    static void workspacesUpdateButtonsAll();
    static void workspacesRelabelButtonsAll();
    static void setWorkspaceActiveAll(long workspace, bool active);
    static void updateFullscreenAll();
    static void relayoutAll();
    static void relayoutNowAll();
    static void refreshAll();
    static void updateLocationAll();
    static void initToolbarAll();
    static void handleCollapseButtonAll();

private:
    virtual void paint(Graphics &g, const YRect &r);
    virtual bool handleKey(const XKeyEvent &key);
    virtual void handleButton(const XButtonEvent &button);
    virtual void handleClick(const XButtonEvent &up, int count);
    virtual void handleDrag(const XButtonEvent &down, const XMotionEvent &motion);
    virtual void handleEndDrag(const XButtonEvent &down, const XButtonEvent &up);
    virtual void handleFocus(const XFocusChangeEvent& focus);
    virtual void handleCrossing(const XCrossingEvent &crossing);
    virtual void handleExpose(const XExposeEvent &expose) {}
    virtual void handleClientMessage(const XClientMessageEvent& message);
    virtual void actionPerformed(YAction action, unsigned int modifiers);
    virtual void handlePopDown(YPopupWindow *popup);

    void updateWMHints();
    void updateWinLayer();
    virtual void configure(const YRect2 &r);
    virtual void repaint();

public:
    bool windowTrayRequestDock(Window w);
    void setWorkspaceActive(int workspace, bool active);
    void workspacesRepaint(int workspace);
    void workspacesUpdateButtons();
    void workspacesRelabelButtons();
    void keyboardUpdate(mstring keyboard);

    void updateFrame(YFrameWindow* frame);
    void delistFrame(YFrameWindow* frame, TaskBarApp* task, TrayApp* tray);
    void removeTasksApp(YFrameWindow* frame);
    TaskBarApp* addTasksApp(YFrameWindow* frame);
    void relayoutTasks();
    void relayoutTray();
    TrayApp* addTrayApp(YFrameWindow* frame);
    void removeTrayApp(YFrameWindow* frame);

    void popupStartMenu();
    void popupWindowListMenu();

    void initToolbar();
    void showAddressBar();
    void showBar();
    void handleCollapseButton();

    void relayout() { fNeedRelayout = true; }
    void relayoutNow();
    void updateLocation();

    void detachDesktopTray();
    bool isCollapsed() const { return fIsCollapsed; }
    bool hidden() const { return fIsCollapsed | fIsHidden | !getFrame(); }
    bool autoTimer(bool show);
    void updateFullscreen();
    Window edgeTriggerWindow() { return fEdgeTrigger->handle(); }
    void switchToPrev();
    void switchToNext();
    void movePrev();
    void moveNext();
    void refresh();

private:
    void popOut();
    void obtainFocus();

    AddressBar *addressBar() const { return fAddressBar; }
    TaskPane *taskPane() const { return fTasks; }
    TrayPane *windowTrayPane() const { return fWindowTray; }

    virtual ref<YImage> getGradient() { return fGradient; }
    const YSurface& getSurface() const { return fSurface; }

    void contextMenu(int x_root, int y_root);
    void buttonUpdate();
    void trayChanged();
    YXTray *netwmTray() { return fDesktopTray; }

    void initApplets();
    void updateLayout(unsigned& size_w, unsigned& size_h);

private:
    int fScreen;
    YSurface fSurface;
    TaskPane *fTasks;

    ObjectButton *fCollapseButton;
    TrayPane *fWindowTray;
    ClockSet* fClock;
    KeyboardStatus *fKeyboardStatus;
    MailBoxControl *fMailBoxControl;
    MEMStatus *fMEMStatus;
    CPUStatusControl *fCPUStatus;
    YApm *fApm;
    NetStatusControl *fNetStatus;

    ObjectBar *fObjectBar;
    ObjectButton *fApplications;
    ObjectButton *fWinList;
    ObjectButton *fShowDesktop;
    AddressBar *fAddressBar;
    AWorkspaces *fWorkspaces;
    YXTray *fDesktopTray;
    EdgeTrigger *fEdgeTrigger;
    YActionListener *wmActionListener;
    YSMListener *smActionListener;
    IApp *app;

    lazy<TaskBarMenu> taskBarMenu;
    ref<YImage> fGradient;
    YArray<YFrameWindow*> fUpdates;

    bool fIsHidden;
    bool fFullscreen;
    bool fIsCollapsed;
    bool fMenuShown;
    bool fNeedRelayout;
    bool fButtonUpdate;
    bool fWorkspacesUpdate;

    class YStrut {
    public:
        Atom left, right, top, bottom;
        // _NET_WM_STRUT_PARTIAL's extra reach-along-the-edge fields, so a
        // bar on one monitor doesn't claim margin along the whole virtual
        // desktop width/height for external (non-icewm) EWMH readers.
        // icewm's own workarea calc already scopes struts per-screen via
        // YFrameWindow::getScreen() regardless of these (see
        // MULTITASKBAR-PLAN.md, Piece 6), but setting them properly keeps
        // the advertised property correct for other tools too.
        Atom top_start_x, top_end_x, bottom_start_x, bottom_end_x;
        YStrut() : left(0), right(0), top(0), bottom(0),
                   top_start_x(0), top_end_x(0),
                   bottom_start_x(0), bottom_end_x(0) { }
        bool operator!=(const YStrut& s) const {
            return left != s.left || right != s.right
                || top != s.top || bottom != s.bottom
                || top_start_x != s.top_start_x
                || top_end_x != s.top_end_x
                || bottom_start_x != s.bottom_start_x
                || bottom_end_x != s.bottom_end_x;
        }
        const Atom* operator&() const { return &left; }
        bool operator*() const { return left | right | top | bottom; }
    } fStrut;
};

extern TaskBar *taskBar; // !!! get rid of this
// All live TaskBar instances (one, unless TaskBarShowOnAllMonitors is set,
// in which case there is one per active monitor). `taskBar` above always
// points at one of these (the primary-screen instance when there is a
// choice) for source compatibility with code not yet updated to be
// screen-aware; see the multi-taskbar plan doc for the migration status.
extern YArray<TaskBar*> taskBars;

extern YColorName taskBarBg;

#endif

// vim: set sw=4 ts=4 et:
