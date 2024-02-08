#pragma once

#include <vector>
#include <unordered_map>
#include <mutex>

#include "ESPressio_IEvent.hpp"
#include "ESPressio_EventReceiver.hpp"

namespace ESPressio {

    namespace Event {

        /*
            `IEventDispatcher` is an interface that defines the methods that an event dispatcher should implement.
            Event Dispatchers act as both an `IEventReceiver`, and an Event-Typed Collection of other `IEventReceiver` objects.
            Their job is to facilitate the transit of Events from one `IEventReceiver` to another.
            This is essential for the Event Engine to function.
        */
        class IEventDispatcher {
            public:
                virtual ~IEventDispatcher() { }
                /// Registers an `IEventReceiver` to receive Events of a specific type from this `IEventDispatcher`.
                virtual void RegisterReceiver(std::type_index type, IEventReceiver* receiver) = 0;
                /// Unregisters an `IEventReceiver` from receiving Events of a specific type from this `IEventDispatcher`.
                virtual void UnregisterReceiver(std::type_index type, IEventReceiver* receiver) = 0;
        };

        /// `EventDispatcher` is a concrete implementation of the `IEventDispatcher` interface.
        class EventDispatcher : public EventReceiver, public IEventDispatcher {
            private:
                typedef std::vector<IEventReceiver*> EventReceiverBucket;
                typedef std::unordered_map<std::type_index, EventReceiverBucket*> EventReceiverTypeMap;

                EventReceiverTypeMap _eventReceivers;
                std::mutex _eventReceiversMutex; // It's necessary because we're using a shared resource (the `_eventReceivers` map) across multiple threads.

                /// BEWARE: This method doesn't lock the `_eventReceiversMutex`! It's the caller's responsibility to lock it!
                EventReceiverBucket* GetEventTypeBucket(std::type_index type) {
                    if (_eventReceivers.find(type) == _eventReceivers.end()) {
                        _eventReceivers[type] = new EventReceiverBucket();
                    }
                    return _eventReceivers[type];
                }
            protected:
                void ClearEventReceivers() {
                    _eventReceiversMutex.lock();
                    for (auto it = _eventReceivers.begin(); it != _eventReceivers.end(); it++) {
                        delete it->second;
                    }
                    _eventReceivers.clear();
                    _eventReceiversMutex.unlock();
                }

                void DispatchEvents() {
                    _eventReceiversMutex.lock();

                    WithEvents([&](IEvent* event, EventDispatchMethod dispatchMethod, EventPriority priority) {
                        std::type_index type = typeid(*event);
                        EventReceiverBucket* bucket = GetEventTypeBucket(type);
                        bool wasHandled = false;
                        for (IEventReceiver* receiver : *bucket) {
                            if (dispatchMethod == EventDispatchMethod::Queue) {
                                receiver->QueueEvent(event, priority);
                            } else {
                                receiver->StackEvent(event, priority);
                            }
                            event->__unref();
                            wasHandled = true;
                        }
                        if (!wasHandled) { event->__unref(); }
                    });

                    _eventReceiversMutex.unlock();
                }
            public:
                EventDispatcher() { }

                virtual ~EventDispatcher() override {
                    ClearEventReceivers();
                }

                void RegisterReceiver(std::type_index type, IEventReceiver* receiver) override {
                    _eventReceiversMutex.lock();
                    EventReceiverBucket* bucket = GetEventTypeBucket(type);
                    for (IEventReceiver* r : *bucket) {
                        if (r == receiver) {
                            _eventReceiversMutex.unlock();
                            return;
                        }
                    }
                    bucket->push_back(receiver);
                    _eventReceiversMutex.unlock();
                }

                void UnregisterReceiver(std::type_index type, IEventReceiver* receiver) override {
                    _eventReceiversMutex.lock();
                    EventReceiverBucket* bucket = GetEventTypeBucket(type);
                    for (auto it = bucket->begin(); it != bucket->end(); it++) {
                        if (*it == receiver) {
                            bucket->erase(it);
                            _eventReceiversMutex.unlock();
                            return;
                        }
                    }
                    _eventReceiversMutex.unlock();
                }
        };

    }

}