#include "devices/mixins/SerializableMixin.h"
#include "devices/mixins/MixinRegistry.h"

// SerializableRegistry is a thin wrapper around the generic MixinRegistry
// template. The actual map and lookup logic lives in MixinRegistry.h, which
// is also used by ControllableRegistry. See MixinRegistry.h for the rationale
// and lifetime contract of the mixin-registry pattern.

namespace mixins {
    void SerializableRegistry::registerDevice(const String &id, ISerializable *ptr)
    {
        MixinRegistry<ISerializable>::registerDevice(id, ptr);
    }

    void SerializableRegistry::unregisterDevice(const String &id)
    {
        MixinRegistry<ISerializable>::unregisterDevice(id);
    }

    ISerializable *SerializableRegistry::get(const String &id)
    {
        return MixinRegistry<ISerializable>::get(id);
    }
}
