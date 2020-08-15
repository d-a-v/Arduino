/**
    StreamString.h

    Copyright (c) 2020 D. Gauchard. All rights reserved.
    This file is part of the esp8266 core for Arduino environment.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/

#ifndef __STREAMSTRING_H
#define __STREAMSTRING_H

#include "WString.h"

///////////////////////////////////////////////////////////////
// SStream points to a String and makes it a Stream
// (it is also the helper for StreamString)

class SStream: public Stream
{
public:

    SStream(String& string): string(&string)
    {
    }

    SStream(String* string): string(string)
    {
    }

    virtual int available() override
    {
        return string->length();
    }

    virtual int availableForWrite() override
    {
        return 32767;
    }

    virtual int read() override
    {
        if (peekPointer < 0)
        {
            // consume chars
            if (string->length())
            {
                char c = string->charAt(0);
                string->remove(0, 1);
                return c;
            }
        }
        else if (peekPointer < (int)string->length())
        {
            // return pointed and move pointer
            return string->charAt(peekPointer++);
        }

        // everything is read
        return -1;
    }

    virtual size_t write(uint8_t data) override
    {
        return string->concat((char)data);
    }

    virtual int read(uint8_t* buffer, size_t len) override
    {
        if (peekPointer < 0)
        {
            // string will be consumed
            size_t l = std::min(len, (size_t)string->length());
            memcpy(buffer, string->c_str(), l);
            string->remove(0, l);
            return l;
        }

        if (peekPointer >= (int)string->length())
        {
            return 0;
        }

        // only the pointer is moved
        size_t l = std::min(len, (size_t)(string->length() - peekPointer));
        memcpy(buffer, string->c_str() + peekPointer, l);
        peekPointer += l;
        return l;
    }

    virtual size_t write(const uint8_t* buffer, size_t len) override
    {
        return string->concat((const char*)buffer, len) ? len : 0;
    }

    virtual int peek() override
    {
        if (peekPointer < 0)
        {
            if (string->length())
            {
                return string->charAt(0);
            }
        }
        else if (peekPointer < (int)string->length())
        {
            return string->charAt(peekPointer);
        }

        return -1;
    }

    virtual void flush() override
    {
        // nothing to do
    }

    virtual bool inputTimeoutPossible() override
    {
        return false;
    }

    virtual bool outputTimeoutPossible() override
    {
        return false;
    }

    //// Stream's peekBufferAPI

    virtual const char* peekBuffer() override
    {
        if (peekPointer < 0)
        {
            return string->c_str();
        }
        if (peekPointer < (int)string->length())
        {
            return string->c_str() + peekPointer;
        }
        return nullptr;
    }

    virtual void peekConsume(size_t consume) override
    {
        if (peekPointer < 0)
        {
            // string is really consumed
            string->remove(0, consume);
        }
        else
        {
            // only the pointer is moved
            peekPointer = std::min((size_t)string->length(), peekPointer + consume);
        }
    }

    // calling setConsume() will consume bytes as the stream is read
    // (not enabled by default)
    void setConsume()
    {
        peekPointer = -1;
    }

    // Reading this stream will mark the string as read without consuming
    // This is the default.
    // Calling reset() resets the read state and allows rereading.
    void reset(int pointer = 0)
    {
        peekPointer = pointer;
    }

protected:

    String* string;

    // default (0): read marks as read without consuming(erasing) the string:
    int peekPointer = 0;
};


// StreamString is a SStream holding the String

class StreamString: public String, public SStream
{
public:

    StreamString(StreamString&& bro): String(bro), SStream(this) { }
    StreamString(const StreamString& bro): String(bro), SStream(this) { }

    // duplicate String contructors and operator=:

    StreamString(const char* text = nullptr): String(text), SStream(this) { }
    StreamString(const String& string): String(string), SStream(this) { }
    StreamString(const __FlashStringHelper *str): String(str), SStream(this) { }
    StreamString(String&& string): String(string), SStream(this) { }
    StreamString(StringSumHelper&& sum): String(sum), SStream(this) { }

    explicit StreamString(char c): String(c), SStream(this) { }
    explicit StreamString(unsigned char c, unsigned char base = 10): String(c, base), SStream(this) { }
    explicit StreamString(int i, unsigned char base = 10): String(i, base), SStream(this) { }
    explicit StreamString(unsigned int i, unsigned char base = 10): String(i, base), SStream(this) { }
    explicit StreamString(long l, unsigned char base = 10): String(l, base), SStream(this) { }
    explicit StreamString(unsigned long l, unsigned char base = 10): String(l, base), SStream(this) { }
    explicit StreamString(float f, unsigned char decimalPlaces = 2): String(f, decimalPlaces), SStream(this) { }
    explicit StreamString(double d, unsigned char decimalPlaces = 2): String(d, decimalPlaces), SStream(this) { }

    StreamString& operator= (const String& rhs)
    {
        String::operator=(rhs);
        return *this;
    }
    StreamString& operator= (const char* cstr)
    {
        String::operator=(cstr);
        return *this;
    }
    StreamString& operator= (const __FlashStringHelper* str)
    {
        String::operator=(str);
        return *this;
    }
    StreamString& operator= (String&& rval)
    {
        String::operator=(rval);
        return *this;
    }
    StreamString& operator= (StringSumHelper&& rval)
    {
        String::operator=(rval);
        return *this;
    }
};

#endif // __STREAMSTRING_H
