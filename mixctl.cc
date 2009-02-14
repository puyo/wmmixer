// mixctl.h - MixCtl class provides control of audio mixer functions
//
// Release 1.5
// Copyright (C) 1998  Sam Hawker <shawkie@geocities.com>
// Copyright (C) 2002 Gordon Fraser <gordon@debian.org>
// This software comes with ABSOLUTELY NO WARRANTY
// This software is free software, and you are welcome to redistribute it
// under certain conditions
// See the COPYING file for details.

#include "mixctl.h"

//----------------------------------------------------------------------
MixCtl::MixCtl(char *device_name):
    device_name(device_name), 
    mixer_device_number(0),
    modify_counter(-1)
{
    openFD();
#if OSS_VERSION >= 0x040004
    getOSSInfo();
#endif
    loadChannels();
}

void MixCtl::openFD() {
    if ((fd = open(device_name.c_str(), O_RDONLY | O_NONBLOCK)) == -1) {
        throw MixerDeviceException(device_name.c_str(), "Could not open file");
    }
}

void MixCtl::getOSSInfo() {
    if (ioctl(fd, SNDCTL_SYSINFO, &sysinfo) == -1)
        throw MixerDeviceException(device_name.c_str(), "Could not get system information");
}

void MixCtl::loadChannels() {
#if OSS_VERSION >= 0x040004
    num_channels = mixer_device_number; // must init argument with device number
    if (ioctl(fd, SNDCTL_MIX_NREXT, &num_channels) == -1)
        throw MixerDeviceException(device_name.c_str(), "Could not read number of channels");

    //printf("Mixer has %d channels\n", num_channels);

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
        MixerDevice *channel;

        switch (ext.type) {
            case MIXT_SLIDER: // fall through
            case MIXT_STEREOSLIDER: // fall through
            case MIXT_STEREOSLIDER16: // fall through
            case MIXT_MONOSLIDER: // fall through
            case MIXT_MONOSLIDER16:
                channels.resize(channels.size() + 1);
                channel = &channels.back();
                channel->support  = 1;
                channel->ctrl     = ext.ctrl;
                channel->stereo   = ext.type == MIXT_STEREOSLIDER || ext.type == MIXT_STEREOSLIDER16;
                channel->records  = ext.flags & MIXF_RECVOL;
                channel->mask     = 0;
                channel->name     = ext.id;
                channel->label    = ext.extname;
                channel->muted    = 0;
                channel->minvalue = ext.minvalue;
                channel->maxvalue = ext.maxvalue;
                channel->timestamp = ext.timestamp;
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

    channels = new MixerDevice[num_channels];
    int mixmask = 1;

    for (unsigned count = 0; count < num_channels; count++) {
        MixerDevice *channel = new MixerDevice;
        channel->support  = devmask & mixmask;
        channel->stereo   = stmask  & mixmask;
        channel->records  = recmask & mixmask;
        channel->mask     = mixmask;
        channel->name     = devnames[count];
        channel->label    = devlabels[count];
        channel->muted    = 0;
        channel->minvalue = 0;
        channel->maxvalue = 256;
        mixmask *= 2;

        channels.push_back(channel);
    }
    doStatus();
#endif
}

//----------------------------------------------------------------------
MixCtl::~MixCtl() {
    close(fd);
}

//----------------------------------------------------------------------
bool MixCtl::isMuted(int channel) const {
    return channels[channel].muted;
}

//----------------------------------------------------------------------
void MixCtl::mute(int channel) {
    channels[channel].muted = channels[channel].value;
    channels[channel].value = 0;
    writeVol(channel);
}

//----------------------------------------------------------------------
void MixCtl::unmute(int channel) {
    channels[channel].value = channels[channel].muted;
    channels[channel].muted = 0;
    writeVol(channel);
}

//----------------------------------------------------------------------
// Read the value of all channels
void MixCtl::doStatus() {
#if OSS_VERSION < 0x040004
    ioctl(fd, SOUND_MIXER_READ_RECSRC, &recsrc);
#endif
    for (unsigned i = 0; i < num_channels; i++) {
        readVol(i, true);
#if OSS_VERSION < 0x040004
        channels[i].recsrc = (recsrc & channels[i].mask);
#endif
    }
}


//----------------------------------------------------------------------
// Return volume for a device, optionally reading it from device first.
// Can be used as a way to avoid calling doStatus().
int MixCtl::readVol(int dev, bool read) {
    if (read) {
#if OSS_VERSION >= 0x040004
        MixerDevice& channel = channels[dev];
        oss_mixer_value val;
        val.dev = mixer_device_number;
        val.ctrl = channel.ctrl;
        val.timestamp = channel.timestamp;
        ioctl(fd, SNDCTL_MIX_READ, &val);
        channel.value = val.value * 256 / channel.maxvalue;
        //printf("Mixer value for %d is %d\n", channel.ctrl, channel.value);
#else
        ioctl(fd, MIXER_READ(dev), &channels[dev].value);
#endif
    }
    return channels[dev].value;
}

//----------------------------------------------------------------------
// Return left and right componenets of volume for a device.
// If you are lazy, you can call readVol to read from the device, then these
// to get left and right values.
int MixCtl::readLeft(int dev) const {
    return channels[dev].value % 256;
}

//----------------------------------------------------------------------
int MixCtl::readRight(int dev) const {
    return channels[dev].value / 256;
}

//----------------------------------------------------------------------
// Write volume to device. Use setVolume, setLeft and setRight first.
void MixCtl::writeVol(int dev) {
    ioctl(fd, MIXER_WRITE(dev), &channels[dev].value);
}

//----------------------------------------------------------------------
// Set volume (or left or right component) for a device. You must call writeVol to write it.
void MixCtl::setVol(int dev, int value) {
    channels[dev].value = value;
}
//----------------------------------------------------------------------
void MixCtl::setBoth(int dev, int l, int r) {
    channels[dev].value = 256*r+l;
}
//----------------------------------------------------------------------
void MixCtl::setLeft(int dev, int l) {
    int r;
    if (channels[dev].stereo)
        r = channels[dev].value/256;
    else
        r = l;
    channels[dev].value = 256*r+l;
}
//----------------------------------------------------------------------
void MixCtl::setRight(int dev, int r) {
    int l;
    if (channels[dev].stereo)
        l = channels[dev].value%256;
    else
        l = r;
    channels[dev].value = 256*r+l;
}

//----------------------------------------------------------------------
// Return record source value for a device, optionally reading it from device first.
bool MixCtl::readRec(int dev, bool read) {
    if (read) {
        ioctl(fd, SOUND_MIXER_READ_RECSRC, &recsrc);
        channels[dev].recsrc = (recsrc & channels[dev].mask);
    }
    return channels[dev].recsrc;
}

//----------------------------------------------------------------------
// Write record source values to device. Use setRec first.
void MixCtl::writeRec() {
    ioctl(fd, SOUND_MIXER_WRITE_RECSRC, &recsrc);
}

//----------------------------------------------------------------------
// Make a device (not) a record source.
void MixCtl::setRec(int dev, bool rec) {
    if (rec) {
        if (caps & SOUND_CAP_EXCL_INPUT)
            recsrc = channels[dev].mask;
        else
            recsrc |= channels[dev].mask;
    } else {
        recsrc &= ~channels[dev].mask;
    }
}

//----------------------------------------------------------------------
const char* MixCtl::getDeviceName() const {
    return device_name.c_str();
}
//----------------------------------------------------------------------
unsigned MixCtl::getNumChannels() const {
    return channels.size();
}
//----------------------------------------------------------------------
bool MixCtl::getSupport(int dev) const {
    return channels[dev].support;
}
//----------------------------------------------------------------------
bool MixCtl::getStereo(int dev) const {
    return channels[dev].stereo;
}
//----------------------------------------------------------------------
bool MixCtl::getRecords(int dev) const {
    return channels[dev].records;
}
//----------------------------------------------------------------------
const char* MixCtl::getName(int dev) const {
    return channels[dev].name;
}
//----------------------------------------------------------------------
const char* MixCtl::getLabel(int dev) const {
    return channels[dev].label;
}

//----------------------------------------------------------------------
bool MixCtl::hasChanged() const {
    struct mixer_info mixer_info;
    ioctl(fd, SOUND_MIXER_INFO, &mixer_info);

    if (mixer_info.modify_counter == modify_counter) {
        return false;
    } else {
        modify_counter = mixer_info.modify_counter;
        return true;
    }
}
