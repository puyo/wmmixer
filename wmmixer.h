// wmmixer.h - A mixer designed for WindowMaker
//
// Release 1.5
// Copyright (C) 1998  Sam Hawker <shawkie@geocities.com>
// Copyright (C) 2002 Gordon Fraser <gordon@debian.org>
// This software comes with ABSOLUTELY NO WARRANTY
// This software is free software, and you are welcome to redistribute it
// under certain conditions
// See the COPYING file for details.


#ifndef __wmmixer_h__
#define __wmmixer_h__

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <getopt.h>
#include <iostream>
#include <string>
#include <X11/X.h>
#include "mixer.h"
#include "xhandler.h"
#include "common.h"
#include "exception.h"

// For repeating next and prev buttons
#define RPTINTERVAL   5


class WMMixer {
public:
    WMMixer();
    ~WMMixer();

    void init(int, char **);
    void loop();

private:
    // Mixer
    Mixer *mixer;

    std::string mixer_device;
    unsigned num_channels;
    unsigned current_channel;
    unsigned current_channel_left;
    unsigned current_channel_right;
    bool     current_recording;
    bool     current_show_recording;

    XHandler *xhandler;

    unsigned *channel_list;

    int repeat_timer;

    // For draggable volume control
    bool dragging;

    // Default scroll amount
    int wheel_scroll;

    // Input/Output
    void readConfigurationFile();
    void displayVersion(void);
    void displayUsage(const char*);
    void checkVol(bool);

    void motionEvent(XMotionEvent *xev);
    void releaseEvent(XButtonEvent *xev);
    void pressEvent(XButtonEvent *xev);
    void parseArgs(int , char **);

    void initMixer();
    void initXHandler();

    void updateDisplay();
};

#endif //__wmmixer_h__
