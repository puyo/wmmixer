// wmmixer.cc - A mixer designed for WindowMaker
//
// Release 1.5
// Copyright (C) 1998  Sam Hawker <shawkie@geocities.com>
// Copyright (C) 2002 Gordon Fraser <gordon@debian.org>
// This software comes with ABSOLUTELY NO WARRANTY
// This software is free software, and you are welcome to redistribute it
// under certain conditions
// See the COPYING file for details.


#include "wmmixer.h"

//--------------------------------------------------------------------
WMMixer::WMMixer():
    mixer_device(MIXERDEV),
    num_channels(0),
    current_channel(0),
    current_channel_left(0),
    current_channel_right(0),
    current_recording(false),
    current_show_recording(false),
    repeat_timer(0),
    dragging(false),
    wheel_scroll(2)
{
    xhandler = new XHandler();
}

//--------------------------------------------------------------------
WMMixer::~WMMixer() {
    delete[] channel_list;
    delete mixer;
    delete xhandler;
}

//--------------------------------------------------------------------
void WMMixer::loop() {
    XEvent xev;

    bool done = false;
    while (!done) {
        while (XPending(xhandler->getDisplay())) {
            XNextEvent(xhandler->getDisplay(), &xev);
            switch (xev.type) {
            case Expose:
                xhandler->repaint();
                break;
            case ButtonPress:
                pressEvent(&xev.xbutton);
                break;
            case ButtonRelease:
                releaseEvent(&xev.xbutton);
                break;
            case MotionNotify:
                motionEvent(&xev.xmotion);
                break;
            case ClientMessage:
                if (xev.xclient.data.l[0] == (int)xhandler->getDeleteWin())
                    done = true;
                break;
            }
        }

        // keep a button pressed causes scrolling throught the channels
        if (xhandler->getButtonState() & (BTNPREV | BTNNEXT)) {
            repeat_timer++;
            if (repeat_timer >= RPTINTERVAL) {
                if (xhandler->getButtonState() & BTNNEXT) {
                    current_channel++;
                    if (current_channel >= num_channels)
                        current_channel = 0;
                } else {
                    if (current_channel < 1)
                        current_channel = num_channels-1;
                    else
                        current_channel--;
                }
                checkVol(true);
                repeat_timer = 0;
            }
        } else {
            checkVol(false);
        }

        XFlush(xhandler->getDisplay());
        usleep(100000);
    }
}


//--------------------------------------------------------------------
void WMMixer::init(int argc, char **argv) {
    parseArgs(argc, argv);

    initMixer();

    xhandler->init(argc, argv);
    readConfigurationFile();

    if (num_channels == 0) {
        std::cerr << NAME << " : Sorry, no supported channels found." << std::endl;
    } else {
        checkVol(true);
    }
}

//--------------------------------------------------------------------
void WMMixer::initMixer() {
    try {
        mixer = new Mixer(mixer_device.c_str());
    } catch (MixerException &exc) {
        std::cerr << NAME << " : " << exc.getMessage() << "'." << std::endl;
        exit(1);
    }

    channel_list = new unsigned[mixer->getNumChannels()];

    for (unsigned count = 0; count < mixer->getNumChannels(); count++) {
        if (mixer->getSupport(count)) {
            channel_list[num_channels] = count;
            num_channels++;
        }
    }
}


//--------------------------------------------------------------------
void WMMixer::checkVol(bool forced = true) {
    if (!forced && !mixer->hasChanged())
        return;

    if (mixer->isMuted(channel_list[current_channel]))
        xhandler->setButtonState(xhandler->getButtonState() | BTNMUTE);
    else
        xhandler->setButtonState(xhandler->getButtonState() & ~BTNMUTE);

    mixer->readVol(channel_list[current_channel]);
    unsigned nl   = mixer->getLeft(channel_list[current_channel]);
    unsigned nr   = mixer->getRight(channel_list[current_channel]);
    bool     nrec = mixer->readRec(channel_list[current_channel], true);

    if (forced) {
        current_channel_left  = nl;
        current_channel_right = nr;
        current_recording     = nrec;
        if (nrec)
            xhandler->setButtonState(xhandler->getButtonState() | BTNREC);
        else
            xhandler->setButtonState(xhandler->getButtonState() & ~BTNREC);
        current_show_recording = mixer->getRecords(channel_list[current_channel]);
        updateDisplay();
    } else {
        if (nl != current_channel_left || nr != current_channel_right || nrec != current_recording) {
            if (nl != current_channel_left) {
                current_channel_left = nl;
                if (mixer->getStereo(channel_list[current_channel]))
                    xhandler->drawLeft(current_channel_left);
                else
                    xhandler->drawMono(current_channel_left);
            }
            if (nr != current_channel_right) {
                current_channel_right = nr;
                if (mixer->getStereo(channel_list[current_channel]))
                    xhandler->drawRight(current_channel_right);
                else
                    xhandler->drawMono(current_channel_left);
            }
            if (nrec != current_recording) {
                current_recording = nrec;
                if (nrec)
                    xhandler->setButtonState(xhandler->getButtonState() | BTNREC);
                else
                    xhandler->setButtonState(xhandler->getButtonState() & ~BTNREC);
                xhandler->drawBtns(BTNREC, current_show_recording);
            }
            updateDisplay();
        }
    }
}



