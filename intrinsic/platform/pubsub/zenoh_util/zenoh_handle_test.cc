// Copyright 2023 Intrinsic Innovation LLC

#include "intrinsic/platform/pubsub/zenoh_util/zenoh_handle.h"
#include "intrinsic/platform/pubsub/zenoh_util/zenoh_config.h"

#include "absl/log/log.h"
#include <gtest/gtest.h>

namespace intrinsic {

void nested_callback(const char *, const void *, const size_t, void *) {
  LOG(INFO) << "nested_callback";
}

void test_callback(const char *, const void *, const size_t, void *context) {
  LOG(INFO) << "test_callback";
  ZenohHandle *h = reinterpret_cast<ZenohHandle *>(context);

  // if the mutex locks inside the imw implementation are overly protective,
  // this call will hang as the call stack becomes publish-callback-publish
  h->imw_publish("nested", "bar", 4);
}

TEST(ZenohHandleTest, Initialize) {
  std::string config = GetZenohPeerConfig();
  std::unique_ptr<ZenohHandle> h(ZenohHandle::CreateZenohHandle());

  EXPECT_NE(h, nullptr);
  EXPECT_EQ(IMW_OK, h->imw_init(config.c_str()));

  EXPECT_EQ(IMW_OK, h->imw_create_publisher("test", "{}"));
  EXPECT_EQ(IMW_OK,
            h->imw_create_subscription("test", test_callback, "{}", h.get()));

  EXPECT_EQ(IMW_OK, h->imw_create_publisher("nested", "{}"));
  EXPECT_EQ(IMW_OK, h->imw_create_subscription("nested", nested_callback, "{}",
                                               nullptr));

  // so long as this doesn't deadlock, we'll consider this test "passed"
  EXPECT_EQ(IMW_OK, h->imw_publish("test", "foo", 4));

  EXPECT_EQ(IMW_OK,
            h->imw_destroy_subscription("nested", nested_callback, nullptr));
  EXPECT_EQ(IMW_OK, h->imw_destroy_publisher("nested"));

  EXPECT_EQ(IMW_OK,
            h->imw_destroy_subscription("test", test_callback, h.get()));
  EXPECT_EQ(IMW_OK, h->imw_destroy_publisher("test"));

  EXPECT_EQ(IMW_OK, h->imw_fini());
}

TEST(ZenohHandleTest, AddTopicPrefix) {
  EXPECT_EQ(*ZenohHandle::add_topic_prefix("foo"), "in/foo");
  EXPECT_EQ(*ZenohHandle::add_topic_prefix("/foo"), "in/foo");
  EXPECT_EQ(ZenohHandle::add_topic_prefix("").ok(), false);
}

TEST(ZenohHandleTest, RemoveTopicPrefix) {
  EXPECT_EQ(*ZenohHandle::remove_topic_prefix("in/foo"), "/foo");
  EXPECT_EQ(*ZenohHandle::remove_topic_prefix("in/"), "/");
  EXPECT_EQ(ZenohHandle::remove_topic_prefix("").ok(), false);
}

}  // namespace intrinsic
