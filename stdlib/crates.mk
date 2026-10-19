CRATES := libc bitflags memchr arrayvec byteorder bytemuck errno linux-raw-sys rustix cfg-if cty valgrind-requests
PUBLIC_CRATES := libc bitflags memchr arrayvec byteorder bytemuck rustix

libc_VERSION := 0.2.190
libc_SHA256 := ce5d3ddc6d3fa000eb1536d85e147bfe31aacaba692ed6a876f95cb7c855be78
libc_EDITION := 2021
libc_CFG_linux := linux_time_bits64

bitflags_VERSION := 2.13.2
bitflags_SHA256 := 3ded4057c258ba199e2d26386d3af3780957ecaee6c4ef4041c6b4b8b97c0b06
bitflags_EDITION := 2021

memchr_VERSION := 2.8.3
memchr_SHA256 := cf8baf1c55e62ffcace7a9f06f4bd9cd3f0c4beb022d3b367256b91b87513d98
memchr_EDITION := 2021
memchr_FEATURES := alloc

arrayvec_VERSION := 0.7.8
arrayvec_SHA256 := d3fb67a6e08acf24fdeccbac2cb6ac4305825bd1f117462e0e6f2f193345ad56
arrayvec_EDITION := 2018

byteorder_VERSION := 1.5.0
byteorder_SHA256 := 1fd0f2584146f6f2ef48085050886acf353beff7305ebd1ae69500e27c67f64b
byteorder_EDITION := 2021

bytemuck_VERSION := 1.25.2
bytemuck_SHA256 := 95832e849adfb21180ccb6826a99da14e5d266ae5c2e668e1602cf234f153797
bytemuck_EDITION := 2018
bytemuck_FEATURES := extern_crate_alloc aarch64_simd avx512_simd align_offset alloc_uninit const_zeroed impl_core_error min_const_generics must_cast must_cast_extra pod_saturating track_caller transparentwrapper_extra zeroable_atomics zeroable_maybe_uninit zeroable_unwind_fn

errno_VERSION := 0.3.14
errno_SHA256 := 39cab71617ae0d63f51a36d69f866391735b51691dbda63cf6f96d042b63efeb
errno_EDITION := 2018
errno_DEPS := libc=libc

linux-raw-sys_VERSION := 0.12.1
linux-raw-sys_SHA256 := 32a66949e030da00e8c7d4434b251670a91556f4144941d37452769c25d58a53
linux-raw-sys_EDITION := 2021
linux-raw-sys_FEATURES := general ioctl no_std prctl system

rustix_VERSION := 1.1.5
rustix_SHA256 := 891efababe418670775f199f0d233d84843c227a0949a883ce15b37c78d6629d
rustix_EDITION := 2021
rustix_FEATURES := use-libc libc libc_errno alloc event fs mm param pipe process rand stdio system thread time
rustix_DEPS := bitflags=bitflags libc=libc libc_errno=errno linux_raw_sys=linux-raw-sys
rustix_CFG := lower_upper_exp_for_non_zero rustc_diagnostics
rustix_CFG_linux := libc linux_raw_dep linux_like linux_kernel
rustix_CFG_macos := libc apple bsd

cfg-if_VERSION := 1.0.5
cfg-if_SHA256 := 4e7648175b45a9a48536d676f68d918270699102aa8dab5496df06904c914600
cfg-if_EDITION := 2018

cty_VERSION := 0.2.2
cty_SHA256 := b365fabc795046672053e29c954733ec3b05e4be654ab130fe8f1f94d7051f35
cty_EDITION := 2015

valgrind-requests_VERSION := 1.1.0
valgrind-requests_SHA256 := 796c44118a551cc842f2ce6fce4983b86e282ec99b6b5e80b09dfeab830e7755
valgrind-requests_EDITION := 2024
valgrind-requests_FEATURES := act stubs
valgrind-requests_DEPS := cfg_if=cfg-if cty=cty
valgrind-requests_CFG_x86_64 := client_requests_support="x86_64"
valgrind-requests_CFG_aarch64 := client_requests_support="aarch64"
valgrind-requests_CFLAGS := -idirafter valgrind/include
valgrind-requests_BINDGEN := valgrind/wrapper.h --use-core --ctypes-prefix cty \
	--allowlist-var VR_IS_PLATFORM_SUPPORTED_BY_VALGRIND --allowlist-var VR_VALGRIND_MAJOR --allowlist-var VR_VALGRIND_MINOR \
	--allowlist-type 'VR_.*ClientRequest' --rustified-enum 'VR_.*ClientRequest' --no-layout-tests
valgrind-requests_CSRC := valgrind/native.c
