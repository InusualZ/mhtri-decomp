---
id: 83
title: `-Cpp_exceptions`
status: ruled-out
problem: `extab`/`extabindex` presence suggests exceptions were on; it adds those sections, and with a `new` expression in the body it moves `.text` too - see row 62.
tags: [flags, sections]
applies: []
demo:
---

# 83. `-Cpp_exceptions`

**Problem.** `extab`/`extabindex` presence suggests exceptions were on; it adds those sections, and with a `new` expression in the body it moves `.text` too - see row 62.

**Why it does not work.** It only adds `extab`/`extabindex` sections (needed at the end for a full match) and does not change `.text` here.

**Result.** Old table status: no. Playbook 62 corrects the "does not change `.text`" half: with a `new` expression in the body it does.
