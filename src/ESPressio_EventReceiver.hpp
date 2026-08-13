#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <typeinfo>
#include <typeindex>

#include "ESPressio_EventEnums.hpp"
#include "ESPressio_IEvent.hpp"


namespace ESPressio {

    namespace Event {

        class IEventReceiver {
            public:
                virtual void QueueEvent(IEvent* event, EventPriority priority = EventPriority::Normal) = 0;
                virtual void StackEvent(IEvent* event, EventPriority priority = EventPriority::Normal) = 0;
        };

        class EventReceiver : public IEventReceiver {
        private:
        // Members

            // We use a "Revolving Door" system in this Event Engine... when the main Queue or Stack is locked, the Alt is not locked... and vice versa!
            // This prevents dead-locking where an Event Handler (Listener) is trying to Queue or Stack an Event while the Event Engine is processing the Queue or Stack...
            std::mutex _mutexQueues;
            std::mutex _mutexQueuesAlt;
            std::mutex _mutexStacks;
            std::mutex _mutexStacksAlt;

            typedef std::vector<IEvent*> EventDispatchCollection;
            typedef std::unordered_map<EventPriority, EventDispatchCollection*> EventCollection;

            EventCollection _priorityQueues;
            EventCollection _priorityQueuesAlt; // This Queue will be used when the primary Queue is being processed!
            EventCollection _priorityStacks;
            EventCollection _priorityStacksAlt; // This Stack will be used when the primary Stack is being processed!

            void ClearEventCollection(EventCollection& eventCollection) {
                for (auto& entry : eventCollection) {
                    EventDispatchCollection* collection = entry.second;
                    if (collection == nullptr) {
                        continue;
                    }
                    for (IEvent* event : *collection) {
                        event->__unref();
                    }
                    delete collection;
                }
                eventCollection.clear();
            }
        
        // Methods

            EventDispatchCollection* GetPriorityQueue(EventPriority priority) {
                EventDispatchCollection* queue = _priorityQueues[priority];
                
                if (queue == nullptr) { // If the queue does not exist, let's create it
                    queue = new EventDispatchCollection();
                    _priorityQueues[priority] = queue;
                }

                return queue;
            }

            EventDispatchCollection* GetPriorityQueueAlt(EventPriority priority) {
                EventDispatchCollection* queue = _priorityQueuesAlt[priority];
                
                if (queue == nullptr) { // If the queue does not exist, let's create it
                    queue = new EventDispatchCollection();
                    _priorityQueuesAlt[priority] = queue;
                }

                return queue;
            }

            EventDispatchCollection* GetPriorityStack(EventPriority priority) {
                EventDispatchCollection* stack = _priorityStacks[priority];
                
                if (stack == nullptr) { // If the stack does not exist, let's create it
                    stack = new EventDispatchCollection();
                    _priorityStacks[priority] = stack;
                }

                return stack;
            }

            EventDispatchCollection* GetPriorityStackAlt(EventPriority priority) {
                EventDispatchCollection* stack = _priorityStacksAlt[priority];
                
                if (stack == nullptr) { // If the stack does not exist, let's create it
                    stack = new EventDispatchCollection();
                    _priorityStacksAlt[priority] = stack;
                }

                return stack;
            }

