#include "b_tree.h"

#include <map>
#include <random>
#include <string>

#include "gtest/gtest.h"

TEST(BTree, InsertsThenRetrievesOneValue) {
  BTree tree(3);
  tree.InsertOrUpdate("foo", "fooooo");

  std::string result;
  ASSERT_TRUE(tree.Get("foo", &result));
  ASSERT_EQ("fooooo", result);
}

TEST(BTree, InsertsThenRetrievesMultipleValues) {
  BTree tree(3);
  tree.InsertOrUpdate("foo", "fooooo");

  std::string result;
  ASSERT_TRUE(tree.Get("foo", &result));
  ASSERT_EQ("fooooo", result);

  tree.InsertOrUpdate("bar", "barbar");

  ASSERT_TRUE(tree.Get("foo", &result));
  ASSERT_EQ("fooooo", result);
  ASSERT_TRUE(tree.Get("bar", &result));
  ASSERT_EQ("barbar", result);

  ASSERT_FALSE(tree.Get("baz", &result));
}

class RandomStringGenerator {
  public:
   RandomStringGenerator()
       : random_device_(),
         generator_(random_device_()),
         char_distribution_(0, 25),
         size_distribution_(2, 5) {}

   std::string GenerateRandomString() {
     int size = size_distribution_(generator_);
     std::string result(size, '-');
     for (int i = 0; i < size; ++i) {
       result[i] = 'a' + static_cast<char>(char_distribution_(generator_));
     }
     return result;
   }

 private:
  std::random_device random_device_;
  std::mt19937 generator_;
  std::uniform_int_distribution<int> char_distribution_;
  std::uniform_int_distribution<int> size_distribution_;
};

TEST(BTree, InsertsThenRetrievesManyValues) {
  RandomStringGenerator string_generator;

  BTree tree(3);
  std::map<std::string, std::string> map;

  for (int i = 0; i < 5000; ++i) {
    std::cout << tree.DebugString() << std::endl;

    std::string key = string_generator.GenerateRandomString();
    std::string value = string_generator.GenerateRandomString();

    std::cout << std::endl << "Inserting " << key << std::endl;
    map[key] = value;
    tree.InsertOrUpdate(key, value);



    if (i % 100 == 0) {
      for (const auto& pair : map) {
        std::string result;
        ASSERT_TRUE(tree.Get(pair.first, &result));
        ASSERT_EQ(result, pair.second);
      }

      for (int j = 0; j < 100; ++j) {
        std::string rnd_key = string_generator.GenerateRandomString();
        if (map.find(rnd_key) == map.end()) {
          std::string result;
          ASSERT_FALSE(tree.Get(rnd_key, &result));
        }
      }
    }
  }
}
