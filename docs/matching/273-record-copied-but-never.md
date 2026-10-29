---
id: 273
title: A record copied but never destroyed is a copy into a destructor-less base of the class whose destructor extab names
status: works
problem: Retail calls a record's copy constructor in some functions and never destroys the copy, while elsewhere the same copy constructor's object gets a cleanup record and a destructor call.
tags: [source-shape, sections]
applies: [sound]
demo: 
---

# 273. A record copied but never destroyed is a copy into a destructor-less base of the class whose destructor extab names

**Problem.** One copy function (`__ct__...FRC...`) is called from several functions. In some of them the copy is
passed by value or is a member array, and retail's `extab` carries a cleanup record naming the record's destructor
(`0x82` local object, `0x89` member array); in others the copy is a local that is never destroyed (no destructor
call, a plain `extab` entry). One class with a destructor emits the destructor call and the record everywhere
(+0x1C B in one function, +0xC in another), and a POD with explicit copy calls emits neither record.

**Why it happens.** MWCC destroys a local only when its type has a destructor. A copy into the base subobject type
(slicing `Base b(derived)`) runs the base's copy constructor and, when the base has no destructor, nothing else.
The derived class adds the constructor and destructor (and nothing else), so the member array and the by-value
parameter take the derived type and get their cleanup records; its converting constructor is an inline forwarder
to the base copy constructor, so every call site names the base's copy constructor directly.

**How to work it.** Read retail's `extab` per function (`extabindex` names the function, the action byte the kind
of cleanup) and list which callers of the copy constructor destroy their copy. Make the destructor-less record the
base (it carries the copy constructor and the fields), derive the class with the constructor and destructor, and
type the never-destroyed locals as the base. An empty destructor body (`~X() {}`) stores no table pointer; a body
that does anything (even `__destroy_arr` written by hand) stores it first, so destroy members by declaring them.
A global of such a class whose `__sinit` registers no destructor is `#pragma no_static_dtors on`.

**Result.** sound/snd_stream_mgr: `extab` 0x170 -> 0x1B0, byte-identical to the target (the manager destructor's
member-array record, the constructor's base record, `cmd_start`'s by-value temporary), `.text` stays the target's
size; sound/sound_obj `extab` 0x148 -> 0x178 (= target) with the same model; every row unchanged.

**Example.**

```cpp
struct StreamRelocSet { StreamRelocSet() {} StreamRelocSet(const StreamRelocSet& o); /* fields */ };
struct StreamBank : public StreamRelocSet {
    StreamBank();
    StreamBank(const StreamRelocSet& o) : StreamRelocSet(o) {}
    ~StreamBank();
};
void play(Mgr* m, s32 slot) { StreamRelocSet rec(m->recs[slot]); /* copied, never destroyed */ }
StreamFormat* stream_rec_format(StreamBank rec, u32 idx);  /* caller copies, calls, destroys (0x82) */
```
