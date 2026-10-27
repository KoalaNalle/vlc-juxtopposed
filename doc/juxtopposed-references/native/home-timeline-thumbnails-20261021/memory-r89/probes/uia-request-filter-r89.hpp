// Private factor control only. Never include in the shipping GUI.
#include <QAbstractNativeEventFilter>
#include <windows.h>

class OwnedUiaRequestFilter final : public QAbstractNativeEventFilter {
public:
    bool block = false;
    qint64 requests = 0;
    bool nativeEventFilter(const QByteArray &, void *message, qintptr *result) override {
        const auto *event = static_cast<const MSG *>(message);
        if (event->message != WM_GETOBJECT) return false;
        ++requests;
        if (!block) return false;
        *result = 0;
        return true;
    }
};
