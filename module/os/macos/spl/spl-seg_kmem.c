/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright (c) 1998, 2010, Oracle and/or its affiliates. All rights reserved.
 */

#include <sys/atomic.h>

#include <sys/vmem.h>
#include <sys/vmem_impl.h>

#include <sys/time.h>
#include <sys/timer.h>
#include <sys/condvar.h>

#include <stdbool.h>

/*
 * seg_kmem is the primary kernel memory segment driver.  It
 * maps the kernel heap [kernelheap, ekernelheap), module text,
 * and all memory which was allocated before the VM was initialized
 * into kas.
 *
 * Pages which belong to seg_kmem are hashed into &kvp vnode at
 * an offset equal to (u_offset_t)virt_addr, and have p_lckcnt >= 1.
 * They must never be paged out since segkmem_fault() is a no-op to
 * prevent recursive faults.
 *
 * Currently, seg_kmem pages are sharelocked (p_sharelock == 1) on
 * __x86 and are unlocked (p_sharelock == 0) on __sparc.  Once __x86
 * supports relocation the #ifdef kludges can be removed.
 *
 * seg_kmem pages may be subject to relocation by page_relocate(),
 * provided that the HAT supports it; if this is so, segkmem_reloc
 * will be set to a nonzero value. All boot time allocated memory as
 * well as static memory is considered off limits to relocation.
 * Pages are "relocatable" if p_state does not have P_NORELOC set, so
 * we request P_NORELOC pages for memory that isn't safe to relocate.
 *
 * The kernel heap is logically divided up into four pieces:
 *
 *   heap32_arena is for allocations that require 32-bit absolute
 *   virtual addresses (e.g. code that uses 32-bit pointers/offsets).
 *
 *   heap_core is for allocations that require 2GB *relative*
 *   offsets; in other words all memory from heap_core is within
 *   2GB of all other memory from the same arena. This is a requirement
 *   of the addressing modes of some processors in supervisor code.
 *
 *   heap_arena is the general heap arena.
 *
 *   static_arena is the static memory arena.  Allocations from it
 *   are not subject to relocation so it is safe to use the memory
 *   physical address as well as the virtual address (e.g. the VA to
 *   PA translations are static).  Caches may import from static_arena;
 *   all other static memory allocations should use static_alloc_arena.
 *
 * On some platforms which have limited virtual address space, seg_kmem
 * may share [kernelheap, ekernelheap) with seg_kp; if this is so,
 * segkp_bitmap is non-NULL, and each bit represents a page of virtual
 * address space which is actually seg_kp mapped.
 */

/*
 * Rough stubbed Port for XNU.
 *
 * Copyright (c) 2014 Brendon Humphrey (brendon.humphrey@mac.com)
 */


#ifdef _KERNEL
#define	XNU_KERNEL_PRIVATE
#include <mach/vm_types.h>
extern vm_map_t kernel_map;

/*
 * These extern prototypes has to be carefully checked against XNU source
 * in case Apple changes them. They are not defined in the "allowed" parts
 * of the kernel.framework
 */
typedef uint8_t vm_tag_t;

/*
 * Tag we use to identify memory we have allocated
 *
 * (VM_KERN_MEMORY_KEXT - mach_vm_statistics.h)
 */
#define	SPL_TAG 6




/*
 * In kernel lowlevel form of malloc.
 */
void *IOMalloc(vm_size_t size);
void *IOMallocAligned(vm_size_t size, vm_offset_t alignment);

/*
 * Free memory
 */
void IOFree(void *address, vm_size_t size);
void IOFreeAligned(void * address, vm_size_t size);

#endif /* _KERNEL */

typedef int page_t;

void *segkmem_alloc(vmem_t *vmp, size_t size, int vmflag);
void segkmem_free(vmem_t *vmp, void *inaddr, size_t size);

/* Total memory held allocated */
uint64_t segkmem_total_mem_allocated = 0;

/* primary kernel heap arena */
vmem_t *heap_arena;

/* qcaches abd */
vmem_t *abd_arena;

