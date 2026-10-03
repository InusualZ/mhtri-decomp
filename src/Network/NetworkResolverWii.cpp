/*
 * Network/NetworkResolverWii.cpp - the Wii name resolver: constructor, destructor, the lookup thread and the
 *   `check` state machine.  Its log string names the class (`NetworkResolverWii::check`).
 *
 * One translation unit of the retail Network transport band, split out of `Network/network_transport.cpp`
 * (docs/network-transport-split.md holds the evidence and the confidence of each cut).  `.text`
 * 0x803CF14C..0x803CF654, `.data` 0x805F9958..0x805F99A0, extab 0x80019A68..0x80019AAC, extabindex
 * 0x8003A2F0..0x8003A338.
 *
 * NAMES.  The thread entry and lookup names are GUESSes.  Every name here is the map's or a derived one; the
 * derived ones are marked GUESS in `Network/network_transport_types.h`.
 *
 * EDGE UNPROVEN: the right edge (0x803CF654) is a guess - `tudiscover` reports only weak signals there; the
 * `.data` seams fix only that the next TU starts inside the SessionBase interval.
 *
 * TABLE.  Its table (0x805F9980, 0x20 B) is emitted from `~NetworkResolverWii`, the key function (rule 10).
 *
 * FLAGS.  C++ under `cflags_network` (`-Cpp_exceptions on` gives the `extab`), per-unit `-O3`/`-pool off` (`configure.py`);
 * file-scope `#pragma peephole off` (playbook 39); each `dont_inline` region keeps a retail `bl` that `-inline auto` folds.
 *
 * RESIDUALS.  `check` 97.27 % (retail's return-0 tail is shared, one `b`; ours duplicates it).
 */
#include "types.h"
#include "Network/network_transport.h"
#include "Network/NetworkSessionManager.h"
/* `unsplit/Network.h` is the Network band's code half (`getNetworkLogger` and the socket-pool helpers).  It
   cannot be included beside `unsplit/OS.h`: the two band headers declare `OSCreateThread`/`OSResumeThread` with
   different signatures and a TU that sees both fails with `(10197) illegal function overloading`. */
#include "unsplit/Network.h"
#include "unsplit/Runtime.PPCEABI.H.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"

/* The console's bus clock in Hz (low memory 0x800000F8), spelled exactly as `unsplit/OS.h` defines
   `OS_BUS_CLOCK`: that header cannot be included here (see above), so the one-line spelling is kept locally. */
#define OS_BUS_CLOCK (*(u32*)0x800000F8)

/* The range keeps retail's unfused forms: a separate `extsh`+`cmpwi` before the free, and the memset argument setup
   in source order (`addi` before the two `li`s) - the peephole pass folds both.  Scoped off for the file. */
#pragma peephole off

