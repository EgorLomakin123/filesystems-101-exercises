#![no_std]

use core::alloc::{GlobalAlloc, Layout};
use core::ffi::{CStr, c_int, c_void};
use core::fmt::{self, Write};
use core::panic::PanicInfo;
use core::ptr;

unsafe extern "C" {
    fn malloc(size: usize) -> *mut c_void;
    fn calloc(nmemb: usize, size: usize) -> *mut c_void;
    fn realloc(p: *mut c_void, size: usize) -> *mut c_void;
    fn free(p: *mut c_void);
    fn posix_memalign(memptr: *mut *mut c_void, alignment: usize, size: usize) -> c_int;
    fn write(fd: c_int, buf: *const c_void, count: usize) -> isize;
    fn abort() -> !;
}

const MALLOC_ALIGN: usize = 2 * size_of::<usize>();

struct LibcAllocator;

unsafe fn alloc_aligned(layout: Layout) -> *mut u8 {
    let align = layout.align().max(size_of::<*mut c_void>());
    let mut p = ptr::null_mut();
    if unsafe { posix_memalign(&mut p, align, layout.size()) } != 0 {
        return ptr::null_mut();
    }
    p.cast()
}

unsafe impl GlobalAlloc for LibcAllocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        if layout.align() <= MALLOC_ALIGN {
            unsafe { malloc(layout.size()).cast() }
        } else {
            unsafe { alloc_aligned(layout) }
        }
    }

    unsafe fn alloc_zeroed(&self, layout: Layout) -> *mut u8 {
        if layout.align() <= MALLOC_ALIGN {
            unsafe { calloc(1, layout.size()).cast() }
        } else {
            let p = unsafe { alloc_aligned(layout) };
            if !p.is_null() {
                unsafe { p.write_bytes(0, layout.size()) };
            }
            p
        }
    }

    unsafe fn dealloc(&self, p: *mut u8, _layout: Layout) {
        unsafe { free(p.cast()) }
    }

    unsafe fn realloc(&self, p: *mut u8, layout: Layout, new_size: usize) -> *mut u8 {
        if layout.align() <= MALLOC_ALIGN {
            return unsafe { realloc(p.cast(), new_size).cast() };
        }
        let new_layout = unsafe { Layout::from_size_align_unchecked(new_size, layout.align()) };
        let q = unsafe { alloc_aligned(new_layout) };
        if !q.is_null() {
            unsafe {
                ptr::copy_nonoverlapping(p, q, layout.size().min(new_size));
                free(p.cast());
            }
        }
        q
    }
}

#[global_allocator]
static ALLOCATOR: LibcAllocator = LibcAllocator;

struct Stderr;

impl Write for Stderr {
    fn write_str(&mut self, s: &str) -> fmt::Result {
        let mut buf = s.as_bytes();
        while !buf.is_empty() {
            let n = unsafe { write(2, buf.as_ptr().cast(), buf.len()) };
            if n <= 0 {
                return Err(fmt::Error);
            }
            buf = &buf[n as usize..];
        }
        Ok(())
    }
}

struct VgMessage {
    buf: [u8; 1024],
    len: usize,
}

impl Write for VgMessage {
    fn write_str(&mut self, s: &str) -> fmt::Result {
        let s = s.as_bytes();
        if self.len + s.len() >= self.buf.len() {
            return Err(fmt::Error);
        }
        self.buf[self.len..self.len + s.len()].copy_from_slice(s);
        self.len += s.len();
        Ok(())
    }
}

#[panic_handler]
fn panic(info: &PanicInfo<'_>) -> ! {
    let _ = writeln!(Stderr, "{info}");

    let mut msg = VgMessage { buf: [0; 1024], len: 0 };
    let _ = writeln!(msg, "{info}");
    if let Ok(msg) = CStr::from_bytes_until_nul(&msg.buf) {
        valgrind_requests::valgrind_print_backtrace(msg);
    }

    unsafe { abort() }
}

#[unsafe(no_mangle)]
extern "C" fn rust_eh_personality() {}
