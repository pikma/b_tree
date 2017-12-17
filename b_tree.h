#include <memory>
#include <string>
#include <vector>

#include "absl/types/optional.h"

class Element {
  public:
    explicit Element(std::string key, std::string value);

    const std::string& key() const { return key_; }
    const std::string& value() const { return value_; }

    void set_value(std::string value) { value_ = std::move(value); }
  private:
    std::string key_;
    std::string value_;
};

class Node {
 public:
  struct Overflow {
    std::unique_ptr<Element> new_median;
    std::unique_ptr<Node> above_median;
  };

  // Creates a root node.
  explicit Node(int order);

  // Creates a leaf node.
  explicit Node(int order, std::vector<std::unique_ptr<Element>> elements);

  explicit Node(int order, std::vector<std::unique_ptr<Element>> elements,
                std::vector<std::unique_ptr<Node>> children);

  bool GetKey(const std::string& key, std::string* value) const;

  absl::optional<Overflow> InsertOrUpdate(const std::string& key,
                                          const std::string& value);

  std::string DebugString() const;

  std::vector<const Node*> Children() const;

 private:
  bool IsLeafNode() const { return children_.empty(); }
  int LowerBound(const std::string& key) const;

  const int order_;

  // Ordered by keys. The element pointers are never null.
  // Max size: order_ - 1.
  std::vector<std::unique_ptr<Element>> elements_;
  // If this is a leaf node, this is empty.
  //
  // Otherwise, this contains elements_.size() + 1 nodes. Node i contains
  // elements whose keys are comprised between elements_[i-1] and elements[i].
  //
  // The node pointers are never null.
  // Max size: order_;
  std::vector<std::unique_ptr<Node>> children_;
};

class BTree {
 public:
  // The order must be odd.
  explicit BTree(int order);

  void InsertOrUpdate(const std::string& key, const std::string& value);
  bool Get(const std::string& key, std::string* value) const;
  bool Remove(const std::string& key);

  std::string DebugString() const;

 private:
  const int order_;
  std::unique_ptr<Node> root_;
};
