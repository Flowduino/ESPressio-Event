#pragma once

#include <algorithm>
#include <memory>
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

                using EventReceiverBucketSnapshot =
                    std::shared_ptr<
                        const EventReceiverBucket
                    >;

                using EventReceiverTypeMap =
                    std::unordered_map<
                        std::type_index,
                        EventReceiverBucketSnapshot
                    >;


                EventReceiverTypeMap
                    _eventReceivers;

                mutable std::mutex
                    _eventReceiversMutex;


                EventReceiverBucketSnapshot
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
                            ? EventReceiverBucketSnapshot{}
                            : found->second;
                }


            protected:
                virtual void OnEventDispatched(
                    IEvent*,
                    EventDispatchMethod,
                    EventPriority
                ) {
                }


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
                            event->__dispatch();

                            OnEventDispatched(
                                event,
                                dispatchMethod,
                                priority
                            );

                            const auto receivers =
                                GetEventTypeBucketSnapshot(
                                    std::type_index(
                                        typeid(*event)
                                    )
                                );

                            if (!receivers) {
                                return;
                            }

                            for (
                                IEventReceiver*
                                    receiver :
                                *receivers
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

                    EventReceiverBucket bucket;

                    const auto found =
                        _eventReceivers.find(
                            type
                        );

                    if (
                        found !=
                            _eventReceivers.end() &&
                        found->second
                    ) {
                        bucket =
                            *found->second;
                    }

                    if (
                        std::find(
                            bucket.begin(),
                            bucket.end(),
                            receiver
                        ) !=
                        bucket.end()
                    ) {
                        return;
                    }

                    bucket.push_back(
                        receiver
                    );

                    _eventReceivers[
                        type
                    ] =
                        std::make_shared<
                            const EventReceiverBucket
                        >(
                            std::move(bucket)
                        );
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
                            _eventReceivers.end() ||
                        !found->second
                    ) {
                        return;
                    }

                    EventReceiverBucket bucket =
                        *found->second;

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
                    } else {
                        _eventReceivers[
                            type
                        ] =
                            std::make_shared<
                                const EventReceiverBucket
                            >(
                                std::move(bucket)
                            );
                    }
                }
        };

    }

}
