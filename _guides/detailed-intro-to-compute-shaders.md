---
title: Detailed Introduction to Computer Shaders
short_desc: My attempt at understanding and explaining how to program a compute shader with all the performance concideration details that one should think of. A short histogram calculation example is included in Vulkan and Direct3D12.
layout: home
show_edit_source: true
add_no_ai_banner: true
---

* Do not remove this line (it will not be displayed)
{:toc}

# Introduction to Compute Shader Programming

I think every graphics programming guide/book/tutorial should probably start with
explaining how compute shaders work (i.e., general-purpose GPU programming)
before jumping into the complex graphics pipeline set up code.

Understanding compute shaders is so fundamental to how GPUs actually work that
even if you do not intend to use them, the underlying model being used is the
same one employed by the graphics pipeline (i.e., same model when executing
pixel shaders) and knowing its internals will, among other things, help you
write better shaders (again, not just compute shaders, but also pixel/vertex
shaders).

There are already a couple of introductory compute shader tutorials but I think
they do not sufficiently explain the *whys* behind the seemingly weird compute
shader execution model (e.g., the concept of 3D thread groups which is
introduced as if it is a given; why model a hardware parallelism thread
submission problem as a 3D grid?). All of these questions are answered in this
guide.

## GPU Architecture: a Very Simplified Overview

At the time of writing this, my knowledge of GPU architectures is very average
(to put it lightly). Unlike CPUs, finding documentation for a sort of *unified
GPU architecture* is really hard. Each provider provides a very high-level
overview of its architectures but that is still extremely hard to understand
(or to connect to meaningfull high-level compute shader concepts such as work
groups; more on these later).

Fundamentally, a GPU contains a lot (think 10s of thousands) of cores with
significantly less capability than conventional single-instruction-single-data
(SISD) CPU cores. These cores are usually referred to as *SIMD units* (or in
some AMD slides, *SIMD Double Units*; ignore these details for the moment).

Single-instruction-multiple-data (SIMD) stream same instructions for multiple
shader invocations with different data inputs. In the context of GPUs, SIMDs
can be N-lanes wide where each lane is denoted as a *thread* (e.g., SIMD-16 for
16-lanes wide SIMD unit).

Throughout this, I will avoid calling these individual shader invocations
"threads" because they are simply not the same kind of "threads" everyone is
familiar with. This misnomer is widely used in compute shaders/slides and I see
no reason to not just call it what it is and avoid introducing confusion into
what already is an infamous subject to grasp.

### Wavefronts (or Warps per NVIDIA's terminology)

The granularity at which the GPU handles/invokes individual shader units is
determined by how wide the SIMD units are. For example, for 16-lane wide units,
a GPU can only invoke individual shader invocations as a groups of 16s. These
groups are called *waves* (or *wavefronts*). The following diagram illustrates
further clarifies this concept:

<figure>
<img loading="lazy"
     src="{{"/assets/svgs/compute_shader_simd_unit.svg" | relative_url }}"
     alt="Very high-level view of a GPU SIMD unit" />
<figcaption>
    A very simplified view (and, to some degree, incomplete/wrong) of a GPU
    compute unit (CU) consiting of just one SIMD with 8 ALUs and 8-lane wide
    wavefront. When every wavefront is executing the same instruction (ideal
    scenario), this architecture is extremely efficient. Note that whithin a
    wavefront, even if a lane has completed work pretty early, it still cannot
    be assigned/occupied until <b>all</b> the lanes in the wavefront finish
    executing. This is probably why it's called a wavefront (i.e., the wave will
    keep going until it hits the shore regardless of whether you ride it or not.
    Or something like that). For certain architectures (E.g., RDNA 1), a
    wavefront instruction is executed within a cycle and for Float64 (i.e.,
    double precision) a wavefront is executed within 2 to 16 cycles. There are
    a lot of omitted details about different precisions that are especially
    important for machine learning/LLM inference applications.
</figcaption>
</figure>

