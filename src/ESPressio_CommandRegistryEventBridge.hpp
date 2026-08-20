#pragma once

#include <ESPressio_Command.hpp>
#include <ESPressio_ICommandRegistryObserver.hpp>

#include "ESPressio_CommandEvents.hpp"

namespace ESPressio::Event {

class CommandRegistryEventBridge final :
    public Command::ICommandRegistryObserver {
private:
    Observable::ObserverHandlePtr _observerHandle;
    bool _initialized = false;

    CommandRegistryEventBridge() = default;

public:
    CommandRegistryEventBridge(const CommandRegistryEventBridge&) = delete;
    CommandRegistryEventBridge& operator=(const CommandRegistryEventBridge&) = delete;

    static CommandRegistryEventBridge& GetInstance() {
        static CommandRegistryEventBridge instance;
        return instance;
    }

    bool Initialize(
        Command::CommandRegistry& registry = Command::CommandRegistry::GetInstance()
    ) {
        if (_initialized) return true;
        _observerHandle = registry.RegisterObserver(this);
        _initialized = static_cast<bool>(_observerHandle);
        return _initialized;
    }

    void Shutdown() {
        _observerHandle.reset();
        _initialized = false;
    }

    bool IsInitialized() const { return _initialized; }

    void OnCommandRegistered(const std::vector<std::string>& path) override {
        (new CommandRegisteredEvent(path))->Queue();
    }

    void OnCommandUnregistered(const std::vector<std::string>& path) override {
        (new CommandUnregisteredEvent(path))->Queue();
    }
};

} // namespace ESPressio::Event
