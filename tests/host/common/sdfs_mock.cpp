/*
 sdfs_mock.cpp - SDFS HAL mock for host side testing
 Copyright (c) 2019 Earle F. Philhower, III

 Based off spiffs_mock.cpp:
 Copyright © 2016 Ivan Grokhotkov

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
*/

#include "sdfs_mock.h"
#include "../../../libraries/SDFS/src/SDFS.h"

#include "debug.h"
#include <flash_utils.h>
#include <stdlib.h>

#include <spiffs_api.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include "flash_hal_mock.h"

FS SDFS(nullptr);

SDFSMock::SDFSMock(ssize_t fs_size, const String& storage)
{
    m_storage = storage;
    if ((m_overwrite = (fs_size < 0)))
        fs_size = -fs_size;

    fprintf(stderr, "SDFS: %zd bytes\n", fs_size);

    m_fs.resize(fs_size, 0xff);
    reset();
}

void SDFSMock::reset()
{
    SDFS = FS(FSImplPtr(new sdfs::SDFSImpl()));
    load();
}

SDFSMock::~SDFSMock()
{
    save();
    m_fs.resize(0);
    SDFS = FS(FSImplPtr(nullptr));
}

void SDFSMock::load ()
{
    if (!m_fs.size() || !m_storage.length())
        return;

    int fs = ::open(m_storage.c_str(), O_RDONLY);
    if (fs == -1)
    {
        fprintf(stderr, "SDFS: loading '%s': %s\n", m_storage.c_str(), strerror(errno));
        return;
    }

    off_t flen = lseek(fs, 0, SEEK_END);
    if (flen == (off_t)-1)
    {
        fprintf(stderr, "SDFS: checking size of '%s': %s\n", m_storage.c_str(), strerror(errno));
        return;
    }
    lseek(fs, 0, SEEK_SET);

    if (flen != (off_t)m_fs.size())
    {
        fprintf(stderr, "SDFS: size of '%s': %d does not match requested size %zd\n", m_storage.c_str(), (int)flen, m_fs.size());
        if (!m_overwrite && flen > 0)
        {
            fprintf(stderr, "SDFS: aborting at user request\n");
            exit(1);
        }
        fprintf(stderr, "SDFS: continuing without loading at user request, '%s' will be overwritten\n", m_storage.c_str());
    }
    else
    {
        fprintf(stderr, "SDFS: loading %zi bytes from '%s'\n", m_fs.size(), m_storage.c_str());
        ssize_t r = ::read(fs, m_fs.data(), m_fs.size());
        if (r != (ssize_t)m_fs.size())
            fprintf(stderr, "SDFS: reading %zi bytes: returned %zd: %s\n", m_fs.size(), r, strerror(errno));
    }
    ::close(fs);
}

void SDFSMock::save ()
{
    if (!m_fs.size() || !m_storage.length())
        return;

    int fs = ::open(m_storage.c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0644);
    if (fs == -1)
    {
        fprintf(stderr, "SDFS: saving '%s': %s\n", m_storage.c_str(), strerror(errno));
        return;
    }
    fprintf(stderr, "SDFS: saving %zi bytes to '%s'\n", m_fs.size(), m_storage.c_str());

    if (::write(fs, m_fs.data(), m_fs.size()) != (ssize_t)m_fs.size())
        fprintf(stderr, "SDFS: writing %zi bytes: %s\n", m_fs.size(), strerror(errno));
    if (::close(fs) == -1)
        fprintf(stderr, "SDFS: closing %s: %s\n", m_storage.c_str(), strerror(errno));
}
