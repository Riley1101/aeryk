//! Port of kernel/src/arch/x86_64/slab.c: a fixed-size-class slab
//! allocator over pages from the C physical memory manager (`pmm_alloc_page`
//! / `pmm_free_page`). Exposes the same `init_slab`/`kmalloc`/`kfree` C ABI
//! so it's a drop-in replacement for the old slab.c/slab.h pair — every
//! other kernel/src/*.c caller is unchanged.
//!
//! Metadata (`Slab`, `SlabChunk`) is written directly into the raw page
//! it describes, exactly as the C version did, so this is inherently
//! `unsafe` raw-pointer work; the port's value is in the cache-selection
//! and list-bookkeeping arithmetic being checked at compile time instead
//! of hand-verified C.

use core::ffi::c_void;
use core::ptr;

use crate::sys::{hhdm_offset, pmm_alloc_page, pmm_free_page, print};

const SLAB_MAGIC: u32 = 0x51AB_51AB;
const NUM_CACHES: usize = 8;
const CACHE_SIZES: [usize; NUM_CACHES] = [16, 32, 64, 128, 256, 512, 1024, 2048];
const PAGE_SIZE: u64 = 4096;

#[repr(C)]
struct SlabChunk {
    next: *mut SlabChunk,
}

#[repr(C)]
struct Slab {
    magic: u32,
    next: *mut Slab,
    prev: *mut Slab,
    cache: *mut SlabCache,
    free_list: *mut SlabChunk,
    free_chunks: usize,
    total_chunks: usize,
}

#[repr(C)]
#[derive(Clone, Copy)]
struct SlabCache {
    object_size: usize,
    slabs_partial: *mut Slab,
    slabs_free: *mut Slab,
    slabs_full: *mut Slab,
}

const EMPTY_CACHE: SlabCache = SlabCache {
    object_size: 0,
    slabs_partial: ptr::null_mut(),
    slabs_free: ptr::null_mut(),
    slabs_full: ptr::null_mut(),
};

static mut KMALLOC_CACHES: [SlabCache; NUM_CACHES] = [EMPTY_CACHE; NUM_CACHES];

unsafe fn list_remove(head: *mut *mut Slab, s: *mut Slab) {
    if !(*s).prev.is_null() {
        (*(*s).prev).next = (*s).next;
    }
    if !(*s).next.is_null() {
        (*(*s).next).prev = (*s).prev;
    }
    if *head == s {
        *head = (*s).next;
    }
}

unsafe fn list_add(head: *mut *mut Slab, s: *mut Slab) {
    (*s).next = *head;
    (*s).prev = ptr::null_mut();
    if !(*head).is_null() {
        (*(*head)).prev = s;
    }
    *head = s;
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn init_slab() {
    for i in 0..NUM_CACHES {
        KMALLOC_CACHES[i] = SlabCache {
            object_size: CACHE_SIZES[i],
            ..EMPTY_CACHE
        };
    }
}

unsafe fn allocate_new_slab(cache: *mut SlabCache) -> *mut Slab {
    let raw_page_phys = pmm_alloc_page();
    if raw_page_phys.is_null() {
        return ptr::null_mut();
    }

    let raw_page_virt = (raw_page_phys as u64 + hhdm_offset) as *mut u8;
    ptr::write_bytes(raw_page_virt, 0, PAGE_SIZE as usize);

    let new_slab = raw_page_virt as *mut Slab;
    (*new_slab).magic = SLAB_MAGIC;
    (*new_slab).cache = cache;
    (*new_slab).next = ptr::null_mut();
    (*new_slab).prev = ptr::null_mut();
    (*new_slab).free_list = ptr::null_mut();
    (*new_slab).free_chunks = 0;

    let data_start = (raw_page_virt as u64 + core::mem::size_of::<Slab>() as u64 + 15) & !15;
    let data_end = raw_page_virt as u64 + PAGE_SIZE;
    let obj_size = (*cache).object_size as u64;

    let mut chunk_addr = data_start;
    while chunk_addr + obj_size <= data_end {
        let chunk = chunk_addr as *mut SlabChunk;
        (*chunk).next = (*new_slab).free_list;
        (*new_slab).free_list = chunk;
        (*new_slab).free_chunks += 1;
        chunk_addr += obj_size;
    }
    (*new_slab).total_chunks = (*new_slab).free_chunks;

    new_slab
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn kmalloc(size: usize) -> *mut c_void {
    if size == 0 {
        return ptr::null_mut();
    }
    if size > 2048 {
        print(c"[!] kmalloc: Allocations > 2048 bytes not supported!\n".as_ptr().cast());
        return ptr::null_mut();
    }

    let mut cache: *mut SlabCache = ptr::null_mut();
    for i in 0..NUM_CACHES {
        if size <= CACHE_SIZES[i] {
            cache = &raw mut KMALLOC_CACHES[i];
            break;
        }
    }
    if cache.is_null() {
        return ptr::null_mut();
    }

    let mut target_slab = (*cache).slabs_partial;
    if target_slab.is_null() {
        target_slab = (*cache).slabs_free;
        if !target_slab.is_null() {
            list_remove(&raw mut (*cache).slabs_free, target_slab);
            list_add(&raw mut (*cache).slabs_partial, target_slab);
        } else {
            target_slab = allocate_new_slab(cache);
            if target_slab.is_null() {
                return ptr::null_mut();
            }
            list_add(&raw mut (*cache).slabs_partial, target_slab);
        }
    }

    let chunk = (*target_slab).free_list;
    (*target_slab).free_list = (*chunk).next;
    (*target_slab).free_chunks -= 1;

    if (*target_slab).free_chunks == 0 {
        list_remove(&raw mut (*cache).slabs_partial, target_slab);
        list_add(&raw mut (*cache).slabs_full, target_slab);
    }

    chunk as *mut c_void
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn kfree(ptr_in: *mut c_void) {
    if ptr_in.is_null() {
        return;
    }

    // Slabs are aligned exactly to 4KiB pages, so the metadata is found by
    // masking out the low 12 bits of the pointer.
    let page_base = (ptr_in as u64) & !0xFFFu64;
    let slab = page_base as *mut Slab;

    if (*slab).magic != SLAB_MAGIC {
        print(
            c"[!] kfree: Invalid pointer or magic mismatch. Memory corruption avoided.\n"
                .as_ptr()
                .cast(),
        );
        return;
    }

    let cache = (*slab).cache;

    if (*slab).free_chunks == 0 {
        list_remove(&raw mut (*cache).slabs_full, slab);
        list_add(&raw mut (*cache).slabs_partial, slab);
    }

    let chunk = ptr_in as *mut SlabChunk;
    (*chunk).next = (*slab).free_list;
    (*slab).free_list = chunk;
    (*slab).free_chunks += 1;

    if (*slab).free_chunks == (*slab).total_chunks {
        list_remove(&raw mut (*cache).slabs_partial, slab);

        let phys = (page_base - hhdm_offset) as *mut c_void;
        pmm_free_page(phys);
    }
}