#ifdef _KERNEL
extern uint64_t total_memory;
uint64_t stat_osif_malloc_success = 0;
uint64_t stat_osif_free = 0;
uint64_t stat_osif_malloc_bytes = 0;
uint64_t stat_osif_free_bytes = 0;
#endif

void *
osif_malloc(uint64_t size)
{
#ifdef _KERNEL
	// vm_offset_t tr = NULL;
	void *tr = NULL;
	kern_return_t kr = -1;

	// kern_return_t kr = kmem_alloc(kernel_map, &tr, size);
	// tr = IOMalloc(size);

	/*
	 * align small allocations on PAGESIZE
	 * and larger ones on the enclosing power of two
	 * but drop to PAGESIZE for huge allocations
	 */
	uint64_t align = PAGESIZE;
	if (size > PAGESIZE && !ISP2(size) && size < UINT32_MAX) {
		uint64_t v = size;
		v--;
		v |= v >> 1;
		v |= v >> 2;
		v |= v >> 4;
		v |= v >> 8;
		v |= v >> 16;
		v++;
		align = v;
	} else if (size > PAGESIZE && ISP2(size)) {
		align = size;
	}

	tr = IOMallocAligned(size, MAX(PAGESIZE, align));
	if (tr != NULL)
		kr = KERN_SUCCESS;

	if (kr == KERN_SUCCESS) {
		atomic_inc_64(&stat_osif_malloc_success);
		atomic_add_64(&segkmem_total_mem_allocated, size);
		atomic_add_64(&stat_osif_malloc_bytes, size);
		return ((void *)tr);
	} else {
		// well, this can't really happen, kernel_memory_allocate
		// would panic instead
		return (NULL);
	}
#else
	return (malloc(size));
#endif
}

void
osif_free(void *buf, uint64_t size)
{
#ifdef _KERNEL
	IOFreeAligned(buf, size);
	atomic_inc_64(&stat_osif_free);
	atomic_sub_64(&segkmem_total_mem_allocated, size);
	atomic_add_64(&stat_osif_free_bytes, size);
#else
	free(buf);
#endif /* _KERNEL */
}

/*
 * Configure vmem, such that the heap arena is fed,
 * and drains to the kernel low level allocator.
 */
void
kernelheap_init()
{
	heap_arena = vmem_init("heap", NULL, 0, PAGESIZE, segkmem_alloc,
	    segkmem_free);
}


void
kernelheap_fini(void)
{
	vmem_fini(heap_arena);
}

void *
segkmem_alloc(vmem_t *vmp, size_t size, int maybe_unmasked_vmflag)
{
	return (osif_malloc(size));
}

void
segkmem_free(vmem_t *vmp, void *inaddr, size_t size)
{
	osif_free(inaddr, size);
	// since this is mainly called by spl_root_arena and free_arena,
	// do we really want to wake up a waiter, just because we have
	// transferred from one to the other?
	// we already have vmem_add_a_gibibyte waking up waiters
	// so specializing here seems wasteful
	// (originally included in vmem_experiments)
	// cv_signal(&vmp->vm_cv);
}

/*
 * OSX does not use separate heaps for the ZIO buffers,
 * the ZFS code is structured such that the zio caches will
 * fallback to using the kmem_default arena same
 * as all the other caches.
 */
// smd: we nevertheless plumb in an arena with heap as parent, so that
// we can track stats and maintain the VM_ / qc settings differently
void
segkmem_abd_init()
{
	/*
	 * OpenZFS does not segregate the abd kmem cache out of the general
	 * heap, leading to large numbers of short-lived slabs exchanged
	 * between the kmem cache and it's parent.  XNU absorbs this with a
	 * qcache, following its history of absorbing the pre-ABD zio file and
	 * metadata caches being qcached (which raises the exchanges with the
	 * general heap from PAGESIZE to 256k).
	 */

	extern vmem_t *spl_heap_arena;

	abd_arena = vmem_create("abd_cache", NULL, 0,
	    PAGESIZE, vmem_alloc, vmem_free, spl_heap_arena,
	    262144, VM_SLEEP | VMC_NO_QCACHE);

	ASSERT(abd_arena != NULL);
}

void
segkmem_abd_fini(void)
{
	if (abd_arena) {
		vmem_destroy(abd_arena);
	}
}
