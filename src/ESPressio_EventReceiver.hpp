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
                EventDispatchCollection* stack = _priorityQueues[priority];
                
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
                // Iterate the `EventPriority` from highest value to lowest value...
                for (int priorityID = static_cast<uint8_t>(EventPriority::High); priorityID >= 0; priorityID--) {
                    EventPriority priority = static_cast<EventPriority>(priorityID);
                    EventDispatchCollection* collection = eventCollection[priority]; // Check that a Vector of `IEvent*` exists for the current `EventPriority`...
                    if (collection != nullptr) {

                        if (iterationOrder == EventDispatchMethod::Stack) {
                            for (auto it = collection->rbegin(); it != collection->rend(); ++it) { // Iterate the `IEvent*` in the Vector from last to first...
                                (*it)->__ref();
                                callback((*it), EventDispatchMethod::Stack, priority); // Call the Callback Method with the `Event` and its Dispatch Time as parameters...
                                (*it)->__unref();
                            }
                        }
                        else {
                            for (auto it = collection->begin(); it != collection->end(); ++it) { // Iterate the `IEvent*` in the Vector from first to last...
                                (*it)->__ref();
                                callback((*it), EventDispatchMethod::Queue, priority); // Call the Callback Method with the `Event` and its Dispatch Time as parameters...
                                (*it)->__unref();
                            }
                        }
                        collection->clear(); // Clear the Vector
                        // Print the number of items in the collection to the Serial console
                    }
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
                _mutexStacks.lock();
                WithEventCollection(_priorityStacks, callback, EventDispatchMethod::Stack);
                _mutexStacks.unlock();

                _mutexStacksAlt.lock();
                WithEventCollection(_priorityStacksAlt, callback, EventDispatchMethod::Stack);
                _mutexStacksAlt.unlock();

                _mutexQueues.lock();
                WithEventCollection(_priorityQueues, callback, EventDispatchMethod::Queue);
                _mutexQueues.unlock();

                _mutexQueuesAlt.lock();
                WithEventCollection(_priorityQueuesAlt, callback, EventDispatchMethod::Queue);
                _mutexQueuesAlt.unlock();
            }

            virtual void EventAdded() {};

        public:

            virtual ~EventReceiver() {
                for (auto it = _priorityQueues.begin(); it != _priorityQueues.end(); ++it) {
                    delete it->second;
                }
                for (auto it = _priorityQueuesAlt.begin(); it != _priorityQueuesAlt.end(); ++it) {
                    delete it->second;
                }
                for (auto it = _priorityStacks.begin(); it != _priorityStacks.end(); ++it) {
                    delete it->second;
                }
                for (auto it = _priorityStacksAlt.begin(); it != _priorityStacksAlt.end(); ++it) {
                    delete it->second;
                }
            }
        
        // Methods

            void QueueEvent(IEvent* event, EventPriority priority = EventPriority::Normal) {
                event->__dispatch();
                event->__ref();
                if (_mutexQueues.try_lock()) {
                    EventDispatchCollection* queue = GetPriorityQueue(priority);
                    queue->push_back(event);
                    _mutexQueues.unlock();
                }
                else {
                    _mutexQueuesAlt.lock();
                    EventDispatchCollection* queue = GetPriorityQueueAlt(priority);
                    queue->push_back(event);
                    _mutexQueuesAlt.unlock();
                }
                EventAdded();
            }

            void StackEvent(IEvent* event, EventPriority priority = EventPriority::Normal) {
                event->__dispatch();
                event->__ref();
                if (_mutexStacks.try_lock()) {
                    EventDispatchCollection* stack = GetPriorityStack(priority);
                    stack->push_back(event);
                    _mutexStacks.unlock();
                }
                else {
                    std::lock_guard<std::mutex> lock(_mutexStacksAlt);
                    EventDispatchCollection* stack = GetPriorityStackAlt(priority);
                    stack->push_back(event);
                }
                EventAdded();
            }
        };

    }

}