// wmmixer - A mixer designed for WindowMaker
//
// Release 1.5
// Copyright (C) 1998  Sam Hawker <shawkie@geocities.com>
// Copyright (C) 2002 Gordon Fraser <gordon@debian.org>
// This software comes with ABSOLUTELY NO WARRANTY
// This software is free software, and you are welcome to redistribute it
// under certain conditions
// See the COPYING file for details.

#include "xhandler.h"
#include "mixer.h" // SOUND_VERSION

//--------------------------------------------------------------------
XHandler::XHandler() {
    is_wmaker = WINDOWMAKER;
    is_ushape = USESHAPE;
    is_astep  = AFTERSTEP;

    strcpy(display_name, "");
    strcpy(position_name, "");
    strcpy(ledcolor_name, LEDCOLOR);
    strcpy(ledcolor_high_name, LEDCOLOR_HIGH);
    strcpy(backcolor_name, BACKCOLOR);

    button_state = 0;
}

//--------------------------------------------------------------------
XHandler::~XHandler() {
    XFreeGC(display_default, graphics_context);
    XFreePixmap(display_default, pixmap_main);
    XFreePixmap(display_default, pixmap_tile);
    XFreePixmap(display_default, pixmap_disp);
    XFreePixmap(display_default, pixmap_mask);
    XFreePixmap(display_default, pixmap_icon);
    XFreePixmap(display_default, pixmap_nrec);

    XDestroyWindow(display_default, window_main);

    if (is_wmaker)
        XDestroyWindow(display_default, window_icon);

    XCloseDisplay(display_default);
}


//--------------------------------------------------------------------
void XHandler::init(int argc, char** argv) {
    int display_depth;
    window_size = is_astep ? ASTEPSIZE : NORMSIZE;
    if ((display_default = XOpenDisplay(display_name))==NULL) {
        std::cerr <<  NAME << " : Unable to open X display '" << XDisplayName(display_name) << "'." << std::endl;
        exit(1);
    }
    initWindow(argc, argv);
    initColors();
    display_depth = DefaultDepth(display_default, DefaultScreen(display_default));
    initPixmaps(display_depth);
    initGraphicsContext();
    initMask();
    initIcons();
}

//--------------------------------------------------------------------
bool XHandler::isLeftButton(int x, int y) {
    return x >= BTN_LEFT_X && y >= BTN_LEFT_Y && x <= BTN_LEFT_X + BTN_WIDTH && y <= BTN_LEFT_Y + BTN_HEIGHT;
}

//--------------------------------------------------------------------
bool XHandler::isRightButton(int x, int y) {
    return x >= BTN_RIGHT_X && y >= BTN_RIGHT_Y && x <= BTN_RIGHT_X + BTN_WIDTH && y <= BTN_RIGHT_Y + BTN_HEIGHT;
}

//--------------------------------------------------------------------
bool XHandler::isMuteButton(int x, int y) {
    return x >= BTN_MUTE_X && y >= BTN_MUTE_Y && x <= BTN_MUTE_X + BTN_WIDTH && y <= BTN_MUTE_Y + BTN_HEIGHT;
}

//--------------------------------------------------------------------
bool XHandler::isRecButton(int x, int y) {
    return(x>=BTN_REC_X && y>=BTN_REC_Y && x<=BTN_REC_X + BTN_WIDTH && y<=BTN_REC_Y + BTN_HEIGHT);
}

//--------------------------------------------------------------------
bool XHandler::isVolumeBar(int x, int y) {
    return(x>=37 && x<=56 && y>=8 && y<=56);
}

//--------------------------------------------------------------------
unsigned long XHandler::getColor(char *colorname) {
    XColor color;
    XWindowAttributes winattr;
    XGetWindowAttributes(display_default, window_root, &winattr);
    color.pixel = 0;
    XParseColor(display_default, winattr.colormap, colorname, &color);
    color.flags = DoRed | DoGreen | DoBlue;
    XAllocColor(display_default, winattr.colormap, &color);
    return color.pixel;
}

//--------------------------------------------------------------------
unsigned long XHandler::mixColor(char *colorname1, int prop1, char *colorname2, int prop2) {
    XColor color, color1, color2;
    XWindowAttributes winattr;

    XGetWindowAttributes(display_default, window_root, &winattr);

    XParseColor(display_default, winattr.colormap, colorname1, &color1);
    XParseColor(display_default, winattr.colormap, colorname2, &color2);

    color.pixel = 0;
    color.red   = (color1.red*prop1+color2.red*prop2)/(prop1+prop2);
    color.green = (color1.green*prop1+color2.green*prop2)/(prop1+prop2);
    color.blue  = (color1.blue*prop1+color2.blue*prop2)/(prop1+prop2);
    color.flags = DoRed | DoGreen | DoBlue;

    XAllocColor(display_default, winattr.colormap, &color);

    return color.pixel;
}

//--------------------------------------------------------------------
void XHandler::repaint() {
    flush_expose(window_icon);
    XCopyArea(display_default, pixmap_disp, window_icon, graphics_context, 0, 0, 64, 64, window_size/2-32, window_size/2-32);
    flush_expose(window_main);
    XCopyArea(display_default, pixmap_disp, window_main, graphics_context, 0, 0, 64, 64, window_size/2-32, window_size/2-32);

    XEvent xev;
    while (XCheckTypedEvent(display_default, Expose, &xev));
}

//--------------------------------------------------------------------
void XHandler::update(unsigned channel) {
    if (is_wmaker || is_ushape || is_astep) {
        XShapeCombineMask(display_default, window_icon, ShapeBounding, window_size/2-32, window_size/2-32, pixmap_mask, ShapeSet);
        XShapeCombineMask(display_default, window_main, ShapeBounding, window_size/2-32, window_size/2-32, pixmap_mask, ShapeSet);
    } else {
        XCopyArea(display_default, pixmap_tile, pixmap_disp, graphics_context, 0, 0, 64, 64, 0, 0);
    }

    XSetClipMask(display_default, graphics_context, pixmap_mask);
    XCopyArea(display_default, pixmap_main, pixmap_disp, graphics_context, 0, 0, 64, 64, 0, 0);
    XSetClipMask(display_default, graphics_context, None);
    XCopyArea(display_default, pixmap_icon, pixmap_disp, graphics_context, icon_list[channel]*22, 0, 22, 22, 6, 5);
}

//--------------------------------------------------------------------
void XHandler::drawLeft(unsigned curleft) {
    XSetForeground(display_default, graphics_context, shade_colors[(curleft*25)/100]);
    for (unsigned i = 0; i < 25; i++) {
        if (i >= (curleft*25)/100) {
            XSetForeground(display_default, graphics_context, colors[3]);
        } else {
            XSetForeground(display_default, graphics_context, shade_colors[i]);
        }
        XFillRectangle(display_default, pixmap_disp, graphics_context, 37, 55-2*i, 9, 1);
    }
}

//--------------------------------------------------------------------
void XHandler::drawRight(unsigned curright) {
    for (unsigned i = 0; i < 25; i++) {
        if (i >= (curright*25)/100) {
            XSetForeground(display_default, graphics_context, colors[3]);
        } else {
            XSetForeground(display_default, graphics_context, shade_colors[i]);
        }
        XFillRectangle(display_default, pixmap_disp, graphics_context, 48, 55-2*i, 9, 1);
    }
}

//--------------------------------------------------------------------
// Based on wmsmixer by Damian Kramer <psiren@hibernaculum.demon.co.uk>
void XHandler::drawMono(unsigned curright) {
    XSetForeground(display_default, graphics_context, colors[1]);
    for (unsigned i = 0; i < 25; i++) {
        if (i >= (curright*25)/100) {
            XSetForeground(display_default, graphics_context, colors[3]);
        } else {
            XSetForeground(display_default, graphics_context, shade_colors[i]);
        }
        XFillRectangle(display_default, pixmap_disp, graphics_context, 37, 55-2*i, 20, 1);
    }
}


//--------------------------------------------------------------------
void XHandler::drawBtns(int buttons, bool curshowrec) {
    if (buttons & BTNPREV)
        drawButton(BTN_LEFT_X, BTN_LEFT_Y, BTN_WIDTH, BTN_HEIGHT, (button_state & BTNPREV));

    if (buttons & BTNNEXT)
        drawButton(BTN_RIGHT_X, BTN_RIGHT_Y, BTN_WIDTH, BTN_HEIGHT, (button_state & BTNNEXT));

    if (buttons & BTNMUTE)
        drawButton(BTN_MUTE_X, BTN_MUTE_Y, BTN_WIDTH, BTN_HEIGHT, (button_state & BTNMUTE));

    if (buttons & BTNREC) {
        drawButton(BTN_REC_X, BTN_REC_Y, BTN_WIDTH, BTN_HEIGHT, (button_state & BTNREC));

        if (!curshowrec)
            XCopyArea(display_default, pixmap_nrec, pixmap_disp, graphics_context, 0, 0, 9, 8, 6, 47);
        else
            XCopyArea(display_default, pixmap_main, pixmap_disp, graphics_context, 6, 48, 9, 8, 6, 47);
    }
}

