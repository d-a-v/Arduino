
#include "uz.h"
#include "uzlib/src/tinflate.c"

#include "uzlib/src/tinfgzip.c"
//#include "uzlib/src/crc32.c"

//#include "uzlib/src/tinfzlib.c"
//#include "uzlib/src/adler32.c"

#define OUT_CHUNK_SIZE 1

uzlibInflater::uzlibInflater (const unsigned char* source, uint32_t len)
{
    // empty uzlib_init();

    m_dlen =            source[len - 1];
    m_dlen = 256*m_dlen + source[len - 2];
    m_dlen = 256*m_dlen + source[len - 3];
    m_dlen = 256*m_dlen + source[len - 4];

    m_outlen = m_dlen;
    m_dlen++;

    uzlib_uncompress_init(&m_uncomp);

    /* source and source_limit must be initialized by user */
    /* (NO_CB=1) */
    m_uncomp.source = source;
    m_uncomp.source_limit = source + len - 4;

    int res = uzlib_gzip_parse_header(&m_uncomp);
    if (res != TINF_OK) {
        // invalid data
        m_outlen = 0;
        return;
    }
}

bool uzlibInflater::inflate (unsigned char* dest)
{   
    int res = TINF_OK;

    m_uncomp.dest_start = m_uncomp.dest = dest;

    while (m_dlen) {
        unsigned int chunk_len = m_dlen < OUT_CHUNK_SIZE ? m_dlen : OUT_CHUNK_SIZE;
        m_uncomp.dest_limit = m_uncomp.dest + chunk_len;
        res = uzlib_uncompress_chksum(&m_uncomp);
        m_dlen -= chunk_len;
        if (res != TINF_OK) {
            break;
        }
    }

    if (res != TINF_DONE) {
        //debug print res
        return false;
    }

    return true;
}

#include <coredecls.h> // uint32_t crc32 (const void* data, size_t length, uint32_t crc = 0xffffffff);
uint32_t uzlib_crc32(const void *data, unsigned int length, uint32_t crc)
{
    return crc32(data, length, crc);
}