How wide a GPU wavefronts are has been a major architectural decision for GPU
designers. E.g., AMD transitioned from GCN's 64-lanes wide SIMD units to
32 lanes (referred to as *wave32*) when it announced its then-new RDNA 1
architecture. If you have PIX installed on your Windows machine, you can see how
wide the wavefront is for your GPU (you will probably see min/max wavefronts;
we'll come back to these later). For my AMD Radeon 7800XT, I see
`wavefront Size (min): 32`.

Wavefronts help reduce the die size since a single instruction scheduler
provides same instructions to multiple shader invocations at once. Wavefronts
also allow for highly efficient memory access across multiple shader invocations
(e.g., on RDNA 1, in an ideal scenario, 32 lanes access the same high-speed L0
cache across a wavefront execution time). Consequently, wavefronts also reduce
power consumption.

In a modern shader model (HLSL Shader Model >= 6.0), wavefronts are even
directly exposed through [wave operations][hlsl-wave-intrinsics]. The reason why
such granularity of control is exposed, is that GPU hardware providers started
employing extremely efficient *cross-lane* operations. The following quote from
AMD's RDNA 1 illustrates this:
> In some previous architectures, cross-lane operations (i.e. between different
> work-items in a wavefront) were fairly expensive. The new data path in the
> SIMD is designed for cross-lane data reading, permutations, swizzles, and
> other operations at full rate.

We will discuss these in greater details in memory-hierarchy chapter.

### RDNA System Architecture

Before any high-level discussions about AMD's RDNA 1 die architecture, there are
some terms one has to be familiar with (skip if you already know these).

#### FLOP/s

Whenever you read about GPU architectures, you will hear terms such as
*compute units* (CU). These are completely arbitrary units whose usage only
makes sense when comparing GPUs within the same architecture family. A measure
that is more suitable is *FLOPS* (floating point operations per second) which
is simply:

```
FLOP_PER_S = cores * (cycles / second) * (FLOPs / cycle);
```

Usually, when writing high-performance software (which compute shaders are a
core part of), you have to establish some theoretical limit on fast a program
can run then you have to measure your own program's FLOPS using some benchmark
and compare to the theoretical limit. If, for instance, you achieve 20% of the
theoretical limit then you know, to some degree, that there is potentially a lot
to improve on (you can also get a great idea on what is bottleneck'ing your
software; e.g., I/O).

FLOPS is usually measured using a well-known/established benchmark. Ideally,
said benchmark should be restricted to a particular floating precision (e.g.,
FP32) because different precisions may significantly influence FLOPS
measurement in unpredictable ways (e.g., you might think that FLOPS for FP64
should be half of that of FP32 but that depends significantly on the
architecture and how double precision is implemented).

To be fair, modern high-performance computing is mostly about memory bandwidth
management rather than FLOP/S management, so to say. Any non-trival modern
program is probably dominated by I/O and memory accesses than simple, pure
FLOPs (which are counted by the Terra in these days; i.e., TFLOPS). Essentially,
it's all about memory access patterns and data layout and much less so about
floating point operations. But it is still beneficial to know about these.

### Branching and Complex Control Flow (i.e., if statements)

I stated above that when every wavefront is executing the same instruction
(ideal scenario), the architecture is extremely efficient. That is, however,
most likely not the case for any sufficiently complex shader with control flow
(i.e., `if`s) statements/instructions.

### Cache and Memory Hierarchy

If you intend to write high-performance compute shaders (well, isn't this the
reason to use computer shaders in the first place?) then you have to **know**
and **care about** how the system is implemented.

#### Bank Conflicts

The way you organize the input/output data (e.g., structs) in your CS may
significantly affect the performance because of group-shared-memory
*bank conflicts*. Group shared memory (or local data share) is *banked* on RDNA
(also the case for other architectures but I have to verfy). This means that
the memory is devided into *banks* as the figure below illustrates:

<figure>
<img loading="lazy"
     src="{{"/assets/svgs/rdna_bank_conflicts.svg" | relative_url }}"
     alt="High-level view of LDS banks in RDNA1 architecture"
     style="width: 75%;"
     />
<figcaption>
    High-level view of local data share (LDS) <i>banks</i> in RDNA1
    architecture. LDS memory is organized into so-called banks. A wavefront
    memory access instruction can lead to serialization if one or more
    wavefront lanes (i.e., threads) access the same data bank but with different
    addresses. Compute shader programmers should take this into consideration,
    especially when deciding on the data layout (e.g., struct definition) of
    their inputs.
</figcaption>
</figure>

Ideally, shared memory performs fastest when no two or more threads of a
wavefront access data from the **same bank** but with **different addresses**.
If, for instance, all threads access the same address (within the same bank,
obviously) then a broadcast occurs and no slowdowns are incured (i.e., no bank
conflict occurs). A multicast, as far as I understood, is a smaller version of
the *broadcast*, can also occur when several (i.e., not all) threads request the
same address (these are technical details; what is important is the next section
about *bank conflicts*).

A **bank conflict** occurs when threads of a wavefront concurrently access
different addresses from the same bank (i.e., same wavefront instruction may
request data from same bank but at different addresses). When a bank conflict
occurs, memory access has to be serialized (i.e., served in a
one-after-the-other fashion) which incurs a performance penalty.

The following two figures illustrate this. Imagine that within each wavefront
lane (i.e., *thread*), you have to access the `x` component of a pixel entry
(i.e., `xyzw`) that is stored contiguously in a buffer:

<figure>
<img loading="lazy"
     src="{{"/assets/svgs/lds_bank_conflict_xyzw.svg" | relative_url }}" alt="" />
<figcaption>
    If each array entry stores the 4 consecutive F32 values x y z w, then bank
    conflicts will occur and performance will potentially be negatively
    impacted.
</figcaption>
</figure>

<figure>
<img loading="lazy"
     src="{{"/assets/svgs/lds_bank_conflict_xyzw_solved.svg" | relative_url }}" alt="" />
<figcaption>
    The bank conflicts can be avoided by re-arranging the buffer data to store
    all F32 x values contiguously, then all F32 y values, and so on.
</figcaption>
</figure>

As always, these are just *good-to-know-about optimization techniques* but you
**should never take these to the extreme without first establishing a proper
profiling framework**.

An additional reason why these bank conflicts may not matter at all, is that
each SIMD32 gets assigned a set of wavefronts. If one stalls because of memory
access latency, then the scheduler will simply switch to execute another
wavefront (i.e., effectively hiding memory latency).

### *Threads* in the Context of GPUs (a misnomer)

In the context of GPUs, particularly compute shaders terminology, a *thread* is
NOT the "threads" we are all familiar with. This is by far the biggest
confusion point I had when reading compute shader "tutorials" that introduced
this word without any explanation whatsoever.

## Compute Shader Execution Model

Compute shaders are executed/dispatched using the following 3D layout model (the
reason why the layout should be provided as 3D is explained below. Additionally,
how this model maps to the actual underlying wavefronts and memory will also be
explained later on. Just keep on reading ...):



Why 3D layout? (I struggled a lot with this when first learning about how
compute work is dispatched. I now, somehow, understand it better but there are
still major gaps and resources on the internet about why this is done this way
are scarce).

The layout has to be provided as 3D and not just a size number because certain
problems map **naturally** to 1D, 2D, or 3D layouts and the chosen
*dimensionality* affects the memory access of resources and how your workflow
gets distributed to the GPU cores. For instance, say you have the following
pseudo-code for a 2D image processing loop that you want to convert into a CS to
achieve potentially significant speed-ups:

{% highlight cpp linenos %}
// row-major loop over pixels of a 1024-by-1024 texture
for (int x = 0; x < 1024; ++x) {
    for (int y = 0; y < 1024; ++y) {
        /* apply some 4-by-4 filter that is centered around this pixel */
        pixels[x * width + y] = /* some_operation() */;
    }
}
{% endhighlight %}

By using a 3D layout system you can subdivide this problem into a set of
*thread groups* with the 2D layout `[8, 8, 1]` = 64 threads per *thread group*
for a total of 128x128 *thread groups*. The wavefront covers an 8x8 square which
may provide a more efficient memory access pattern than, say, a 1D
`[64, 1, 1]` = 64 pattern.

AMD's RDNA whitepaper states the following which is extremely relevant in
understanding the terminology around compute shaders:
> In all AMD graphics architectures, a kernel is a single stream of instructions
> that operate on a large number of data parallel work-items. The work-items are
> organized into architecturally visible work-groups that can communicate
> through an explicit local data share (LDS). The shader compiler further
> subdivides work-groups into microarchitectural wavefronts that are scheduled
> and executed in parallel on a given hardware implementation.

- **Threads**: these denote single shader invocations. Each thread, as discussed
above, is assigned to a lane within a wavefront/warp.

- **Thread Group**: a group of single shader invocations specified using three
dimensions (TODO: why 3D?). Threads within a thread group are *supposed* to run
concurrently (but this is not given as this largely depends on wavefront size
and the size/layout of the thread group you assigned).

- **Dispatch**: TODO

So, if you are trying to solve a problem using compute shaders, then you have
to do the following high-level steps:

1. Decide the "dimensionality" of your thread group layout. I.e., if 2D then
set the Z component to 1. This is largely dependent on underlying resources
your CS will work on.
  - Are you working on 2D textures/images? => use 2D => \[X, Y, 1\]
  - Or, are you simply working on arbitrary data? => use 1D => \[X, 1, 1\]

1. Decide the 3D thread group layout that devides the workspace. E.g., you have
4096 elements you want to do a computation on. You subdivide these into a 3D
layout of thread groups; say \[64, 1, 1\]. This choice is **extremely**
important in determining how efficient your whole overall computation (i.e.,
dispatch group) will run.

## How CS Execution Model Maps to Actual Hardware

One question arises, in a compute Shader, what maps exactly to a single core
invocation? Put it differently, is a thread group executed on a single core or
assigned a set of cores? Is a single *thread* mean to be executed on a single
core or what? And so on...

I will answer these questions according to the RDNA1 architecture (this is
probably the same for other architectures since we are still talking about
relatively high-level concepts).

At arround 10:53 of [this video by AMD][lou-kramer-compute-shader]:

> Each thread group gets scheduled to an available dual compute unit

In turn, the threads get scheduled to a SIMD in wavefronts (remember that each
dual CU has 4 SIMDs in RDNA1). So we can somewhat confidently(?) state that
**each thread group maps to *wavefront* on the hardware that is assigned to a
dual compute unit's SIMD**. This explains why most CS I come across have a
thread group size of either 32 or 64.

This information is extremely important especially for writing efficient compute
shaders where threads communicate with each other. As stated above, the cheapest
thread communication mechanism occurs at the wavefront level (i.e., cross-lane
communication). Therefore when you write compute shaders try to bring the
communication between threads to the wavefront level if possible (at worst,
try to keep it to closest next cache level, if possible).

## Direct3D 12 Example

A histogram calculation sample is provided. Given an input image, simply compute
a histogram. For how to compile the HLSL shader and other stuff, see the
following CMake and C++ gists: TODO

{% highlight cpp linenos %}
{% endhighlight %}

## Vulkan Example

{% highlight cpp linenos %}
// row-major loop over pixels of a 1024-by-1024 texture
for (int x = 0; x < 1024; ++x) {
    for (int y = 0; y < 1024; ++y) {
        /* apply some 4-by-4 filter that is centered around this pixel */
        pixels[x * width + y] = /* some_operation() */;
    }
}
{% endhighlight %}

## RDNA Hardware Advices


[amd-occupancy-explained]: https://gpuopen.com/learn/occupancy-explained/
[simd-in-the-gpu-world]: https://www.rastergrid.com/blog/gpu-tech/2022/02/simd-in-the-gpu-world/
[amd-rdna-1-whitepaper]: https://www.techpowerup.com/gpu-specs/docs/amd-rdna-whitepaper.pdf
[hlsl-wave-intrinsics]: https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/hlsl-shader-model-6-0-features-for-direct3d-12#shading-language-intrinsics
[into-to-gpu-programming]: https://people.maths.ox.ac.uk/gilesm/old/pp10/lec2_2x2.pdf
[lou-kramer-compute-shader]: https://youtu.be/eDLilzy2mq0
[optimize-compute-shaders]: https://frguthmann.github.io/posts/compute_shaders/
[cuda-bank-conflicts]: https://www.youtube.com/watch?v=CZgM3DEBplE
