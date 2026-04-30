#pragma once

#include <Arduino.h>
#include <map>

namespace mixins
{

/**
 * @brief Generic registry mapping device-id (String) to a mixin interface pointer.
 *
 * Each unique interface type Iface gets its own static map (one per template
 * instantiation). Used as the single backing implementation for both
 * SerializableRegistry (= MixinRegistry<ISerializable>) and ControllableRegistry
 * (= MixinRegistry<IControllable>).
 *
 * Adding a new mixin registry now means: declare a thin wrapper class that
 * forwards to MixinRegistry<INewMixin>; no copy-pasted map / find / erase logic.
 *
 * Lifetime: a device is expected to register itself at construction and
 * unregister at destruction (see SerializableMixin / ControllableMixin), so
 * lookups always return either a valid pointer or nullptr (never a dangler).
 */
template <typename Iface>
class MixinRegistry
{
public:
    static void registerDevice(const String &id, Iface *ptr)
    {
        instances()[id] = ptr;
    }

    static void unregisterDevice(const String &id)
    {
        instances().erase(id);
    }

    static Iface *get(const String &id)
    {
        auto &m = instances();
        auto it = m.find(id);
        return (it != m.end()) ? it->second : nullptr;
    }

private:
    // Function-local static guarantees safe initialization order across
    // translation units.
    static std::map<String, Iface *> &instances()
    {
        static std::map<String, Iface *> s_map;
        return s_map;
    }
};

} // namespace mixins
