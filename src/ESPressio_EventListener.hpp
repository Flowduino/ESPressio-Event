#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <functional>
#include <shared_mutex>

#include <ESPressio_ThreadSafe.hpp>

#include "ESPressio_IEvent.hpp"
#include "ESPressio_EventEnums.hpp"


namespace ESPressio {

    namespace Event {

        class IEventListenerHandler {
            public:
                virtual ~IEventListenerHandler() { };
                virtual void Unregister() = 0;
                virtual bool IsRegistered() const = 0;
        };

        /*
            `IEventListener` is an Interface which you can implement to listen for Events.
            You can register a Listener for a specific Event Type and when that Event is Dispatched, your Listener will be called.
        */
        class IEventListener {
            public:
                virtual ~IEventListener() { }

                virtual IEventListenerHandler* RegisterListener(
                    std::type_index eventType,
                    std::function<void(
                        IEvent*,
                        EventDispatchMethod dispatchMethod,
                        EventPriority priority
                    )> callback,
                    EventListenerInterest interest = EventListenerInterest::All,
                    unsigned long maximumTimeSinceDispatch = 0,
                    std::function<bool(IEvent*)> customInterestCallback = nullptr
                ) = 0;

                template <typename EventType>
                IEventListenerHandler* RegisterListener(
                    std::function<void(
                        IEvent*,
                        EventDispatchMethod dispatchMethod,
                        EventPriority priority)> callback,
                        EventListenerInterest interest = EventListenerInterest::All,
                        unsigned long maximumTimeSinceDispatch = 0,
                        std::function<bool(IEvent*)> customInterestCallback = nullptr
                );

                virtual void UnregisterListener(std::type_index eventType, IEventListenerHandler* handler) = 0;

                template <typename EventType>
                void UnregisterListener(IEventListenerHandler* handler);
        };

        /*
            `EventListenerHandler` is returned when invoking `RegisterListener` against any implementor of `IEventListener`.
            This class is used to manage the lifetime of the Listener and to unregister the Listener when it is no longer needed.
            You should retain your reference to this Handler and call `Unregister` against it when you are done with the Listener (and on your objects' Destructor if applicable).
            DON'T FORGET: YOUR code takes ownership of the EventListenerHandler, and you must destroy (`delete`) it when you are done with it.
        */
        class EventListenerHandler : public IEventListenerHandler {
            private:
                ReadWriteMutex<bool>* _isRegistered = new ReadWriteMutex(true); // _isRegistered can be altered by multiple threads, so we need to protect it with a Mutex
                IEventListener* _listener; // This is a Weak Reference to the Listener (it will be nullified automatically when the Event Listener is destroyed)
                std::type_index _eventType; // This is the Event Type (Hash) which we can use to quickly look up the Listeners for this Event Type
            public:
            // Constructor/Deconstructor

                EventListenerHandler(std::type_index eventType, IEventListener* listener) : _eventType(eventType), _listener(listener) { }

                template <typename EventType>
                EventListenerHandler(IEventListener* listener) : _eventType(typeid(EventType)), _listener(listener) { }
                
                ~EventListenerHandler() override {
                    Unregister();
                    delete _isRegistered;
                }

            // Methods
                // Will safely `Unregister` the Listener
                void Unregister() {
                    if (!_isRegistered->Get() || _listener == nullptr) { return; } // If the Listener is already Unregistered, or the Listener is no longer alive, then we don't need to do anything
                    _listener->UnregisterListener(_eventType, this);
                    _isRegistered->Set(false);
                }

            // Getters
                
                bool IsRegistered() const { return _isRegistered->Get(); }

                void ForceUnregister() {
                    _isRegistered->Set(false);
                    _listener = nullptr;
                }
        };

        class EventListener : public IEventListener {
            private:
            
                /// This is an ugly hack to work around a limitation in the C++ language
                /// Basically, Vectors cannot contain template types, so we need a common base class for the Vector instead.
                /// All templated methods are excluded from the Interface.
                class IEventListenerContainer {
                    public:
                        virtual ~IEventListenerContainer() { }
                        virtual IEventListenerHandler* GetListenerHandler() const = 0;
                        virtual IEventListener* GetRequester() const = 0;
                        virtual EventListenerInterest GetInterest() const = 0;
                        virtual unsigned long GetMaximumTimeSinceDispatch() const = 0;
                };

