#ifndef YPREFS_H
#define YPREFS_H

#include "yconfig.h"
#include "fontmacro.h"

XIV(bool, dontRotateMenuPointer,                true)
XIV(bool, fontPreferFreetype,                   true)
XIV(bool, menuMouseTracking,                    false)
XIV(bool, replayMenuCancelClick,                false)
XIV(bool, showPopupsAbovePointer,               false)
XIV(bool, showEllipsis,                         true)
XIV(bool, rightToLeft,                          false)
XIV(bool, leftToRight,                          true)
#ifdef CONFIG_I18N
XIV(bool, multiByte,                            true)
#endif
XIV(bool, modSuperIsCtrlAlt,                    false)
XIV(bool, xrrDisable,                           false)
XIV(int, xineramaPrimaryScreen,                 0)
XIV(bool, taskBarShowOnAllMonitors,             false)
XIV(bool, taskBarWindowsHomeScreenOnly,         false)
XIV(int, newWindowScreenPolicy,                 0)
XIV(int, MenuActivateDelay,                     40)
XIV(int, SubmenuActivateDelay,                  300)
extern int DelayFuzziness;
XIV(int, ClickMotionDistance,                   4)
XIV(int, ClickMotionDelay,                      200)
XIV(int, MultiClickTime,                        400)
XIV(int, autoScrollStartDelay,                  500)
XIV(int, autoScrollDelay,                       60)
XIV(int, ToolTipDelay,                          500)
XIV(int, ToolTipTime,                           0)
XIV(bool, ToolTipIcon,                          true)

///#warning "move this one back to WM"
XIV(bool, grabRootWindow,                       true)

XSV(const char *, iconPath,
                                                "/usr/local/share/icons:"
                                                "/usr/local/share/pixmaps:"
                                                "/usr/share/icons:"
                                                "/usr/share/pixmaps:"
                                                )
XSV(const char *, iconThemes,                   "*:-HighContrast")
XSV(const char *, themeName,                    CONFIG_DEFAULT_THEME)
XSV(const char *, xineramaPrimaryScreenName,    0)

// Per-widget monitor selection for the "singleton" taskbar applets (see
// MULTITASKBAR-PLAN.md, D2/Piece 3). Each is empty by default, meaning
// "follow XineramaPrimaryScreen/XRRPrimaryScreenName as before"; resolved
// via YDesktop::screenFromDescriptor(), so a number or an XRandR output
// name both work, and an unplugged/invalid value falls back to primary.
XSV(const char *, taskBarMailboxScreen,         0)
XSV(const char *, taskBarCPUStatusScreen,       0)
XSV(const char *, taskBarMEMStatusScreen,       0)
XSV(const char *, taskBarNetStatusScreen,       0)
XSV(const char *, taskBarAPMScreen,             0)
XSV(const char *, taskBarKeyboardScreen,        0)
XSV(const char *, taskBarSystemTrayScreen,      0)
XSV(const char *, taskBarAddressBarScreen,      0)

enum WMLook {
    lookWin95  = 1 << 0,
    lookMotif  = 1 << 1,
    lookWarp3  = 1 << 2,
    lookWarp4  = 1 << 3,
    lookNice   = 1 << 4,
    lookPixmap = 1 << 5,
    lookMetal  = 1 << 6,
    lookGtk    = 1 << 7,
    lookFlat   = 1 << 8,
};
#define LOOK(looks)             (((looks) & wmLook) != 0)
#define CONFIG_DEFAULT_LOOK     lookNice
XIV(WMLook, wmLook,                             CONFIG_DEFAULT_LOOK)

XSV(const char *, clrToolTip,                   "rgb:E0/E0/00")
XSV(const char *, clrToolTipText,               "rgb:00/00/00")
XFV(const char *, toolTipFontName,              FONT(12,120), FSANS)

#endif

// vim: set sw=4 ts=4 et:
