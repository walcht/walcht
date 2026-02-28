---
title: Walcht's Introduction to Vulkan Programming
short_desc: An introduction to Vulkan graphics programming written by a Vulkan beginner
layout: home
---

* Do not remove this line (it will not be displayed)
{:toc}

# Introduction

It should be noted that this guide/mini-book was (and is being maintained) by a
somewhat beginner Vulkan user. The motivation behind that is that I want to both
really understand the underlying process and why things are they way they are in
Vulkan, and also provide a high quality Vulkan introduction that is 
desktop-agnostic, IDE-agnostic (no Visual Studio screenshots - this is the only
time Visual Studio is mentioned), and accompagned with a lot of diagrams (which
a lot of introductory Vulkan guides luck).

Vulkan is a very verbose and relatively hard API to use. Contrary to OpenGL,
a lot of responsibility is pushed to the application. For instance, knowledge
about synchronization mechanisms is at the core of the API and must me mastered
before jumping into any more complex projects.

Vulkan is infamous for being hard but do not let that distract you from the fact
that once you learn it, you will understand a lot about how GPUs work and the
tremendous amount of assumptions other graphics API make. Not only that, the
skillset here is highly transferable so even if you do not end up using Vulkan,
you will still learn a lot about high-performance programming, synchronization
techniques, making no assumptions about the underlying API you are using, and
refering to the specification as much as possible.

The point about learning to make no assumptions about the underlying API is
crucial in the case of Vulkan - there are a lot of gotchas that are simply
avoidable by refering to the specs. For instance, there is this concept in
Vulkan (that will be explained later on) that commands that are to be executed
on the GPU have to be submitted to this data structure called
**command buffer** which is then submitted to the GPU for execution. So if you
submit command A then command B then command C the GPU will execute A->B->C in
sequence. Right? Nope - there is no gurantee about the execution order of these
submitted commands. Such assumptions should always be verified by refering to
the specs.

For each chapter, the theoretical model/reasoning is introduced then a bulk of
code with detailed comments is given. That way you can *reason* about the
potential implementation structure **before** seing the the given implementation
sample.

It is assumed that you are familiar with modern C++ (C++ >= 20) and somewhat
familiar with CMake (it is completely fine if you are not - afterall, no one
should force anyone to learn that abysmal syntax :-)).

Vulkan is a C API but we will be using VulkanHpp to reduce boilerplate code.
If you want to use the C API (also feel free to use C and skip C++) then
equivalent C API code is also included.

To make introductions into Vulkan concepts digestable and not as dull as reading
the specs, some assumptions will be made. Think of this as a sculptor gradually
sculpting and improving the details of their sculpture.

## Patterns in the Vulkan C API

- sType
- *2() funtions

## Physical Device

## Logical Device

## Index Buffer

In the real-word, vertix buffers are rarely uploaded by their own to the GPU.
They are, instead, usually submitted along side their *index buffers*.

Imagine a rectangle. A rectangle is simply .

An index buffer is simply an array of indices into some vertex buffer. Using
a rectangle as an example

<figure>
<img loading="lazy" src="{{"/assets/svgs/index_buffer.svg" | relative_url }}" alt="Vulkan Index Buffer" />
<figcaption>
</figcaption>
</figure>

## Resources in Vulkan

Before delving into any code, the usage of resources in Vulkan has to properly
introduced. As per the Vulkan specs

> Vulkan supports three primary resource types: buffers, images, and tensors.
> Resources are views of memory with associated formatting and dimensionality.
> Buffers provide access to raw arrays of bytes, whereas images can be
> multidimensional and may have associated metadata. Tensors can be
> multidimensional, contain format information like images and may have
> associated metadata.

Let's translate and simplify that by stating that Vulkan has 2 resources (we
will ignore tensors):

  - **Buffers**: are general-purpose linear arrays of data that are represented
  by a `VkBuffer` resource handle.

  - **Images**: are specialized multidimensional resources that are represented
  by a `VkImage` resource handle. Image resources are usually used to store
  *textures* which are usually actual images that are *sampled* from the GPU
  via some Shader program (notice here that *usually* is used because images can
  be used to represent any arbitrary data - but we are trying to keep things
  simple here).

<figure>
<img loading="lazy" src="{{"/assets/svgs/vkbuffer_creation.svg" | relative_url }}" alt="Vulkan Index Buffer" />
<figcaption>
</figcaption>
</figure>

Each resource is mainly accessed via a resource handle (`VkBuffer` for buffers
and `VkImage` for images). As the name suggests, a resource handle is simply a
handle to some memory resource that exists somewhere in memory (we will see
later a discussion on which kind of memory that might be - GPU, CPU, or else).

In Vulkan, resources are backed by a `VkDeviceMemory` which represents the
actual memory that the resource handle (again, e.g., `VkBuffer`) refers to.

