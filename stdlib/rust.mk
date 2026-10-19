.PHONY: build test rust-project.json

test: build
	./a.out

build: a.out

include ../stdlib/crates.mk

RUSTC ?= rustc
BINDGEN ?= bindgen

SHA256SUM := $(shell command -v sha256sum >/dev/null 2>&1 && echo sha256sum || echo 'shasum -a 256')
TARGET_OS := $(shell $(RUSTC) --print cfg | sed -n 's/^target_os="\(.*\)"$$/\1/p')
TARGET_ARCH := $(shell $(RUSTC) --print cfg | sed -n 's/^target_arch="\(.*\)"$$/\1/p')

CRATE_RUSTFLAGS := -C panic=abort -C opt-level=1 -g --cap-lints=allow
RUST_KEY := $(shell { $(RUSTC) -vV; $(BINDGEN) --version 2>/dev/null; echo '$(CRATE_RUSTFLAGS)'; cat ../stdlib/crates.mk; } | $(SHA256SUM) | cut -c1-16)

FS101_RUST_CACHE ?= $(or $(XDG_CACHE_HOME),$(HOME)/.cache)/fs101-rust
CRATE_SRC := $(FS101_RUST_CACHE)/src
CRATE_LIB := $(FS101_RUST_CACHE)/lib/$(RUST_KEY)

crate_lib = $(subst -,_,$(1))
crate_dir = $(CRATE_SRC)/$(1)-$($(1)_VERSION)
crate_rlib = $(CRATE_LIB)/lib$(call crate_lib,$(1)).rlib
crate_out = $(CRATE_LIB)/$(1)-out
crate_bindings = $(if $($(1)_BINDGEN),$(call crate_out,$(1))/bindings.rs)
crate_cobj = $(if $($(1)_CSRC),$(CRATE_LIB)/$(call crate_lib,$(1)).o)
dep_alias = $(firstword $(subst =, ,$(1)))
dep_crate = $(lastword $(subst =, ,$(1)))
crate_cfgs = $($(1)_CFG) $($(1)_CFG_$(TARGET_OS)) $($(1)_CFG_$(TARGET_ARCH))
crate_cfg_flags = $(foreach f,$($(1)_FEATURES),--cfg 'feature="$(f)"') $(foreach c,$(call crate_cfgs,$(1)),--cfg '$(c)')
crate_externs = $(foreach d,$($(1)_DEPS),--extern $(call dep_alias,$(d))=$(call crate_rlib,$(call dep_crate,$(d))))

define crate_rules
$(call crate_dir,$(1))/Cargo.toml:
	@mkdir -p $(CRATE_SRC)
	@echo "downloading $(1) $($(1)_VERSION)"
	@set -e; tmp=$$$$(mktemp -d $(CRATE_SRC)/.tmp.XXXXXX); trap 'rm -rf "$$$$tmp"' EXIT; \
	curl -sSfL -o "$$$$tmp/crate" https://static.crates.io/crates/$(1)/$(1)-$($(1)_VERSION).crate; \
	echo "$($(1)_SHA256)  $$$$tmp/crate" | $(SHA256SUM) -c --status || { echo "$(1)-$($(1)_VERSION).crate: sha256 mismatch" >&2; exit 1; }; \
	tar -xzf "$$$$tmp/crate" -C "$$$$tmp"; \
	mv "$$$$tmp/$(1)-$($(1)_VERSION)" $(CRATE_SRC)/ 2>/dev/null || test -f $$@

$(call crate_rlib,$(1)): $(call crate_dir,$(1))/Cargo.toml $(call crate_bindings,$(1)) $(foreach d,$($(1)_DEPS),$(call crate_rlib,$(call dep_crate,$(d))))
	@mkdir -p $(CRATE_LIB)
	@echo "compiling $(1) $($(1)_VERSION)"
	@set -e; tmp=$$$$(mktemp -d $(CRATE_LIB)/.tmp.XXXXXX); trap 'rm -rf "$$$$tmp"' EXIT; \
	$(if $($(1)_BINDGEN),OUT_DIR=$(call crate_out,$(1))) $(RUSTC) --crate-name=$(call crate_lib,$(1)) --crate-type=rlib --edition=$($(1)_EDITION) \
		$(CRATE_RUSTFLAGS) -C metadata=$(1)-$($(1)_VERSION) -L dependency=$(CRATE_LIB) \
		$(call crate_cfg_flags,$(1)) $(call crate_externs,$(1)) \
		-o "$$$$tmp/lib.rlib" $(call crate_dir,$(1))/src/lib.rs; \
	mv "$$$$tmp/lib.rlib" $$@
