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

fire_and_forget GetMobileHotspot() {
    NetworkOperatorTetheringManager tetheringManager = TryGetCurrentNetworkOperatorTetheringManager();

    bool isTethered = tetheringManager.TetheringOperationalState() == TetheringOperationalState::On;
    NetworkOperatorTetheringOperationResult result{ nullptr };
    if (!isTethered) {
        result = co_await tetheringManager.StartTetheringAsync();
        if (result.Status() == TetheringOperationStatus::Success) {
            NetworkOperatorTetheringAccessPointConfiguration configuration = nullptr;
            if (tetheringManager != nullptr) {
                configuration = tetheringManager.GetCurrentAccessPointConfiguration();
                std::wcout << "SSID: " << configuration.Ssid() << " Passphrase: " << configuration.Passphrase() << std::endl;
            }
            co_return;
        }
    }

    NetworkOperatorTetheringAccessPointConfiguration configuration = nullptr;
    if (tetheringManager != nullptr) {
        configuration = tetheringManager.GetCurrentAccessPointConfiguration();
        std::wcout << "SSID: " << configuration.Ssid() << " Passphrase: " << configuration.Passphrase() << std::endl;
    }

    auto hostNames = NetworkInformation::GetHostNames();
    for (auto const& hostName : hostNames) {
        if (hostName.IPInformation() != nullptr) {
            std::wcout << "IP Address: " << hostName.CanonicalName() << std::endl;
            std::wcout<<"Display Name: "<<hostName.DisplayName().c_str()<<std::endl;
        }
        
    }
}

int main(){
    GetMobileHotspot();
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
