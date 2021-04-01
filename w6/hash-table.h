#pragma once

#include <cassert>
#include <functional>

template<class Key, class T, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>>
class hash_table {
public:
  using key_type = Key;
  using mapped_type = T;
  using value_type = std::pair<const Key, T>;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using hasher = Hash;
  using key_equal = KeyEqual;
  using reference = value_type &;
  using const_reference = const value_type &;
  using pointer = value_type *;
  using const_pointer = const value_type *;

private:
  struct node_type {
    pointer get_stored_value() noexcept {
      return reinterpret_cast<pointer>(&storage);
    }

    const_pointer get_stored_value() const noexcept {
      return reinterpret_cast<const_pointer>(&storage);
    }

    template<class I>
    void init_with(I &&initializer, size_type key_hash_value) {
      new(&storage) value_type{std::forward<I>(initializer)};
      has_value = true;
      key_hash = key_hash_value;
    }

    void reset() {
      get_stored_value()->~value_type();
      has_value = false;
    }

    std::aligned_storage_t<sizeof(value_type), alignof(value_type)> storage{};
    size_type key_hash{0};
    bool has_value{false};
  };

public:
  explicit hash_table(size_type initial_buckets = 17,
                      const hasher &hash = hasher{},
                      const key_equal &equal = key_equal{}) :
          key_hasher_{hash},
          key_equal_comparator_{equal},
          buckets_{std::max(size_type{17}, initial_buckets)},
          table_{new node_type[buckets_]} {
  }

  std::pair<pointer, bool> insert(const value_type &val) {
    if (5 * size_ > 3 * buckets_) {
      rehash(2 * buckets_ + 3);
    }

    const size_type key_hash = key_hasher_(val.first);
    node_type &node = table_[get_bucket_id(val.first, key_hash)];

    const bool has_value = node.has_value;
    if (!has_value) {
      node.init_with(val, key_hash);
      ++size_;
    }
    return {node.get_stored_value(), !has_value};
  }

  pointer find(const key_type &k) {
    node_type &node = table_[get_bucket_id(k, key_hasher_(k))];
    return node.has_value ? node.get_stored_value() : nullptr;
  }

  const_pointer find(const key_type &k) const {
    const node_type &node = table_[get_bucket_id(k, key_hasher_(k))];
    return node.has_value ? node.get_stored_value() : nullptr;
  }

  size_type count(const key_type &k) const {
    return find(k) ? 1 : 0;
  }

  size_type erase(const key_type &k) {
    size_type erased_bucket_id = get_bucket_id(k, key_hasher_(k));
    node_type *erased_node = &table_[erased_bucket_id];
    if (!erased_node->has_value) {
      return 0;
    }

    erased_node->reset();
    --size_;

    size_type moving_bucket_id = get_next_bucket_id(erased_bucket_id);
    for (;;) {
      node_type *moving_node = &table_[moving_bucket_id];
      if (!moving_node->has_value) {
        break;
      }

      const size_type required_bucket = moving_node->key_hash % buckets_;
      if (required_bucket != moving_bucket_id && required_bucket <= erased_bucket_id) {
        erased_node->init_with(std::move(*moving_node->get_stored_value()), moving_node->key_hash);
        moving_node->reset();
        erased_node = moving_node;
        erased_bucket_id = moving_bucket_id;
      }
      moving_bucket_id = get_next_bucket_id(moving_bucket_id);
    }

    return 1;
  }

  mapped_type &operator[](const key_type &k) {
    return insert({k, mapped_type{}}).first->second;
  }

  [[nodiscard]] bool empty() const noexcept {
    return !size_;
  }

  size_type size() const noexcept {
    return size_;
  }

  void rehash(size_type new_buckets_count) {
    if (new_buckets_count <= buckets_) {
      return;
    }

    const size_type old_buckets_count = buckets_;
    node_type *old_table = table_;

    new_buckets_count |= 1;
    table_ = new node_type[new_buckets_count];
    buckets_ = new_buckets_count;

    try {
      for (size_type i = 0; i != old_buckets_count; ++i) {
        node_type &old_node = old_table[i];
        if (old_node.has_value) {
          pointer old_stored_value = old_node.get_stored_value();
          node_type &new_node = table_[get_bucket_id(old_stored_value->first, old_node.key_hash)];
          assert(!new_node.has_value);

          new_node.init_with(std::move(*old_stored_value), old_node.key_hash);
          old_stored_value->~value_type();
        }
      }
    } catch (...) {
      assert(!"got exception on rehashing");
    }
    delete[] old_table;
  }

  void clear() noexcept {
    for (size_type i = 0; i != buckets_; ++i) {
      node_type &node = table_[i];
      if (node.has_value) {
        node.reset();
      }
    }
    size_ = 0;
  }

  ~hash_table() noexcept {
    clear();
    delete[] table_;
  }

private:
  size_type get_bucket_id(const key_type &key, size_type key_hash) const {
    size_type bucket_id = key_hash % buckets_;
    for (;;) {
      auto *node = &table_[bucket_id];
      if (!node->has_value) {
        return bucket_id;
      }
      if (key_hash == node->key_hash && key_equal_comparator_(key, node->get_stored_value()->first)) {
        return bucket_id;
      }
      bucket_id = get_next_bucket_id(bucket_id);
    }
  }

  size_type get_next_bucket_id(size_type bucket_id) const noexcept {
    ++bucket_id;
    return bucket_id == buckets_ ? 0 : bucket_id;
  }

  hasher key_hasher_;
  key_equal key_equal_comparator_;
  size_type size_{0};
  size_type buckets_{0};
  node_type *table_{nullptr};
};