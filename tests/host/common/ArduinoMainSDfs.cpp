
#include "sdfs_mock.h"

SDFSMock* sdfs_mock = nullptr;

void mock_start_sdfs (const String& fname, size_t size_kb)
{
	sdfs_mock = new SDFSMock(size_kb * 1024, fname);
}

void mock_stop_sdfs ()
{
	if (sdfs_mock)
		delete sdfs_mock;
	sdfs_mock = nullptr;
}

