#include "lf_queue.hpp"

#include <gtest/gtest.h>
#include <vector>
#include <memory>

TEST(TypeTests, IntType) {
  lf_lab::LFQueue<int> queue;

  queue.enqueue(42);
  queue.enqueue(-123);
  queue.enqueue(0);
  queue.enqueue(999999);

  int result;
  ASSERT_TRUE(queue.dequeue(result));
  EXPECT_EQ(result, 42);
  ASSERT_TRUE(queue.dequeue(result));
  EXPECT_EQ(result, -123);
  ASSERT_TRUE(queue.dequeue(result));
  EXPECT_EQ(result, 0);
  ASSERT_TRUE(queue.dequeue(result));
  EXPECT_EQ(result, 999999);

  EXPECT_TRUE(queue.empty());
}

TEST(TypeTests, DoubleType) {
  lf_lab::LFQueue<double> queue;

  queue.enqueue(3.14159);
  queue.enqueue(-2.71828);
  queue.enqueue(1.61803);
  queue.enqueue(0.0);

  double result;
  ASSERT_TRUE(queue.dequeue(result));
  EXPECT_DOUBLE_EQ(result, 3.14159);
  ASSERT_TRUE(queue.dequeue(result));
  EXPECT_DOUBLE_EQ(result, -2.71828);
  ASSERT_TRUE(queue.dequeue(result));
  EXPECT_DOUBLE_EQ(result, 1.61803);
  ASSERT_TRUE(queue.dequeue(result));
  EXPECT_DOUBLE_EQ(result, 0.0);

  EXPECT_TRUE(queue.empty());
}

TEST(TypeTests, StringType) {
  lf_lab::LFQueue<std::string> queue;

  std::string str1 = "Hello";
  std::string str2 = "World";
  std::string str3 = "Test";
  std::string str4 = "String";

  queue.enqueue(str1);
  queue.enqueue(str2);
  queue.enqueue(str3);
  queue.enqueue(str4);

  std::string result;
  ASSERT_TRUE(queue.dequeue(result));
  EXPECT_EQ(result, "Hello");
  ASSERT_TRUE(queue.dequeue(result));
  EXPECT_EQ(result, "World");
  ASSERT_TRUE(queue.dequeue(result));
  EXPECT_EQ(result, "Test");
  ASSERT_TRUE(queue.dequeue(result));
  EXPECT_EQ(result, "String");

  EXPECT_TRUE(queue.empty());
}

TEST(TypeTests, VectorType) {
  lf_lab::LFQueue<std::vector<int>> queue;

  std::vector<int> v1 = {1, 2, 3};
  std::vector<int> v2 = {4, 5, 6, 7};
  std::vector<int> v3 = {8, 9};
  std::vector<int> v4 = {};

  queue.enqueue(v1);
  queue.enqueue(v2);
  queue.enqueue(v3);
  queue.enqueue(v4);

  std::vector<int> result;
  ASSERT_TRUE(queue.dequeue(result));
  EXPECT_EQ(result, std::vector<int>({1, 2, 3}));
  ASSERT_TRUE(queue.dequeue(result));
  EXPECT_EQ(result, std::vector<int>({4, 5, 6, 7}));
  ASSERT_TRUE(queue.dequeue(result));
  EXPECT_EQ(result, std::vector<int>({8, 9}));
  ASSERT_TRUE(queue.dequeue(result));
  EXPECT_TRUE(result.empty());

  EXPECT_TRUE(queue.empty());
}

TEST(TypeTests, UniquePtr) {
  lf_lab::LFQueue<std::unique_ptr<int>> queue;

  queue.enqueue(std::make_unique<int>(42));
  queue.enqueue(std::make_unique<int>(123));
  queue.enqueue(std::make_unique<int>(456));

  std::unique_ptr<int> result;
  ASSERT_TRUE(queue.dequeue(result));
  EXPECT_EQ(*result, 42);
  ASSERT_TRUE(queue.dequeue(result));
  EXPECT_EQ(*result, 123);
  ASSERT_TRUE(queue.dequeue(result));
  EXPECT_EQ(*result, 456);

  EXPECT_TRUE(queue.empty());
}

struct TestStruct {
  int a;
  double b;
  std::string c;

  TestStruct(int a, double b, const std::string& c) : a(a), b(b), c(c) {}

  bool operator==(const TestStruct& other) const {
    return a == other.a && b == other.b && c == other.c;
  }
};

TEST(TypeTests, CustomStruct) {
  lf_lab::LFQueue<TestStruct> queue;

  queue.enqueue(TestStruct(1, 1.5, "one"));
  queue.enqueue(TestStruct(2, 2.5, "two"));
  queue.enqueue(TestStruct(3, 3.5, "three"));

  TestStruct result(0, 0.0, "");
  ASSERT_TRUE(queue.dequeue(result));
  EXPECT_EQ(result, TestStruct(1, 1.5, "one"));
  ASSERT_TRUE(queue.dequeue(result));
  EXPECT_EQ(result, TestStruct(2, 2.5, "two"));
  ASSERT_TRUE(queue.dequeue(result));
  EXPECT_EQ(result, TestStruct(3, 3.5, "three"));

  EXPECT_TRUE(queue.empty());
}

class MoveOnlyType {
public:
  MoveOnlyType() : value_(0) {}
  explicit MoveOnlyType(int value) : value_(value) {}
  MoveOnlyType(const MoveOnlyType&) = delete;
  MoveOnlyType& operator=(const MoveOnlyType&) = delete;
  MoveOnlyType(MoveOnlyType&& other) noexcept : value_(other.value_) {
    other.value_ = 0;
  }
  MoveOnlyType& operator=(MoveOnlyType&& other) noexcept {
    if (this != &other) {
      value_ = other.value_;
      other.value_ = 0;
    }
    return *this;
  }
  int value() const { return value_; }

private:
  int value_;
};

TEST(TypeTests, MoveOnlyType) {
  lf_lab::LFQueue<MoveOnlyType> queue;

  queue.enqueue(MoveOnlyType(42));
  queue.enqueue(MoveOnlyType(123));
  queue.enqueue(MoveOnlyType(456));

  MoveOnlyType result;
  ASSERT_TRUE(queue.dequeue(result));
  EXPECT_EQ(result.value(), 42);
  ASSERT_TRUE(queue.dequeue(result));
  EXPECT_EQ(result.value(), 123);
  ASSERT_TRUE(queue.dequeue(result));
  EXPECT_EQ(result.value(), 456);

  EXPECT_TRUE(queue.empty());
}
