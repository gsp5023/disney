/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

#include <stdint.h>

#ifdef FFI_GEN
#define FFI_EXPORT __attribute__((ffi_app_export))
#define FFI_NAME(_name) __attribute__((ffi_export_name(_name)))
#define FFI_PTR_WASM __attribute__((ffi_app_ptr))
#define FFI_WASM_CALLBACK __attribute((ffi_app_callback))
#define FFI_PTR_NATIVE __attribute__((ffi_m5_ptr))
#define FFI_TYPE_OVERRIDE(_type) __attribute__((ffi_app_type(_type)))
#define FFI_MALLOC_TAG __attribute__((ffi_emit_malloc_tag))
#define FFI_CAN_BE_NULL __attribute__((ffi_can_be_null))
#define FFI_FIELD_NAME(_old, _name) __attribute__((ffi_field_name(_old, _name)))
#define FFI_ENUM_BITFLAGS __attribute__((ffi_enum_bitflags))
#define FFI_ENUM_CLEAN_NAMES __attribute__((ffi_enum_clean_names))
#define FFI_ENUM_TRIM_START_NAMES(_prefix) __attribute__((ffi_enum_trim_start_names(_prefix)))
#define FFI_ENUM_CAPITALIZE_NAMES __attribute__((ffi_enum_capitalize_names))
#define FFI_ENUM_DEFAULT(_default_choice) __attribute__((ffi_enum_default(_default_choice)))
#define FFI_DERIVE_DEFAULT __attribute__((ffi_derive_default))
#define FFI_TYPE_MODULE(_mod) __attribute__((ffi_type_module(_mod)))
#define FFI_DROP(_destructor) __attribute__((ffi_type_drop(_destructor)))
#define FFI_SINGLE __attribute__((ffi_single))
#define FFI_SLICE __attribute__((ffi_slice))
#define FFI_ELEMENT_SIZE_OF(_arg) __attribute__((ffi_element_size_of(_arg)))
#define FFI_RETURN(_arg) __attribute__((ffi_return_arg(_arg)))
#define FFI_ALWAYS_TYPED __attribute__((ffi_always_typed))
#define FFI_NO_RUST_THUNK __attribute__((ffi_no_rust_thunk))
#define FFI_PRIVATE(_field_name) __attribute__((ffi_attr_private(_field_name)))
#define FFI_IMPLEMENT_IF_DEF(_macro) __attribute__((ffi_attr_impl_if_def(_macro)))
#define FFI_UNSAFE __attribute__((ffi_unsafe))
#define FFI_PUB_CRATE __attribute__((ffi_pub_crate))
#define FFI_DISCRIMINATED_UNION __attribute__((ffi_attr_discriminated_union))

#define EXT_EXPORT __attribute__((ffi_ext_export))
#define EXT_LINK_IF_DEF(_macro) __attribute__((ffi_ext_if_def(_macro)))
#else

///
/// This function or type declaration will be made available to the app.
///
/// (This annotation can also be applied to a typedef, in which case it will refer to the canonical type.)
///
#define FFI_EXPORT

///
/// The function or type declaration will have an explicitly specified name on the app side, instead of defaulting to the name it has on the engine side.
///
#define FFI_NAME(_name)

///
/// This pointer (argument or return value) points to an address in the app address space, which (in principle) is running in Wasm.
///
/// Examples:
/// - A string literal in the app will be passed as a `const char * const` pointer to the engine. Since the string itself points to an array allocated in Rust, it should be marked as a Wasm pointer.
/// - A pointer to a local variable in the app. Since the Rust stack is in Wasm memory, it should be marked as a Wasm pointer.
///
#define FFI_PTR_WASM

///
/// this pointer argument is an offset into the wasm's heap space (interpreter)
///
#define FFI_WASM_CALLBACK