Additionally, a view can be built on top of resource handles (e.g.,
`VkBufferView` on top of a `VkBuffer` handle). Views, as the name suggest,
simply provide a way to *view* the underlying resource. E.g., a linear array
of float32 values can be viewed as a vecf3 using a `VkBufferView`. The concept
is very similar to that in the core C++ language where views are simply
non-owning, read-only range to the underlying resource.

Smaller resources (e.g., `VkBuffer` objects, `VkSampler` sampler objects, etc.)
and inlined resources do not require separate memory
allocations (i.e., do not require creation of VkDeviceMemory object, binding,
etc.). These resources are simply trivially allocated by the driver.

### Buffer Creation and Usage

Buffers are usually created via the following Vulkan C API calls (as usual, read
the comments and try to understand the workflow. Details will be later
explained):

{% highlight cpp linenos %}

/* fill up a VkBufferCreateInfo struct */
VkBufferCreateInfo buffer_info{
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .size = 1024, /* sice of buffer in bytes */
    .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
};

/* create the buffer handle (the backing memory that will actually contain
 * the requested buffer size of data is NOT yet allocated - this is just a
 * resource handle) */
VkBuffer buffer;
vkCreateBuffer(device, &buffer_info, nullptr, &buffer);

/* now we allocate the actual memory that will back the previously created
 * resource handle. To do so, firstly we need to provide the requirements for
 * this memory (its type and its size) */
VkMemoryRequirements memory_reqs;
vkGetBufferMemoryRequirements(device, buffer, &memory_reqs);

VkMemoryAllocateInfo memory_info{
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .allocationSize = memory_reqs.size, /* can be != buffer_info.size */
    .memoryTypeIndex = 0,               /* TODO: find using memory_reqs */
};
VkDeviceMemory memory;
vkAllocateMemory(device, &memory_info, nullptr, &memory);

/* finally bind the resource handle with its resource (i.e., bind VkBuffer
 * with its backing VkDeviceMemory) */
vkBindBufferMemory(device, buffer, memory, 0);

{% endhighlight %}

### Image Creation and Usage

There are mainly 3 different usage types for images in Vulkan:

  - **Storage Images**: supports store and load operations. *Pixels* are
  accessed via integers - that is, exactly through their coordinates.

  - **Sampled Images**: provides *sampled* load operations (i.e., image *texels*
  can be accessed via a floating-point normalized coordinate system - usually
  [0.0, 1.0] for x and y - and neighboring texels are interpolated accordingly).
  This mode does not provide support for store operations.

  - **Input Attachments**: load-only access and framebuffer-local.

Contrary to buffers which can optionally be used via buffer views, images are
often (always?) used via *image views* (in case you forgot, views are simply
non-owning ways that change the way we *view* the underlying data - just keep
reading and it will click when you see the code). The process to create an image
using the Vulkan C API is usually as follows:

{% highlight cpp linenos %}

  /* fill up a VkImageCreateInfo struct */
  VkImageCreateInfo image_info{
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .imageType = VK_IMAGE_TYPE_2D,       /* 1D or 2D or 3D */
      .usage = VK_IMAGE_USAGE_SAMPLED_BIT, /* */
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      /* ... other settings ... */
  };

  /* create the image handle (the backing memory that will actually contain
   * the requested the image data is NOT yet allocated - this is just a resource
   * handle) */
  VkImage image;
  vkCreateImage(device, &image_info, nullptr, &image);

  /* now we allocate the actual memory that will back the previously created
   * resource handle. To do so, firstly we need to provide the requirements for
   * this memory (its type and its size) */
  VkMemoryRequirements memory_reqs;
  vkGetImageMemoryRequirements(device, image, &memory_reqs);

  VkMemoryAllocateInfo memory_info{
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = memory_reqs.size, /* can be != buffer_info.size */
      .memoryTypeIndex = 0,               /* TODO: find using memory_reqs */
  };
  VkDeviceMemory memory;
  vkAllocateMemory(device, &memory_info, nullptr, &memory);

  /* then bind the resource handle with its resource (i.e., bind VkImage
   * with its backing VkDeviceMemory) */
  vkBindImageMemory(device, image, memory, 0);

  /* finally, create the image view wrapper around the image handle */
  VkImageViewCreateInfo view_info{
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = VK_FORMAT_R8G8B8A8_SNORM,  /* does NOT have to much the VkImage format */
      .components =
          {
              .r = VK_COMPONENT_SWIZZLE_B,
              .g = VK_COMPONENT_SWIZZLE_G,
              .b = VK_COMPONENT_SWIZZLE_R,
              .a = VK_COMPONENT_SWIZZLE_A,
          },
      .subresourceRange =
          {
              .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
              .baseMipLevel = 0,
              .levelCount = 1,
              .baseArrayLayer = 0,
              .layerCount = 1,
          },
  };

  VkImageView image_view;
  vkCreateImageView(device, &view_info, nullptr, &image_view);

