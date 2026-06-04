// Copyright 2021 GHA Test Team

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <chrono>
#include "TimedDoor.h"

using ::testing::Return;
using ::testing::Exactly;
using ::testing::AtLeast;
using ::testing::HasSubstr;

class MockTimerClient : public TimerClient {
 public:
    MOCK_METHOD(void, Timeout, (), (override));
};

class MockDoor : public Door {
 public:
    MOCK_METHOD(void, lock, (), (override));
    MOCK_METHOD(void, unlock, (), (override));
    MOCK_METHOD(bool, isDoorOpened, (), (override));
};

class TimedDoorTest : public ::testing::Test {
 protected:
    std::unique_ptr<TimedDoor> door;

    void SetUp() override {
        door = std::make_unique<TimedDoor>(5);
        door->lock();
    }

    void TearDown() override {
        door.reset();
    }
};

TEST_F(TimedDoorTest, InitiallyClosed) {
    EXPECT_FALSE(door->isDoorOpened());
}

TEST_F(TimedDoorTest, UnlockOpens) {
    std::thread t([this]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        door->lock();
    });
    EXPECT_NO_THROW(door->unlock());
    t.join();
}

TEST_F(TimedDoorTest, LockCloses) {
    std::thread t([this]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        door->lock();
    });
    door->unlock();
    t.join();
    EXPECT_FALSE(door->isDoorOpened());
}

TEST_F(TimedDoorTest, TimeoutStored) {
    EXPECT_EQ(door->getTimeOut(), 5);
}

TEST(TimedDoorConstruction, CustomTimeout) {
    TimedDoor d(42);
    EXPECT_EQ(d.getTimeOut(), 42);
}

TEST(TimedDoorConstruction, InvalidTimeout) {
    TimedDoor d1(0);
    EXPECT_EQ(d1.getTimeOut(), 1);
    
    TimedDoor d2(-10);
    EXPECT_EQ(d2.getTimeOut(), 1);
}

TEST_F(TimedDoorTest, AdapterNoThrowWhenClosed) {
    DoorTimerAdapter adapter(*door);
    door->lock();
    EXPECT_NO_THROW(adapter.Timeout());
}

TEST_F(TimedDoorTest, AdapterThrowsWhenOpen) {
    TimedDoor testDoor(1000);
    std::thread t([&testDoor]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        testDoor.lock();
    });
    t.join();
    
    DoorTimerAdapter adapter(testDoor);
    EXPECT_NO_THROW(adapter.Timeout());
}

TEST_F(TimedDoorTest, ThrowStateThrows) {
    EXPECT_THROW(door->throwState(), std::runtime_error);
}

TEST_F(TimedDoorTest, ExceptionMessage) {
    try {
        door->throwState();
        FAIL();
    } catch (const std::runtime_error& e) {
        EXPECT_THAT(std::string(e.what()), HasSubstr("open"));
    }
}

TEST(MockTest, TimerClientMock) {
    MockTimerClient mock;
    EXPECT_CALL(mock, Timeout()).Times(Exactly(1));
    mock.Timeout();
}

TEST(MockTest, DoorMock) {
    MockDoor mock;
    EXPECT_CALL(mock, unlock()).Times(Exactly(1));
    EXPECT_CALL(mock, isDoorOpened())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(mock, lock()).Times(Exactly(1));
    
    mock.unlock();
    EXPECT_TRUE(mock.isDoorOpened());
    mock.lock();
}

TEST(TimerTest, TimerCallsClient) {
    Timer timer;
    MockTimerClient mock;
    EXPECT_CALL(mock, Timeout()).Times(Exactly(1));
    timer.tregister(2, &mock);
}

TEST(TimerTest, NullClientSafe) {
    Timer timer;
    EXPECT_NO_THROW(timer.tregister(5, nullptr));
}
