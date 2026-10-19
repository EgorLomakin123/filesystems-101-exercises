#![no_std]

use core::ffi::{CStr, c_int};

pub mod ctest {
    use core::ffi::{c_char, c_int};

    unsafe extern "C" {
        pub fn report_file(path: *const c_char);
        pub fn report_error(path: *const c_char, errno_code: c_int);
    }
}

pub fn report_file(path: &CStr) {
    unsafe { ctest::report_file(path.as_ptr()) }
}

pub fn report_error(path: &CStr, errno_code: c_int) {
    unsafe { ctest::report_error(path.as_ptr(), errno_code) }
}

#[doc(hidden)]
#[macro_export]
macro_rules! export_solution {
    ($solution:ident) => {
        #[unsafe(no_mangle)]
        extern "C" fn lsof() {
            let f: fn() = $solution::lsof;
            f()
        }
    };
}
