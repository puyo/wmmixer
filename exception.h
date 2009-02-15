// wmmixer - A mixer designed for WindowMaker
//
// Release 1.5
// Copyright (C) 1998  Sam Hawker <shawkie@geocities.com>
// Copyright (C) 2002 Gordon Fraser <gordon@debian.org>
// This software comes with ABSOLUTELY NO WARRANTY
// This software is free software, and you are welcome to redistribute it
// under certain conditions
// See the COPYING file for details.


#ifndef __exception_h__
#define __exception_h__

#include <string>

//--------------------------------------------------------------------
class Exception {
public:
    const char* getMessage() const;

protected:
    std::string message;
};


//--------------------------------------------------------------------
class MixerException : public Exception {
public:
    MixerException(const char *device, const char *msg);
};

#endif
