// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "audio_windows_devices.hpp"
#include <QVariantMap>
#ifdef _WIN32
# include <windows.h>
# include <mmdeviceapi.h>
# include <propsys.h>
# include <memory>
namespace {
template <typename T> struct Release { void operator()(T *value) const { if (value) value->Release(); } };
template <typename T> using Com = std::unique_ptr<T, Release<T>>;
// Same documented friendly-name PROPERTYKEY used by the genuine mmdevice.c.
const PROPERTYKEY friendlyName = {{0xa45c254e,0xdf1c,0x4efd,{0x80,0x20,0x67,0xd1,0x46,0xa8,0x50,0xe0}},14};
}
#endif
bool enumerateAudioWindowsDevices(QVariantList &devices)
{
    devices.clear();
#ifdef _WIN32
    const HRESULT apartment = CoInitializeEx(nullptr, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(apartment) && apartment != RPC_E_CHANGED_MODE) return false;
    struct Apartment { bool owned; ~Apartment() { if (owned) CoUninitialize(); } } scope{SUCCEEDED(apartment)};
    IMMDeviceEnumerator *rawEnumerator = nullptr;
    HRESULT result = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER,
                                     __uuidof(IMMDeviceEnumerator), reinterpret_cast<void **>(&rawEnumerator));
    if (FAILED(result) || !rawEnumerator) return false;
    Com<IMMDeviceEnumerator> enumerator(rawEnumerator);
    IMMDeviceCollection *rawCollection = nullptr;
    if (FAILED(enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &rawCollection)) || !rawCollection) return false;
    Com<IMMDeviceCollection> collection(rawCollection); UINT count = 0;
    if (FAILED(collection->GetCount(&count))) return false;
    QVariantList rows{QVariantMap{{QStringLiteral("value"), QString{}}, {QStringLiteral("name"), QStringLiteral("System default")}}};
    for (UINT i = 0; i < count; ++i) {
        IMMDevice *rawDevice = nullptr;
        if (FAILED(collection->Item(i, &rawDevice)) || !rawDevice) return false;
        Com<IMMDevice> device(rawDevice); LPWSTR id = nullptr;
        if (FAILED(device->GetId(&id)) || !id) return false;
        const QString identifier = QString::fromWCharArray(id); CoTaskMemFree(id);
        IPropertyStore *rawProperties = nullptr;
        if (FAILED(device->OpenPropertyStore(STGM_READ, &rawProperties)) || !rawProperties) return false;
        Com<IPropertyStore> properties(rawProperties); PROPVARIANT title{};
        result = properties->GetValue(friendlyName, &title);
        const QString label = SUCCEEDED(result) && title.vt == VT_LPWSTR && title.pwszVal
            ? QString::fromWCharArray(title.pwszVal) : identifier;
        PropVariantClear(&title);
        rows.push_back(QVariantMap{{QStringLiteral("value"),identifier},{QStringLiteral("name"),label}});
    }
    devices = rows; return true;
#else
    return false;
#endif
}
