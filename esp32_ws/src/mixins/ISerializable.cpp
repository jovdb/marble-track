#include "devices/mixins/SerializableMixin.h"

// Mixin registry pattern
//
// Devices use a composition model rather than deep inheritance. A device that
// supports serialization inherits from ISerializable (via SerializableMixin<>)
// AND registers itself here in its constructor. Code outside the device (e.g.
// WebSocketManager) queries the registry by device-id to obtain the
// ISerializable interface without needing to know the concrete type, keeping
// the coupling loose.
//
// The same pattern is used for IControllable (ControllableRegistry in
// IControllable.cpp). Both registries use String keys (device id), which is
// slightly slower than integer keys but avoids adding an id-integer mapping.
//
// Lifetime: a device registers on construction and unregisters in its
// destructor, so the registry entry is always valid while the device exists.

namespace mixins {
    static std::map<String, ISerializable*> s_serializableRegistry;

    void SerializableRegistry::registerDevice(const String &id, ISerializable *ptr)
    {
        s_serializableRegistry[id] = ptr;
    }

    void SerializableRegistry::unregisterDevice(const String &id)
    {
        auto it = s_serializableRegistry.find(id);
        if (it != s_serializableRegistry.end())
            s_serializableRegistry.erase(it);
    }

    ISerializable *SerializableRegistry::get(const String &id)
    {
        auto it = s_serializableRegistry.find(id);
        if (it != s_serializableRegistry.end())
            return it->second;
        return nullptr;
    }
}
