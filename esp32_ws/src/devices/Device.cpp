#include "devices/Device.h"

Device::~Device() {
    for (Device *child : children) {
        delete child;
    }
}

void Device::addChild(Device *child) {
    if (child) {
        children.push_back(child);
    }
}

void Device::setup() {
    for (Device *child : children) {
        if (child) child->setup();
    }
}

void Device::loop() {
    for (Device *child : children) {
        if (child) child->loop();
    }
}

bool Device::control(const String &action, JsonObject *payload) {
    return false;
}

String Device::getState() {
    return "{}";
}

std::vector<int> Device::getPins() const {
    std::vector<int> pins;
    for (const Device *child : children) {
        if (child) {
            auto childPins = child->getPins();
            pins.insert(pins.end(), childPins.begin(), childPins.end());
        }
    }
    return pins;
}

void Device::setStateChangeCallback(StateChangeCallback callback) {
    stateChangeCallback = callback;
    for (Device *child : children) {
        if (child) child->setStateChangeCallback(callback);
    }
}

void Device::notifyStateChange() {
    if (stateChangeCallback) {
        stateChangeCallback(getId(), getState());
    }
}
