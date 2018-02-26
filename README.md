# ithare::obf
C++ Code+Data Obfuscation Library with Build-Time Polymorphism

## UPDATE: from now on, ithare::obf is built on top of ithare::kscope 
To use ithare::obf, you have to checkout ithare::obf and ithare::kscope side-by-side, so your top-level folder looks as 
```
some_folder 
  |
  +-- obf
  +-- kscope
```
**NOTE: while current version of ithare::obf does provide obfuscated-injections for kscope and can be tested using kscope mechanisms, it doesn't provide reasonable interfaces to be used within your program; this is a result of incomplete integration with ithare::kscope and is being worked on. Stay tuned!**

## Primary Target Audience: MOG developers

Primarily, the library is intended for **multiplayer game developers concerned about bots**. Use of the library to protect programs from piracy, while potentially possible (especially if speaking about the programs with online protection), is not among the goals (and while it might improve things, IMO anti-piracy fight will be still next-to-hopeless; in contrast - anti-bot fight DOES have a fighting chance). **ithare::obf aims to withstand the-best-available reverse-engineering tools such as IDA Pro's decompiler** (see also [first results](#first-results) below). 

### The Big Idea
Automatically generated obfuscation code+data code while leaving the source (mostly) readable, with obfuscation only *declared* in the source. Obfuscation code is heavily randomized depending on ITHARE_OBF_SEED macro, so changing it causes a major reshuffling even if the source code is 100% the same. 

### Rationale
See http://ithare.com/bot-fighting-201-declarative-datacode-obfuscation-with-build-time-polymorphism-in-c/ and http://ithare.com/bot-fighting-201-part-2-obfuscating-literals/ 

### First Results
<a name="first-results"></a>
See http://ithare.com/bot-fighting-201-part-3-ithareobf-an-open-source-datasource-randomized-obfuscation-library


## Secondary Target Audience: Improving Security

Those concerned about attacks (in particular, zero-day attacks) on classical apps. Very roughly, the idea is to trade  attack surface attack surface in a protocol such as TLS to (supposedly significantly smaller) attack surface within obfuscator - AND **to avoid each zero-day attack from becoming a "class break"**; all of these while preserving crypto-based security of the system. For some discussion - see [Advocating Obscurity Pockets](http://ithare.com/advocating-obscurity-pockets-as-a-complement-to-security-part-ii-deployment-scenarios-more-crypto-primitives-and-obscurity-pocket-as-security/) and [Obfuscating protocols](http://ithare.com/bot-fighting-201-part-4-obfuscating-protocols-versioning/). **WON'T work for public protocols, but is expected to improve things for your-own-apps which have a luxury of working with only your own server.**

## Implementation
Based on kscope, but extending it with obfuscation-specific stuff in mind. For discussion on kscope - see [C++17 Compiler Bug Hunt](http://ithare.com/c17-compiler-bug-hunt-very-first-results-12-bugs-reported-3-already-fixed/)


## Requirements 

While formally, it is probably possible to implement the same thing under C++11 (and maybe even with C++03) - without proper support for constexprs I wouldn't be able to write it, so **C++17 is THE MUST** (="I am not going to support anything lower than that"). 

More specifically, Visual Studio 15.5.5+ with /std:c++latest (and /cgthreads1 recommended as a workaround for a suspected MT bug in MSVC linker), Apple Clang 9.0+ (with -std=c++1z), non-Apple Clang-5.0+, or GCC 7.2+ is recommended. 

## Current Status

Finalizing switch to ithare::kscope. As soon as it is done, will work on resolving issues towards beta release v0.1.

## How to Help

Until v0.1 is out - there isn't much to delegate, but **if you're interested in the library - PLEASE leave a comment** on http://ithare.com/bot-fighting-201-part-3-ithareobf-an-open-source-datasource-randomized-obfuscation-library encouraging me to work on ithare::obf harder (I am serious, there is no point in doing things if there is little interest in using them). After v0.1 is out, ideas and implementations of new injections and various generated trickery would be possible (and certainly VERY welcome; the whole point of this project is in having LOTS of different blocks-to-build-randomized-code-from). 
