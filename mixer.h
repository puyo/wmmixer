// mixer.h - Mixer class provides control of audio mixer functions
//
// Release 1.5
// Copyright (C) 1998  Sam Hawker <shawkie@geocities.com>
// Copyright (C) 2002 Gordon Fraser <gordon@debian.org>
// This software comes with ABSOLUTELY NO WARRANTY
// This software is free software, and you are welcome to redistribute it
// under certain conditions
// See the COPYING file for details.

#ifndef __mixer_h__
#define __mixer_h__

#include <cstdio>
#include <cstdlib>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <vector>
#ifdef __NetBSD__
#include <soundcard.h>
#endif
#if defined(__FreeBSD_) || defined(__FreeBSD_kernel__) || defined(__linux__)
#include <sys/soundcard.h>
#endif
#include "exception.h"

//----------------------------------------------------------------------

typedef struct {
    bool support;
    bool stereo;
    bool recsrc;
    bool records;
    std::string name;
    std::string label;
    int value;
    int mask;
    int muted;
    int minvalue;
    int maxvalue;
    int shift;
    int value_mask;
#if SOUND_VERSION >= 0x040004
    int num;
    int ctrl;
    int timestamp;
#endif
} Channel;

//----------------------------------------------------------------------
class Mixer {
public:
    Mixer(const char *device_name);
    virtual ~Mixer();
    void readVol(int);
    int getLeft(int) const;
    int getRight(int) const;
    void writeVol(int);

    void setLeft(int, int);
    void setRight(int, int);

    bool readRec(int, bool);
    void writeRec();
    void setRec(int, bool);

    const char *getDeviceName() const;
    unsigned getNumChannels() const;
    bool getSupport(int) const;
    bool getStereo(int) const;
    bool getRecords(int) const;
    const char *getName(int) const;
    const char *getLabel(int) const;
    bool hasChanged() const;

    bool isMuted(int) const;
    void mute(int);
    void unmute(int);

    int find(const char *name);

private:
    void openFD();
    void getOSSInfo();
    void loadChannels();
    void doStatus();
#if SOUND_VERSION >= 0x040004
    void newChannel(oss_mixext& ext, int shift, int value_mask);
#endif
    void printChannel(int chan);

#if SOUND_VERSION >= 0x040004
    oss_sysinfo sysinfo;
    oss_mixerinfo mi;
#endif

    int fd;
    std::string device_name;
    int mixer_device_number;
    int muted;

    unsigned num_channels;       // maximum number of devices
    int devmask;         // supported devices
    int stmask;          // stereo devices
    int recmask;         // devices which can be recorded from
    int caps;            // capabilities
    int recsrc;          // devices which are being recorded from
    mutable int modify_counter;
    typedef std::vector<Channel> ChannelArray;
    ChannelArray channels;
};

#endif // __mixer_h__