endef

define bindgen_rules
$(call crate_bindings,$(1)): $(call crate_dir,$(1))/Cargo.toml
	@mkdir -p $(CRATE_LIB)
	@echo "generating bindings for $(1) $($(1)_VERSION)"
	@set -e; tmp=$$$$(mktemp -d $(CRATE_LIB)/.tmp.XXXXXX); trap 'rm -rf "$$$$tmp"' EXIT; \
	(cd $(call crate_dir,$(1)) && $(BINDGEN) $($(1)_BINDGEN) -o "$$$$tmp/bindings.rs" -- $($(1)_CFLAGS)); \
	mkdir -p $(call crate_out,$(1)); \
	mv "$$$$tmp/bindings.rs" $$@
endef

define csrc_rules
$(call crate_cobj,$(1)): $(call crate_dir,$(1))/Cargo.toml
	@mkdir -p $(CRATE_LIB)
	@echo "compiling $(1) $($(1)_VERSION) C sources"
	@set -e; tmp=$$$$(mktemp -d $(CRATE_LIB)/.tmp.XXXXXX); trap 'rm -rf "$$$$tmp"' EXIT; \
	(cd $(call crate_dir,$(1)) && gcc -c -g -O1 $($(1)_CFLAGS) -o "$$$$tmp/c.o" $($(1)_CSRC)); \
	mv "$$$$tmp/c.o" $$@
endef

$(foreach c,$(CRATES),$(eval $(call crate_rules,$(c))))
$(foreach c,$(CRATES),$(if $($(c)_BINDGEN),$(eval $(call bindgen_rules,$(c)))))
$(foreach c,$(CRATES),$(if $($(c)_CSRC),$(eval $(call csrc_rules,$(c)))))

