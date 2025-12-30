/**
 * @file Device.cpp
 * @brief Implementation of minimal device base class
 */

#include "devices/Device.h"
#include "Logging.h"

Device::Device(const String &id, const String &type) : _id(id), _type(type), _name(id) {}

void Device::setup()
{
    _isInitialized = true;

    for (Device *child : _children)
    {
        if (child)
        {
            child->setup();
        }
    }
}

void Device::loop()
{
    if (!_isInitialized)
    {
        MLOG_WARN("%s: loop() called before setup()", toString().c_str());
    }

    for (Device *child : _children)
    {
        if (child)
        {
            child->loop();
        }
    }
}

void Device::addChild(Device *child)
{
    if (child)
    {
        _children.push_back(child);
    }
}

String Device::toString() const
{
    String upperType = _type;
    upperType.toUpperCase();
    return upperType + "[" + _id + "]";
}

void Device::registerMixin(const String &mixinName)
{
    // Avoid duplicates
    for (const auto &mixin : _mixins)
    {
        if (mixin == mixinName)
        {
            return;
        }
    }
    _mixins.push_back(mixinName);
}

bool Device::hasMixin(const String &mixinName) const
{
    for (const auto &mixin : _mixins)
    {
        if (mixin == mixinName)
        {
            return true;
        }
    }
    return false;
}
