/*
 * homebutton/hbm_text_buffer_set.h - the text panel's set of three text buffers (homebutton/hbm_text_panel.cpp owns
 * its destructor, 0x805425E4, and emits its class table, 0x8064F0B0); homebutton/hbm_value.cpp builds an empty one.
 */
#ifndef HOMEBUTTON_HBM_TEXT_BUFFER_SET_H
#define HOMEBUTTON_HBM_TEXT_BUFFER_SET_H

#include "types.h"
#include "MEM/MEMAllocFromAllocator.h"

class HbmKbTextBuffer;
class HbmKbCandidateBuffer;
class HbmKbConverterBuffer;

/* The set of the panel's three text buffers: hbmTextBuffersInit builds them, the destructor (0x805425E4) destroys
 * them; its class table (0x8064F0B0) holds only the destructor.  size: 0x14 */
class HbmTextBufferSet {
public:
    HbmTextBufferSet() : textField(NULL), candidateList(NULL), converter(NULL) {}

    /* untyped: byte range - the object's storage from the band allocator */
    static void* operator new(unsigned long size, MEMAllocator* allocator) {
        return MEMAllocFromAllocator(allocator, size);
    }

    /* slot +0x008 */ virtual ~HbmTextBufferSet();

    /* +0x00 is the compiler-emitted vptr */
    /* +0x04 */ HbmKbTextBuffer* textField;
    /* +0x08 */ HbmKbCandidateBuffer* candidateList;
    /* +0x0C */ HbmKbConverterBuffer* converter;
    /* +0x10 */ MEMAllocator* allocator;
}; /* size: 0x14 */

#endif
