/*
 * tkPlatDecls.h --
 *
 *	Declarations of functions in the platform-specific public Tcl API.
 *
 * Copyright (c) 1998-1999 by Scriptics Corporation.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#ifndef _TKPLATDECLS
#define _TKPLATDECLS

#ifdef BUILD_tk
#undef TCL_STORAGE_CLASS
#define TCL_STORAGE_CLASS DLLEXPORT
#endif

/*
 * WARNING: This file is automatically generated by the tools/genStubs.tcl
 * script.  Any modifications to the function declarations below should be made
 * in the generic/tk.decls script.
 */

/* !BEGIN!: Do not edit below this line. */

/*
 * Exported function declarations:
 */

#if defined(__WIN32__) || defined(__CYGWIN__) /* WIN */
#ifndef Tk_AttachHWND_TCL_DECLARED
#define Tk_AttachHWND_TCL_DECLARED
/* 0 */
EXTERN Window		Tk_AttachHWND(Tk_Window tkwin, HWND hwnd);
#endif
#ifndef Tk_GetHINSTANCE_TCL_DECLARED
#define Tk_GetHINSTANCE_TCL_DECLARED
/* 1 */
EXTERN HINSTANCE	Tk_GetHINSTANCE(void);
#endif
#ifndef Tk_GetHWND_TCL_DECLARED
#define Tk_GetHWND_TCL_DECLARED
/* 2 */
EXTERN HWND		Tk_GetHWND(Window window);
#endif
#ifndef Tk_HWNDToWindow_TCL_DECLARED
#define Tk_HWNDToWindow_TCL_DECLARED
/* 3 */
EXTERN Tk_Window	Tk_HWNDToWindow(HWND hwnd);
#endif
#ifndef Tk_PointerEvent_TCL_DECLARED
#define Tk_PointerEvent_TCL_DECLARED
/* 4 */
EXTERN void		Tk_PointerEvent(HWND hwnd, int x, int y);
#endif
#ifndef Tk_TranslateWinEvent_TCL_DECLARED
#define Tk_TranslateWinEvent_TCL_DECLARED
/* 5 */
EXTERN int		Tk_TranslateWinEvent(HWND hwnd, UINT message,
				WPARAM wParam, LPARAM lParam,
				LRESULT *result);
#endif
#endif /* WIN */
#ifdef MAC_OSX_TK /* AQUA */
#ifndef Tk_MacOSXSetEmbedHandler_TCL_DECLARED
#define Tk_MacOSXSetEmbedHandler_TCL_DECLARED
/* 0 */
EXTERN void		Tk_MacOSXSetEmbedHandler(
				Tk_MacOSXEmbedRegisterWinProc *registerWinProcPtr,
				Tk_MacOSXEmbedGetGrafPortProc *getPortProcPtr,
				Tk_MacOSXEmbedMakeContainerExistProc *containerExistProcPtr,
				Tk_MacOSXEmbedGetClipProc *getClipProc,
				Tk_MacOSXEmbedGetOffsetInParentProc *getOffsetProc);
#endif
#ifndef Tk_MacOSXTurnOffMenus_TCL_DECLARED
#define Tk_MacOSXTurnOffMenus_TCL_DECLARED
/* 1 */
EXTERN void		Tk_MacOSXTurnOffMenus(void);
#endif
#ifndef Tk_MacOSXTkOwnsCursor_TCL_DECLARED
#define Tk_MacOSXTkOwnsCursor_TCL_DECLARED
/* 2 */
EXTERN void		Tk_MacOSXTkOwnsCursor(int tkOwnsIt);
#endif
#ifndef TkMacOSXInitMenus_TCL_DECLARED
#define TkMacOSXInitMenus_TCL_DECLARED
/* 3 */
EXTERN void		TkMacOSXInitMenus(Tcl_Interp *interp);
#endif
#ifndef TkMacOSXInitAppleEvents_TCL_DECLARED
#define TkMacOSXInitAppleEvents_TCL_DECLARED
/* 4 */
EXTERN void		TkMacOSXInitAppleEvents(Tcl_Interp *interp);
#endif
#ifndef TkGenWMConfigureEvent_TCL_DECLARED
#define TkGenWMConfigureEvent_TCL_DECLARED
/* 5 */
EXTERN void		TkGenWMConfigureEvent(Tk_Window tkwin, int x, int y,
				int width, int height, int flags);
