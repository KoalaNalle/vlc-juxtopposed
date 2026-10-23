// Read-only request counting; every accessibility request is forwarded.
#include <QAbstractNativeEventFilter>
#include <windows.h>

class OwnedUiaRequestFilter final : public QAbstractNativeEventFilter {
public:
    qint64 requests = 0, uiaRootRequests = 0, msaaClientRequests = 0, otherRequests = 0;
    bool nativeEventFilter(const QByteArray &, void *message, qintptr *) override {
        const auto *event = static_cast<const MSG *>(message);
        if (event->message != WM_GETOBJECT) return false;
        ++requests;
        const DWORD identifier = static_cast<DWORD>(event->lParam);
        if (identifier == static_cast<DWORD>(-25)) ++uiaRootRequests;
        else if (identifier == static_cast<DWORD>(OBJID_CLIENT)) ++msaaClientRequests;
        else ++otherRequests;
        return false;
    }
};
