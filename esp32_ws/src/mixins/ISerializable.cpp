#include "devices/mixins/SerializableMixin.h"

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
