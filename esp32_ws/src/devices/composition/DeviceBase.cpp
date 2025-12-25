/**
 * @file DeviceBase.cpp
 * @brief Implementation of minimal device base class
 */

#include "devices/composition/DeviceBase.h"
#include "Logging.h"

DeviceBase::DeviceBase(const String &id, const String &type) : _id(id), _type(type), _name(id) {}

void DeviceBase::setup()
{
    _isInitialized = true;

    for (DeviceBase *child : _children)
    {
        if (child)
        {
            child->setup();
        }
    }
}

void DeviceBase::loop()
{
    if (!_isInitialized)
    {
        MLOG_WARN("%s: loop() called before setup()", toString().c_str());
    }

    for (DeviceBase *child : _children)
    {
        if (child)
        {
            child->loop();
        }
    }
}

void DeviceBase::addChild(DeviceBase *child)
{
    if (child)
    {
        _children.push_back(child);
    }
}

String DeviceBase::toString() const
{
    String upperType = _type;
    upperType.toUpperCase();
    return upperType + "[" + _id + "]";
}

void DeviceBase::registerMixin(const String &mixinName)
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

bool DeviceBase::hasMixin(const String &mixinName) const
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