{% endhighlight %}

### Sampler Creation and Usage

### Acceleration Structure Creation and Usage

## Passing Resources to Commands

There are multiple ways to provide data to commands executing on a command
buffer (e.g., provide data to shaders):

  1. **Attributes**: data that is streamed to vertex shaders and that is
  accessible from said shaders via input locations. Only specific to classical
  graphics pipelines (not to modern mesh-shader based rasterization pipelines).

  1. **Descriptors**:
  
  1. **Push constants**: a very small amount of data (min of 128B per command -
  NOT per command buffer) that can be stored with the recorded commands
  themselves within a command buffer.

  1. **Parameters**:

### Attributes

### Descriptors

The concept of *descriptors* in Vulkan is one of its most confusing aspects.
You will probably end-up being more confused by the end of this section. That is
fine. Just keep reading on and it will click later on when everything is
combined together (that is, when you see a full working example).

Descriptors describe to the device (i.e., GPU) where to find a particular
resource within the context of command buffer execution (i.e., their state is
**local to command buffers** and NOT globally). Contrary to the global state
in OpenGL, the state (i.e., the set of parameters that describes, for instance,
which resources are to be used currently) is **local to command buffers**
through the use of *descriptors* and *descriptor sets*.

For instance, if `VkCmdDraw` is recorded to some command buffer and a particular
fragment shader is expected to access some resources at certain set binding
points then a descriptor set HAS to be properly set at that point in time to
point to the correct/expected resources.

In order for descriptors to *click*, we have to take on the shoes of,
for instance, a vertex shader `vert_shader_example`. Other than the vertex input
attributes resource that do NOT require descriptor sets, let's assume that we
also need to access a global (i.e., global to all vertices within this shader's
execution) buffer resource that contains the MVP matrix.

Descriptors are **always** organized within the so-called *descriptor sets*.
As the name suggests, these are a set that contains one or more descriptors
which can be combined and used in conjunction. You must be very confused at this
point - which is expected. The following diagram might make it easier to
understand this concept:

To quote Johannes Unterguggenberger (from some YouTube comment answer):
> A shader is always hard-coded for a certain state, and the recorded state
> (within the command buffer) must match whatever the shader expects, otherwise
> there will be (validation) errors.

Descriptor sets are allocated from a `Descriptor Pool`.

 1. 


```glsl
#version 450

/* VK_DESCRIPTOR_TYPE_SAMPLER */
layout (set = 0, binding = 0) uniform sampler   s;

/* VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE */
layout (set = 0, binding = 1) uniform texture2D t;

/* ... some other code ... */

vec4 rgba = texture(sampler2D(t, s), vec2(0.5, 0.5));
```

{% highlight cpp linenos %}



{% endhighlight %}

### Push Constants

A very small amount of data (min of 128B per command - NOT per command buffer)
that can be stored with the recorded commands themselves within a command buffer
Not all commands support push constants.

```glsl
layout(push_constant) uniform PushConstants {
    vec4 col;
    mat4 mvp;
} pushConstants;

// ...

vec4 rgba   = pushConstants.col;
mat4 mvp    = pushConstants.mvp;
```

### Parameters



## Command Buffers

### Command Types

### Command Buffer Creation and Usage

{% highlight cpp linenos %}

  /* command buffers are allocated from command pools (this is a very common
   * design pattern in Vulkan because of performance). */
  VkCommandPool command_pool;
  VkCommandPoolCreateInfo pool_create_info{
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
      .queueFamilyIndex = 0, /* specify your queue family index here */
  };
  vkCreateCommandPool(device, &pool_create_info, nullptr, &command_pool);

  /* then in your game loop (or whatever you call it), record your cb using
   * current-frame application state (e.g., only frustrum-culled 3D objects)
   * then submit the cb to the graphics queue */
  while (true) {

    /* then create the command buffer handle (binding to the allocated memory
     * is automatically performed here because of the object pool) */
    VkCommandBuffer cb;
    VkCommandBufferAllocateInfo alloc_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = command_pool, /* pool from where to 'create' this cb */
        .level =
            VK_COMMAND_BUFFER_LEVEL_PRIMARY, /* ignore this for the moment */
        .commandBufferCount = 1,             /* how many cbs to create */
    };
    vkAllocateCommandBuffers(device, &alloc_info, &cb);

    /* ... */

    VkCommandBufferBeginInfo cb_begin_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = 0,
    };

    /* record your cb */
    {
      vkBeginCommandBuffer(cb, &cb_begin_info);

      /* ... */

      /* set the state BEFORE recording the draw call */
      // vkBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, ...);
      /* draw using CURRENT state */
      // vkCmdDraw(cb, ...);

      /* ... */

      vkEndCommandBuffer(cb);
    }

    /* submit the previously recorded cb */
    VkSubmitInfo submit_info{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &cb,
    };
    vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
  }

{% endhighlight %}
