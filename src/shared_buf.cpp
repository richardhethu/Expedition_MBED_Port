/*
 * Read and write shared buffer.
 *
 * Created by Ruining He
 */
#include "shared_buf.h"
#include "mmap.h"
#include "locks.h"

// copy contents of WRITEBUF to another buffer
unsigned int jtag_writebuffer(JTAG *pJtag, char *buffer) {
    union byte_chunk_union chunk;
    
    // read data from ram buffer
    bool finished = false;
    unsigned int len = 0;
    JTAG_mutex.lock();
    for (unsigned int i = WRITEBUF_BEGIN; i < WRITEBUF_END; i += 4) {
        chunk.word = pJtag->readMemory(i);
        for (int j = 0; j < 4; ++j) {
            if (chunk.bytes[j] != '\0') {
                *buffer++ = chunk.bytes[j];
                ++len;
            }
            else {
                finished = true;
                break;
            }
        }
        if (finished)
            break;
    }
    JTAG_mutex.unlock();
    *buffer = '\0';
    return len;
}

// copy cotents of buffer to READBUF
unsigned int jtag_readbuffer(const char *buffer, JTAG *pJtag) {
    union byte_chunk_union chunk;
    
    bool finished = false;
    unsigned int len = 0;
    JTAG_mutex.lock();
    for (int i = READBUF_BEGIN; i < READBUF_END-1; i+= 4) {
        for (int j = 0; j < 4; ++j) {
            if (*buffer) {
                chunk.bytes[j] = *buffer++;
                ++len;
            } else {
                chunk.bytes[j] = '\0';
                finished = true;
            }
        }
        pJtag->writeMemory(i, chunk.word);
        if (finished)
            break;
    }
    JTAG_mutex.unlock();
    return len;
}