#endif
#ifndef TkMacOSXInvalClipRgns_TCL_DECLARED
#define TkMacOSXInvalClipRgns_TCL_DECLARED
/* 6 */
EXTERN void		TkMacOSXInvalClipRgns(Tk_Window tkwin);
#endif
#ifndef TkMacOSXGetDrawablePort_TCL_DECLARED
#define TkMacOSXGetDrawablePort_TCL_DECLARED
/* 7 */
EXTERN VOID *		TkMacOSXGetDrawablePort(Drawable drawable);
#endif
#ifndef TkMacOSXGetRootControl_TCL_DECLARED
#define TkMacOSXGetRootControl_TCL_DECLARED
/* 8 */
EXTERN VOID *		TkMacOSXGetRootControl(Drawable drawable);
#endif
#ifndef Tk_MacOSXSetupTkNotifier_TCL_DECLARED
#define Tk_MacOSXSetupTkNotifier_TCL_DECLARED
/* 9 */
EXTERN void		Tk_MacOSXSetupTkNotifier(void);
#endif
#ifndef Tk_MacOSXIsAppInFront_TCL_DECLARED
#define Tk_MacOSXIsAppInFront_TCL_DECLARED
/* 10 */
EXTERN int		Tk_MacOSXIsAppInFront(void);
#endif
#endif /* AQUA */

typedef struct TkPlatStubs {
    int magic;
    struct TkPlatStubHooks *hooks;

#if defined(__WIN32__) || defined(__CYGWIN__) /* WIN */
    Window (*tk_AttachHWND) (Tk_Window tkwin, HWND hwnd); /* 0 */
    HINSTANCE (*tk_GetHINSTANCE) (void); /* 1 */
    HWND (*tk_GetHWND) (Window window); /* 2 */
    Tk_Window (*tk_HWNDToWindow) (HWND hwnd); /* 3 */
    void (*tk_PointerEvent) (HWND hwnd, int x, int y); /* 4 */
    int (*tk_TranslateWinEvent) (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT *result); /* 5 */
#endif /* WIN */
#ifdef MAC_OSX_TK /* AQUA */
    void (*tk_MacOSXSetEmbedHandler) (Tk_MacOSXEmbedRegisterWinProc *registerWinProcPtr, Tk_MacOSXEmbedGetGrafPortProc *getPortProcPtr, Tk_MacOSXEmbedMakeContainerExistProc *containerExistProcPtr, Tk_MacOSXEmbedGetClipProc *getClipProc, Tk_MacOSXEmbedGetOffsetInParentProc *getOffsetProc); /* 0 */
    void (*tk_MacOSXTurnOffMenus) (void); /* 1 */
    void (*tk_MacOSXTkOwnsCursor) (int tkOwnsIt); /* 2 */
    void (*tkMacOSXInitMenus) (Tcl_Interp *interp); /* 3 */
    void (*tkMacOSXInitAppleEvents) (Tcl_Interp *interp); /* 4 */
    void (*tkGenWMConfigureEvent) (Tk_Window tkwin, int x, int y, int width, int height, int flags); /* 5 */
    void (*tkMacOSXInvalClipRgns) (Tk_Window tkwin); /* 6 */
    VOID * (*tkMacOSXGetDrawablePort) (Drawable drawable); /* 7 */
    VOID * (*tkMacOSXGetRootControl) (Drawable drawable); /* 8 */
    void (*tk_MacOSXSetupTkNotifier) (void); /* 9 */
    int (*tk_MacOSXIsAppInFront) (void); /* 10 */
#endif /* AQUA */
} TkPlatStubs;

