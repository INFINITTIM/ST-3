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

class MockClient : public TimerClient {
 public:
    MOCK_METHOD(void, Timeout, (), (override));
};

class DoorMock : public Door {
 public:
    MOCK_METHOD(void, lock, (), (override));
    MOCK_METHOD(void, unlock, (), (override));
    MOCK_METHOD(bool, isDoorOpened, (), (override));
};

class DoorFixture : public ::testing::Test {
 protected:
    std::unique_ptr<TimedDoor> myDoor;

    void SetUp() override {
        myDoor = std::make_unique<TimedDoor>(10);
        myDoor->lock();
    }

    void TearDown() override {
        myDoor.reset();
    }
};

TEST_F(DoorFixture, InitiallyClosed) {
    EXPECT_FALSE(myDoor->isDoorOpened());
}

TEST_F(DoorFixture, UnlockOpens) {
    std::thread t([this]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        myDoor->lock();
    });
    EXPECT_NO_THROW(myDoor->unlock());
    t.join();
}

TEST_F(DoorFixture, LockCloses) {
    std::thread t([this]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        myDoor->lock();
    });
    myDoor->unlock();
    t.join();
    EXPECT_FALSE(myDoor->isDoorOpened());
}

TEST_F(DoorFixture, TimeoutStored) {
    EXPECT_EQ(myDoor->getTimeOut(), 10);
}

TEST(TimedDoorConstruction, CustomTimeout) {
    TimedDoor d(123);
    EXPECT_EQ(d.getTimeOut(), 123);
}

TEST(TimedDoorConstruction, InvalidTimeout) {
    TimedDoor d1(0);
    EXPECT_EQ(d1.getTimeOut(), 1);

    TimedDoor d2(-10);
    EXPECT_EQ(d2.getTimeOut(), 1);
}

TEST_F(DoorFixture, AdapterNoThrowWhenClosed) {
    DoorTimerAdapter adapter(*myDoor);
    myDoor->lock();
    EXPECT_NO_THROW(adapter.Timeout());
}

TEST_F(DoorFixture, AdapterThrowsWhenOpen) {
    TimedDoor testDoor(200);
    std::thread t([&testDoor]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        testDoor.lock();
    });
    t.join();

    DoorTimerAdapter adapter(testDoor);
    EXPECT_NO_THROW(adapter.Timeout());
}

TEST_F(DoorFixture, ThrowStateThrows) {
    EXPECT_THROW(myDoor->throwState(), std::runtime_error);
}

TEST_F(DoorFixture, ExceptionMessage) {
    try {
        myDoor->throwState();
        FAIL();
    } catch (const std::runtime_error& e) {
        EXPECT_THAT(std::string(e.what()), HasSubstr("open"));
    }
}

TEST(MockTest, TimerClientMock) {
    MockClient mock;
    EXPECT_CALL(mock, Timeout()).Times(Exactly(1));
    mock.Timeout();
}

TEST(MockTest, DoorMock) {
    DoorMock mock;
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
    MockClient mock;
    EXPECT_CALL(mock, Timeout()).Times(Exactly(1));
    timer.tregister(2, &mock);
}

TEST(TimerTest, NullClientSafe) {
    Timer timer;
    EXPECT_NO_THROW(timer.tregister(5, nullptr));
}

TEST_F(DoorFixture, MultipleOpenCloseCycles) {
    for (int i = 0; i < 3; ++i) {
        std::thread t([this]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            myDoor->lock();
        });
        myDoor->unlock();
        t.join();
        EXPECT_FALSE(myDoor->isDoorOpened());
    }
}

TEST(MultipleDoorsTest, IndependentOperation) {
    TimedDoor door1(77);
    TimedDoor door2(33);

    EXPECT_EQ(door1.getTimeOut(), 77);
    EXPECT_EQ(door2.getTimeOut(), 33);

    EXPECT_FALSE(door1.isDoorOpened());
    EXPECT_FALSE(door2.isDoorOpened());
}

TEST(AdapterReferenceTest, CorrectDoorReference) {
    TimedDoor door1(100);
    TimedDoor door2(200);

    DoorTimerAdapter adapter1(door1);
    DoorTimerAdapter adapter2(door2);

    EXPECT_NO_THROW(adapter1.Timeout());
    EXPECT_NO_THROW(adapter2.Timeout());
}
