/*
* Copyright (c) 2018 Isetta
*/
#pragma once
#include <map>
#include "Core/IsettaAlias.h"

namespace Isetta {

template<typename T>
class ObjectHandle;

class MemoryArena {
 public:
  ~MemoryArena() = default;

 private:
  MemoryArena() = default;
  explicit MemoryArena(SizeInt size);
  PtrInt leftAddress{};
  PtrInt rightAddress{};
  void* memHead{};

  template <typename T>
  ObjectHandle<T>& NewDynamic();

  template <typename T>
  void DeleteDynamic(ObjectHandle<T>& objectToFree);

  void* Alloc(SizeInt size, SizeInt& outSize);
  void Defragment();
  void MoveLeft(U32 index);
  void Erase() const;
  void Print() const;
  PtrInt GetSize() const;

  // can't be put in ObjectHandle because it creates new ones for each type
  inline static U32 nextUniqueID = 0;
  const static U32 maxHandleCount = 2048;
  const static U8 alignment = 16;
  static class HandleEntry entryArr[];
  // TODO(YIDI): Make a custom container rather than using map
  // O(1) random access, O(log n) insert
  std::map<PtrInt, int> addressIndexMap;
  U32 index = 0;

  template <typename T>
  friend class ObjectHandle;
  friend class MemoryManager;
};

template <typename T>
ObjectHandle<T>& MemoryArena::NewDynamic() {
  // TODO(YIDI): I'm not sure this is the right way to implement object handles,
  // the handle itself should not need to be freed. But I think usage-wise this
  // is optimal
  SizeInt size;
  void* mem = Alloc(sizeof(T), size);
  auto handle = ObjectHandle<T>{mem, nextUniqueID++, size};
  addressIndexMap.emplace(handle.GetObjAddress(), handle.index);
  return handle;
}

template <typename T>
void MemoryArena::DeleteDynamic(ObjectHandle<T>& objectToFree) {
  auto addressIndexPair = addressIndexMap.find(objectToFree.GetObjAddress());

  if (addressIndexPair != addressIndexMap.end()) {
    addressIndexMap.erase(addressIndexPair);
    objectToFree.EraseObject();
  } else {
    throw std::exception{"MemoryArena::DeleteDynamic => Double deleting handle!"};
  }
}

}  // namespace Isetta