//--------------------------------------------------------------------
void XHandler::drawButton(int x, int y, int w, int h, bool down) {
    if (!down)
        XCopyArea(display_default, pixmap_main, pixmap_disp, graphics_context, x, y, w, h, x, y);
    else {
        XCopyArea(display_default, pixmap_main, pixmap_disp, graphics_context, x, y, 1, h-1, x+w-1, y+1);
        XCopyArea(display_default, pixmap_main, pixmap_disp, graphics_context, x+w-1, y+1, 1, h-1, x, y);
        XCopyArea(display_default, pixmap_main, pixmap_disp, graphics_context, x, y, w-1, 1, x+1, y+h-1);
        XCopyArea(display_default, pixmap_main, pixmap_disp, graphics_context, x+1, y+h-1, w-1, 1, x, y);
    }
}

//--------------------------------------------------------------------
int XHandler::flush_expose(Window w) {
    XEvent dummy;
    int i = 0;

    while (XCheckTypedWindowEvent(display_default, w, Expose, &dummy))
        i++;

    return i;
}


//--------------------------------------------------------------------
int XHandler::getWindowSize() {
    return window_size;
}

//--------------------------------------------------------------------
int XHandler::getButtonState() {
    return button_state;
}

//--------------------------------------------------------------------
void XHandler::setButtonState(int button_state) {
    button_state = button_state;
}

//--------------------------------------------------------------------
void XHandler::setDisplay(char* arg) {
    sprintf(display_name, "%s", arg);
}

//--------------------------------------------------------------------
void XHandler::setPosition(char* arg) {
    sprintf(position_name, "%s", arg);
}

//--------------------------------------------------------------------
void XHandler::setLedColor(char* arg) {
    sprintf(ledcolor_name, "%s", arg);
}

//--------------------------------------------------------------------
void XHandler::setLedHighColor(char* arg) {
    sprintf(ledcolor_high_name, "%s", arg);
}

//--------------------------------------------------------------------
void XHandler::setBackColor(char* arg) {
    sprintf(backcolor_name, "%s", arg);
}

//--------------------------------------------------------------------
void XHandler::setUnshaped() {
    is_ushape = 1;
}

//--------------------------------------------------------------------
void XHandler::setWindowMaker() {
    is_wmaker = 1;
}

//--------------------------------------------------------------------
void XHandler::setAfterStep() {
    is_astep = 1;
}

//--------------------------------------------------------------------
Atom XHandler::getDeleteWin() {
    return deleteWin;
}

//--------------------------------------------------------------------
void XHandler::initIcons() {
#if SOUND_VERSION >= 0x040004
    // no defaults
#else
    icon_list[0] = 0;
    icon_list[1] = 7;
    icon_list[2] = 8;
    icon_list[3] = 2;
    icon_list[4] = 1;
    icon_list[5] = 6;
    icon_list[6] = 4;
    icon_list[7] = 5;
    icon_list[8] = 3;
#endif
}

//--------------------------------------------------------------------
void XHandler::setIcon(int chan, int icon) {
    if (0 <= icon && icon <= 9 && 0 <= chan)
        icon_list[chan] = icon;
}

//--------------------------------------------------------------------
void XHandler::initGraphicsContext() {
    XGCValues gcv;
    unsigned long gcm;

    gcm = GCForeground | GCBackground | GCGraphicsExposures;
    gcv.graphics_exposures = 0;
    gcv.foreground = fore_pix;
    gcv.background = back_pix;
    graphics_context = XCreateGC(display_default, window_root, gcm, &gcv);
}

//--------------------------------------------------------------------
void XHandler::initPixmaps(int display_depth) {
    XpmColorSymbol xpmcsym[4]={{"back_color",     NULL, colors[0]},
        {"led_color_high", NULL, colors[1]},
        {"led_color_med",  NULL, colors[2]},
        {"led_color_low",  NULL, colors[3]}
    };
    XpmAttributes xpmattr;

    xpmattr.numsymbols   = 4;
    xpmattr.colorsymbols = xpmcsym;
    xpmattr.exactColors  = false;
    xpmattr.closeness    = 40000;
    xpmattr.valuemask    = XpmColorSymbols | XpmExactColors | XpmCloseness;

    XpmCreatePixmapFromData(display_default, window_root, wmmixer_xpm, &pixmap_main, &pixmap_mask, &xpmattr);
    XpmCreatePixmapFromData(display_default, window_root, tile_xpm, &pixmap_tile, NULL, &xpmattr);
    XpmCreatePixmapFromData(display_default, window_root, icons_xpm, &pixmap_icon, NULL, &xpmattr);
    XpmCreatePixmapFromData(display_default, window_root, norec_xpm, &pixmap_nrec, NULL, &xpmattr);

    pixmap_disp = XCreatePixmap(display_default, window_root, 64, 64, display_depth);
}