                /// `EventListenerContainer` is a class which holds all information about a specific Listener for a specific Event Type.
                template <typename EventType>
                class EventListenerContainer : public IEventListenerContainer {
                    private:
                        IEventListenerHandler* _listenerHandler;
                        IEventListener* _requester; // We will use this to determine if the requester is still alive
                        std::function<void(EventType*, EventDispatchMethod dispatchMethod, EventPriority priority)> _callback;
                        EventListenerInterest _interest = EventListenerInterest::All; // Default to All
                        unsigned long _maximumTimeSinceDispatch = 0; // Default to 0 because we only use this if the interest is YoungerThan
                        std::function<bool(EventType*)> _customInterestCallback = nullptr; // Default to nullptr because we only use this Callback if the interest is Custom
                    public:
                    // Constructor

                        EventListenerContainer(
                            IEventListener* requester,
                            std::function<void(
                                EventType* eventType,
                                EventDispatchMethod dispatchMethod,
                                EventPriority priority
                            )> callback,
                            IEventListenerHandler* listenerHandler,
                            EventListenerInterest interest = EventListenerInterest::All,
                            unsigned long maximumTimeSinceDispatch = 0,
                            std::function<bool(EventType*)> customInterestCallback = nullptr
                        ) : _requester(requester), _callback(callback), _listenerHandler(listenerHandler), _interest(interest), _maximumTimeSinceDispatch(maximumTimeSinceDispatch), _customInterestCallback(customInterestCallback) { }

                    // Destructor
                        
                        ~EventListenerContainer() override { 
                            // Do nothing, because the Container is owned by the Listener and it'll take responsibility for destroying it only when it's safe to do so!
                        }

                    // Getters

                        IEventListenerHandler* GetListenerHandler() const { return _listenerHandler; }
                        IEventListener* GetRequester() const { return _requester; }
                        inline std::function<void(EventType*, EventDispatchMethod dispatchMethod, EventPriority priority)> GetCallback() const { return _callback; }
                        EventListenerInterest GetInterest() const { return _interest; }
                        unsigned long GetMaximumTimeSinceDispatch() const { return _maximumTimeSinceDispatch; }
                        std::function<bool(EventType*)> GetCustomInterestCallback() const { return _customInterestCallback; }

                    // Setters

                        void SetRequester(IEventListener* requester) { _requester = requester; }
                        void SetCallback(std::function<void(EventType*, EventDispatchMethod dispatchMethod, EventPriority priority)> callback) { _callback = callback; }
                        void SetInterest(EventListenerInterest interest) { _interest = interest; }
                        void SetMaximumTimeSinceDispatch(unsigned long maximumTimeSinceDispatch) { _maximumTimeSinceDispatch = maximumTimeSinceDispatch; }
                        void SetCustomInterestCallback(std::function<bool(EventType*)> customInterestCallback) { _customInterestCallback = customInterestCallback; }
                };

                typedef std::vector<IEventListenerContainer*> EventListeners;
                typedef std::unordered_map<std::type_index, EventListeners*> EventListenersMap;

                /// This is a map of Event Types to a collection of Listeners for that Event Type
                EventListenersMap _eventListeners;
                std::shared_mutex _eventListenersMutex;

                /// THE CALLER MUST LOCK AND UNLOCK THE MUTEX!
                EventListeners* GetListenersForEventType(std::type_index eventType) {
                    EventListeners* typeListeners = _eventListeners[eventType]; // Get the Listeners collection for this Event Type
                    if (typeListeners == nullptr) { // If it doesn't exist...
                        typeListeners = new EventListeners(); // ...let's create it...
                        _eventListeners[eventType] = typeListeners; // ...and add it to the map
                    }
                    return typeListeners;
                }

            protected:

                inline virtual void OnListenerRegistered(std::type_index eventType) { }

                inline virtual void OnListenerUnregistered(std::type_index eventType) { }

            public:
                virtual ~EventListener() {
                    _eventListenersMutex.lock();
                    for (auto it = _eventListeners.begin(); it != _eventListeners.end(); it++) {
                        for (auto listener : *it->second) {
                            delete listener;
                        }
                        delete it->second;
                    }
                    _eventListeners.clear();
                    _eventListenersMutex.unlock();
                }

                IEventListenerHandler* RegisterListener(
                    std::type_index eventType,
                    std::function<void(
                        IEvent*,
                        EventDispatchMethod dispatchMethod,
                        EventPriority priority
                    )> callback,
                    EventListenerInterest interest = EventListenerInterest::All,
                    unsigned long maximumTimeSinceDispatch = 0,
                    std::function<bool(IEvent*)> customInterestCallback = nullptr

                ) override {
                    _eventListenersMutex.lock(); // Because we MIGHT be adding a new Listeners collection, we need to exclusively lock the Mutex
                    EventListeners* typeListeners = GetListenersForEventType(eventType); // Get the Listeners collection for this Event Type (will create the Listeners collection if it doesn't exist)

                    IEventListenerHandler* handler = new EventListenerHandler(eventType, this);

                    EventListenerContainer<IEvent>* listener = new EventListenerContainer<IEvent>(this, callback, handler, interest, maximumTimeSinceDispatch, customInterestCallback); // Create a new Listener (EventListenerContainer)

                    typeListeners->push_back(listener); // Add it into the collection

                    OnListenerRegistered(eventType);
                    _eventListenersMutex.unlock();
                    return handler;
                }

