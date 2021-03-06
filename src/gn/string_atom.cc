// Copyright (c) 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/string_atom.h"

#include <array>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

#include "base/containers/flat_set.h"

namespace {

// Implementation note:
//
// StringAtomSet implements the global shared state, which is:
//
//    - a group of std::string instances with a persistent address, allocated
//      through a fast slab allocator.
//
//    - a set of string pointers, corresponding to the known strings in the
//      group.
//
//    - a mutex to ensure correct thread-safety.
//
//    - a find() method that takes an std::string_view argument, and uses it
//      to find a matching entry in the string tree. If none is available,
//      a new std::string is allocated and its address inserted into the tree
//      before being returned.
//
// Because the mutex is a large bottleneck, each thread implements
// its own local string pointer cache, and will only call StringAtomSet::find()
// in case of a lookup miss. This is critical for good performance.
//

static const std::string kEmptyString;

using KeyType = const std::string*;

// This is a trivial hash table of string pointers, using open addressing.
// It is faster in practice than using a standard container or even a
// base::flat_set<>.
//
// Usage is the following:
//
//     1) Compute string hash value.
//
//     2) Call Lookup() with the hash value and the string_view key,
//        this always returns a mutable Node* pointer, say |node|.
//
//     3) If |node->key| is not nullptr, this is the key to use.
//        Otherwise, allocate a new string with persistent address,
//        and call Insert(), passing the |node|, |hash| and new string
//        address as arguments.
//
struct KeySet {
  struct Node {
    size_t hash = 0;
    KeyType key = nullptr;
  };

  // Compute hash for |str|. Replace with faster hash function if available.
  static size_t Hash(std::string_view str) {
    return std::hash<std::string_view>()(str);
  }

  // Lookup for |str| with specific |hash| value.
  // Return a Node pointer. If the key was found, |node.key| is its value.
  // Otherwise, the caller should create a new key value, then call Insert()
  // below.
  //
  // NOTE: Even though this method is const, because it doesn't modify the
  //       state of the KeySet, it returns a *mutable* node pointer, to be
  //       passed to Insert() in case of a miss.
  //
  Node* Lookup(size_t hash, std::string_view str) const {
    size_t index = hash & (size_ - 1);
    const Node* nodes = &buckets_[0];
    const Node* nodes_limit = nodes + size_;
    const Node* node = nodes + index;
    for (;;) {
      if (!node->key || (node->hash == hash && *node->key == str))
        return const_cast<Node*>(node);
      if (++node == nodes_limit)
        node = nodes;
    }
  }

  // Insert a new key in this set. |node| must be a value returned by
  // a previous Lookup() call. |hash| is the hash value for |key|.
  void Insert(Node* node, size_t hash, KeyType key) {
    node->hash = hash;
    node->key = key;
    count_ += 1;
    if (count_ * 4 >= size_ * 3)  // 75% max load factor
      GrowBuckets();
  }

  void GrowBuckets() {
    size_t size = buckets_.size();
    size_t new_size = size * 2;
    size_t new_mask = new_size - 1;

    std::vector<Node> new_buckets;
    new_buckets.resize(new_size);
    for (const Node& node : buckets_) {
      size_t index = node.hash & new_mask;
      for (;;) {
        Node& node2 = new_buckets[index];
        if (!node2.key) {
          node2 = node;
          break;
        }
        index = (index + 1) & new_mask;
      }
    }
    buckets_ = std::move(new_buckets);
    size_ = new_size;
  }

  size_t size_ = 2;
  size_t count_ = 0;
  std::vector<Node> buckets_ = {Node{}, Node{}};
};

class StringAtomSet {
 public:
  StringAtomSet() {
    // Ensure kEmptyString is in our set while not being allocated
    // from a slab. The end result is that find("") should always
    // return this address.
    //
    // This allows the StringAtom() default initializer to use the same
    // address directly, avoiding a table lookup.
    //
    size_t hash = set_.Hash("");
    auto* node = set_.Lookup(hash, "");
    set_.Insert(node, hash, &kEmptyString);
  }

  // Find the unique constant string pointer for |key|.
  const std::string* find(std::string_view key) {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t hash = set_.Hash(key);
    auto* node = set_.Lookup(hash, key);
    if (node->key)
      return node->key;

    // Allocate new string, insert its address in the set.
    if (slab_index_ >= kStringsPerSlab) {
      slabs_.push_back(new Slab());
      slab_index_ = 0;
    }
    std::string* result = slabs_.back()->init(slab_index_++, key);
    set_.Insert(node, hash, result);
    return result;
  }

 private:
  static constexpr unsigned int kStringsPerSlab = 128;

  // Each slab is allocated independently, has a fixed address and stores
  // kStringsPerSlab items of type StringStorage. The latter has the same
  // size and alignment as std::string, but doesn't need default-initialization.
  // This is used to slightly speed up Slab allocation and string
  // initialization. The drawback is that on process exit, allocated strings
  // are leaked (but GN already leaks several hundred MiBs of memory anyway).

  // A C++ union that can store an std::string but without default
  // initialization and destruction.
  union StringStorage {
    StringStorage() {}
    ~StringStorage() {}
    char dummy;
    std::string str;
  };

  // A fixed array of StringStorage items. Can be allocated cheaply.
  class Slab {
   public:
    // Init the n-th string in the slab with |str|.
    // Return its location as well.
    std::string* init(size_t index, const std::string_view& str) {
      std::string* result = &items_[index].str;
      new (result) std::string(str);
      return result;
    }

   private:
    StringStorage items_[kStringsPerSlab];
  };

  std::mutex mutex_;
  KeySet set_;
  std::vector<Slab*> slabs_;
  unsigned int slab_index_ = kStringsPerSlab;
};

StringAtomSet& GetStringAtomSet() {
  static StringAtomSet s_string_atom_set;
  return s_string_atom_set;
}

// Each thread maintains its own ThreadLocalCache to perform fast lookups
// without taking any mutex in most cases.
class ThreadLocalCache {
 public:
  // Find the unique constant string pointer for |key| in this cache,
  // and fallback to the global one in case of a miss.
  KeyType find(std::string_view key) {
    size_t hash = local_set_.Hash(key);
    auto* node = local_set_.Lookup(hash, key);
    if (node->key)
      return node->key;

    KeyType result = GetStringAtomSet().find(key);
    local_set_.Insert(node, hash, result);
    return result;
  }

 private:
  KeySet local_set_;
};

thread_local ThreadLocalCache s_local_cache;

}  // namespace

StringAtom::StringAtom() : value_(kEmptyString) {}

StringAtom::StringAtom(std::string_view str) noexcept
    : value_(*s_local_cache.find(str)) {}
