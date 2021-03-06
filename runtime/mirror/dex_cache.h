/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ART_RUNTIME_MIRROR_DEX_CACHE_H_
#define ART_RUNTIME_MIRROR_DEX_CACHE_H_

#include "array.h"
#include "base/bit_utils.h"
#include "dex_file_types.h"
#include "object.h"
#include "object_array.h"

namespace art {

class ArtField;
class ArtMethod;
struct DexCacheOffsets;
class DexFile;
class ImageWriter;
union JValue;
class LinearAlloc;
class Thread;

namespace mirror {

class CallSite;
class Class;
class MethodType;
class String;

template <typename T> struct PACKED(8) DexCachePair {
  GcRoot<T> object;
  uint32_t index;
  // The array is initially [ {0,0}, {0,0}, {0,0} ... ]
  // We maintain the invariant that once a dex cache entry is populated,
  // the pointer is always non-0
  // Any given entry would thus be:
  // {non-0, non-0} OR {0,0}
  //
  // It's generally sufficiently enough then to check if the
  // lookup index matches the stored index (for a >0 lookup index)
  // because if it's true the pointer is also non-null.
  //
  // For the 0th entry which is a special case, the value is either
  // {0,0} (initial state) or {non-0, 0} which indicates
  // that a valid object is stored at that index for a dex section id of 0.
  //
  // As an optimization, we want to avoid branching on the object pointer since
  // it's always non-null if the id branch succeeds (except for the 0th id).
  // Set the initial state for the 0th entry to be {0,1} which is guaranteed to fail
  // the lookup id == stored id branch.
  DexCachePair(ObjPtr<T> object, uint32_t index)
      : object(object),
        index(index) {}
  DexCachePair() = default;
  DexCachePair(const DexCachePair<T>&) = default;
  DexCachePair& operator=(const DexCachePair<T>&) = default;

  static void Initialize(std::atomic<DexCachePair<T>>* dex_cache) {
    DexCachePair<T> first_elem;
    first_elem.object = GcRoot<T>(nullptr);
    first_elem.index = InvalidIndexForSlot(0);
    dex_cache[0].store(first_elem, std::memory_order_relaxed);
  }

  static uint32_t InvalidIndexForSlot(uint32_t slot) {
    // Since the cache size is a power of two, 0 will always map to slot 0.
    // Use 1 for slot 0 and 0 for all other slots.
    return (slot == 0) ? 1u : 0u;
  }

  T* GetObjectForIndex(uint32_t idx) REQUIRES_SHARED(Locks::mutator_lock_) {
    if (idx != index) {
      return nullptr;
    }
    DCHECK(!object.IsNull());
    return object.Read();
  }
};

using TypeDexCachePair = DexCachePair<Class>;
using TypeDexCacheType = std::atomic<TypeDexCachePair>;

using StringDexCachePair = DexCachePair<String>;
using StringDexCacheType = std::atomic<StringDexCachePair>;

using MethodTypeDexCachePair = DexCachePair<MethodType>;
using MethodTypeDexCacheType = std::atomic<MethodTypeDexCachePair>;

// C++ mirror of java.lang.DexCache.
class MANAGED DexCache FINAL : public Object {
 public:
  // Size of java.lang.DexCache.class.
  static uint32_t ClassSize(PointerSize pointer_size);

  // Size of type dex cache. Needs to be a power of 2 for entrypoint assumptions to hold.
  static constexpr size_t kDexCacheTypeCacheSize = 1024;
  static_assert(IsPowerOfTwo(kDexCacheTypeCacheSize),
                "Type dex cache size is not a power of 2.");

  // Size of string dex cache. Needs to be a power of 2 for entrypoint assumptions to hold.
  static constexpr size_t kDexCacheStringCacheSize = 1024;
  static_assert(IsPowerOfTwo(kDexCacheStringCacheSize),
                "String dex cache size is not a power of 2.");

