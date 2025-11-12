---
title: Set of C++ (and C) Compilation Notes and Code Snippets
short_desc: A set of unorganized/random C++ (and C) compilation notes, code snippets, and gotchas
layout: home
show_edit_source: true
add_no_ai_banner: true
---

* Do not remove this line (it will not be displayed)
{:toc}

# C Compilation Notes and Code Snippets

Whenever you are thrown into a new project/codebase, the first most important
step is to compile it successfully. This is simply an unorganized set of C
compilation-related small topics/snippets.

## C Standard Library (libc)

The C standard library is a set of functions, macros and typedefs that provide
additional core functionalities on top of the *vanilla* C language -- think of
input/output, memory management, string operations, etc. The C standard library
is merely a standard and does not provide any implementation details.

Important to note that the generic term **libc** is used to refer to all C
standard libraries (essentially an alias for the "C standard library").

The de-facto way to access the C standard library documentation on Linux is
through MAN pages: `man 3 SYMBOL_NAME` or sometimes `man 7 SYMBOL_NAME` for
general documentation around related topics.

Important to know that glibc is NOT the only implementation of the C standard
library (it may probably be the most widely used). The libc implementations that
I know off (and are widely used) are:

- **glibc** -- a widely used implementation of the C standard library found on
most (all?) Linux systems
- **musl** -- optimized for static linking which usually (always?) results in
smaller binary sizes compared to glibc. Used extensively in embedded devices
(IoT)
- **bionic** -- Android's C library (provided by Google)

To check which version of glibc you have on your system:

  ```bash
  getconf GNU_LIBC_VERSION
  ```
  
  You might see something like this: `glibc 2.35`

## Compiling GCC From Source

1. Got to any of the official [GCC mirrors][gcc_mirrors]
1. Check the SHA256 sum by running:

  ```bash
  ```
1. 

## clang Vs. gcc


# C++ Compilation Notes and Code Snippets

This is simply a random and unorganized set of C++ compilation notes. I always
face some new issues whenever I compile a particularly new C++ project/code.
Documenting such issues here is the goal of this article.

Always important to remember that understanding the compilation process of a
particular language is crucial for trully understanding that language (wording
is a bit off since some languages can be used with an interpreter - but you
get the point).

## C++ Standard Library

## clang vs clang++

- `clang` -- only links against the C standard library if it performs a link
- `clang++` -- links against both the C++ and C standard libraries

## libstdc++ (GNU C++ Standard Library)

If you try to compile a project that relied on a particular C++ feature that is
only supported from version X of libc then (say `<format>` functionalities from
C++ 20) against an older libc implementation (say something up-to C++ 17) then
the compilation will (obviously) fail.

This usually happens when you test a project that was developed against a newer
C++ standard (say 20) on another system (say an old Ubuntu version) that
provides an old system-wide libstdc++. To check the version of your system's
libstc++:

```bash
ldconfig -p | grep -i "libstdc"
```

which might output something like this:

```
libstdc++.so.6 (libc6,x86-64) => /lib/x86_64-linux-gnu/libstdc++.so.6
```

then:

```bash
ls -l /lib/x86_64-linux-gnu/libstdc++.so.6
```

which might output the following:

```
lrwxrwxrwx 1 root root 19 Jul 15 05:45 /lib/x86_64-linux-gnu/libstdc++.so.6 -> libstdc++.so.6.0.30
```

Which indicates that the version of the libstdc++ in our system is `6.0.30` with
gcc version `11.4.0`.

**Note: if you need a newer libstdc++ version then do NOT update the one
provided by your system! Even if programs can, theoretically, link against 
newer versions of libstdc++, it is not guranteed that they shader the same
ABI (i.e., executables linked against older libstdc++ will not work anymore) **

If you have clang installed on your system and query its version through
`clang++ -v` (or `clang++-21 -v` in my case):

```
Ubuntu clang version 21.1.5 (++20251023083201+45afac62e373-1~exp1~20251023083316.53)
Target: x86_64-pc-linux-gnu
Thread model: posix
InstalledDir: /usr/lib/llvm-21/bin
Found candidate GCC installation: /usr/lib/gcc/x86_64-linux-gnu/11
Selected GCC installation: /usr/lib/gcc/x86_64-linux-gnu/11
Candidate multilib: .;@m64
Selected multilib: .;@m64
```

This line: `Selected GCC installation: /usr/lib/gcc/x86_64-linux-gnu/11` tells
us the `clang++` will use the GNU C++ standard library at:
`/usr/lib/gcc/x86_64-linux-gnu/11`, if you grep it:

```bash
ls -l /usr/lib/gcc/x86_64-linux-gnu/11 | grep "libstdc++"
```

you might get the following:

```
lrwxrwxrwx 1 root root       40 Apr 22  2025 libstdc++.so -> ../../../x86_64-linux-gnu/libstdc++.so.6
```

which means you are using the major version 6 of libstdc++.

Important to note that `libstdc++` is usually "tied" with the `gcc` compiler
it was compiled/distributed with. The usual workflow here, is to install both
when one is missing.

Now assume you have some C++ 20 features in your code (say <format>) and you
want to successfully compile it. The problem is that `clang++` is using an
old `libstdc++` which does not support new C++ 20 features. But wait, how did
I know that, you may or may not ask.

You can check the [C++ 20 compiler support][cpp_20_compiler_support] by
looking for the **GCC libstdc++** column. You will find that C++ 20 text
formatting features were added (or are fully supported) since GCC 13.
You can further verify this by going to the [release notes of GCC 13][gcc_13_release_notes].
You should notice the `<format> header and std::format` line which proves that
this is the least version you should get.

Alright, how do we get this if there are not any officially distributed packages
(say - like on Ubuntu 22.04 or older where apt only provides gcc up-to version
11)?. Answer: by compiling gcc (and therefore libstdc++) from source code - see
[compiling gcc from source](#compiling-gcc-from-source).

```bash
```

## libc++ (LLVM C++ Standard Library)

Most Linux distributions do NOT distribute `libc++` by default - therefore, if
you are on Linux (say Ubuntu) and you want to use `clang++` it will use the GNU
C++ Standard Library provided by default.

## libc++ Vs. libstdc++

As per which one to use, as a rule of thumb, if you do not have a toolchain that
takes this decision for you then I would most probably stick with the one
provided by the OS (or a newer version of it), that is: **libstdc++ on 
GNU/Linux** and **libc++ on Mac OS X and FreeBSD**.

[cpp_20_compiler_support]: https://en.cppreference.com/w/cpp/compiler_support/20
[gcc_13_release_notes]: https://gcc.gnu.org/gcc-13/changes.html#libstdcxx
[gcc_mirrors]: https://gcc.gnu.org/mirrors.html
