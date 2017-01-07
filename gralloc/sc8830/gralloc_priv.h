/*
 * Copyright (C) 2010 ARM Limited. All rights reserved.
 *
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef GRALLOC_PRIV_H_
#define GRALLOC_PRIV_H_

#include <stdint.h>
#include <pthread.h>
#include <errno.h>
#include <linux/fb.h>
#include <sys/types.h>
#include <unistd.h>

#include <hardware/gralloc.h>
#include <cutils/native_handle.h>
#include <alloc_device.h>
#include <utils/Log.h>

#ifdef MALI_600
#define GRALLOC_ARM_UMP_MODULE 0
#define GRALLOC_ARM_DMA_BUF_MODULE 1
#else

enum {
    GRALLOC_USAGE_OVERLAY_BUFFER        = 0x01000000,
    GRALLOC_USAGE_VIDEO_BUFFER          = 0x02000000,
    GRALLOC_USAGE_CAMERA_BUFFER         = 0x04000000,
};

/* NOTE:
 * If your framebuffer device driver is integrated with UMP, you will have to
 * change this IOCTL definition to reflect your integration with the framebuffer
 * device.
 * Expected return value is a UMP secure id backing your framebuffer device memory.
 */

/*#define IOCTL_GET_FB_UMP_SECURE_ID    _IOR('F', 311, unsigned int)*/
#define GRALLOC_ARM_UMP_MODULE 0
#define GRALLOC_ARM_DMA_BUF_MODULE 1

/* NOTE:
 * If your framebuffer device driver is integrated with dma_buf, you will have to
 * change this IOCTL definition to reflect your integration with the framebuffer
 * device.
 * Expected return value is a structure filled with a file descriptor
 * backing your framebuffer device memory.
 */
#if GRALLOC_ARM_DMA_BUF_MODULE
struct fb_dmabuf_export
{
	__u32 fd;
	__u32 flags;
};
/*#define FBIOGET_DMABUF    _IOR('F', 0x21, struct fb_dmabuf_export)*/
#endif /* GRALLOC_ARM_DMA_BUF_MODULE */


#endif

static int mDebug=1;

/* the max string size of GRALLOC_HARDWARE_GPU0 & GRALLOC_HARDWARE_FB0
 * 8 is big enough for "gpu0" & "fb0" currently
 */
#define MALI_GRALLOC_HARDWARE_MAX_STR_LEN 8

#if GRALLOC_ARM_UMP_MODULE
#include <ump/ump.h>
#endif

#define MALI_IGNORE(x) (void)x
typedef enum
{
	MALI_YUV_NO_INFO,
	MALI_YUV_BT601_NARROW,
	MALI_YUV_BT601_WIDE,
	MALI_YUV_BT709_NARROW,
	MALI_YUV_BT709_WIDE,
} mali_gralloc_yuv_info;

struct private_handle_t;

struct private_module_t
{
	gralloc_module_t base;

	private_handle_t *framebuffer;
	uint32_t flags;
	uint32_t numBuffers;
	uint32_t bufferMask;
	pthread_mutex_t lock;
	buffer_handle_t currentBuffer;
	int ion_client;

	struct fb_var_screeninfo info;
	struct fb_fix_screeninfo finfo;
	float xdpi;
	float ydpi;
	float fps;

	enum
	{
		// flag to indicate we'll post this buffer
		PRIV_USAGE_LOCKED_FOR_POST = 0x80000000
	};

	/* default constructor */
	private_module_t();
};

#ifdef __cplusplus
struct private_handle_t : public native_handle
{
#else
struct private_handle_t
{
	struct native_handle nativeHandle;
#endif

	enum
	{
		PRIV_FLAGS_FRAMEBUFFER = 0x00000001,
		PRIV_FLAGS_USES_UMP    = 0x00000002,
		PRIV_FLAGS_USES_ION    = 0x00000004,
		PRIV_FLAGS_USES_PHY	 = 0x00000008,
		PRIV_FLAGS_NOT_OVERLAY	 = 0x00000010,
#ifdef SPRD_DITHER_ENABLE
		PRIV_FLAGS_SPRD_DITHER = 0x80000000
#endif
	};

	enum
	{
		LOCK_STATE_WRITE     =   1 << 31,
		LOCK_STATE_MAPPED    =   1 << 30,
		LOCK_STATE_READ_MASK =   0x3FFFFFFF
	};

	// ints
#if GRALLOC_ARM_DMA_BUF_MODULE
	/*shared file descriptor for dma_buf sharing*/
	int     share_fd;
#endif
	int     magic;
	int     flags;
	int     usage;
	int     size;
	int     width;
	int     height;
	int     format;
	int     stride;
	void    *base;
	int     lockState;
	int     writeOwner;
	int     pid;

	mali_gralloc_yuv_info yuv_info;

