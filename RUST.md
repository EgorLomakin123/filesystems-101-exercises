# Solving exercises in Rust

You can write solutions to these exercises in Rust instead of C:

| exercise           | the function lib.rs must define           |
| ------------------ | ----------------------------------------- |
| 00-ps              | `pub fn ps()`                             |
| 01-lsof            | `pub fn lsof()`                           |
| 02-fuse-helloworld | `pub fn helloworld(mntp: &CStr) -> c_int` |
| 13-realpath        | `pub fn abspath(path: &CStr)`             |

To do that, edit `<exercise>/lib.rs`. Your code would be called via the C test
harness the same way the C solution would. The tests are the same for both.

You should only change one of the `lib.rs` and `solution.c`, don't change both,
the grader chooses the solution to test based on the one you did change.
Submitting both would result in a rejection.

## Rules

- You're not allowed to use `std` and your solutions need to start with `#![no_std]`.
- We use the 2024 edition and the warnings are treated as errors.
- The C glue is generated for you, so you are free to use any rust features in your solution. Do not change the type of the function you are implementing.
- Call the harness callbacks through the `fs101` crate (`fs101.rs` in
  `<exercise>`): for example `fs101::report_file(path)`. The raw C
  declarations are in `fs101::ctest`
- You're given a set of crates, including `rustix` for safe system call wrappers and `libc` for raw system calls when needed.
- Tests run under Valgrind so don't leak memory or leave file descriptors open. The allocator used is the global allocator (`libc` `malloc()`/`free()`).
- Don't define your own `#[panic_handler]` or `#[global_allocator]`.

These crates are available with `use` (no `Cargo.toml` needed):

| crate     | version | notes                                                                                                               |
| --------- | ------- | ------------------------------------------------------------------------------------------------------------------- |
| rustix    | 1.1.5   | libc backend; `fs`, `process`, `pipe`, `mm`, `event`, `thread`, `time`, `stdio`, `param`, `rand`, `system`, `alloc` |
| libc      | 0.2.190 |                                                                                                                     |
| bitflags  | 2.13.2  |                                                                                                                     |
| memchr    | 2.8.3   | feature `alloc`                                                                                                     |
| arrayvec  | 0.7.8   | no `std`                                                                                                            |
| byteorder | 1.5.0   | no `std`                                                                                                            |
| bytemuck  | 1.25.2  | no `derive`                                                                                                         |

## Example: 01-lsof

```rust
#![no_std]
#![forbid(unsafe_code)]

extern crate alloc;

use alloc::ffi::CString;
use alloc::format;
use alloc::vec::Vec;

use rustix::fs::{CWD, Dir, Mode, OFlags, openat, readlinkat};

pub fn lsof() {
    let dir = openat(CWD, c"/proc/self/fd", OFlags::RDONLY | OFlags::DIRECTORY, Mode::empty())
        .and_then(Dir::new);
    let dir = match dir {
        Ok(dir) => dir,
        Err(e) => return fs101::report_error(c"/proc/self/fd", e.raw_os_error()),
    };
    for entry in dir {
        let Ok(entry) = entry else { break };
        let name = entry.file_name().to_str().unwrap();
        let link = CString::new(format!("/proc/self/fd/{name}")).unwrap();
        if let Ok(target) = readlinkat(CWD, link.as_c_str(), Vec::new()) {
            fs101::report_file(&target);
        }
    }
}
```

(This lists only the solution's own files, so it doesn't pass the tests.)

## Building

To build run `make -f Makefile.rust` in `<exercise>`. It eeds rustc 1.85 or
newer on `PATH`, plus `bindgen`, `curl`, `tar`, `sha256sum` and the Valgrind
headers.

The first build downloads the crates above and compiles them into
`~/.cache/fs101-rust`, later builds reuse them until rustc changes. Build
artifacts go to `.rust/`.

N.B. Keep in mind that the grader may run a different rustc version than yours.
Since warnings are errors, a lint that only one of the versions knows about can
make the build fail on the grader side and vice versa.

For rust-analyzer, run `make -f Makefile.rust rust-project.json` in
`<exercise>` and open that directory in your editor.
