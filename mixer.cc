// mixer.h - Mixer class provides control of audio mixer functions
//
// Release 1.5
// Copyright (C) 1998  Sam Hawker <shawkie@geocities.com>
// Copyright (C) 2002 Gordon Fraser <gordon@debian.org>
// This software comes with ABSOLUTELY NO WARRANTY
// This software is free software, and you are welcome to redistribute it
// under certain conditions
// See the COPYING file for details.

#include "mixer.h"

//----------------------------------------------------------------------
Mixer::Mixer(const char *device_name):
    device_name(device_name), 
    mixer_device_number(0),
    modify_counter(-1)
{
    printf("Compiled with OSS version %x\n", SOUND_VERSION);
    openFD();
#if SOUND_VERSION >= 0x040004
    getOSSInfo();
#endif
    loadChannels();
}

void Mixer::openFD() {
    if ((fd = open(device_name.c_str(), O_RDONLY | O_NONBLOCK)) == -1) {
        throw MixerException(device_name.c_str(), "Could not open file");
    }
}

#if SOUND_VERSION >= 0x040004
void Mixer::getOSSInfo() {
    if (ioctl(fd, SNDCTL_SYSINFO, &sysinfo) == -1)
        throw MixerException(device_name.c_str(), "Could not get system information");
}

void Mixer::newChannel(oss_mixext& ext, int shift, int value_mask) {
    channels.resize(channels.size() + 1);
    Channel& channel = channels.back();
    channel.num      = channels.size() - 1;
    channel.support  = (ext.flags & MIXF_WRITEABLE) != 0;
    channel.ctrl     = ext.ctrl;
    channel.stereo   = (ext.type == MIXT_STEREOSLIDER) ||
        (ext.type == MIXT_STEREOSLIDER16);
    channel.records  = ext.flags & MIXF_RECVOL;
    channel.mask     = 0;
    channel.name     = ext.id;
    channel.label    = ext.extname;
    channel.muted    = 0;
    channel.minvalue = ext.minvalue;
    channel.maxvalue = ext.maxvalue;
    channel.timestamp = ext.timestamp;
    channel.value_mask = value_mask;
    channel.shift    = shift;
}
#endif

void Mixer::loadChannels() {
#if SOUND_VERSION >= 0x040004
    num_channels = mixer_device_number; // must init argument with device number
    if (ioctl(fd, SNDCTL_MIX_NREXT, &num_channels) == -1)
        throw MixerException(device_name.c_str(), "Could not read number of channels");

    channels.reserve(num_channels);

    int marker_seen = 0;

    for (unsigned count = 0; count < num_channels; count++) {
        oss_mixext ext;

        ext.dev = mixer_device_number;
        ext.ctrl = count;
        if (ioctl(fd, SNDCTL_MIX_EXTINFO, &ext) == -1) {
            perror("SNDCTL_MIX_EXTINFO");
            continue;
        }

        if (ext.type == MIXT_MARKER) {
            marker_seen = 1;
            continue;
        } else if (!marker_seen) {
            continue;
        }

        switch (ext.type) {
            case MIXT_SLIDER:
                newChannel(ext, 0, ~0);
                break;
            case MIXT_STEREODB: // fall through
            case MIXT_STEREOSLIDER:
                newChannel(ext, 8, 0xff);
                break;
            case MIXT_STEREOSLIDER16:
                newChannel(ext, 16, 0xffff);
                break;
            case MIXT_MONODB: // fall through
            case MIXT_MONOSLIDER:
                newChannel(ext, 0, 0xff);
                break;
            case MIXT_MONOSLIDER16:
                newChannel(ext, 0, 0xffff);
                break;
            default:
                break; // control type not supported e.g. mute, on/off, etc.
        }
    }
#else
    num_channels      = SOUND_MIXER_NRDEVICES;
    char *devnames[]  = SOUND_DEVICE_NAMES;
    char *devlabels[] = SOUND_DEVICE_LABELS;

    ioctl(fd, SOUND_MIXER_READ_DEVMASK, &devmask);
    ioctl(fd, SOUND_MIXER_READ_STEREODEVS, &stmask);
    ioctl(fd, SOUND_MIXER_READ_RECMASK, &recmask);
    ioctl(fd, SOUND_MIXER_READ_CAPS, &caps);

    channels.reserve(num_channels);
    int mixmask = 1;

    for (unsigned count = 0; count < num_channels; count++) {
        bool support = devmask & mixmask;
        if (support) {
            channels.resize(channels.size() + 1);
            Channel& channel = channels.back();
            channel.support  = support;
            channel.stereo   = stmask  & mixmask;
            channel.records  = recmask & mixmask;
            channel.mask     = mixmask;
            channel.name     = devnames[count];
            channel.label    = devlabels[count];
            channel.muted    = 0;
            channel.minvalue = 0;
            channel.maxvalue = 256;
            channel.shift    = 8;
            channel.value_mask = 0xff;
        }
        mixmask *= 2;
    }
#endif
    doStatus();
}