//--------------------------------------------------------------------
void WMMixer::parseArgs(int argc, char **argv) {
    static struct option long_opts[] = {
        {"help",       0, NULL, 'h'},
        {"version",    0, NULL, 'v'},
        {"display",    1, NULL, 'd'},
        {"geometry",   1, NULL, 'g'},
        {"withdrawn",  0, NULL, 'w'},
        {"afterstep",  0, NULL, 'a'},
        {"shaped",     0, NULL, 's'},
        {"led-color",  1, NULL, 'l'},
        {"led-highcolor",  1, NULL, 'L'},
        {"back-color", 1, NULL, 'b'},
        {"mix-device", 1, NULL, 'm'},
        {"scrollwheel",1, NULL, 'r'},
        {NULL,         0, NULL, 0  }
    };
    int i, opt_index = 0;


    // For backward compatibility
    for (i = 1; i < argc; i++) {
        if (strcmp("-position", argv[i]) == 0) {
            sprintf(argv[i], "%s", "-g");
        } else if (strcmp("-help", argv[i]) == 0) {
            sprintf(argv[i], "%s", "-h");
        } else if (strcmp("-display", argv[i]) == 0) {
            sprintf(argv[i], "%s", "-d");
        }
    }

    while ((i = getopt_long(argc, argv, "hvd:g:wasl:L:b:m:r:", long_opts, &opt_index)) != -1) {
        switch (i) {
        case 'h':
        case ':':
        case '?':
            displayUsage(argv[0]);
            break;
        case 'v':
            displayVersion();
            break;
        case 'd':
            xhandler->setDisplay(optarg);
            break;
        case 'g':
            xhandler->setPosition(optarg);
            break;
        case 'w':
            xhandler->setWindowMaker();
            break;
        case 'a':
            xhandler->setAfterStep();
            break;
        case 's':
            xhandler->setUnshaped();
            break;
        case 'l':
            xhandler->setLedColor(optarg);
            break;
        case 'L':
            xhandler->setLedHighColor(optarg);
            break;
        case 'b':
            xhandler->setBackColor(optarg);
            break;
        case 'm':
            mixer_device = optarg;
            break;
        case 'r':
            if (atoi(optarg) > 0)
                wheel_scroll = atoi(optarg);
            break;
        }
    }
}

