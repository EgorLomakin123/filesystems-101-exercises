#![no_std]

use core::ffi::{CStr, c_int};

pub mod ctest {
    use core::ffi::{c_char, c_int};

    unsafe extern "C" {
        pub fn report_path(path: *const c_char);
        pub fn report_error(parent: *const c_char, child: *const c_char, errno_code: c_int);
    }
}

pub fn report_path(path: &CStr) {
    unsafe { ctest::report_path(path.as_ptr()) }
}

pub fn report_error(parent: &CStr, child: &CStr, errno_code: c_int) {
    unsafe { ctest::report_error(parent.as_ptr(), child.as_ptr(), errno_code) }
}

#[doc(hidden)]
#[macro_export]
macro_rules! export_solution {
    ($solution:ident) => {
        const _: () = {
            use ::core::ffi::{CStr, c_char};

            #[unsafe(no_mangle)]
            unsafe extern "C" fn abspath(path: *const c_char) {
                let f: fn(&CStr) = $solution::abspath;
                f(unsafe { CStr::from_ptr(path) })
            }
        };
    };
}