  // Size of method type dex cache. Needs to be a power of 2 for entrypoint assumptions
  // to hold.
  static constexpr size_t kDexCacheMethodTypeCacheSize = 1024;
  static_assert(IsPowerOfTwo(kDexCacheMethodTypeCacheSize),
                "MethodType dex cache size is not a power of 2.");

  static constexpr size_t StaticTypeSize() {
    return kDexCacheTypeCacheSize;
  }

  static constexpr size_t StaticStringSize() {
    return kDexCacheStringCacheSize;
  }

  static constexpr size_t StaticMethodTypeSize() {
    return kDexCacheMethodTypeCacheSize;
  }

  // Size of an instance of java.lang.DexCache not including referenced values.
  static constexpr uint32_t InstanceSize() {
    return sizeof(DexCache);
  }

  static void InitializeDexCache(Thread* self,
                                 ObjPtr<mirror::DexCache> dex_cache,
                                 ObjPtr<mirror::String> location,
                                 const DexFile* dex_file,
                                 LinearAlloc* linear_alloc,
                                 PointerSize image_pointer_size)
      REQUIRES_SHARED(Locks::mutator_lock_)
      REQUIRES(Locks::dex_lock_);

  void Fixup(ArtMethod* trampoline, PointerSize pointer_size)
      REQUIRES_SHARED(Locks::mutator_lock_);

  template <ReadBarrierOption kReadBarrierOption = kWithReadBarrier, typename Visitor>
  void FixupStrings(StringDexCacheType* dest, const Visitor& visitor)
      REQUIRES_SHARED(Locks::mutator_lock_);

  template <ReadBarrierOption kReadBarrierOption = kWithReadBarrier, typename Visitor>
  void FixupResolvedTypes(TypeDexCacheType* dest, const Visitor& visitor)
      REQUIRES_SHARED(Locks::mutator_lock_);

  template <ReadBarrierOption kReadBarrierOption = kWithReadBarrier, typename Visitor>
  void FixupResolvedMethodTypes(MethodTypeDexCacheType* dest, const Visitor& visitor)
      REQUIRES_SHARED(Locks::mutator_lock_);

  template <ReadBarrierOption kReadBarrierOption = kWithReadBarrier, typename Visitor>
  void FixupResolvedCallSites(GcRoot<mirror::CallSite>* dest, const Visitor& visitor)
      REQUIRES_SHARED(Locks::mutator_lock_);

  String* GetLocation() REQUIRES_SHARED(Locks::mutator_lock_) {
    return GetFieldObject<String>(OFFSET_OF_OBJECT_MEMBER(DexCache, location_));
  }

  static MemberOffset DexOffset() {
    return OFFSET_OF_OBJECT_MEMBER(DexCache, dex_);
  }

  static MemberOffset StringsOffset() {
    return OFFSET_OF_OBJECT_MEMBER(DexCache, strings_);
  }

  static MemberOffset ResolvedTypesOffset() {
    return OFFSET_OF_OBJECT_MEMBER(DexCache, resolved_types_);
  }

  static MemberOffset ResolvedFieldsOffset() {
    return OFFSET_OF_OBJECT_MEMBER(DexCache, resolved_fields_);
  }

  static MemberOffset ResolvedMethodsOffset() {
    return OFFSET_OF_OBJECT_MEMBER(DexCache, resolved_methods_);
  }

  static MemberOffset ResolvedMethodTypesOffset() {
    return OFFSET_OF_OBJECT_MEMBER(DexCache, resolved_method_types_);
  }

  static MemberOffset ResolvedCallSitesOffset() {
    return OFFSET_OF_OBJECT_MEMBER(DexCache, resolved_call_sites_);
  }

  static MemberOffset NumStringsOffset() {
    return OFFSET_OF_OBJECT_MEMBER(DexCache, num_strings_);
  }

  static MemberOffset NumResolvedTypesOffset() {
    return OFFSET_OF_OBJECT_MEMBER(DexCache, num_resolved_types_);
  }

