#include "pch.h"
#include <Windows.h>
#include <iostream>

using namespace winrt;
using winrt::Windows::Networking::NetworkOperators::NetworkOperatorTetheringManager;
using winrt::Windows::Networking::NetworkOperators::TetheringWiFiBand;
using winrt::Windows::Networking::NetworkOperators::NetworkOperatorTetheringAccessPointConfiguration;
using winrt::Windows::Networking::Connectivity::ConnectionProfile;
using winrt::Windows::Networking::Connectivity::NetworkInformation;
using winrt::Windows::Networking::NetworkOperators::TetheringCapability;
using winrt::Windows::Networking::NetworkOperators::TetheringOperationalState;
using winrt::Windows::Networking::NetworkOperators::NetworkOperatorTetheringOperationResult;
using winrt::Windows::Networking::NetworkOperators::TetheringOperationStatus;
using namespace Windows::Networking::Connectivity;

NetworkOperatorTetheringManager TryGetCurrentNetworkOperatorTetheringManager();
hstring GetFriendlyName(TetheringWiFiBand value);
bool IsBandSupported(NetworkOperatorTetheringAccessPointConfiguration const& configuration,
    TetheringWiFiBand band);
TetheringWiFiBand GetHighestSupportedBand(NetworkOperatorTetheringAccessPointConfiguration const& configuration);

fire_and_forget GetMobileHotspot(std::wstring ssid = L"Xiaomi 13 Pro_DuQ54Tj_MI",std::wstring passphrase= L"fWSfffJfXf") {
    NetworkOperatorTetheringManager tetheringManager = TryGetCurrentNetworkOperatorTetheringManager();
    TetheringOperationalState state = TetheringOperationalState::Unknown;
    do
    {
        state = tetheringManager.TetheringOperationalState();
        std::wcout << "Mobile Hotspot is in transition" << std::endl;
        Sleep(3000);
    } while (state == TetheringOperationalState::InTransition);
   
    if (state == TetheringOperationalState::Off) {
        NetworkOperatorTetheringOperationResult result{ nullptr };
        auto ioAsync = tetheringManager.StartTetheringAsync();
        result = ioAsync.get();
        if (result.Status() == TetheringOperationStatus::Success) {
            NetworkOperatorTetheringAccessPointConfiguration configuration = nullptr;
            if (tetheringManager != nullptr) {
                configuration = tetheringManager.GetCurrentAccessPointConfiguration();
                configuration.Ssid(ssid.c_str());
                configuration.Passphrase(passphrase.c_str());
                TetheringWiFiBand band = GetHighestSupportedBand(configuration);
                configuration.Band(band);
                tetheringManager.ConfigureAccessPointAsync(configuration);
                std::wcout << "SSID: " << configuration.Ssid() << " Passphrase: " << configuration.Passphrase() << std::endl;
            }
        }
    }
    else if (state == TetheringOperationalState::On) {
        NetworkOperatorTetheringAccessPointConfiguration configuration = nullptr;
        if (tetheringManager != nullptr) {
            configuration = tetheringManager.GetCurrentAccessPointConfiguration();
            configuration.Ssid(ssid.c_str());
            configuration.Passphrase(passphrase.c_str());
            TetheringWiFiBand band = GetHighestSupportedBand(configuration);
            configuration.Band(band);
            tetheringManager.ConfigureAccessPointAsync(configuration);
            std::wcout << "SSID: " << configuration.Ssid() << " Passphrase: " << configuration.Passphrase() << std::endl;
        }
    }
    else {
		std::wcout << "Mobile Hotspot is not available" << std::endl;
	}

    do
    {
        state = tetheringManager.TetheringOperationalState();
        std::wcout << "Mobile Hotspot is in transition" << std::endl;
        Sleep(3000);
    } while (state == TetheringOperationalState::InTransition);

    std::wcout << "Mobile Hotspot is " << (state == TetheringOperationalState::On ? "On" : "Off") << std::endl;
    co_return;
}

void CloseTethering() {
    NetworkOperatorTetheringManager tetheringManager = TryGetCurrentNetworkOperatorTetheringManager();
    TetheringOperationalState state = TetheringOperationalState::Unknown;
    do
    {
        state = tetheringManager.TetheringOperationalState();
        std::wcout << "Mobile Hotspot is in transition" << std::endl;
        Sleep(3000);
    } while (state == TetheringOperationalState::InTransition);

    if (state == TetheringOperationalState::On) {
        tetheringManager.StopTetheringAsync();

        do
        {
            state = tetheringManager.TetheringOperationalState();
            std::wcout << "Mobile Hotspot is in transition" << std::endl;
            Sleep(3000);
        } while (state == TetheringOperationalState::InTransition);
    }
    std::wcout << "Mobile Hotspot is off" << std::endl;
}

