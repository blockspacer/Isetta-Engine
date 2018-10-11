/*
 * Copyright (c) 2018 Isetta
 */
#pragma once

#include <string>
#include "Core/Debug/Assert.h"
#include "Core/Memory/MemoryManager.h"
#include "yojimbo/yojimbo.h"

namespace Isetta {
/**
 * @brief Code-generated struct to be used for sending integer values across the
 * network.
 *
 */
struct HandleMessage : public yojimbo::Message {
  int handle;

  HandleMessage() { handle = 0; }

  // TODO(Caleb): choose a more reasonable range for the int serialization
  template <typename Stream>
  bool Serialize(Stream& stream) {
    serialize_int(stream, handle, 0, 64);

    return true;
  }

  YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};

/**
 * @brief Code-generated struct to be used for sending string messages across
 * the network.
 *
 */
struct StringMessage : public yojimbo::Message {
  std::string string;

  StringMessage() { string = ""; }

  // TODO(Caleb): choose a more reasonable range for the int serialization
  template <typename Stream>
  bool Serialize(Stream& stream) {
    serialize_string(stream, const_cast<char*>(string.c_str()), 512);

    return true;
  }

  YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};

/**
 * @brief Enumeration of the message types that can be created by the
 * IsettaMessageFactory class.
 *
 */
enum IsettaMessageType { HANDLE_MESSAGE, STRING_MESSAGE, NUM_MESSAGE_TYPES };

/// Code-generate the IsettaMessageFactory class, which creates Message objects
/// upon request that can be sent over the network.
YOJIMBO_MESSAGE_FACTORY_START(IsettaMessageFactory, NUM_MESSAGE_TYPES);
YOJIMBO_DECLARE_MESSAGE_TYPE(HANDLE_MESSAGE, HandleMessage);
YOJIMBO_DECLARE_MESSAGE_TYPE(STRING_MESSAGE, StringMessage);
YOJIMBO_MESSAGE_FACTORY_FINISH();

class IsettaAllocator : public yojimbo::Allocator {
 public:
  // Network allocation is currently assumed to be LSR
  IsettaAllocator(void* memory, Size size) {
    ASSERT(size > 0);

    // SetErrorLevel(yojimbo::ALLOCATOR_ERROR_NONE);

    memPointer = memory;
    nextAvailable = reinterpret_cast<Size>(memPointer);
    endAddress = size + nextAvailable;
  }

  // TODO(Caleb): Clean up this hacky copy constructor
  IsettaAllocator(const IsettaAllocator& a) {
    memPointer = a.memPointer;
    nextAvailable = a.nextAvailable;
  }

  void* Allocate(Size size, const char* file, int line) {
    void* p = reinterpret_cast<void*>(nextAvailable);

    if (nextAvailable + size > endAddress) {
      // SetErrorLevel(yojimbo::ALLOCATOR_ERROR_OUT_OF_MEMORY);
      throw std::exception("Bad memory!");  // TODO(Caleb) better exception
    }

    //TrackAlloc(p, size, file, line);  // This causes a 64 byte memory leak

    nextAvailable += size;

    return p;
  }

  void Free(void* p, const char* file, int line) {
    if (!p) {
      return;
    }

    //TrackFree(p, file, line);  // This causes a 64 byte memory leak

    // Do nothing I guess? This is only supposed to be an LSR allocator
  }

 private:
  void* memPointer;
  Size nextAvailable;
  Size endAddress;
};

/**
 * @brief Container class for several utilities used by the networking module.
 *
 */
class CustomAdapter : public yojimbo::Adapter {
 public:
  // TODO(Caleb): Change the CreateAllocator function to use our mem alloc
  // instead of TLSF
  // Actually, TLSF might be good if it all remains in LSR

  // TODO(Caleb): something about the Linter with a const ref
  /**
   * @brief Creates a MessageFactory object that can be used to generate Message
   * objects specified by the IsettaMessageType enum.
   *
   * @param allocator Memory allocator used to allocate memory for the created
   * Message objects.
   * @return yojimbo::MessageFactory*
   */
  yojimbo::MessageFactory* CreateMessageFactory(yojimbo::Allocator* allocator) {
    return YOJIMBO_NEW(*allocator, IsettaMessageFactory, *allocator);
  }
};
}  // namespace Isetta