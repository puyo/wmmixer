// wmmixer - A mixer designed for WindowMaker
//
// Release 1.5
// Copyright (C) 1998  Sam Hawker <shawkie@geocities.com>
// Copyright (C) 2002 Gordon Fraser <gordon@debian.org>
// This software comes with ABSOLUTELY NO WARRANTY
// This software is free software, and you are welcome to redistribute it
// under certain conditions
// See the COPYING file for details.


#ifndef __xhandler_h__
#define __xhandler_h__

//--------------------------------------------------------------------

// X-Windows includes - standard
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/xpm.h>
#include <X11/extensions/shape.h>

#include <iostream>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include "common.h"

// Xpm images - standard
#include "XPM/wmmixer.xpm"
#include "XPM/tile.xpm"

// Xpm images - custom
#include "XPM/icons.xpm"
#include "XPM/norec.xpm"

//--------------------------------------------------------------------
#define BTN_LEFT_X 5
#define BTN_LEFT_Y 34

#define BTN_RIGHT_X 17
#define BTN_RIGHT_Y 34

#define BTN_MUTE_X 17
#define BTN_MUTE_Y 47

#define BTN_REC_X 5
#define BTN_REC_Y 47

#define BTN_WIDTH  12
#define BTN_HEIGHT 11

#define BTNNEXT  1
#define BTNPREV  2
#define BTNREC   4
#define BTNMUTE  8


//--------------------------------------------------------------------
class XHandler {
public:
    XHandler();
    virtual  ~XHandler();
    void init(int, char**, int);

    void repaint();
    void update(unsigned);
    void drawLeft(unsigned);
    void drawRight(unsigned);
    void drawBtns(int, bool);
    void drawMono(unsigned);

    bool isLeftButton(int, int);
    bool isRightButton(int, int);
    bool isMuteButton(int, int);
    bool isRecButton(int, int);
    bool isVolumeBar(int, int);

    Display* getDisplay() { return display_default; }
    int  getButtonState();
    void setButtonState(int);
    void setDisplay(char* arg);
    void setPosition(char* arg);
    void setLedColor(char* arg);
    void setLedHighColor(char* arg);
    void setBackColor(char* arg);
    void setUnshaped();
    void setWindowMaker();
    void setAfterStep();
    int  getWindowSize();
    Atom getDeleteWin();
    void setIcon(int chan, int icon);

private:
    int button_state;
    int window_size;

    bool is_wmaker;
    bool is_ushape;
    bool is_astep;

    unsigned *icon_list;

    Display *display_default;
    Window window_icon;
    Window window_main;
    Window window_root;

    GC graphics_context;
    unsigned long colors[4];
    unsigned long shade_colors[25];

    char display_name[256];
    char position_name[256];
    char ledcolor_name[256];
    char ledcolor_high_name[256];
    char backcolor_name[256];

    Pixel back_pix;
    Pixel fore_pix;

    // Pixmaps - standard
    Pixmap pixmap_main;
    Pixmap pixmap_tile;
    Pixmap pixmap_disp;
    Pixmap pixmap_mask;

    // Pixmaps - custom
    Pixmap pixmap_icon;
    Pixmap pixmap_nrec;

    // X-Windows basics - standard
    Atom _XA_GNUSTEP_WM_FUNC;
    Atom deleteWin;

    unsigned long getColor(char*);
    unsigned long mixColor(char*, int, char*, int);
    void drawButton(int, int, int, int, bool);
    void initPixmaps(int);
    void initWindow(int, char**);
    void initGraphicsContext();
    void initMask();
    void initColors();
    void initIcons(int);
    int flush_expose(Window);
};

#endif //__xhandler_h__
