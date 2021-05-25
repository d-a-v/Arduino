/*
 sdfs.h - SDFS mock for host side testing
 Copyright (c) 2019 Earle F. Philhower, III

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
*/

#ifndef sdfs_mock_hpp
#define sdfs_mock_hpp

#include <stdint.h>
#include <stddef.h>
#include <vector>
#include <FS.h>

class SDFSMock {
public:
    SDFSMock(ssize_t fs_size, const String& storage = emptyString);
    void reset();
    ~SDFSMock();

protected:
    void load ();
    void save ();

    std::vector<uint8_t> m_fs;
    String m_storage;
    bool m_overwrite;

    uint64_t _sdCardSizeB;
    uint8_t* _sdCard;
};

#define SDFS_MOCK_DECLARE(size_kb, block_kb, page_b, storage) SDFSMock sdfs_mock(size_kb * 1024, storage)
#define SDFS_MOCK_RESET() sdfs_mock.reset()

#endif /* sdfs_mock_hpp */
