// Copyright 2021 GHA Test Team

// src/TimedDoor.cpp
#include "TimedDoor.h"

#include <stdexcept>
#include <thread>
#include <chrono>

DoorTimerAdapter::DoorTimerAdapter(TimedDoor& d) : door(d) {}

void DoorTimerAdapter::Timeout() {
    if (door.isDoorOpened()) {
        door.throwState();
    }
}

TimedDoor::TimedDoor(int timeout) 
    : iTimeout(timeout), 
      isOpened(false) {
}

bool TimedDoor::isDoorOpened() {
    return isOpened;
}

void TimedDoor::unlock() {
    isOpened = true;
}

void TimedDoor::lock() {
    isOpened = false;
}

int TimedDoor::getTimeOut() const {
    return iTimeout;
}

void TimedDoor::throwState() {
    throw std::runtime_error("Door timeout error: door left open too long!");
}

void Timer::sleep(int milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

void Timer::tregister(int timeout, TimerClient* newClient) {
    client = newClient;
    sleep(timeout);
    if (client != nullptr) {
        client->Timeout();
    }
}