SRC_STDLIB := $(wildcard ../stdlib/*.c)
HDR_STDLIB := $(wildcard ../stdlib/*.h)

RUST_OUT := .rust/$(RUST_KEY)
RUST_SRC := $(filter-out fs101.rs,$(wildcard *.rs */*.rs */*/*.rs))
RUST_STDLIB := $(wildcard ../stdlib/*.rs)

RUSTFLAGS := \
	--edition=2024 \
	-C panic=abort -C opt-level=1 -g -C debug-assertions=on \
	-L dependency=$(CRATE_LIB) -L dependency=$(RUST_OUT)
PUBLIC_RLIBS := $(foreach c,$(PUBLIC_CRATES),$(call crate_rlib,$(c)))
RUST_EXTERNS := $(foreach c,$(PUBLIC_CRATES),--extern $(call crate_lib,$(c))=$(call crate_rlib,$(c)))

RUST_STATICLIB := $(RUST_OUT)/libfs101_main.a
CRATE_COBJS := $(strip $(foreach c,$(CRATES),$(call crate_cobj,$(c))))

a.out: $(HARNESS_SRC) $(wildcard *.h) $(SRC_STDLIB) $(HDR_STDLIB) $(RUST_STATICLIB) $(CRATE_COBJS)
	gcc \
		-std=gnu11 -Wall -Wextra -Werror \
		-I. -I../stdlib \
		-D_GNU_SOURCE \
		$(HARNESS_CFLAGS) \
		-g -Og \
		$(HARNESS_SRC) $(RUST_STATICLIB) $(CRATE_COBJS) $(SRC_STDLIB) \
		$(HARNESS_LDLIBS)

$(RUST_OUT)/libstd_forbidden.rlib: ../stdlib/std_forbidden.rs
	@mkdir -p $(RUST_OUT)
	$(RUSTC) $(RUSTFLAGS) --crate-type=rlib --crate-name=std_forbidden -o $@ $<

$(RUST_OUT)/libfs101_rt.rlib: ../stdlib/fs101_rt.rs $(call crate_rlib,valgrind-requests)
	@mkdir -p $(RUST_OUT)
	$(RUSTC) $(RUSTFLAGS) --extern valgrind_requests=$(call crate_rlib,valgrind-requests) \
		--crate-type=rlib --crate-name=fs101_rt -o $@ $<

$(RUST_OUT)/libfs101.rlib: fs101.rs $(RUST_STDLIB) $(PUBLIC_RLIBS)
	@mkdir -p $(RUST_OUT)
	$(RUSTC) $(RUSTFLAGS) $(RUST_EXTERNS) --crate-type=rlib --crate-name=fs101 -o $@ fs101.rs

$(RUST_OUT)/libsolution.rlib: $(RUST_SRC) $(RUST_OUT)/libfs101.rlib $(RUST_OUT)/libstd_forbidden.rlib
	@grep -Eq '^[[:space:]]*#!\[no_std\]' lib.rs || { \
		echo 'lib.rs: error: a Rust solution must be a #![no_std] crate.' >&2; \
		echo 'Add #![no_std] (and `extern crate alloc;` to use Vec, String, Box) to the top of lib.rs.' >&2; \
		exit 1; }
	$(RUSTC) $(RUSTFLAGS) -D warnings \
		--extern std=$(RUST_OUT)/libstd_forbidden.rlib \
		--extern fs101=$(RUST_OUT)/libfs101.rlib \
		$(RUST_EXTERNS) \
		--crate-type=rlib --crate-name=solution -o $@ lib.rs

$(RUST_STATICLIB): ../stdlib/fs101_main.rs $(RUST_OUT)/libsolution.rlib $(RUST_OUT)/libfs101_rt.rlib
	$(RUSTC) $(RUSTFLAGS) \
		--extern fs101_rt=$(RUST_OUT)/libfs101_rt.rlib \
		--extern fs101=$(RUST_OUT)/libfs101.rlib \
		--extern solution=$(RUST_OUT)/libsolution.rlib \
		--crate-type=staticlib --crate-name=fs101_main -o $@ $<

comma := ,
empty :=
space := $(empty) $(empty)
json_list = [$(subst $(space),$(comma),$(strip $(1)))]
$(foreach c,$(CRATES),$(eval crate_index_$(c) := $(words $(crate_indices)))$(eval crate_indices += $(c)))

SYSROOT = $(shell $(RUSTC) --print sysroot)
SYSROOT_SRC = $(or $(wildcard $(SYSROOT)/lib/rustlib/src/rust/library),$(RUST_SRC_PATH))

json_dep = {"crate":$(crate_index_$(1)),"name":"$(2)"}
crate_deps_json = $(foreach d,$($(1)_DEPS),$(call json_dep,$(call dep_crate,$(d)),$(call dep_alias,$(d))))
crate_cfgs_json = $(foreach f,$($(1)_FEATURES),"feature=\"$(f)\"") $(foreach c,$(call crate_cfgs,$(1)),"$(subst ",\",$(c))")
crate_env_json = $(if $($(1)_BINDGEN),"env":{"OUT_DIR":"$(call crate_out,$(1))"}$(comma))
crate_json = {"display_name":"$(1)","root_module":"$(call crate_dir,$(1))/src/lib.rs","edition":"$($(1)_EDITION)",$(call crate_env_json,$(1))"deps":$(call json_list,$(call crate_deps_json,$(1))),"cfg":$(call json_list,$(call crate_cfgs_json,$(1))),"is_workspace_member":false}
fs101_json = {"display_name":"fs101","root_module":"fs101.rs","edition":"2024","deps":[{"crate":$(crate_index_libc),"name":"libc"}],"cfg":[],"is_workspace_member":true,"source":{"include_dirs":[".","../stdlib"],"exclude_dirs":[]}}
solution_deps_json = {"crate":$(words $(CRATES)),"name":"fs101"} $(foreach c,$(PUBLIC_CRATES),$(call json_dep,$(c),$(call crate_lib,$(c))))
solution_json = {"display_name":"solution","root_module":"lib.rs","edition":"2024","deps":$(call json_list,$(solution_deps_json)),"cfg":[],"is_workspace_member":true}
sysroot_src_json = $(if $(SYSROOT_SRC),"sysroot_src":"$(SYSROOT_SRC)"$(comma))
rust_project_json = {"sysroot":"$(SYSROOT)",$(sysroot_src_json)"crates":$(call json_list,$(foreach c,$(CRATES),$(call crate_json,$(c))) $(fs101_json) $(solution_json))}

rust-project.json: $(foreach c,$(CRATES),$(call crate_dir,$(c))/Cargo.toml $(call crate_bindings,$(c)))
	@printf '%s\n' '$(rust_project_json)' > $@
