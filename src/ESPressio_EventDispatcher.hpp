#pragma once

#include <algorithm>
#include <mutex>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "ESPressio_IEvent.hpp"
#include "ESPressio_EventReceiver.hpp"

namespace ESPressio {

    namespace Event {

        class IEventDispatcher {
            public:
                virtual ~IEventDispatcher() =
                    default;

                virtual void RegisterReceiver(
                    std::type_index type,
                    IEventReceiver* receiver
                ) = 0;

                virtual void UnregisterReceiver(
                    std::type_index type,
                    IEventReceiver* receiver
                ) = 0;
        };


        class EventDispatcher :
            public EventReceiver,
            public IEventDispatcher {

            private:
                using EventReceiverBucket =
                    std::vector<
                        IEventReceiver*
                    >;

                using EventReceiverTypeMap =
                    std::unordered_map<
                        std::type_index,
                        EventReceiverBucket
                    >;


                EventReceiverTypeMap
                    _eventReceivers;

                mutable std::mutex
                    _eventReceiversMutex;


                EventReceiverBucket
                GetEventTypeBucketSnapshot(
                    std::type_index type
                ) const {
                    std::lock_guard<
                        std::mutex
                    > lock(
                        _eventReceiversMutex
                    );

                    const auto found =
                        _eventReceivers.find(
                            type
                        );

                    return
                        found ==
                            _eventReceivers.end()
                            ? EventReceiverBucket()
                            : found->second;
                }


            protected:
                void ClearEventReceivers() {
                    std::lock_guard<
                        std::mutex
                    > lock(
                        _eventReceiversMutex
                    );

                    _eventReceivers.clear();
                }


                void DispatchEvents() {
                    WithEvents(
                        [&](
                            IEvent* event,
                            EventDispatchMethod
                                dispatchMethod,
                            EventPriority priority
                        ) {
                            const auto receivers =
                                GetEventTypeBucketSnapshot(
                                    std::type_index(
                                        typeid(*event)
                                    )
                                );

                            for (
                                IEventReceiver*
                                    receiver :
                                receivers
                            ) {
                                if (
                                    receiver ==
                                    nullptr
                                ) {
                                    continue;
                                }

                                if (
                                    dispatchMethod ==
                                    EventDispatchMethod::
                                        Queue
                                ) {
                                    receiver->
                                        QueueEvent(
                                            event,
                                            priority
                                        );
                                } else {
                                    receiver->
                                        StackEvent(
                                            event,
                                            priority
                                        );
                                }
                            }
                        }
                    );
                }


            public:
                EventDispatcher() =
                    default;


                ~EventDispatcher()
                    override {
                    ClearEventReceivers();
                }


                void RegisterReceiver(
                    std::type_index type,
                    IEventReceiver* receiver
                ) override {
                    if (receiver == nullptr) {
                        return;
                    }

                    std::lock_guard<
                        std::mutex
                    > lock(
                        _eventReceiversMutex
                    );

                    auto& bucket =
                        _eventReceivers[
                            type
                        ];

                    if (
                        std::find(
                            bucket.begin(),
                            bucket.end(),
                            receiver
                        ) ==
                        bucket.end()
                    ) {
                        bucket.push_back(
                            receiver
                        );
                    }
                }


                void UnregisterReceiver(
                    std::type_index type,
                    IEventReceiver* receiver
                ) override {
                    std::lock_guard<
                        std::mutex
                    > lock(
                        _eventReceiversMutex
                    );

                    const auto found =
                        _eventReceivers.find(
                            type
                        );

                    if (
                        found ==
                        _eventReceivers.end()
                    ) {
                        return;
                    }

                    auto& bucket =
                        found->second;

                    bucket.erase(
                        std::remove(
                            bucket.begin(),
                            bucket.end(),
                            receiver
                        ),
                        bucket.end()
                    );

                    if (bucket.empty()) {
                        _eventReceivers.erase(
                            found
                        );
                    }
                }
        };

    }

}