//--------------------------------------------------------------------
void WMMixer::readConfigurationFile() {
    FILE *rcfile;
    std::string rcfilename;
    char buf[256];
    int done;
    unsigned current = mixer->getNumChannels() + 1;

    rcfilename = getenv("HOME");
    rcfilename += "/.wmmixer";
    if ((rcfile = fopen(rcfilename.c_str(), "r")) == NULL) {
        fprintf(stderr, "%s : Could not read configuration file '%s'.\n", NAME, rcfilename.c_str());
        return;
    }

    num_channels = 0;
    do {
        if (fgets(buf, 250, rcfile) == NULL){
            return;
        }
        if ((done = feof(rcfile)) == 0) {
            buf[strlen(buf)-1] = 0;
            if (strncmp(buf, "addchannel ", strlen("addchannel ")) == 0) {
#if SOUND_VERSION >= 0x040004
                char name[512];
                int icon = 0;
                int chan;
                sscanf(buf, "addchannel %511s %i", name, &icon);
                chan = mixer->find(name);
                if (chan == -1) {
                    fprintf(stderr, "%s : Sorry, the channel (%s) could not be found. Run ossmix for a list of channels.\n", NAME, name);
                    exit(-1);
                } else {
                    current = (unsigned)chan;
                    xhandler->setIcon(chan, icon);
                }
#else
                sscanf(buf, "addchannel %i", &current);
#endif
                if (current >= mixer->getNumChannels() || mixer->getSupport(current) == false) {
                    fprintf(stderr, "%s : Sorry, this channel (%i) is not supported.\n", NAME, current);
                    current = mixer->getNumChannels() + 1;
                } else {
                    channel_list[num_channels] = current;
                    num_channels++;
                }
            }
            if (strncmp(buf, "setchannel ", strlen("setchannel ")) == 0) {
                sscanf(buf, "setchannel %i", &current);
                if (current >= mixer->getNumChannels() || mixer->getSupport(current) == false) {
                    fprintf(stderr, "%s : Sorry, this channel (%i) is not supported.\n", NAME, current);
                    current = mixer->getNumChannels() + 1;
                }
            }
            if (strncmp(buf, "setmono ", strlen("setmono ")) == 0) {
                if (current== mixer->getNumChannels() + 1)
                    fprintf(stderr, "%s : Sorry, no current channel.\n", NAME);
                else {
                    int value;
                    sscanf(buf, "setmono %i", &value);
                    mixer->setLeft(current, value);
                    mixer->setRight(current, value);
                    mixer->writeVol(current);
                }
            }
            if (strncmp(buf, "setleft ", strlen("setleft ")) == 0) {
                if (current== mixer->getNumChannels() + 1)
                    fprintf(stderr, "%s : Sorry, no current channel.\n", NAME);
                else {
                    int value;
                    sscanf(buf, "setleft %i", &value);
                    mixer->setLeft(current, value);
                    mixer->writeVol(current);
                }
            }
            if (strncmp(buf, "setright ", strlen("setright ")) == 0) {
                if (current== mixer->getNumChannels() + 1)
                    fprintf(stderr, "%s : Sorry, no current channel.\n", NAME);
                else {
                    int value;
                    sscanf(buf, "setleft %i", &value);
                    mixer->setRight(current, value);
                    mixer->writeVol(current);
                }
            }
            if (strncmp(buf, "setrecsrc ", strlen("setrecsrc ")) == 0) {
                if (current== mixer->getNumChannels() + 1)
                    fprintf(stderr, "%s : Sorry, no current channel.\n", NAME);
                else
                    mixer->setRec(current, (strncmp(buf+strlen("setrecsrc "), "true", strlen("true")) == 0));
            }
        }
    } while (done == 0);
    fclose(rcfile);
    mixer->writeRec();
}

//--------------------------------------------------------------------
void WMMixer::displayUsage(const char* name) {
    std::cout << "Usage: " << name << "[options]" << std::endl;
    std::cout << "  -h,  --help                    display this help screen" << std::endl;
    std::cout << "  -v,  --version                 display program version" << std::endl;
    std::cout << "  -d,  --display <string>        display to use (see X manual pages)" << std::endl;
    std::cout << "  -g,  --geometry +XPOS+YPOS     geometry to use (see X manual pages)" << std::endl;
    std::cout << "  -w,  --withdrawn               run the application in withdrawn mode" << std::endl;
    std::cout << "                                 (for WindowMaker, etc)" << std::endl;
    std::cout << "  -a,  --afterstep               use smaller window (for AfterStep Wharf)" << std::endl;
    std::cout << "  -s,  --shaped                  shaped window" << std::endl;
    std::cout << "  -l,  --led-color <string>      use the specified color for led display" << std::endl;
    std::cout << "  -L,  --led-highcolor <string>  use the specified color for led shading" << std::endl;
    std::cout << "  -b,  --back-color <string>     use the specified color for backgrounds" << std::endl;
    std::cout << "  -m,  --mix-device              use specified device (rather than /dev/mixer)" << std::endl;
    std::cout << "  -r,  --scrollwheel <number>    volume increase/decrease with mouse wheel (default: 2)" << std::endl;
    std::cout << "\nFor backward compatibility the following obsolete options are still supported:" << std::endl;
    std::cout << "  -help                          display this help screen" << std::endl;
    std::cout << "  -position                      geometry to use (see X manual pages)" << std::endl;
    std::cout << "  -display                       display to use (see X manual pages)" << std::endl;
    exit(0);

}

//--------------------------------------------------------------------
void WMMixer::displayVersion() {
    std::cout << "wmmixer version 1.5" << std::endl;
    exit(0);
}