//--------------------------------------------------------------------
void XHandler::initWindow(int argc, char** argv) {
    char *wname = argv[0];
    int screen, dummy = 0;
    XWMHints wmhints;
    XSizeHints shints;
    XClassHint classHint;
    XTextProperty	name;

    screen = DefaultScreen(display_default);
    _XA_GNUSTEP_WM_FUNC = XInternAtom(display_default, "_GNUSTEP_WM_FUNCTION", false);
    deleteWin = XInternAtom(display_default, "WM_DELETE_WINDOW", false);

    shints.x = 0;
    shints.y = 0;
    //  shints.flags  = USSize;
    shints.flags  = 0; // Gordon

    bool pos = (XWMGeometry(display_default, DefaultScreen(display_default),
                            position_name, NULL, 0, &shints, &shints.x, &shints.y,
                            &shints.width, &shints.height, &dummy)
                & (XValue | YValue));
    shints.min_width   = window_size;
    shints.min_height  = window_size;
    shints.max_width   = window_size;
    shints.max_height  = window_size;
    shints.base_width  = window_size;
    shints.base_height = window_size;
    shints.width       = window_size;
    shints.height      = window_size;
    shints.flags = PMinSize | PMaxSize | PBaseSize; // Gordon

    window_root = RootWindow(display_default, screen);

    back_pix = getColor("white");
    fore_pix = getColor("black");

    window_main = XCreateSimpleWindow(display_default, window_root, shints.x, shints.y,
                                       shints.width, shints.height, 0, fore_pix, back_pix);

    window_icon = XCreateSimpleWindow(display_default, window_root, shints.x, shints.y,
                                       shints.width, shints.height, 0, fore_pix, back_pix);

    XSetWMNormalHints(display_default, window_main, &shints);

    wmhints.icon_x = shints.x;
    wmhints.icon_y = shints.y;

    if (is_wmaker || is_astep || pos)
        shints.flags |= USPosition;

    if (is_wmaker) {
        wmhints.initial_state = WithdrawnState;
        wmhints.flags = StateHint | IconWindowHint | IconPositionHint | WindowGroupHint;
        wmhints.icon_window = window_icon;

        wmhints.icon_x = shints.x;
        wmhints.icon_y = shints.y;
        wmhints.window_group = window_main;
    } else {
        wmhints.initial_state = NormalState;
        wmhints.flags = WindowGroupHint | StateHint;
    }

    classHint.res_name = NAME;
    classHint.res_class = CLASS;

    XSetClassHint(display_default, window_main, &classHint);
    XSetClassHint(display_default, window_icon, &classHint);

    if (XStringListToTextProperty(&wname, 1, &name) == 0) {
        std::cerr << wname << ": can't allocate window name" << std::endl;
        exit(1);
    }

    XSetWMName(display_default, window_main, &name);
    XSetWMHints(display_default, window_main, &wmhints);
    XSetCommand(display_default, window_main, argv, argc);
    XSetWMProtocols(display_default, window_main, &deleteWin, 1); // Close
}


//--------------------------------------------------------------------
// Initialize main colors and shaded color-array for bars
void XHandler::initColors() {
    colors[0] = mixColor(ledcolor_name, 0,   backcolor_name, 100);
    colors[1] = mixColor(ledcolor_name, 100, backcolor_name, 0);
    colors[2] = mixColor(ledcolor_name, 60,  backcolor_name, 40);
    colors[3] = mixColor(ledcolor_name, 25,  backcolor_name, 75);

    for (int count = 0; count < 25; count++) {
        shade_colors[count] = mixColor(ledcolor_high_name, count*2, ledcolor_name, 100-count*4);
    }
}


//--------------------------------------------------------------------
void XHandler::initMask() {
    XSetClipMask(display_default, graphics_context, pixmap_mask);
    XCopyArea(   display_default, pixmap_main, pixmap_disp, graphics_context, 0, 0, 64, 64, 0, 0);
    XSetClipMask(display_default, graphics_context, None);
    XStoreName(  display_default, window_main, NAME);
    XSetIconName(display_default, window_main, NAME);

    if (is_wmaker || is_ushape || is_astep) {
        XShapeCombineMask(display_default, window_icon, ShapeBounding,
                window_size/2-32, window_size/2-32, pixmap_mask, ShapeSet);
        XShapeCombineMask(display_default, window_main, ShapeBounding,
                window_size/2-32, window_size/2-32, pixmap_mask, ShapeSet);
    } else {
        XCopyArea(display_default, pixmap_tile, pixmap_disp, graphics_context,
                0, 0, 64, 64, 0, 0);
    }

    XSelectInput(display_default, window_main, ButtonPressMask | ExposureMask |
            ButtonReleaseMask | PointerMotionMask | StructureNotifyMask);
    XSelectInput(display_default, window_icon, ButtonPressMask | ExposureMask |
            ButtonReleaseMask | PointerMotionMask | StructureNotifyMask);
    XMapWindow(display_default, window_main);
}