  static MemberOffset NumResolvedFieldsOffset() {
    return OFFSET_OF_OBJECT_MEMBER(DexCache, num_resolved_fields_);
  }

  static MemberOffset NumResolvedMethodsOffset() {
    return OFFSET_OF_OBJECT_MEMBER(DexCache, num_resolved_methods_);
  }

  static MemberOffset NumResolvedMethodTypesOffset() {
    return OFFSET_OF_OBJECT_MEMBER(DexCache, num_resolved_method_types_);
  }

  static MemberOffset NumResolvedCallSitesOffset() {
    return OFFSET_OF_OBJECT_MEMBER(DexCache, num_resolved_call_sites_);
  }

  String* GetResolvedString(dex::StringIndex string_idx) ALWAYS_INLINE
      REQUIRES_SHARED(Locks::mutator_lock_);

  void SetResolvedString(dex::StringIndex string_idx, ObjPtr<mirror::String> resolved) ALWAYS_INLINE
      REQUIRES_SHARED(Locks::mutator_lock_);

  // Clear a string for a string_idx, used to undo string intern transactions to make sure
  // the string isn't kept live.
  void ClearString(dex::StringIndex string_idx) REQUIRES_SHARED(Locks::mutator_lock_);

  Class* GetResolvedType(dex::TypeIndex type_idx) REQUIRES_SHARED(Locks::mutator_lock_);

  void SetResolvedType(dex::TypeIndex type_idx, ObjPtr<Class> resolved)
      REQUIRES_SHARED(Locks::mutator_lock_);

  void ClearResolvedType(dex::TypeIndex type_idx) REQUIRES_SHARED(Locks::mutator_lock_);

  ALWAYS_INLINE ArtMethod* GetResolvedMethod(uint32_t method_idx, PointerSize ptr_size)
      REQUIRES_SHARED(Locks::mutator_lock_);

  ALWAYS_INLINE void SetResolvedMethod(uint32_t method_idx,
                                       ArtMethod* resolved,
                                       PointerSize ptr_size)
      REQUIRES_SHARED(Locks::mutator_lock_);

  // Pointer sized variant, used for patching.
  ALWAYS_INLINE ArtField* GetResolvedField(uint32_t idx, PointerSize ptr_size)
      REQUIRES_SHARED(Locks::mutator_lock_);

  // Pointer sized variant, used for patching.
  ALWAYS_INLINE void SetResolvedField(uint32_t idx, ArtField* field, PointerSize ptr_size)
      REQUIRES_SHARED(Locks::mutator_lock_);

  MethodType* GetResolvedMethodType(uint32_t proto_idx) REQUIRES_SHARED(Locks::mutator_lock_);

  void SetResolvedMethodType(uint32_t proto_idx, MethodType* resolved)
      REQUIRES_SHARED(Locks::mutator_lock_);

  CallSite* GetResolvedCallSite(uint32_t call_site_idx) REQUIRES_SHARED(Locks::mutator_lock_);

  // Attempts to bind |call_site_idx| to the call site |resolved|. The
  // caller must use the return value in place of |resolved|. This is
  // because multiple threads can invoke the bootstrap method each
  // producing a call site, but the method handle invocation on the
  // call site must be on a common agreed value.
  CallSite* SetResolvedCallSite(uint32_t call_site_idx, CallSite* resolved) WARN_UNUSED
      REQUIRES_SHARED(Locks::mutator_lock_);

  StringDexCacheType* GetStrings() ALWAYS_INLINE REQUIRES_SHARED(Locks::mutator_lock_) {
    return GetFieldPtr64<StringDexCacheType*>(StringsOffset());
  }

  void SetStrings(StringDexCacheType* strings) ALWAYS_INLINE REQUIRES_SHARED(Locks::mutator_lock_) {
    SetFieldPtr<false>(StringsOffset(), strings);
  }

  TypeDexCacheType* GetResolvedTypes() ALWAYS_INLINE REQUIRES_SHARED(Locks::mutator_lock_) {
    return GetFieldPtr<TypeDexCacheType*>(ResolvedTypesOffset());
  }

