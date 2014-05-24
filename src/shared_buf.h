#ifndef SHARED_BUF_H_
#define SHARED_BUF_H_

#include "jtag.h"

// Each time we can only read/write 4 bytes of data from/to ram buffer through JTAG
union byte_chunk_union {
    unsigned int word;
    char bytes[4];
};

// copy contents of WRITEBUF to another buffer
unsigned int jtag_writebuffer(JTAG *pJtag, char *buffer);

// copy cotents of buffer to READBUF
unsigned int jtag_readbuffer(const char *buffer, JTAG *pJtag);


#endif