#![allow(non_camel_case_types)]

use core::ffi::{CStr, c_char, c_int, c_uint, c_void};
use core::ptr;

use libc::{flock, gid_t, mode_t, off_t, pid_t, size_t, ssize_t, stat, statvfs, timespec, uid_t};

#[repr(C)]
#[derive(Debug)]
pub struct fuse_file_info {
    pub flags: c_int,
    pub bits: c_uint,
    padding2: c_uint,
    pub fh: u64,
    pub lock_owner: u64,
    pub poll_events: u32,
}

macro_rules! file_info_bits {
    ($($get:ident, $set:ident = $bit:expr;)*) => {
        impl fuse_file_info {
            $(
                pub fn $get(&self) -> bool {
                    self.bits & (1 << $bit) != 0
                }

                pub fn $set(&mut self, on: bool) {
                    if on {
                        self.bits |= 1 << $bit;
                    } else {
                        self.bits &= !(1 << $bit);
                    }
                }
            )*
        }
    };
}

file_info_bits! {
    writepage, set_writepage = 0;
    direct_io, set_direct_io = 1;
    keep_cache, set_keep_cache = 2;
    flush, set_flush = 3;
    nonseekable, set_nonseekable = 4;
    flock_release, set_flock_release = 5;
    cache_readdir, set_cache_readdir = 6;
    noflush, set_noflush = 7;
}

#[repr(C)]
pub struct fuse_conn_info {
    _private: [u8; 0],
}

#[repr(C)]
pub struct fuse_config {
    _private: [u8; 0],
}

#[repr(C)]
pub struct fuse_pollhandle {
    _private: [u8; 0],
}

#[repr(C)]
pub struct fuse_bufvec {
    _private: [u8; 0],
}

#[repr(C)]
pub struct fuse {
    _private: [u8; 0],
}

pub type fuse_readdir_flags = c_uint;
pub const FUSE_READDIR_PLUS: fuse_readdir_flags = 1 << 0;

pub type fuse_fill_dir_flags = c_uint;
pub const FUSE_FILL_DIR_PLUS: fuse_fill_dir_flags = 1 << 1;

pub type fuse_fill_dir_t = Option<
    unsafe extern "C" fn(
        buf: *mut c_void,
        name: *const c_char,
        stbuf: *const stat,
        off: off_t,
        flags: fuse_fill_dir_flags,
    ) -> c_int,
>;

