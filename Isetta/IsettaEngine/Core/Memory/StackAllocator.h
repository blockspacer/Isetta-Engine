/*
 * Copyright (c) 2018 Isetta
 */
#pragma once
#include "Core/IsettaAlias.h"
#include "Core/Memory/MemUtil.h"
#include "ISETTA_API.h"

namespace Isetta {
class ISETTA_API StackAllocator {
 public:
  typedef Size Marker;

  StackAllocator() = default;
  explicit StackAllocator(Size stackSize);
  ~StackAllocator() = default;

  /**
   * \brief Grab properly aligned memory from the stack allocator. You probably
   * want to `GetMarker()` before calling this if you want to free to that
   * marker
   *
   * \param size Number of bytes you want, in byte
   * \param alignment Alignment requirement of the memory, must be power of 2
   * and less than 128 \return pointer to the allocated memory
   * \return Pointer to allocated memory
   */
  void* Alloc(Size size, U8 alignment);

  /**
   * \brief Create a new object on the stack allocator. The constructor is
   * automatically called. The memory is 16 aligned by default. If you are using
   * this, you probably need to call the destructor on your own.
   * \tparam T type of object you want to create
   * \tparam args Arguments for the constructor
   * \return Pointer to new object
   */
  template<typename T, typename ...args>
  T* New(args...);

  template<typename T>
  T* NewArr(Size length, U8 alignment);

  /**
   * \brief Free the stack allocator to a specific marker
   * \param marker Marker to free to
   */
  void FreeToMarker(const Marker marker) { top = marker; };

  /**
   * \brief Clear the whole stack allocator to its bottom. All memory will be
   * available for new allocations again
   */
  void Clear() { top = 0; };

  /**
   * \brief Free all memory in this stack allocator. The stack allocator will
   * become unusable after calling this
   */
  void Erase();

  /**
   * \brief Get the current marker position
   * \return 
   */
  Marker GetMarker() const { return top; };

 private:
  Marker top;
  Size totalSize;
  void* bottom;
  PtrInt bottomAddress;
};

template <typename T, typename ... args>
T* StackAllocator::New(args... argList) {
  void* mem = Alloc(sizeof(T), MemUtil::ALIGNMENT);
  return new (mem) T(argList...);
}

template <typename T>
T* StackAllocator::NewArr(Size length, const U8 alignment) {
  void* mem = Alloc(sizeof(T) * length, alignment);
  return new (mem) T[length];
}
}  // namespace Isetta