int wmain(int argc,wchar_t *argv[]) {
    if (argc == 2) {
        std::wstring cmd = argv[1];
        if (cmd == L"off")
            CloseTethering();
        return 0;
    }
    if(argc != 3)
        GetMobileHotspot();
    else {
        std::wstring ssid = argv[1];
        std::wstring passpharse = argv[2];
        GetMobileHotspot(ssid, passpharse);
    }
    return 0;
}

NetworkOperatorTetheringManager TryGetCurrentNetworkOperatorTetheringManager() {
    // Get the connection profile associated with the internet connection currently used by the local machine.
    ConnectionProfile currentConnectionProfile = NetworkInformation::GetInternetConnectionProfile();
    if (currentConnectionProfile == nullptr) {
		return nullptr;
	}
    
    TetheringCapability tetheringCapability 
        = NetworkOperatorTetheringManager::GetTetheringCapabilityFromConnectionProfile(currentConnectionProfile);
    if (tetheringCapability != TetheringCapability::Enabled) {
        hstring message;
        switch (tetheringCapability)
        {
        case TetheringCapability::DisabledByGroupPolicy:
            message = L"Tethering is disabled due to group policy.";
            break;
        case TetheringCapability::DisabledByHardwareLimitation:
            message = L"Tethering is not available due to hardware limitations.";
            break;
        case TetheringCapability::DisabledByOperator:
            message = L"Tethering operations are disabled for this account by the network operator.";
            break;
        case TetheringCapability::DisabledByRequiredAppNotInstalled:
            message = L"An application required for tethering operations is not available.";
            break;
        case TetheringCapability::DisabledBySku:
            message = L"Tethering is not supported by the current account services.";
            break;
        case TetheringCapability::DisabledBySystemCapability:
            // This will occur if the "wiFiControl" capability is missing from the App.
            message = L"This app is not configured to access Wi-Fi devices on this machine.";
            break;
        default:
            message = L"Tethering is disabled on this machine. (Code " + to_hstring(static_cast<int32_t>(tetheringCapability)) + L").";
            break;
        }
        return nullptr;
    }

    try
    {
        return NetworkOperatorTetheringManager::CreateFromConnectionProfile(currentConnectionProfile);
    }
    catch (hresult_error const& ex) {
        if (ex.code() == HRESULT_FROM_WIN32(ERROR_NOT_FOUND)) {
            return nullptr;
        }
        throw;
	}
}

hstring GetFriendlyName(TetheringWiFiBand value)
{
    switch (value)
    {
    case TetheringWiFiBand::Auto: return L"Any available";
    case TetheringWiFiBand::TwoPointFourGigahertz: return L"2.4 GHz";
    case TetheringWiFiBand::FiveGigahertz: return L"5 Ghz";
    default: return L"Unknown (" + to_hstring(static_cast<uint32_t>(value)) + L")";
    }
}

bool IsBandSupported(NetworkOperatorTetheringAccessPointConfiguration const& configuration, TetheringWiFiBand band)
{
    // "Auto" mode is always supported even though it is technically not a frequency band.
    if (band == TetheringWiFiBand::Auto)
    {
        return true;
    }

    try
    {
        return configuration.IsBandSupported(band);
    }
    catch (hresult_error const& ex)
    {
        if (ex.code() == HRESULT_FROM_WIN32(ERROR_INVALID_STATE))
        {
            // The WiFi device has been disconnected. Report that we support nothing.
            return false;
        }
        throw;
    }
}

TetheringWiFiBand GetHighestSupportedBand(NetworkOperatorTetheringAccessPointConfiguration const& configuration)
{
    TetheringWiFiBand highestSupportedBand = TetheringWiFiBand::Auto;

    // List of possible bands in ascending order
    std::vector<TetheringWiFiBand> bands = {
        TetheringWiFiBand::Auto,
        TetheringWiFiBand::TwoPointFourGigahertz,
        TetheringWiFiBand::FiveGigahertz
    };

    for (auto band : bands)
    {
        if (IsBandSupported(configuration, band))
        {
            highestSupportedBand = band;
        }
    }

    return highestSupportedBand;
}
