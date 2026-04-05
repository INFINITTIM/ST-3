// Copyright 2021 GHA Test Team

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "TimedDoor.h"
#include <memory>
#include <stdexcept>

class MockDoor : public Door {
 public:
  MOCK_METHOD(void, lock, (), (override));
  MOCK_METHOD(void, unlock, (), (override));
  MOCK_METHOD(bool, isDoorOpened, (), (override));
};

class MockTimerClient : public TimerClient {
 public:
  MOCK_METHOD(void, Timeout, (), (override));
};

class TimedDoorTest : public ::testing::Test {
 protected:
  std::unique_ptr<TimedDoor> door;
  static constexpr int DEFAULT_TIMEOUT = 1;

  void SetUp() override {
    door = std::make_unique<TimedDoor>(DEFAULT_TIMEOUT);
    door->lock();
  }

  void TearDown() override {
    door.reset();
  }
};

TEST_F(TimedDoorTest, DoorIsInitiallyClosed) {
  EXPECT_FALSE(door->isDoorOpened());
}

TEST_F(TimedDoorTest, UnlockOpensDoor) {
  door->unlock();
  EXPECT_TRUE(door->isDoorOpened());
}

TEST_F(TimedDoorTest, LockClosesDoor) {
  door->unlock();
  door->lock();
  EXPECT_FALSE(door->isDoorOpened());
}

TEST_F(TimedDoorTest, GetTimeOutReturnsCorrectValue) {
  EXPECT_EQ(door->getTimeOut(), DEFAULT_TIMEOUT);
}

TEST_F(TimedDoorTest, ConstructorSetsCustomTimeout) {
  TimedDoor customDoor(500);
  EXPECT_EQ(customDoor.getTimeOut(), 500);
}

TEST_F(TimedDoorTest, MultipleLockUnlockCycles) {
  for (int i = 0; i < 3; ++i) {
    door->unlock();
    EXPECT_TRUE(door->isDoorOpened());
    door->lock();
    EXPECT_FALSE(door->isDoorOpened());
  }
}

TEST_F(TimedDoorTest, AdapterTimeoutThrowsWhenDoorOpen) {
  door->unlock();
  DoorTimerAdapter adapter(*door);
  EXPECT_THROW(adapter.Timeout(), std::runtime_error);
}

TEST_F(TimedDoorTest, AdapterTimeoutNoThrowWhenDoorClosed) {
  door->lock();
  DoorTimerAdapter adapter(*door);
  EXPECT_NO_THROW(adapter.Timeout());
}

TEST_F(TimedDoorTest, AdapterUsesDoorReferenceCorrectly) {
  door->unlock();
  DoorTimerAdapter adapter(*door);
  door->lock();
  EXPECT_NO_THROW(adapter.Timeout());
}

class TimerTest : public ::testing::Test {
 protected:
  Timer timer;
  std::unique_ptr<MockTimerClient> mockClient;

  void SetUp() override {
    mockClient = std::make_unique<MockTimerClient>();
  }
};

TEST_F(TimerTest, TimerCallsTimeoutAfterRegistration) {
  EXPECT_CALL(*mockClient, Timeout()).Times(1);
  timer.tregister(1, mockClient.get());
}

TEST_F(TimerTest, TimerHandlesNullClient) {
  EXPECT_NO_THROW(timer.tregister(1, nullptr));
}

TEST(MockDoorTest, MockTracksCalls) {
  MockDoor mockDoor;
  EXPECT_CALL(mockDoor, unlock()).Times(1);
  EXPECT_CALL(mockDoor, isDoorOpened()).WillOnce(::testing::Return(true));
  mockDoor.unlock();
  EXPECT_TRUE(mockDoor.isDoorOpened());
}

TEST(TimedDoorIntegrationTest, OpenDoorTimeoutThrows) {
  TimedDoor testDoor(1);
  testDoor.unlock();
  DoorTimerAdapter adapter(testDoor);
  EXPECT_THROW(adapter.Timeout(), std::runtime_error);
}

TEST(TimedDoorIntegrationTest, ExceptionHasDescriptiveMessage) {
  TimedDoor testDoor(1);
  testDoor.unlock();
  DoorTimerAdapter adapter(testDoor);
  try {
    adapter.Timeout();
    FAIL() << "Expected exception";
  } catch (const std::runtime_error& e) {
    EXPECT_THAT(e.what(), ::testing::HasSubstr("timeout"));
  }
}
