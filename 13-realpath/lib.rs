#![no_std]

use core::ffi::CStr;

/// Implement this function to expand all symlinks in `path` and
/// convert it to an absolute path that points to the same file
/// or directory as `path` does. It must call fs101::report_path() when
/// all replacements are made. If an error occurs while walking
/// the path, it must call fs101::report_error().
pub fn abspath(path: &CStr) {
    let _ = path;
    todo!()
}