  void SetResolvedTypes(TypeDexCacheType* resolved_types)
      ALWAYS_INLINE
      REQUIRES_SHARED(Locks::mutator_lock_) {
    SetFieldPtr<false>(ResolvedTypesOffset(), resolved_types);
  }

  ArtMethod** GetResolvedMethods() ALWAYS_INLINE REQUIRES_SHARED(Locks::mutator_lock_) {
    return GetFieldPtr<ArtMethod**>(ResolvedMethodsOffset());
  }

  void SetResolvedMethods(ArtMethod** resolved_methods)
      ALWAYS_INLINE
      REQUIRES_SHARED(Locks::mutator_lock_) {
    SetFieldPtr<false>(ResolvedMethodsOffset(), resolved_methods);
  }

  ArtField** GetResolvedFields() ALWAYS_INLINE REQUIRES_SHARED(Locks::mutator_lock_) {
    return GetFieldPtr<ArtField**>(ResolvedFieldsOffset());
  }

  void SetResolvedFields(ArtField** resolved_fields)
      ALWAYS_INLINE
      REQUIRES_SHARED(Locks::mutator_lock_) {
    SetFieldPtr<false>(ResolvedFieldsOffset(), resolved_fields);
  }

  MethodTypeDexCacheType* GetResolvedMethodTypes()
      ALWAYS_INLINE REQUIRES_SHARED(Locks::mutator_lock_) {
    return GetFieldPtr64<MethodTypeDexCacheType*>(ResolvedMethodTypesOffset());
  }

  void SetResolvedMethodTypes(MethodTypeDexCacheType* resolved_method_types)
      ALWAYS_INLINE
      REQUIRES_SHARED(Locks::mutator_lock_) {
    SetFieldPtr<false>(ResolvedMethodTypesOffset(), resolved_method_types);
  }

  GcRoot<CallSite>* GetResolvedCallSites()
      ALWAYS_INLINE
      REQUIRES_SHARED(Locks::mutator_lock_) {
    return GetFieldPtr<GcRoot<CallSite>*>(ResolvedCallSitesOffset());
  }

  void SetResolvedCallSites(GcRoot<CallSite>* resolved_call_sites)
      ALWAYS_INLINE
      REQUIRES_SHARED(Locks::mutator_lock_) {
    SetFieldPtr<false>(ResolvedCallSitesOffset(), resolved_call_sites);
  }

  size_t NumStrings() REQUIRES_SHARED(Locks::mutator_lock_) {
    return GetField32(NumStringsOffset());
  }

  size_t NumResolvedTypes() REQUIRES_SHARED(Locks::mutator_lock_) {
    return GetField32(NumResolvedTypesOffset());
  }

  size_t NumResolvedMethods() REQUIRES_SHARED(Locks::mutator_lock_) {
    return GetField32(NumResolvedMethodsOffset());
  }

  size_t NumResolvedFields() REQUIRES_SHARED(Locks::mutator_lock_) {
    return GetField32(NumResolvedFieldsOffset());
  }

  size_t NumResolvedMethodTypes() REQUIRES_SHARED(Locks::mutator_lock_) {
    return GetField32(NumResolvedMethodTypesOffset());
  }

  size_t NumResolvedCallSites() REQUIRES_SHARED(Locks::mutator_lock_) {
    return GetField32(NumResolvedCallSitesOffset());
  }

  const DexFile* GetDexFile() ALWAYS_INLINE REQUIRES_SHARED(Locks::mutator_lock_) {
    return GetFieldPtr<const DexFile*>(OFFSET_OF_OBJECT_MEMBER(DexCache, dex_file_));
  }

  void SetDexFile(const DexFile* dex_file) REQUIRES_SHARED(Locks::mutator_lock_) {
    SetFieldPtr<false>(OFFSET_OF_OBJECT_MEMBER(DexCache, dex_file_), dex_file);
  }

