---
title: Set of Miscelaneous C++ Notes
short_desc: A set of short unorganized/random C++ notes from projects and books
layout: home
show_edit_source: true
add_no_ai_banner: true
---

* Do not remove this line (it will not be displayed)
{:toc}

# C++ Misc Notes

## How and When Special Class Functions are Invoked

I got a bit confused about passing objects as values vs. references to certain
functions while I was developing some Vulkan application using the C++ VulkanHpp
light-weight wrapper library (particularly its RAII components).

For objects owning resources (e.g., handles) it is crucial to understand in
detail what C++ invokes when you pass objects around so that you do not end up
duplicating and/or misusing resources.

## std::memcpy Vs. std::copy


