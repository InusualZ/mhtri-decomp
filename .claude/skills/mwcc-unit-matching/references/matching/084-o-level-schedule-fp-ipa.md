---
id: 84
title: `-O4`/`-O4,p`/`-O2`, `-schedule off`, `-fp_contract off`, `-ipa off`
status: ruled-out
problem: Another optimizer level or codegen switch might be the retail setting.
tags: [flags]
applies: []
demo:
---

# 84. `-O4`/`-O4,p`/`-O2`, `-schedule off`, `-fp_contract off`, `-ipa off`

**Problem.** Another optimizer level or codegen switch might be the retail setting.

**Why it does not work.** Worth exactly one sweep each; if the level is wrong the symptom is unmistakable (sizes off by hundreds of bytes and fused/hoisted code everywhere). (Bullet text above: "`-O` level and `-schedule`/`-fp_contract`/`-ipa`.")

**Result.** Old table status: no. `-fp_contract off` did later earn its own idea for the units whose retail code keeps unfused multiply-adds (playbook 40).