//--------------------------------------------------------------------
void WMMixer::pressEvent(XButtonEvent *xev) {
    bool forced_update = true;
    int x = xev->x-(xhandler->getWindowSize()/2-32);
    int y = xev->y-(xhandler->getWindowSize()/2-32);

    if (xhandler->isLeftButton(x, y)) {
        if (current_channel < 1)
            current_channel = num_channels-1;
        else
            current_channel--;

        xhandler->setButtonState(xhandler->getButtonState() | BTNPREV);
        repeat_timer = 0;
        xhandler->drawBtns(BTNPREV, current_show_recording);
    }

    if (xhandler->isRightButton(x, y)) {
        current_channel++;
        if (current_channel >= num_channels)
            current_channel = 0;

        xhandler->setButtonState(xhandler->getButtonState() | BTNNEXT);
        repeat_timer = 0;
        xhandler->drawBtns(BTNNEXT, current_show_recording);
    }

    // Volume settings
    if (xhandler->isVolumeBar(x, y)) {
        int vl = 0, vr = 0;

        if (xev->button < 4) {
            vl = ((60-y)*100)/(2*25);
            vr = vl;
            dragging = true;
        } else if (xev->button == 4) {
            vr = mixer->getRight(channel_list[current_channel]) + wheel_scroll;
            vl = mixer->getLeft(channel_list[current_channel])  + wheel_scroll;

        } else if (xev->button == 5) {
            vr = mixer->getRight(channel_list[current_channel]) - wheel_scroll;
            vl = mixer->getLeft(channel_list[current_channel])  - wheel_scroll;
        }

        if (vl <= 0)
            vl = 0;
        if (vr <= 0)
            vr = 0;

        if (x <= 50)
            mixer->setLeft(channel_list[current_channel], vl);
        if (x >= 45)
            mixer->setRight(channel_list[current_channel], vr);
        mixer->writeVol(channel_list[current_channel]);

        forced_update = false;
    }

    // Toggle record
    if (xhandler->isRecButton(x, y)) {
        mixer->setRec(channel_list[current_channel], !mixer->readRec(channel_list[current_channel], false));
        mixer->writeRec();
        forced_update = false;
    }

    // Toggle mute
    if (xhandler->isMuteButton(x, y)) {
        if (mixer->isMuted(channel_list[current_channel])) {
            xhandler->setButtonState(xhandler->getButtonState() & ~BTNMUTE);
            mixer->unmute(channel_list[current_channel]);
        } else {
            mixer->mute(channel_list[current_channel]);
            xhandler->setButtonState(xhandler->getButtonState() | BTNMUTE);
        }

        xhandler->drawBtns(BTNMUTE, current_show_recording);
    }

    // Update volume display
    checkVol(forced_update);
}

//--------------------------------------------------------------------
void WMMixer::releaseEvent(XButtonEvent *xev) {
    dragging = false;
    xhandler->setButtonState(xhandler->getButtonState() & ~(BTNPREV | BTNNEXT));
    xhandler->drawBtns(BTNPREV | BTNNEXT, current_show_recording);
    xhandler->repaint();
}

//--------------------------------------------------------------------
void WMMixer::motionEvent(XMotionEvent *xev) {
    int x = xev->x-(xhandler->getWindowSize()/2-32);
    int y = xev->y-(xhandler->getWindowSize()/2-32);
    //  if(x >= 37 && x <= 56 && y >= 8 && dragging){
    if (xhandler->isVolumeBar(x, y) && dragging) {
        int v=((60-y)*100)/(2*25);
        if (v < 0)
            v = 0;
        if (x <= 50)
            mixer->setLeft(channel_list[current_channel], v);
        if (x >= 45)
            mixer->setRight(channel_list[current_channel], v);
        mixer->writeVol(channel_list[current_channel]);
        checkVol(false);
    }
}

//--------------------------------------------------------------------
void WMMixer::updateDisplay() {
    xhandler->update(channel_list[current_channel]);
    if (mixer->getStereo(channel_list[current_channel])) {
        xhandler->drawLeft(current_channel_left);
        xhandler->drawRight(current_channel_right);
    } else {
        xhandler->drawMono(current_channel_right);
    }
    xhandler->drawBtns(BTNREC | BTNNEXT | BTNPREV | BTNMUTE, current_show_recording);
    xhandler->repaint();
}



//====================================================================
int main(int argc, char** argv) {
    WMMixer mixer = WMMixer();
    mixer.init(argc, argv);
    mixer.loop();
}