  void SetLocation(ObjPtr<String> location) REQUIRES_SHARED(Locks::mutator_lock_);

  // NOTE: Get/SetElementPtrSize() are intended for working with ArtMethod** and ArtField**
  // provided by GetResolvedMethods/Fields() and ArtMethod::GetDexCacheResolvedMethods(),
  // so they need to be public.

  template <typename PtrType>
  static PtrType GetElementPtrSize(PtrType* ptr_array, size_t idx, PointerSize ptr_size);

  template <typename PtrType>
  static void SetElementPtrSize(PtrType* ptr_array, size_t idx, PtrType ptr, PointerSize ptr_size);

 private:
  void Init(const DexFile* dex_file,
            ObjPtr<String> location,
            StringDexCacheType* strings,
            uint32_t num_strings,
            TypeDexCacheType* resolved_types,
            uint32_t num_resolved_types,
            ArtMethod** resolved_methods,
            uint32_t num_resolved_methods,
            ArtField** resolved_fields,
            uint32_t num_resolved_fields,
            MethodTypeDexCacheType* resolved_method_types,
            uint32_t num_resolved_method_types,
            GcRoot<CallSite>* resolved_call_sites,
            uint32_t num_resolved_call_sites,
            PointerSize pointer_size)
      REQUIRES_SHARED(Locks::mutator_lock_);

  uint32_t StringSlotIndex(dex::StringIndex string_idx) REQUIRES_SHARED(Locks::mutator_lock_);
  uint32_t TypeSlotIndex(dex::TypeIndex type_idx) REQUIRES_SHARED(Locks::mutator_lock_);
  uint32_t MethodTypeSlotIndex(uint32_t proto_idx) REQUIRES_SHARED(Locks::mutator_lock_);

  // Visit instance fields of the dex cache as well as its associated arrays.
  template <bool kVisitNativeRoots,
            VerifyObjectFlags kVerifyFlags = kDefaultVerifyFlags,
            ReadBarrierOption kReadBarrierOption = kWithReadBarrier,
            typename Visitor>
  void VisitReferences(ObjPtr<Class> klass, const Visitor& visitor)
      REQUIRES_SHARED(Locks::mutator_lock_) REQUIRES(Locks::heap_bitmap_lock_);

  HeapReference<Object> dex_;
  HeapReference<String> location_;
  uint64_t dex_file_;               // const DexFile*
  uint64_t resolved_call_sites_;    // GcRoot<CallSite>* array with num_resolved_call_sites_
                                    // elements.
  uint64_t resolved_fields_;        // ArtField*, array with num_resolved_fields_ elements.
  uint64_t resolved_method_types_;  // std::atomic<MethodTypeDexCachePair>* array with
                                    // num_resolved_method_types_ elements.
  uint64_t resolved_methods_;       // ArtMethod*, array with num_resolved_methods_ elements.
  uint64_t resolved_types_;         // TypeDexCacheType*, array with num_resolved_types_ elements.
  uint64_t strings_;                // std::atomic<StringDexCachePair>*, array with num_strings_
                                    // elements.

  uint32_t num_resolved_call_sites_;    // Number of elements in the call_sites_ array.
  uint32_t num_resolved_fields_;        // Number of elements in the resolved_fields_ array.
  uint32_t num_resolved_method_types_;  // Number of elements in the resolved_method_types_ array.
  uint32_t num_resolved_methods_;       // Number of elements in the resolved_methods_ array.
  uint32_t num_resolved_types_;         // Number of elements in the resolved_types_ array.
  uint32_t num_strings_;                // Number of elements in the strings_ array.

  friend struct art::DexCacheOffsets;  // for verifying offset information
  friend class Object;  // For VisitReferences
  DISALLOW_IMPLICIT_CONSTRUCTORS(DexCache);
};

}  // namespace mirror
}  // namespace art

#endif  // ART_RUNTIME_MIRROR_DEX_CACHE_H_