extern "C" {

#pragma dont_inline on

/* The lookup thread's entry: resolves the name, no result of its own. */
/* untyped: opaque handle passed through - the OS thread hands back the argument it was created with */
void* networkResolver_threadEntry(void* self)
{
    networkResolver_lookup((NetworkResolverWii*)self);
    return NULL;
}

/* Builds the Wii resolver: the base, its own table, the idle state and no result list. */
NetworkResolverWii::NetworkResolverWii()
{
    this->code_218 = 0xFF;
    this->addrInfo_1560 = NULL;
}

/* Waits for a running lookup to finish, frees its result list and destroys the base. */
NetworkResolverWii::~NetworkResolverWii()
{
    if (code_218 == 0) {
        code_218 = 0xFF;
    }
    while (check() == 0) {
        OSSleepTicks((s64)17 * ((OS_BUS_CLOCK / 4) / 1000));
    }
    if (addrInfo_1560 != NULL) {
        SOFreeAddrInfo(addrInfo_1560);
        addrInfo_1560 = NULL;
    }
}

#pragma dont_inline off

/* Publishes the peer's name (at most 0x1FF bytes): copies it, empties the record table and the
   error code, and refuses while the peer's rename guard is up. */
s32 NetworkResolverWii::setName(const char* name)
{
    u32 length;

    if (strlen(name) < 0x1FF) {
        length = strlen(name);
    } else {
        length = 0x1FF;
    }
    memcpy(this->name_04, name, length);
    this->name_04[length] = 0;
    memset(this->records_204, 0, 0x10);
    this->count_214 = 0;
    this->code_218 = 0;
    if (this->addrInfo_1560 != NULL) {
        this->code_218 = 0x5A;
        return -1;
    }
    return 0;
}

/* Puts the peer's error code back to its idle value. */
void NetworkResolverWii::resetCode()
{
    if (this->code_218 == 0) {
        this->code_218 = 0xFF;
    }
}

/* Resolves the name on the lookup thread and keeps the result. */
void networkResolver_lookup(NetworkResolverWii* self)
{
    self->result_1538 = SOGetAddrInfo(self->lookupName_153C, NULL, &self->hints_1540, &self->addrInfo_1560);
}

/* Walks the lookup state: an address literal resolves at once, anything else starts the lookup thread
   (state 0), waits for it (0x0A), collects up to four addresses (0x0F) and then reports how many it has
   (0x14).  A failure parks the machine in 0x5A. */
s32 NetworkResolverWii::check()
{
    u8 literal[4];
    SOSockAddrIn sockaddr;
    SOAddrInfo* node;
    s32 result;
    s32 count;

    switch (this->code_218) {
    case 0:
        getNetworkLogger()->signal_0C(1, "NetworkResolverWii::check(): NAME[%s]\n", this->name_04);
        if (this->name_04[0] == 0) {
            this->code_218 = 0x5A;
            return (s32)0x80020002;
        }
        result = SOInetAtoN(this->name_04, literal);
        if (result == 1) {
            this->code_218 = 0x14;
            memcpy(&this->records_204[0], literal, 4);
            this->count_214 = 1;
            return 1;
        }
        if (result < 0) {
            this->code_218 = 0x5A;
            return result;
        }
        this->lookupName_153C = this->name_04;
        memset(&this->hints_1540, 0, 0x20);
        this->hints_1540.family = 2;
        if (OSCreateThread(this->thread_220, (void*)networkResolver_threadEntry, this,
                           this->stack_538 + sizeof(this->stack_538), 0x1000, 0xE, 1) == 0) {
            return -1;
        }
        this->result_1538 = 0;
        OSResumeThread(this->thread_220);
        this->code_218 = 0xA;
        return 0;
    case 10:
        if (OSIsThreadTerminated(this->thread_220) != 0) {
            result = this->result_1538;
            if (result < 0) {
                this->code_218 = 0x5A;
                if (this->addrInfo_1560 != NULL) {
                    SOFreeAddrInfo(this->addrInfo_1560);
                    this->addrInfo_1560 = NULL;
                }
                return result;
            }
            this->code_218 = 0xF;
        }
        return 0;
    case 15:
        this->code_218 = 0x14;
        this->count_214 = 0;
        node = this->addrInfo_1560;
        while (node != NULL && (s32)this->count_214 < 4) {
            memcpy(&sockaddr, node->addr, 8);
            memcpy(&this->records_204[this->count_214], &sockaddr.addr, 4);
            node = node->next;
            this->count_214++;
        }
        if (this->addrInfo_1560 != NULL) {
            SOFreeAddrInfo(this->addrInfo_1560);
            this->addrInfo_1560 = NULL;
        }
        count = this->count_214;
        if (count == 0) {
            this->code_218 = 0x5A;
            return -1;
        }
        return count;
    case 20:
        return this->count_214;
    default:
        return -1;
    }
}

/* Copies one live record out of the peer's four-entry table. */
void NetworkResolverWii::recordGet(s32 index, u32* out)
{
    if (index < 0) {
        return;
    }
    if ((s32)this->count_214 <= index) {
        return;
    }
    memcpy(out, &this->records_204[index], 4);
}

}
