#![no_std]

extern crate alloc;

use alloc::vec::Vec;
use core::ffi::{CStr, c_char, c_int};
use core::ptr;

pub use libc::pid_t;

pub mod ctest {
    use core::ffi::{c_char, c_int};

    unsafe extern "C" {
        pub fn report_process(
            pid: libc::pid_t,
            exe: *const c_char,
            argv: *mut *mut c_char,
            envp: *mut *mut c_char,
        );
        pub fn report_error(path: *const c_char, errno_code: c_int);
    }
}

pub fn report_process(pid: pid_t, exe: &CStr, argv: &[&CStr], envp: &[&CStr]) {
    fn to_c(v: &[&CStr]) -> Vec<*mut c_char> {
        v.iter()
            .map(|s| s.as_ptr().cast_mut())
            .chain([ptr::null_mut()])
            .collect()
    }

    let mut argv = to_c(argv);
    let mut envp = to_c(envp);
    unsafe { ctest::report_process(pid, exe.as_ptr(), argv.as_mut_ptr(), envp.as_mut_ptr()) }
}

pub fn report_error(path: &CStr, errno_code: c_int) {
    unsafe { ctest::report_error(path.as_ptr(), errno_code) }
}

#[doc(hidden)]
#[macro_export]
macro_rules! export_solution {
    ($solution:ident) => {
        #[unsafe(no_mangle)]
        extern "C" fn ps() {
            let f: fn() = $solution::ps;
            f()
        }
    };
}
