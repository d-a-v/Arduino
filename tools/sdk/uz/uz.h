
#define NO_DICT 1
#define NO_CB 1
#define NO_ADLER 1
#include "uzlib.h"

class uzlibInflater
{
protected:
    
    uint32_t m_outlen;
    uint32_t m_dlen;
    uzlib_uncomp m_uncomp;
    
public:

    uzlibInflater (const unsigned char* source, uint32_t len);

    operator bool () const { return m_outlen > 0; }

    uint32_t realInflatedLen () const { return m_outlen; }

    /* there can be mismatch between length in the trailer and actual
       data stream; to avoid buffer overruns on overlong streams, reserve
       one extra byte */
    uint32_t writableLen () const { return realInflatedLen() + 1; }
    
    bool inflate (unsigned char* dest);
};
