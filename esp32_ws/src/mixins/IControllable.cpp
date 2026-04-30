#include "devices/mixins/IControllable.h"
#include "devices/mixins/MixinRegistry.h"

// ControllableRegistry is a thin wrapper around the generic MixinRegistry
// template (see MixinRegistry.h).

namespace mixins {
    void ControllableRegistry::registerDevice(const String &id, IControllable *ptr)
    {
        MixinRegistry<IControllable>::registerDevice(id, ptr);
    }

    void ControllableRegistry::unregisterDevice(const String &id)
    {
        MixinRegistry<IControllable>::unregisterDevice(id);
    }

    IControllable *ControllableRegistry::get(const String &id)
    {
        return MixinRegistry<IControllable>::get(id);
    }
}