///
/// This pointer (argument or return value) points to an address in the native address space.
///
/// Examples:
/// - A file handle will initially be passed from the engine to the app and subsequently back and forth, as an opaque pointer. Since it was originally allocated by the C standard library, it should be marked as a native pointer.
///
#define FFI_PTR_NATIVE

#define FFI_TYPE_OVERRIDE(_type)

///
/// This char * pointer (argument only) will not be exposed to the app. Instead, it will be populated by an auto-generated string value that can be used to keep track of (and debug) allocations and memory leaks.
///
#define FFI_MALLOC_TAG

///
/// This pointer (argument or return value) is allowed to be NULL. If this annotation is in the return position, the type will be exposed to the app wrapped in an Option.
///
#define FFI_CAN_BE_NULL

///
/// This enum contains a field that should have an explicitly specified name on the app side, instead of defaulting to the name it has on the engine side.
///
#define FFI_FIELD_NAME(_old, _name)

///
/// This enum's values are collections of individual bits and the choices can be combined and inspected using bitwise operations.
///
#define FFI_ENUM_BITFLAGS

///
/// This enum's choices will be exposed to the app such that the longest common substring in the beginning of their names will be removed.
///
#define FFI_ENUM_CLEAN_NAMES

///
/// This enum's choices will be exposed to the app such that the specified prefix will be removed from their names.
///
#define FFI_ENUM_TRIM_START_NAMES(_prefix)

///
/// This enum's choices will be exposed to the app such that the names will be fully capitalized.
///
#define FFI_ENUM_CAPITALIZE_NAMES

///
/// This enum has a reasonable default value that will be marked as such on the app side.
///
#define FFI_ENUM_DEFAULT(_default_choice)

///
/// This struct has a reasonable default value that can be derived by the Rust compiler.
///
#define FFI_DERIVE_DEFAULT

///
/// This function or type will be placed in a module.
///
/// NOTE: This is currently unsupported and the annotation will be ignored.
///
#define FFI_TYPE_MODULE(_mod)

///
/// This opaque handle has an associated function that takes one argument and acts as a `Drop` implementation.
///
/// This opaque handle will necessarily always be exposed as its own type, even if the manifest declares a preference for untyped handles.
///
#define FFI_DROP(_destructor)

///
/// This Wasm pointer argument is a reference to a single value, not a slice.
///
#define FFI_SINGLE

///
/// This Wasm pointer argument is not a reference to a single value, but rather a slice (like the beginning of an array or a part of an array).
///
#define FFI_SLICE

///
/// This int32_t argument will not be exposed to the app. Instead, it will contain the size (in bytes) of the element of the specified slice argument.
///
#define FFI_ELEMENT_SIZE_OF(_arg)

///
/// The specified Wasm pointer argument will not be exposed to the app as an out argument, but rather as a return value. The original return value of this function will be ignored by the app.
///
#define FFI_RETURN(_arg)

///
/// This opaque handle will always be exposed as its own type, even if the manifest declares a preference for untyped handles.
///
#define FFI_ALWAYS_TYPED

///
/// This function will be exposed to Rust via Wasm and native bindings, but no wrapper thunk will be emitted. To expose the function to the app, a thunk must be written manually.
///
#define FFI_NO_RUST_THUNK

///
/// This field will not be exposed to the app.
///
#define FFI_PRIVATE(_field_name)

///
/// This function will be exposed to Rust only if the specified macro is defined. Otherwise, an empty stub with the same signature will be exposed instead.
///
#define FFI_IMPLEMENT_IF_DEF(_macro)

///
/// This function will be marked as unsafe on rust
///
#define FFI_UNSAFE

///
/// This tagged entity will only be visible inside the ffi crate.
///
#define FFI_PUB_CRATE

/// This struct will be exposed to Rust as an enum with fields.
///
#define FFI_DISCRIMINATED_UNION

///
/// This function will be made available to extensions.
///
#define EXT_EXPORT

///
/// This function will be linked only if the specified macro is defined.
///
#define EXT_LINK_IF_DEF(_macro)
#endif
