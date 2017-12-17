#include "b_tree.h"

#include <algorithm>
#include <deque>
#include <iostream>
#include <iterator>

Element::Element(std::string key, std::string value)
    : key_(std::move(key)), value_(std::move(value)) {}

Node::Node(int order, std::vector<std::unique_ptr<Element>> elements,
           std::vector<std::unique_ptr<Node>> children)
    : order_(order), elements_(std::move(elements)), children_(std::move(children)) {}

Node::Node(int order, std::vector<std::unique_ptr<Element>> elements)
    : order_(order), elements_(std::move(elements)), children_() {}

Node::Node(int order) : order_(order) {}

int Node::LowerBound(const std::string& key) const {
  int first = 0;
  int last = elements_.size();
  int distance;
  while (distance = last - first, distance > 0) {
    auto middle = first + distance / 2;
    const std::string& middle_key = elements_[middle]->key();
    if (middle_key < key) {
      first = ++middle;
    } else if (middle_key > key) {
      last = middle;
    } else {
      return middle;
    }
  }
  return first;
}

bool Node::GetKey(const std::string& key, std::string* value) const {
  int lower_bound = LowerBound(key);

  if (lower_bound != elements_.size() && elements_[lower_bound]->key() == key) {
    *value = elements_[lower_bound]->value();
    return true;
  }

  if (IsLeafNode()) {
    return false;
  }

  const Node& child_to_search = *children_[lower_bound];
  return child_to_search.GetKey(key, value);
}

namespace {
template <typename T>
void InsertAndShift(int at_position, int max_shift_position, T* value,
                    std::vector<T>* vec) {
  for (int i = at_position; i < max_shift_position; ++i) {
    (*vec)[i].swap(*value);
  }
}

template <typename T>
void Transfer(int min, int max, std::vector<T>* from, std::vector<T>* to) {
  for (int i = min; i < max; ++i) {
    to->push_back(std::move((*from)[i]));
  }
}

}  // namespace

std::string Node::DebugString() const {
  std::string result = IsLeafNode() ? "{" : "[";
  for (int i = 0; i < elements_.size(); ++i) {
    if (i != 0) {
      result += ", ";
    }
    result += elements_[i]->key();  // TODO: don't concatenate.
  }
  result += IsLeafNode()? "}" : "]";
  return result;
}

std::vector<const Node*> Node::Children() const {
  std::vector<const Node*> result;
  for (const auto& child : children_) {
    result.push_back(child.get());
  }
  return result;
}

absl::optional<Node::Overflow> Node::InsertOrUpdate(const std::string& key,
                                                    const std::string& value) {
  const int lower_bound = LowerBound(key);
  if (lower_bound < elements_.size() && elements_[lower_bound]->key() == key) {
    elements_[lower_bound]->set_value(value);
    return absl::nullopt;
  }

  const int middle = order_ / 2;

  if (IsLeafNode()) {
    auto new_element = absl::make_unique<Element>(key, value);

    if (elements_.size() < order_ - 1) {
      // There is enough space.
      InsertAndShift(lower_bound, elements_.size(), &new_element, &elements_);
      elements_.push_back(std::move(new_element));
      return absl::nullopt;
    }

    // We need to split the node.
    Overflow overflow;
    std::vector<std::unique_ptr<Element>> new_elements;
    new_elements.reserve(order_ / 2);

    if (lower_bound <= middle) {
      InsertAndShift(lower_bound, middle, &new_element, &elements_);
      overflow.new_median = std::move(new_element);
      Transfer(middle, elements_.size(), &elements_, &new_elements);
    } else {
      overflow.new_median = std::move(elements_[middle]);
      Transfer(middle + 1, lower_bound, &elements_, &new_elements);
      new_elements.push_back(std::move(new_element));
      Transfer(lower_bound, elements_.size(), &elements_, &new_elements);
    }
    elements_.resize(middle);

    overflow.above_median =
        absl::make_unique<Node>(order_, std::move(new_elements));
    return absl::make_optional(std::move(overflow));
  }

  // This is a non-leaf node.
  auto child_overflow = children_[lower_bound]->InsertOrUpdate(key, value);
  if (!child_overflow) {
    return absl::nullopt;
  }
  if (elements_.size() < order_ - 1) {
    InsertAndShift(lower_bound, elements_.size(), &child_overflow->new_median,
                   &elements_);
    elements_.push_back(std::move(child_overflow->new_median));

    InsertAndShift(lower_bound + 1, children_.size(), &child_overflow->above_median,
                   &children_);
    children_.push_back(std::move(child_overflow->above_median));
    return absl::nullopt;
  }

  // We need to split the node.
  Overflow overflow;
  std::vector<std::unique_ptr<Element>> new_elements;
  new_elements.reserve(order_ / 2);
  std::vector<std::unique_ptr<Node>> new_children;
  new_children.reserve((order_ + 1) / 2);

  if (lower_bound <= middle) {
    InsertAndShift(lower_bound, middle, &child_overflow->new_median, &elements_);
    overflow.new_median = std::move(child_overflow->new_median);
    Transfer(middle, elements_.size(), &elements_, &new_elements);

    InsertAndShift(lower_bound + 1, middle + 1, &child_overflow->above_median,
                   &children_);
    new_children.push_back(std::move(child_overflow->above_median));
    Transfer(middle + 1, children_.size(), &children_, &new_children);
  } else {
    overflow.new_median = std::move(elements_[middle]);
    Transfer(middle + 1, lower_bound, &elements_, &new_elements);
    new_elements.push_back(std::move(child_overflow->new_median));
    Transfer(lower_bound, elements_.size(), &elements_, &new_elements);

    Transfer(middle + 1, lower_bound + 1, &children_, &new_children);
    new_children.push_back(std::move(child_overflow->above_median));
    Transfer(lower_bound + 1, children_.size(), &children_, &new_children);
  }

  elements_.resize(middle);
  children_.resize(middle + 1);

  overflow.above_median = absl::make_unique<Node>(
      order_, std::move(new_elements), std::move(new_children));
  return absl::make_optional(std::move(overflow));
}

BTree::BTree(int order)
    : order_(order), root_(absl::make_unique<Node>(order_)) {}

bool BTree::Get(const std::string& key, std::string* value) const {
  return root_->GetKey(key, value);
}

void BTree::InsertOrUpdate(const std::string& key, const std::string& value) {
  auto overflow = root_->InsertOrUpdate(key, value);

  if (!overflow) {
    return;
  }

  std::vector<std::unique_ptr<Element>> elements(1);
  elements[0] = std::move(overflow->new_median);

  std::vector<std::unique_ptr<Node>> children(2);
  children[0] = std::move(root_);
  children[1] = std::move(overflow->above_median);

  root_ =
      absl::make_unique<Node>(order_, std::move(elements), std::move(children));
}

std::string BTree::DebugString() const {
  std::string result;

  std::deque<std::pair<const Node*, int>> nodes = {{root_.get(), 0}};
  int current_level = 0;
  while (!nodes.empty()) {
    auto node_and_level = nodes.front();
    nodes.pop_front();

    if (node_and_level.second > current_level) {
      result += "\n";
      ++current_level;
    }

    const Node* node = node_and_level.first;
    result += node->DebugString() + "  ";

    for (const Node* child : node->Children()) {
      nodes.push_back({child, current_level+1});
    }
  }
  return result;
}

bool BTree::Remove(const std::string& key) { return false; }
