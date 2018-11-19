/*
 * Copyright (c) 2018 Isetta
 */
#pragma once
#include "Core/IsettaAlias.h"
#include "ISETTA_API.h"
#include "MemUtil.h"

namespace Isetta {
/*
 * Requirements:
 * 1. Alloc: Go through the sorted (by address) list and find the first one that
 * fits size need
 * 2. Free: Add the pointer back to the list, and try to merge adjacent ones.
 * It's easy to find the next Node. Merging with last node can be done during
 * insertion
 * 3. Satisfy alignment requirement
 */
// TODO(YIDI): Add expansion functionality
// TODO(YIDI): Optimize to use a tree as underlying structure to reduce time
// complexity
class ISETTA_API FreeListAllocator {
 public:
  // This class is using RAII
  FreeListAllocator() = default;
  explicit FreeListAllocator(Size size);
  ~FreeListAllocator() = default;

  void* Alloc(Size size, U8 alignment);
  void Free(void* memPtr);
  void* Realloc(void* memPtr, Size newSize, U8 alignment);

  template <typename T, typename... args>
  T* New(args... argList);

  template <typename T>
  T* NewArr(Size length, U8 alignment);

 private:
  struct Node {
    Size size;
    Node* next;
    explicit Node(const Size size) : size(size), next(nullptr) {}
  };

  struct AllocHeader {
    Size size;
    U64 adjustment;  // using U64 for alignment. U8 is big enough
    AllocHeader(const Size size, const U64 adjustment)
        : size(size), adjustment(adjustment) {}
  };

  void Expand();
  void Erase() const;
  void RemoveNode(Node* last, Node* nodeToRemove);
  void InsertNode(Node* newNode);
  static void InsertNodeAt(Node* pos, Node* newNode);
  static void TryMergeWithNext(Node* node);

  Node* head = nullptr;
  void* memHead = nullptr;
  std::list<void*> additionalMemory;

  // A lesson learned here: if these two variables are not static, it will
  // implicitly involve in the copy constructor's copying process. But as they
  // are const, they can't be copied over. This caused copy constructor to be
  // implicitly declared as deleted
  static const Size nodeSize = sizeof(Node);
  static const Size headerSize = sizeof(AllocHeader);

#if _DEBUG
  Size totalSize{0};
  Size sizeUsed{0};
  void Print() const;
#endif

  friend class MemoryManager;
};

template <typename T, typename... args>
T* FreeListAllocator::New(args... argList) {
  return new (Alloc(sizeof(T), MemUtil::ALIGNMENT)) T(argList...);
}

template <typename T>
T* FreeListAllocator::NewArr(Size length, const U8 alignment) {
  void* alloc = Alloc(sizeof(T) * length, alignment);
  char* allocAddress = static_cast<char*>(alloc);
  for (int i = 0; i < length; ++i) new (allocAddress + i * sizeof(T)) T;
  return static_cast<T*>(alloc);
}

}  // namespace Isetta
