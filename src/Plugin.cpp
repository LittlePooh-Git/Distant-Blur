#include "Logger.h"
#include "Manager.h"
#include "MCP.h"

namespace 
{
    void OnSKSEMessage(SKSE::MessagingInterface::Message* message)
    {
        switch (message->type) {
            case SKSE::MessagingInterface::kDataLoaded:
                Logger::trace("SKSE: DataLoaded event received from sender {}. Initializing Manager.", message->sender);
                Manager::Initialize();
                break;
            
            default:
                Logger::trace("SKSE: Message type {} received from sender {}.", message->type, message->sender);
                break;
        }
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    SetupLog();

    SKSE::Init(skse);
    Logger::trace("Plugin registered to SKSE");

    const auto messaging = SKSE::GetMessagingInterface();
    if (!messaging) {
        Logger::critical("Failed to acquire SKSE Messaging interface. Plugin cannot continue.");
        return false;
    }

    messaging->RegisterListener(OnSKSEMessage);

	MCP::Register();

    return true;
}