//----------------------------------------------------------------------
Mixer::~Mixer() {
    close(fd);
}

//----------------------------------------------------------------------
bool Mixer::isMuted(int channel) const {
    return channels[channel].muted;
}

//----------------------------------------------------------------------
void Mixer::mute(int channel) {
    channels[channel].muted = channels[channel].value;
    channels[channel].value = 0;
    writeVol(channel);
}

//----------------------------------------------------------------------
void Mixer::unmute(int channel) {
    channels[channel].value = channels[channel].muted;
    channels[channel].muted = 0;
    writeVol(channel);
}

//----------------------------------------------------------------------
int Mixer::find(const char *name) {
    if (name == NULL)
        return -1; // not found
    for (unsigned i = 0; i < channels.size(); i++) {
        Channel& channel = channels[i];
        if (strncmp(channel.label.c_str(), name, strlen(name)) == 0)
            return i;
    }
    return -1; // not found
}

//----------------------------------------------------------------------
// Read the value of all channels
void Mixer::doStatus() {
#if SOUND_VERSION < 0x040004
    ioctl(fd, SOUND_MIXER_READ_RECSRC, &recsrc);
#endif
    for (unsigned i = 0; i < channels.size(); i++) {
        readVol(i);
#if SOUND_VERSION < 0x040004
        channels[i].recsrc = (recsrc & channels[i].mask);
#endif
    }
}

//----------------------------------------------------------------------
void Mixer::printChannel(int chan) {
    Channel& channel = channels[chan];
    if (channel.stereo) {
        printf("Mixer value for %d '%s' is %d (%d,%d)\n", chan, channel.label.c_str(), channel.value, getLeft(chan), getRight(chan));
    } else {
        printf("Mixer value for %d '%s' is %d (min %d, max %d)\n", chan, channel.label.c_str(), getLeft(chan), channel.minvalue, channel.maxvalue);
    }
}

//----------------------------------------------------------------------
// Return a channel's volume.
void Mixer::readVol(int chan) {
    Channel& channel = channels[chan];
#if SOUND_VERSION >= 0x040004
    int value;
    oss_mixer_value val;

    val.dev = mixer_device_number;
    val.ctrl = channel.ctrl;
    val.timestamp = channel.timestamp;
    ioctl(fd, SNDCTL_MIX_READ, &val);
    channel.value = val.value;
    //printChannel(chan);
#else
    ioctl(fd, MIXER_READ(chan), &channel.value);
#endif
}

//----------------------------------------------------------------------
int Mixer::getLeft(int chan) const {
    const Channel& channel = channels[chan];
#if SOUND_VERSION >= 0x040004
    return ((channel.value & channel.value_mask)) 
        * 100 / channel.maxvalue;
#else
    return channel.value % 256;
#endif
}

//----------------------------------------------------------------------
int Mixer::getRight(int chan) const {
#if SOUND_VERSION >= 0x040004
    const Channel& channel = channels[chan];
    return ((channel.value >> channel.shift) & channel.value_mask) 
        * 100 / channel.maxvalue;
#else
    return channels[chan].value / 256;
#endif
}