#ifdef __cplusplus
extern "C" {
#endif
extern TkPlatStubs *tkPlatStubsPtr;
#ifdef __cplusplus
}
#endif

#if defined(USE_TK_STUBS) && !defined(USE_TK_STUB_PROCS)

/*
 * Inline function declarations:
 */

#if defined(__WIN32__) || defined(__CYGWIN__) /* WIN */
#ifndef Tk_AttachHWND
#define Tk_AttachHWND \
	(tkPlatStubsPtr->tk_AttachHWND) /* 0 */
#endif
#ifndef Tk_GetHINSTANCE
#define Tk_GetHINSTANCE \
	(tkPlatStubsPtr->tk_GetHINSTANCE) /* 1 */
#endif
#ifndef Tk_GetHWND
#define Tk_GetHWND \
	(tkPlatStubsPtr->tk_GetHWND) /* 2 */
#endif
#ifndef Tk_HWNDToWindow
#define Tk_HWNDToWindow \
	(tkPlatStubsPtr->tk_HWNDToWindow) /* 3 */
#endif
#ifndef Tk_PointerEvent
#define Tk_PointerEvent \
	(tkPlatStubsPtr->tk_PointerEvent) /* 4 */
#endif
#ifndef Tk_TranslateWinEvent
#define Tk_TranslateWinEvent \
	(tkPlatStubsPtr->tk_TranslateWinEvent) /* 5 */
#endif
#endif /* WIN */
#ifdef MAC_OSX_TK /* AQUA */
#ifndef Tk_MacOSXSetEmbedHandler
#define Tk_MacOSXSetEmbedHandler \
	(tkPlatStubsPtr->tk_MacOSXSetEmbedHandler) /* 0 */
#endif
#ifndef Tk_MacOSXTurnOffMenus
#define Tk_MacOSXTurnOffMenus \
	(tkPlatStubsPtr->tk_MacOSXTurnOffMenus) /* 1 */
#endif
#ifndef Tk_MacOSXTkOwnsCursor
#define Tk_MacOSXTkOwnsCursor \
	(tkPlatStubsPtr->tk_MacOSXTkOwnsCursor) /* 2 */
#endif
#ifndef TkMacOSXInitMenus
#define TkMacOSXInitMenus \
	(tkPlatStubsPtr->tkMacOSXInitMenus) /* 3 */
#endif
#ifndef TkMacOSXInitAppleEvents
#define TkMacOSXInitAppleEvents \
	(tkPlatStubsPtr->tkMacOSXInitAppleEvents) /* 4 */
#endif
#ifndef TkGenWMConfigureEvent
#define TkGenWMConfigureEvent \
	(tkPlatStubsPtr->tkGenWMConfigureEvent) /* 5 */
#endif
#ifndef TkMacOSXInvalClipRgns
#define TkMacOSXInvalClipRgns \
	(tkPlatStubsPtr->tkMacOSXInvalClipRgns) /* 6 */
#endif
#ifndef TkMacOSXGetDrawablePort
#define TkMacOSXGetDrawablePort \
	(tkPlatStubsPtr->tkMacOSXGetDrawablePort) /* 7 */
#endif
#ifndef TkMacOSXGetRootControl
#define TkMacOSXGetRootControl \
	(tkPlatStubsPtr->tkMacOSXGetRootControl) /* 8 */
#endif
#ifndef Tk_MacOSXSetupTkNotifier
#define Tk_MacOSXSetupTkNotifier \
	(tkPlatStubsPtr->tk_MacOSXSetupTkNotifier) /* 9 */
#endif
#ifndef Tk_MacOSXIsAppInFront
#define Tk_MacOSXIsAppInFront \
	(tkPlatStubsPtr->tk_MacOSXIsAppInFront) /* 10 */
#endif
#endif /* AQUA */

#endif /* defined(USE_TK_STUBS) && !defined(USE_TK_STUB_PROCS) */

/* !END!: Do not edit above this line. */

#undef TCL_STORAGE_CLASS
#define TCL_STORAGE_CLASS DLLIMPORT

#endif /* _TKPLATDECLS */