	// Following members are for UMP memory only
#if GRALLOC_ARM_UMP_MODULE
	int     ump_id;
	int     ump_mem_handle;
#define GRALLOC_ARM_UMP_NUM_INTS 2
#else
#define GRALLOC_ARM_UMP_NUM_INTS 0
#endif

	// Following members is for framebuffer only
	int     fd;
	int     offset;

	int     phyaddr;

#if GRALLOC_ARM_DMA_BUF_MODULE
	struct ion_handle *ion_hnd;
#define GRALLOC_ARM_DMA_BUF_NUM_INTS 1
#else
#define GRALLOC_ARM_DMA_BUF_NUM_INTS 0
#endif

#if GRALLOC_ARM_DMA_BUF_MODULE
#define GRALLOC_ARM_NUM_FDS 1
#else
#define GRALLOC_ARM_NUM_FDS 0
#endif

#ifdef __cplusplus
	/*
	 * We track the number of integers in the structure. There are 11 unconditional
	 * integers (magic - pid, yuv_info, fd and offset). The GRALLOC_ARM_XXX_NUM_INTS
	 * variables are used to track the number of integers that are conditionally
	 * included.
	 */
	static const int sNumInts = 16 + GRALLOC_ARM_UMP_NUM_INTS + GRALLOC_ARM_DMA_BUF_NUM_INTS;
	static const int sNumFds = GRALLOC_ARM_NUM_FDS;
	static const int sMagic = 0x3141592;

#if GRALLOC_ARM_UMP_MODULE
	private_handle_t(int flags, int usage, int size, void *base, int lock_state, ump_secure_id secure_id, ump_handle handle):
#if GRALLOC_ARM_DMA_BUF_MODULE
		share_fd(-1),
#endif
		magic(sMagic),
		flags(flags),
		usage(usage),
		size(size),
		width(0),
		height(0),
		format(0),
		stride(0),
		base(base),
		lockState(lock_state),
		writeOwner(0),
		pid(getpid()),
		yuv_info(MALI_YUV_NO_INFO),
		ump_id((int)secure_id),
		ump_mem_handle((int)handle),
		fd(0),
		offset(0)
#if GRALLOC_ARM_DMA_BUF_MODULE
		,ion_hnd(NULL)
#endif

	{
		version = sizeof(native_handle);
		numFds = sNumFds;
		numInts = sNumInts;
	}
#endif

#if GRALLOC_ARM_DMA_BUF_MODULE
	private_handle_t(int flags, int usage, int size, void *base, int lock_state):
		share_fd(-1),
		magic(sMagic),
		flags(flags),
		usage(usage),
		size(size),
		width(0),
		height(0),
		format(0),
		stride(0),
		base(base),
		lockState(lock_state),
		writeOwner(0),
		pid(getpid()),
		yuv_info(MALI_YUV_NO_INFO),
#if GRALLOC_ARM_UMP_MODULE
		ump_id((int)UMP_INVALID_SECURE_ID),
		ump_mem_handle((int)UMP_INVALID_MEMORY_HANDLE),
#endif
		fd(0),
		offset(0),
		ion_hnd(NULL)

	{
		version = sizeof(native_handle);
		numFds = sNumFds;
		numInts = sNumInts;
	}

#endif

	private_handle_t(int flags, int usage, int size, void *base, int lock_state, int fb_file, int fb_offset):
#if GRALLOC_ARM_DMA_BUF_MODULE
		share_fd(-1),
#endif
		magic(sMagic),
		flags(flags),
		usage(usage),
		size(size),
		width(0),
		height(0),
		format(0),
		stride(0),
		base(base),
		lockState(lock_state),
		writeOwner(0),
		pid(getpid()),
		yuv_info(MALI_YUV_NO_INFO),
#if GRALLOC_ARM_UMP_MODULE
		ump_id((int)UMP_INVALID_SECURE_ID),
		ump_mem_handle((int)UMP_INVALID_MEMORY_HANDLE),
#endif
		fd(fb_file),
		offset(fb_offset)
#if GRALLOC_ARM_DMA_BUF_MODULE
		,ion_hnd(NULL)
#endif

	{
		version = sizeof(native_handle);
		numFds = sNumFds;
		numInts = sNumInts;
	}

	~private_handle_t()
	{
		magic = 0;
	}

	bool usesPhysicallyContiguousMemory()
	{
		return (flags & (PRIV_FLAGS_FRAMEBUFFER|PRIV_FLAGS_USES_PHY)) ? true : false;
	}

	static int validate(const native_handle *h)
	{
		const private_handle_t *hnd = (const private_handle_t *)h;

		if (!h || h->version != sizeof(native_handle) || h->numInts != sNumInts || h->numFds != sNumFds || hnd->magic != sMagic)
		{
			return -EINVAL;
		}

		return 0;
	}

	static private_handle_t *dynamicCast(const native_handle *in)
	{
		if (validate(in) == 0)
		{
			return (private_handle_t *) in;
		}

		return NULL;
	}
#endif
};

#endif /* GRALLOC_PRIV_H_ */
