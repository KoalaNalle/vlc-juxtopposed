// Private link-time factor control; this is not a production fix.
#include <qpa/qplatformaccessibility.h>
#include <atomic>

namespace OwnedActivation {
inline bool deduplicate = false;
inline std::atomic<quint64> calls{0}, forwarded{0}, duplicatesSkipped{0};
}
extern "C" void __real__ZN22QPlatformAccessibility9setActiveEb(QPlatformAccessibility *, bool);
extern "C" void __wrap__ZN22QPlatformAccessibility9setActiveEb(QPlatformAccessibility *platform, bool active)
{
    ++OwnedActivation::calls;
    if (OwnedActivation::deduplicate && platform->isActive() == active) {
        ++OwnedActivation::duplicatesSkipped;
        return;
    }
    ++OwnedActivation::forwarded;
    __real__ZN22QPlatformAccessibility9setActiveEb(platform, active);
}
