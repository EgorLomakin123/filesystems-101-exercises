#![no_std]

#[path = "../stdlib/fs101_fuse.rs"]
pub mod fuse;

#[doc(hidden)]
#[macro_export]
macro_rules! export_solution {
    ($solution:ident) => {
        const _: () = {
            use ::core::ffi::{CStr, c_char, c_int};

            #[unsafe(no_mangle)]
            unsafe extern "C" fn helloworld(mntp: *const c_char) -> c_int {
                let f: fn(&CStr) -> c_int = $solution::helloworld;
                f(unsafe { CStr::from_ptr(mntp) })
            }
        };
    };
}