//----------------------------------------------------------------------
// Write volume to device. Use setVolume, setLeft and setRight first.
void Mixer::writeVol(int chan) {
    Channel& channel = channels[chan];
#if SOUND_VERSION >= 0x040004
    oss_mixer_value val;
    val.dev = mixer_device_number;
    val.ctrl = channel.ctrl;
    val.timestamp = channel.timestamp;
    val.value = channels[chan].value;
    ioctl(fd, SNDCTL_MIX_WRITE, &val);
#else
    ioctl(fd, MIXER_WRITE(chan), &channel.value);
#endif
}

//----------------------------------------------------------------------
inline int scaleFromInput(int val, const Channel& channel) {
    return (val * channel.maxvalue / 100) & channel.value_mask;
}

//----------------------------------------------------------------------
void Mixer::setLeft(int chan, int l) {
    int r;
    Channel& channel = channels[chan];
#if SOUND_VERSION >= 0x040004
    l = scaleFromInput(l, channel);
    if (channel.stereo) {
        r = (channel.value >> channel.shift) & channel.value_mask; // preserve
    } else {
        r = l;
    }
    channel.value = l | (r << channel.shift);
#else
    if (channel.stereo)
        r = getRight(chan);
    else
        r = l;
    channel.value = 256*r+l;
#endif
}

//----------------------------------------------------------------------
void Mixer::setRight(int chan, int r) {
    int l;
#if SOUND_VERSION >= 0x040004
    Channel& channel = channels[chan];
    r = scaleFromInput(r, channel);
    if (channel.stereo) {
        l = channel.value & channel.value_mask; // preserve
    } else {
        l = r;
    }
    channel.value = l | (r << channel.shift);
#else
    if (channels[chan].stereo)
        l = getLeft(chan);
    else
        l = r;
    channels[chan].value = 256*r+l;
#endif
}

//----------------------------------------------------------------------
// Return record source value for a device, optionally reading it from device
// first.
bool Mixer::readRec(int chan, bool read) {
    if (read) {
        ioctl(fd, SOUND_MIXER_READ_RECSRC, &recsrc);
        channels[chan].recsrc = (recsrc & channels[chan].mask);
    }
    return channels[chan].recsrc;
}

//----------------------------------------------------------------------
// Write record source values to device. Use setRec first.
void Mixer::writeRec() {
    ioctl(fd, SOUND_MIXER_WRITE_RECSRC, &recsrc);
}

//----------------------------------------------------------------------
// Make a device (not) a record source.
void Mixer::setRec(int chan, bool rec) {
    if (rec) {
        if (caps & SOUND_CAP_EXCL_INPUT)
            recsrc = channels[chan].mask;
        else
            recsrc |= channels[chan].mask;
    } else {
        recsrc &= ~channels[chan].mask;
    }
}

//----------------------------------------------------------------------
const char* Mixer::getDeviceName() const {
    return device_name.c_str();
}
//----------------------------------------------------------------------
unsigned Mixer::getNumChannels() const {
    return channels.size();
}
//----------------------------------------------------------------------
bool Mixer::getSupport(int chan) const {
    return channels[chan].support;
}
//----------------------------------------------------------------------
bool Mixer::getStereo(int chan) const {
    return channels[chan].stereo;
}
//----------------------------------------------------------------------
bool Mixer::getRecords(int chan) const {
    return channels[chan].records;
}
//----------------------------------------------------------------------
const char* Mixer::getName(int chan) const {
    return channels[chan].name.c_str();
}
//----------------------------------------------------------------------
const char* Mixer::getLabel(int chan) const {
    return channels[chan].label.c_str();
}

//----------------------------------------------------------------------
bool Mixer::hasChanged() const {
#if SOUND_VERSION >= 0x040004
    oss_mixerinfo mixer_info1;
    mixer_info1.dev = mixer_device_number;
    ioctl(fd, SNDCTL_MIXERINFO, &mixer_info1);
#else
    struct mixer_info mixer_info1;
    ioctl(fd, SOUND_MIXER_INFO, &mixer_info1);
#endif
    if (mixer_info1.modify_counter == modify_counter) {
        return false;
    } else {
        modify_counter = mixer_info1.modify_counter;
        return true;
    }
}
