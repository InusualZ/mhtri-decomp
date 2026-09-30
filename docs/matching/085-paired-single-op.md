---
id: 85
title: A paired-single op in a function (the SDK's vector library, `fn_8007270C`'s fill loop)
status: ruled-out
problem: It reads as a codegen lever and eats flag and shape sweeps.
tags: [source-shape, measurement]
applies: []
demo:
---

# 85. A paired-single op in a function (the SDK's vector library, `fn_8007270C`'s fill loop)

**Problem.** It reads as a codegen lever and eats flag and shape sweeps.

**Why it does not work.** Measured 2026-09-25: **176 of 19,916** functions contain one and **0** of ~2,450 matched functions do, so the frontend cannot emit the body-store form. Record it and move on.

**Result.** Old table status: no. See also `notes/paired-single.md`.
