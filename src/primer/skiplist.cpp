//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// skiplist.cpp
//
// Identification: src/primer/skiplist.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "primer/skiplist.h"
#include <cassert>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <vector>
#include <algorithm>
#include "common/macros.h"
#include "fmt/core.h"

namespace bustub {

/** @brief Checks whether the container is empty. */
SKIPLIST_TEMPLATE_ARGUMENTS auto SkipList<K, Compare, MaxHeight, Seed>::Empty() -> bool {
  return this->size_ == 0;
}

/** @brief Returns the number of elements in the skip list. */
SKIPLIST_TEMPLATE_ARGUMENTS auto SkipList<K, Compare, MaxHeight, Seed>::Size() -> size_t {
  return this->size_;
}

/**
 * @brief Iteratively deallocate all the nodes.
 *
 * We do this to avoid stack overflow when the skip list is large.
 *
 * If we let the compiler handle the deallocation, it will recursively call the destructor of each node,
 * which could block up the the stack.
 */
SKIPLIST_TEMPLATE_ARGUMENTS void SkipList<K, Compare, MaxHeight, Seed>::Drop() {
  for (size_t i = 0; i < MaxHeight; i++) {
    auto curr = std::move(header_->links_[i]);
    while (curr != nullptr) {
      // std::move sets `curr` to the old value of `curr->links_[i]`,
      // and then resets `curr->links_[i]` to `nullptr`.
      curr = std::move(curr->links_[i]);
    }
  }
}

/**
 * @brief Removes all elements from the skip list.
 *
 * Note: You might want to use the provided `Drop` helper function.
 */
SKIPLIST_TEMPLATE_ARGUMENTS void SkipList<K, Compare, MaxHeight, Seed>::Clear() {
  UNIMPLEMENTED("TODO(P0): Add implementation.");
}

/**
 * @brief Finds the next node with key that breaks the comparison
 * @param key key to find.
 * @return the nearest pointer that breaks the comparison function, nullptr if the key already exists or the size is 0. 
 */
SKIPLIST_TEMPLATE_ARGUMENTS auto SkipList<K, Compare, MaxHeight, Seed>::Find(const K &key, std::shared_ptr<SkipNode> start_node, size_t height) -> std::shared_ptr<SkipNode> {
  std::shared_ptr<SkipNode> curr_node = start_node;


  for(int i = height-1; i >= 0; i--) {
    while(curr_node != nullptr) {

      if(curr_node->links_[i] != nullptr) {
        std::shared_ptr<SkipNode> next_node = curr_node->links_[i];

        if(this->compare_(key, next_node->key_)) {
          return curr_node;  
        }
       
        curr_node = next_node;
      }
      else {
        break;
      }
    }
    break;
  }

  return curr_node;
}

SKIPLIST_TEMPLATE_ARGUMENTS auto SkipList<K, Compare, MaxHeight, Seed>::CheckNearestMatching(const K &key, std::shared_ptr<SkipNode> start_node, size_t height) -> bool {
  std::shared_ptr<SkipNode> curr_node = start_node;


  for(int i = height-1; i >= 0; i--) {
    while(curr_node != nullptr) {

      if(curr_node->links_[i] != nullptr) {
        std::shared_ptr<SkipNode> next_node = curr_node->links_[i];

        if((!this->compare_(key, next_node->key_) && !this->compare_(next_node->key_,key))) {
          return true;  
        }
        
        if (!this->compare_(key, next_node->key_)) {
            curr_node = next_node;
        }
        else {
          break;
        }
      }
      else {
        break;
      }
    }
  }

  return false;
}

/**
 * @brief Inserts a key into the skip list.
 *
 * Note: `Insert` will not insert the key if it already exists in the skip list.
 *
 * @param key key to insert.
 * @return true if the insertion is successful, false if the key already exists.
 */
SKIPLIST_TEMPLATE_ARGUMENTS auto SkipList<K, Compare, MaxHeight, Seed>::Insert(const K &key) -> bool {
  bool was_successful = false;


  std::unique_lock<std::shared_mutex> lock(this->rwlock_);

  std::shared_ptr<SkipNode> new_node = std::make_shared<SkipNode>(this->RandomHeight(), key);

  if (this->size_ == 0) {
    for(int i = new_node->Height()-1; i > -1; i--) {
      this->header_->links_[i] = new_node;
    }
    this->size_ += 1;
    return true;
  }


  std::shared_ptr<SkipNode> found_node = this->Find(key, this->header_, new_node->Height());

  bool is_inserted = this->CheckNearestMatching(key, found_node, found_node->Height());

  if ((!this->compare_(key, found_node->key_) && !this->compare_(found_node->key_, key) && found_node != this->header_) || is_inserted) {
    return false;
  }

  std::shared_ptr<SkipNode> next_node = found_node->links_[new_node->Height()-1];

  for (int i = new_node->Height()-1; i > -1; i--) {
    if(next_node != found_node->links_[i]) {
      found_node = this->Find(key, found_node, i+1);
    }

    next_node = found_node->links_[i];
  
    new_node->links_[i] = found_node->links_[i];
    found_node->links_[i] = new_node;
  }

  was_successful = true;
  this->size_ += 1;


  return was_successful;
}

/**
 * @brief Erases the key from the skip list.
 *
 * @param key key to erase.
 * @return bool true if the element got erased, false otherwise.
 */
SKIPLIST_TEMPLATE_ARGUMENTS auto SkipList<K, Compare, MaxHeight, Seed>::Erase(const K &key) -> bool {
  UNIMPLEMENTED("TODO(P0): Add implementation.");
}

/**
 * @brief Checks whether a key exists in the skip list.
 *
 * 
 * @param key key to look up.
 * @return bool true if the element exists, false otherwise.
 */
SKIPLIST_TEMPLATE_ARGUMENTS auto SkipList<K, Compare, MaxHeight, Seed>::Contains(const K &key) -> bool {
  // Following the standard library: Key `a` and `b` are considered equivalent if neither compares less
  // than the other: `!compare_(a, b) && !compare_(b, a)`.

  std::shared_ptr<SkipNode> found_node = this->Find(key, this->header_, MaxHeight);

  bool is_equal = false;
  
  std::shared_ptr<SkipNode> next_node = found_node->links_[found_node->Height()-1];

  for (int i = found_node->Height()-1; i > -1; i--) {
    if (next_node != found_node->links_[i]) {
      found_node = this->Find(key, found_node, i+1);
    }
      is_equal = is_equal ? is_equal : (!this->compare_(key, found_node->key_) && !this->compare_(found_node->key_, key));
  }

  return is_equal;
}

/**
 * @brief Prints the skip list for debugging purposes.
 *
 * Note: You may modify the functions in any way and the output is not tested.
 */
SKIPLIST_TEMPLATE_ARGUMENTS void SkipList<K, Compare, MaxHeight, Seed>::Print() {
  auto node = header_->Next(LOWEST_LEVEL);
  while (node != nullptr) {
    fmt::println("Node {{ key: {}, height: {} }}", node->Key(), node->Height());
    node = node->Next(LOWEST_LEVEL);
  }
}

/**
 * @brief Generate a random height. The height should be cappped at `MaxHeight`.
 * Note: we implement/simulate the geometric process to ensure platform independence.
 */
SKIPLIST_TEMPLATE_ARGUMENTS auto SkipList<K, Compare, MaxHeight, Seed>::RandomHeight() -> size_t {
  // Branching factor (1 in 4 chance), see Pugh's paper.
  static constexpr unsigned int branching_factor = 4;
  // Start with the minimum height
  size_t height = 1;
  while (height < MaxHeight && (rng_() % branching_factor == 0)) {
    height++;
  }
  return height;
}

/**
 * @brief Gets the current node height.
 */
SKIPLIST_TEMPLATE_ARGUMENTS auto SkipList<K, Compare, MaxHeight, Seed>::SkipNode::Height() const -> size_t {
  return this->links_.size();
}

/**
 * @brief Gets the next node by following the link at `level`.
 *
 * @param level index to the link.
 * @return std::shared_ptr<SkipNode> the next node, or `nullptr` if such node does not exist.
 */
SKIPLIST_TEMPLATE_ARGUMENTS auto SkipList<K, Compare, MaxHeight, Seed>::SkipNode::Next(size_t level) const
    -> std::shared_ptr<SkipNode> {
  if(level > this->links_.size()) {
    throw std::invalid_argument("The level provided exceeds the height of the current node");
  }

  return this->links_[level];
}

/**
 * @brief Set the `node` to be linked at `level`.
 *
 * @param level index to the link.
 */
SKIPLIST_TEMPLATE_ARGUMENTS void SkipList<K, Compare, MaxHeight, Seed>::SkipNode::SetNext(
    size_t level, const std::shared_ptr<SkipNode> &node) {

  if(level > MaxHeight) {
    throw std::invalid_argument("The level exceeds the max height set for the skip list");
  }
  
  if(level > this->links_.size()) {
    this->links_.resize(level);
  }

  this->links_[level-1] = node;
}

/** @brief Returns a reference to the key stored in the node. */
SKIPLIST_TEMPLATE_ARGUMENTS auto SkipList<K, Compare, MaxHeight, Seed>::SkipNode::Key() const -> const K & {
  return this->key_;
}

// Below are explicit instantiation of template classes.
template class SkipList<int>;
template class SkipList<std::string>;
template class SkipList<int, std::greater<>>;
template class SkipList<int, std::less<>, 8>;

}  // namespace bustub