                template <typename EventType>
                IEventListenerHandler* RegisterListener(
                    std::function<void(
                        EventType*,
                        EventDispatchMethod dispatchMethod,
                        EventPriority priority)> callback,
                        EventListenerInterest interest = EventListenerInterest::All,
                        unsigned long maximumTimeSinceDispatch = 0,
                        std::function<bool(EventType*)> customInterestCallback = nullptr

                ) {
                    _eventListenersMutex.lock(); // Because we MIGHT be adding a new Listeners collection, we need to exclusively lock the Mutex
                    std::type_index eventType = typeid(EventType);
                    EventListeners* typeListeners = GetListenersForEventType(eventType); // Get the Listeners collection for this Event Type (will create the Listeners collection if it doesn't exist)
                    IEventListenerHandler* handler = new EventListenerHandler(eventType, this);
    
                    EventListenerContainer<EventType>* listener = new EventListenerContainer<EventType>(this, callback, handler, interest, maximumTimeSinceDispatch, customInterestCallback); // Create a new Listener (EventListenerContainer)

                    typeListeners->push_back(listener); // Add it into the collection

                    OnListenerRegistered(eventType);

                    _eventListenersMutex.unlock();
                    return handler;
                }

                void UnregisterListener(std::type_index eventType, IEventListenerHandler* handler) {
                    _eventListenersMutex.lock(); // Because we MIGHT be removing the Listeners collection, we need to exclusively lock the Mutex
                    EventListeners* typeListeners = _eventListeners[eventType]; // Get the Listeners collection for this Event Type
                    if (typeListeners == nullptr) {
                        _eventListenersMutex.unlock();
                        return;
                    }
                    for (auto it = typeListeners->begin(); it != typeListeners->end(); it++) {
                        if ((*it)->GetListenerHandler() == handler) {
                            static_cast<EventListenerHandler*>(handler)->ForceUnregister(); // Forcibly Unregister the Handle
                            delete *it; // Delete the Listener
                            typeListeners->erase(it);
                            break;
                        }
                    }
                    if (typeListeners->size() == 0) {
                        _eventListeners.erase(eventType);
                        
                    }
                    _eventListenersMutex.unlock();
                    OnListenerUnregistered(eventType);
                }

                template <typename EventType>
                void UnregisterListener(IEventListenerHandler* handler) {
                    std::type_index eventType = typeid(EventType);
                    UnregisterListener(eventType, handler);
                }

                inline void ProcessEvent(IEvent* event, EventDispatchMethod dispatchMethod, EventPriority priority) {
                    _eventListenersMutex.lock_shared(); // Because we are only reading from the Listeners collection, we can use a Shared Lock
                    EventListeners* typeListeners = _eventListeners[typeid(*event)]; // Get the Listeners collection for this Event Type

                    if (typeListeners == nullptr) {
                        event->__unref();
                        _eventListenersMutex.unlock_shared();
                        return;
                    }

                    for (auto ilistener : *typeListeners) {
                        EventListenerContainer<IEvent>* listener = static_cast<EventListenerContainer<IEvent>*>(ilistener);
                        if (listener->GetInterest() == EventListenerInterest::All) { // If the interest is All...
                            listener->GetCallback()(event, dispatchMethod, priority); // ...call the Listener's Callback
                        }
                        else if (listener->GetInterest() == EventListenerInterest::YoungerThan) { // If the interest is YoungerThan...
                            if (event->GetTimeSinceDispatch() < listener->GetMaximumTimeSinceDispatch()) { // ...and the time since dispatch is less than the maximum time since dispatch...
                                listener->GetCallback()(event, dispatchMethod, priority); // ...call the Listener's Callback
                            }
                        }
                        else if (listener->GetInterest() == EventListenerInterest::Custom) { // If the interest is Custom...
                            bool interested = listener->GetCustomInterestCallback()(event); // ...call the Listener's Custom Interest Callback
                            if (interested) { // If the Listener is interested...
                                listener->GetCallback()(event, dispatchMethod, priority); // ...call the Listener's Callback
                            }
                        }
                    }

                    event->__unref();
                    _eventListenersMutex.unlock_shared();
                }
        };

    }

}