            inline void WithEventCollection(
                EventCollection& eventCollection,
                std::function<void(
                    IEvent*,
                    EventDispatchMethod,
                    EventPriority)> callback,
                EventDispatchMethod iterationOrder
            ) {
                // Iterate EventPriority from highest value to lowest value.
                for (int priorityID = static_cast<uint8_t>(EventPriority::High); priorityID >= 0; priorityID--) {
                    EventPriority priority = static_cast<EventPriority>(priorityID);
                    const auto collectionEntry = eventCollection.find(priority);
                    if (collectionEntry == eventCollection.end() ||
                        collectionEntry->second == nullptr) {
                        continue;
                    }

                    EventDispatchCollection* collection =
                        collectionEntry->second;
                    EventDispatchCollection pendingEvents;
                    pendingEvents.swap(*collection);

                    class PendingEventReferences final {
                        private:
                            EventDispatchCollection& _events;

                        public:
                            explicit PendingEventReferences(
                                EventDispatchCollection& events
                            ) : _events(events) { }

                            ~PendingEventReferences() {
                                for (IEvent* event : _events) {
                                    if (event != nullptr) {
                                        event->__unref();
                                    }
                                }
                            }

                            void Release(size_t index) {
                                _events[index]->__unref();
                                _events[index] = nullptr;
                            }
                    } pendingEventReferences(pendingEvents);

                    if (iterationOrder == EventDispatchMethod::Stack) {
                        for (size_t index = pendingEvents.size();
                            index > 0; --index) {
                            const size_t eventIndex = index - 1;
                            callback(
                                pendingEvents[eventIndex],
                                EventDispatchMethod::Stack,
                                priority
                            );
                            pendingEventReferences.Release(eventIndex);
                        }
                    } else {
                        for (size_t index = 0;
                            index < pendingEvents.size(); ++index) {
                            callback(
                                pendingEvents[index],
                                EventDispatchMethod::Queue,
                                priority
                            );
                            pendingEventReferences.Release(index);
                        }
                    }

                    // Retain the allocation for the next drain without
                    // retaining any Event pointers.
                    pendingEvents.clear();
                    pendingEvents.swap(*collection);
                }
            }
        protected:
        
        // Methods

            /// `WithEvents` is a method that iterates all of the Events in the Stacks and Queues (in the correct order) and calls your given Callback Method with the Event and its Dispatch Time as parameters.
            void WithEvents(
                std::function<void(
                    IEvent*,
                    EventDispatchMethod,
                    EventPriority)> callback
            ) {
                // We process the Stacks first in Priority Order (Highest to Lowest)
                {
                    std::lock_guard<std::mutex> lock(_mutexStacks);
                    WithEventCollection(
                        _priorityStacks, callback,
                        EventDispatchMethod::Stack
                    );
                }
                {
                    std::lock_guard<std::mutex> lock(_mutexStacksAlt);
                    WithEventCollection(
                        _priorityStacksAlt, callback,
                        EventDispatchMethod::Stack
                    );
                }
                {
                    std::lock_guard<std::mutex> lock(_mutexQueues);
                    WithEventCollection(
                        _priorityQueues, callback,
                        EventDispatchMethod::Queue
                    );
                }
                {
                    std::lock_guard<std::mutex> lock(_mutexQueuesAlt);
                    WithEventCollection(
                        _priorityQueuesAlt, callback,
                        EventDispatchMethod::Queue
                    );
                }
            }

            virtual void EventAdded() {};

        public:

            virtual ~EventReceiver() {
                ClearEventCollection(_priorityQueues);
                ClearEventCollection(_priorityQueuesAlt);
                ClearEventCollection(_priorityStacks);
                ClearEventCollection(_priorityStacksAlt);
            }
        
        // Methods

            void QueueEvent(IEvent* event, EventPriority priority = EventPriority::Normal) {
                event->__dispatch();
                event->__ref();
                try {
                    std::unique_lock<std::mutex> queueLock(
                        _mutexQueues, std::try_to_lock
                    );
                    if (queueLock.owns_lock()) {
                        GetPriorityQueue(priority)->push_back(event);
                    } else {
                        std::lock_guard<std::mutex> alternateQueueLock(
                            _mutexQueuesAlt
                        );
                        GetPriorityQueueAlt(priority)->push_back(event);
                    }
                } catch (...) {
                    event->__unref();
                    throw;
                }
                EventAdded();
            }

            void StackEvent(IEvent* event, EventPriority priority = EventPriority::Normal) {
                event->__dispatch();
                event->__ref();
                try {
                    std::unique_lock<std::mutex> stackLock(
                        _mutexStacks, std::try_to_lock
                    );
                    if (stackLock.owns_lock()) {
                        GetPriorityStack(priority)->push_back(event);
                    } else {
                        std::lock_guard<std::mutex> alternateStackLock(
                            _mutexStacksAlt
                        );
                        GetPriorityStackAlt(priority)->push_back(event);
                    }
                } catch (...) {
                    event->__unref();
                    throw;
                }
                EventAdded();
            }
        };

    }

}
