#include "devices/mixins/IControllable.h"

namespace mixins {
    static std::map<String, IControllable*> s_registry;

    void ControllableRegistry::registerDevice(const String &id, IControllable *ptr)
    {
        s_registry[id] = ptr;
    }

    void ControllableRegistry::unregisterDevice(const String &id)
    {
        auto it = s_registry.find(id);
        if (it != s_registry.end())
            s_registry.erase(it);
    }

    IControllable *ControllableRegistry::get(const String &id)
    {
        auto it = s_registry.find(id);
        if (it != s_registry.end())
            return it->second;
        return nullptr;
    }
}
