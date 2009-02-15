// wmmixer - A mixer designed for WindowMaker
//
// Release 1.5
// Copyright (C) 1998  Sam Hawker <shawkie@geocities.com>
// Copyright (C) 2002 Gordon Fraser <gordon@debian.org>
// This software comes with ABSOLUTELY NO WARRANTY
// This software is free software, and you are welcome to redistribute it
// under certain conditions
// See the COPYING file for details.

#include "exception.h"
#include <cstring>
#include <cerrno>

//--------------------------------------------------------------------
const char* Exception::getMessage() const {
    return message.c_str();
}

//--------------------------------------------------------------------
MixerException::MixerException(const char* device, const char* msg) {
    message = "Mixer device ";
    message += device;
    message += " error: ";
    message += msg;
    message += " (";
    message += strerror(errno);
    message += ")";
}