#[repr(C)]
#[derive(Clone, Copy)]
pub struct fuse_operations {
    pub getattr: Option<unsafe extern "C" fn(*const c_char, *mut stat, *mut fuse_file_info) -> c_int>,
    pub readlink: Option<unsafe extern "C" fn(*const c_char, *mut c_char, size_t) -> c_int>,
    pub mknod: Option<unsafe extern "C" fn(*const c_char, mode_t, libc::dev_t) -> c_int>,
    pub mkdir: Option<unsafe extern "C" fn(*const c_char, mode_t) -> c_int>,
    pub unlink: Option<unsafe extern "C" fn(*const c_char) -> c_int>,
    pub rmdir: Option<unsafe extern "C" fn(*const c_char) -> c_int>,
    pub symlink: Option<unsafe extern "C" fn(*const c_char, *const c_char) -> c_int>,
    pub rename: Option<unsafe extern "C" fn(*const c_char, *const c_char, c_uint) -> c_int>,
    pub link: Option<unsafe extern "C" fn(*const c_char, *const c_char) -> c_int>,
    pub chmod: Option<unsafe extern "C" fn(*const c_char, mode_t, *mut fuse_file_info) -> c_int>,
    pub chown: Option<unsafe extern "C" fn(*const c_char, uid_t, gid_t, *mut fuse_file_info) -> c_int>,
    pub truncate: Option<unsafe extern "C" fn(*const c_char, off_t, *mut fuse_file_info) -> c_int>,
    pub open: Option<unsafe extern "C" fn(*const c_char, *mut fuse_file_info) -> c_int>,
    pub read: Option<
        unsafe extern "C" fn(*const c_char, *mut c_char, size_t, off_t, *mut fuse_file_info) -> c_int,
    >,
    pub write: Option<
        unsafe extern "C" fn(*const c_char, *const c_char, size_t, off_t, *mut fuse_file_info) -> c_int,
    >,
    pub statfs: Option<unsafe extern "C" fn(*const c_char, *mut statvfs) -> c_int>,
    pub flush: Option<unsafe extern "C" fn(*const c_char, *mut fuse_file_info) -> c_int>,
    pub release: Option<unsafe extern "C" fn(*const c_char, *mut fuse_file_info) -> c_int>,
    pub fsync: Option<unsafe extern "C" fn(*const c_char, c_int, *mut fuse_file_info) -> c_int>,
    pub setxattr: Option<
        unsafe extern "C" fn(*const c_char, *const c_char, *const c_char, size_t, c_int) -> c_int,
    >,
    pub getxattr: Option<unsafe extern "C" fn(*const c_char, *const c_char, *mut c_char, size_t) -> c_int>,
    pub listxattr: Option<unsafe extern "C" fn(*const c_char, *mut c_char, size_t) -> c_int>,
    pub removexattr: Option<unsafe extern "C" fn(*const c_char, *const c_char) -> c_int>,
    pub opendir: Option<unsafe extern "C" fn(*const c_char, *mut fuse_file_info) -> c_int>,
    pub readdir: Option<
        unsafe extern "C" fn(
            *const c_char,
            *mut c_void,
            fuse_fill_dir_t,
            off_t,
            *mut fuse_file_info,
            fuse_readdir_flags,
        ) -> c_int,
    >,
    pub releasedir: Option<unsafe extern "C" fn(*const c_char, *mut fuse_file_info) -> c_int>,
    pub fsyncdir: Option<unsafe extern "C" fn(*const c_char, c_int, *mut fuse_file_info) -> c_int>,
    pub init: Option<unsafe extern "C" fn(*mut fuse_conn_info, *mut fuse_config) -> *mut c_void>,
    pub destroy: Option<unsafe extern "C" fn(*mut c_void)>,
    pub access: Option<unsafe extern "C" fn(*const c_char, c_int) -> c_int>,
    pub create: Option<unsafe extern "C" fn(*const c_char, mode_t, *mut fuse_file_info) -> c_int>,
    pub lock: Option<unsafe extern "C" fn(*const c_char, *mut fuse_file_info, c_int, *mut flock) -> c_int>,
    pub utimens: Option<unsafe extern "C" fn(*const c_char, *const timespec, *mut fuse_file_info) -> c_int>,
    pub bmap: Option<unsafe extern "C" fn(*const c_char, size_t, *mut u64) -> c_int>,
    pub ioctl: Option<
        unsafe extern "C" fn(*const c_char, c_int, *mut c_void, *mut fuse_file_info, c_uint, *mut c_void) -> c_int,
    >,
    pub poll: Option<
        unsafe extern "C" fn(*const c_char, *mut fuse_file_info, *mut fuse_pollhandle, *mut c_uint) -> c_int,
    >,
    pub write_buf: Option<
        unsafe extern "C" fn(*const c_char, *mut fuse_bufvec, off_t, *mut fuse_file_info) -> c_int,
    >,
    pub read_buf: Option<
        unsafe extern "C" fn(*const c_char, *mut *mut fuse_bufvec, size_t, off_t, *mut fuse_file_info) -> c_int,
    >,
    pub flock: Option<unsafe extern "C" fn(*const c_char, *mut fuse_file_info, c_int) -> c_int>,
    pub fallocate: Option<unsafe extern "C" fn(*const c_char, c_int, off_t, off_t, *mut fuse_file_info) -> c_int>,
    pub copy_file_range: Option<
        unsafe extern "C" fn(
            *const c_char,
            *mut fuse_file_info,
            off_t,
            *const c_char,
            *mut fuse_file_info,
            off_t,
            size_t,
            c_int,
        ) -> ssize_t,
    >,
    pub lseek: Option<unsafe extern "C" fn(*const c_char, off_t, c_int, *mut fuse_file_info) -> off_t>,
}

impl fuse_operations {
    pub const EMPTY: Self = Self {
        getattr: None,
        readlink: None,
        mknod: None,
        mkdir: None,
        unlink: None,
        rmdir: None,
        symlink: None,
        rename: None,
        link: None,
        chmod: None,
        chown: None,
        truncate: None,
        open: None,
        read: None,
        write: None,
        statfs: None,
        flush: None,
        release: None,
        fsync: None,
        setxattr: None,
        getxattr: None,
        listxattr: None,
        removexattr: None,
        opendir: None,
        readdir: None,
        releasedir: None,
        fsyncdir: None,
        init: None,
        destroy: None,
        access: None,
        create: None,
        lock: None,
        utimens: None,
        bmap: None,
        ioctl: None,
        poll: None,
        write_buf: None,
        read_buf: None,
        flock: None,
        fallocate: None,
        copy_file_range: None,
        lseek: None,
    };
}

impl Default for fuse_operations {
    fn default() -> Self {
        Self::EMPTY
    }
}

#[repr(C)]
#[derive(Debug)]
pub struct fuse_context {
    pub fuse: *mut fuse,
    pub uid: uid_t,
    pub gid: gid_t,
    pub pid: pid_t,
    pub private_data: *mut c_void,
    pub umask: mode_t,
}

unsafe extern "C" {
    pub fn fuse_get_context() -> *mut fuse_context;

    pub fn fuse_main_real(
        argc: c_int,
        argv: *mut *mut c_char,
        op: *const fuse_operations,
        op_size: size_t,
        private_data: *mut c_void,
    ) -> c_int;
}

pub fn fuse_main(args: &[&CStr], op: &fuse_operations, private_data: *mut c_void) -> c_int {
    const MAX_ARGS: usize = 32;
    assert!(args.len() < MAX_ARGS, "fuse_main: too many arguments");

    let mut argv = [ptr::null_mut::<c_char>(); MAX_ARGS];
    for (slot, arg) in argv.iter_mut().zip(args) {
        *slot = arg.as_ptr().cast_mut();
    }
    unsafe {
        fuse_main_real(
            args.len() as c_int,
            argv.as_mut_ptr(),
            op,
            size_of::<fuse_operations>(),
            private_data,
        )
    }
}
