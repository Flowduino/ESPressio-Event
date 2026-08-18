#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

#include <ESPressio_IObservable.hpp>
#include <ESPressio_ThreadSafe.hpp>
#include <ESPressio_TimeTraits.hpp>

#include "ESPressio_IEvent.hpp"
#include "ESPressio_EventEnums.hpp"
#include "ESPressio_EventObserver.hpp"

namespace ESPressio {

    namespace Event {

        class IEventListenerHandle {
            public:
                virtual ~IEventListenerHandle() = default;

                virtual void Unregister() = 0;
                virtual bool IsRegistered() const = 0;
        };


        using EventListenerHandlePtr =
            std::unique_ptr<
                IEventListenerHandle
            >;


        class IEventListener {
            public:
                virtual ~IEventListener() = default;


                virtual EventListenerHandlePtr
                RegisterListener(
                    std::type_index eventType,
                    std::function<
                        void(
                            IEvent*,
                            EventDispatchMethod,
                            EventPriority
                        )
                    > callback,
                    EventListenerInterest interest =
                        EventListenerInterest::All,
                    EventTime maximumTimeSinceDispatch =
                        EventTime(0),
                    std::function<
                        bool(IEvent*)
                    > customInterestCallback =
                        nullptr
                ) = 0;


                template<typename EventType>
                EventListenerHandlePtr
                RegisterListener(
                    std::function<
                        void(
                            EventType*,
                            EventDispatchMethod,
                            EventPriority
                        )
                    > callback,
                    EventListenerInterest interest =
                        EventListenerInterest::All,
                    EventTime maximumTimeSinceDispatch =
                        EventTime(0),
                    std::function<
                        bool(EventType*)
                    > customInterestCallback =
                        nullptr
                ) {
                    return RegisterListener(
                        std::type_index(
                            typeid(EventType)
                        ),
                        [
                            callback
                        ](
                            IEvent* event,
                            EventDispatchMethod
                                dispatchMethod,
                            EventPriority priority
                        ) {
                            EventType* typedEvent =
                                dynamic_cast<
                                    EventType*
                                >(event);

                            if (
                                typedEvent !=
                                nullptr
                            ) {
                                callback(
                                    typedEvent,
                                    dispatchMethod,
                                    priority
                                );
                            }
                        },
                        interest,
                        maximumTimeSinceDispatch,
                        customInterestCallback ==
                            nullptr
                            ? std::function<
                                bool(IEvent*)
                              >()
                            : std::function<
                                bool(IEvent*)
                              >(
                                [
                                    customInterestCallback
                                ](
                                    IEvent* event
                                ) {
                                    EventType*
                                        typedEvent =
                                            dynamic_cast<
                                                EventType*
                                            >(event);

                                    return
                                        typedEvent !=
                                            nullptr &&
                                        customInterestCallback(
                                            typedEvent
                                        );
                                }
                              )
                    );
                }


                template<typename EventType>
                EventListenerHandlePtr
                RegisterObserver(
                    IEventObserver<
                        EventType
                    >* observer,
                    EventListenerInterest interest =
                        EventListenerInterest::All,
                    EventTime maximumTimeSinceDispatch =
                        EventTime(0)
                ) {
                    if (observer == nullptr) {
                        throw
                            Observable::
                                InvalidObserverRegistrationException();
                    }

                    std::function<
                        bool(EventType*)
                    > customInterestCallback =
                        nullptr;

                    if (
                        interest ==
                        EventListenerInterest::
                            Custom
                    ) {
                        customInterestCallback =
                            [
                                observer
                            ](
                                EventType* event
                            ) {
                                return
                                    observer->
                                        IsInterestedInEvent(
                                            event
                                        );
                            };
                    }

                    return
                        RegisterListener<
                            EventType
                        >(
                            [
                                observer
                            ](
                                EventType* event,
                                EventDispatchMethod
                                    dispatchMethod,
                                EventPriority
                                    priority
                            ) {
                                observer->OnEvent(
                                    event,
                                    dispatchMethod,
                                    priority
                                );
                            },
                            interest,
                            maximumTimeSinceDispatch,
                            customInterestCallback
                        );
                }


                virtual void
                UnregisterListener(
                    std::type_index eventType,
                    IEventListenerHandle*
                        handler
                ) = 0;


                template<typename EventType>
                void UnregisterListener(
                    IEventListenerHandle*
                        handler
                ) {
                    UnregisterListener(
                        std::type_index(
                            typeid(EventType)
                        ),
                        handler
                    );
                }
        };


        class EventListenerHandle :
            public IEventListenerHandle {

            private:
                mutable
                    Threads::
                        ReadWriteMutex<bool>
                            _isRegistered{
                                true
                            };

                std::type_index
                    _eventType;

                IEventListener*
                    _listener;


            public:
                EventListenerHandle(
                    std::type_index eventType,
                    IEventListener* listener
                ) :
                    _eventType(eventType),
                    _listener(listener) {
                }


                ~EventListenerHandle()
                    noexcept override {
                    try {
                        Unregister();
                    } catch (...) {
                        ForceUnregister();
                    }
                }


                void Unregister() override {
                    if (
                        !_isRegistered.Get() ||
                        _listener == nullptr
                    ) {
                        return;
                    }

                    _listener->
                        UnregisterListener(
                            _eventType,
                            this
                        );

                    _isRegistered.Set(
                        false
                    );
                }


                bool IsRegistered()
                    const override {
                    return
                        _isRegistered.Get();
                }


                void ForceUnregister() {
                    _isRegistered.Set(
                        false
                    );

                    _listener =
                        nullptr;
                }
        };


        class EventListener :
            public IEventListener {

            private:
                class IEventListenerContainer {
                    public:
                        virtual
                            ~IEventListenerContainer() =
                                default;

                        virtual
                            IEventListenerHandle*
                            GetListenerHandler()
                                const = 0;

                        virtual
                            EventListenerInterest
                            GetInterest()
                                const = 0;

                        virtual void
                            ProcessEvent(
                                IEvent* event,
                                EventDispatchMethod
                                    dispatchMethod,
                                EventPriority
                                    priority
                            ) = 0;
                };


                template<typename EventType>
                class EventListenerContainer :
                    public IEventListenerContainer {

                    private:
                        std::function<
                            void(
                                EventType*,
                                EventDispatchMethod,
                                EventPriority
                            )
                        > _callback;

                        IEventListenerHandle*
                            _listenerHandler;

                        EventListenerInterest
                            _interest =
                                EventListenerInterest::
                                    All;

                        uint64_t
                            _maximumTimeSinceDispatchNanoseconds =
                                0;

                        std::function<
                            bool(IEvent*)
                        >
                            _customInterestCallback =
                                nullptr;


                    public:
                        EventListenerContainer(
                            std::function<
                                void(
                                    EventType*,
                                    EventDispatchMethod,
                                    EventPriority
                                )
                            > callback,
                            IEventListenerHandle*
                                listenerHandler,
                            EventListenerInterest
                                interest,
                            EventTime
                                maximumTimeSinceDispatch,
                            std::function<
                                bool(IEvent*)
                            >
                                customInterestCallback
                        ) :
                            _callback(
                                std::move(
                                    callback
                                )
                            ),
                            _listenerHandler(
                                listenerHandler
                            ),
                            _interest(
                                interest
                            ),
                            _maximumTimeSinceDispatchNanoseconds(
                                Timing::
                                    TimeTraits<
                                        EventTime
                                    >::template
                                        ToNanoseconds<
                                            uint64_t
                                        >(
                                            maximumTimeSinceDispatch
                                        )
                            ),
                            _customInterestCallback(
                                std::move(
                                    customInterestCallback
                                )
                            ) {
                        }


                        IEventListenerHandle*
                        GetListenerHandler()
                            const override {
                            return
                                _listenerHandler;
                        }


                        EventListenerInterest
                        GetInterest()
                            const override {
                            return
                                _interest;
                        }


                        void ProcessEvent(
                            IEvent* event,
                            EventDispatchMethod
                                dispatchMethod,
                            EventPriority
                                priority
                        ) override {
                            EventType*
                                typedEvent =
                                    dynamic_cast<
                                        EventType*
                                    >(event);

                            if (
                                typedEvent ==
                                nullptr
                            ) {
                                return;
                            }

                            bool interested =
                                _interest ==
                                EventListenerInterest::
                                    All;

                            if (
                                _interest ==
                                EventListenerInterest::
                                    YoungerThan
                            ) {
                                interested =
                                    event->
                                        GetTimeSinceDispatchNanoseconds() <
                                    _maximumTimeSinceDispatchNanoseconds;
                            } else if (
                                _interest ==
                                EventListenerInterest::
                                    Custom
                            ) {
                                interested =
                                    _customInterestCallback !=
                                        nullptr &&
                                    _customInterestCallback(
                                        event
                                    );
                            }

                            if (interested) {
                                _callback(
                                    typedEvent,
                                    dispatchMethod,
                                    priority
                                );
                            }
                        }
                };


                using EventListeners =
                    std::vector<
                        std::shared_ptr<
                            IEventListenerContainer
                        >
                    >;

                using EventListenersSnapshot =
                    std::shared_ptr<
                        const EventListeners
                    >;

                using EventListenersMap =
                    std::unordered_map<
                        std::type_index,
                        EventListenersSnapshot
                    >;


                EventListenersMap
                    _eventListeners;

                mutable
                    std::shared_mutex
                        _eventListenersMutex;


                std::shared_ptr<
                    EventListeners
                >
                CopyListenersForEventType(
                    std::type_index eventType
                ) const {
                    const auto found =
                        _eventListeners.find(
                            eventType
                        );

                    return
                        found ==
                            _eventListeners.end() ||
                        !found->second
                            ? std::make_shared<
                                EventListeners
                              >()
                            : std::make_shared<
                                EventListeners
                              >(
                                *found->second
                              );
                }


            protected:
                virtual void
                OnListenerRegistered(
                    std::type_index
                ) {
                }


                virtual void
                OnListenerUnregistered(
                    std::type_index
                ) {
                }


                void
                UnregisterAllListeners()
                    noexcept {
                    for (;;) {
                        std::type_index
                            eventType(
                                typeid(void)
                            );

                        {
                            std::unique_lock<
                                std::shared_mutex
                            > lock(
                                _eventListenersMutex
                            );

                            if (
                                _eventListeners.empty()
                            ) {
                                return;
                            }

                            const auto entry =
                                _eventListeners.begin();

                            eventType =
                                entry->first;

                            for (
                                const auto&
                                    listener :
                                *entry->second
                            ) {
                                static_cast<
                                    EventListenerHandle*
                                >(
                                    listener->
                                        GetListenerHandler()
                                )->ForceUnregister();
                            }

                            _eventListeners.erase(
                                entry
                            );
                        }

                        try {
                            OnListenerUnregistered(
                                eventType
                            );
                        } catch (...) {
                        }
                    }
                }


            public:
                ~EventListener() override {
                    UnregisterAllListeners();
                }


                EventListenerHandlePtr
                RegisterListener(
                    std::type_index eventType,
                    std::function<
                        void(
                            IEvent*,
                            EventDispatchMethod,
                            EventPriority
                        )
                    > callback,
                    EventListenerInterest
                        interest =
                            EventListenerInterest::
                                All,
                    EventTime
                        maximumTimeSinceDispatch =
                            EventTime(0),
                    std::function<
                        bool(IEvent*)
                    >
                        customInterestCallback =
                            nullptr
                ) override {
                    std::unique_ptr<
                        EventListenerHandle
                    > handler(
                        new EventListenerHandle(
                            eventType,
                            this
                        )
                    );

                    bool
                        firstListener =
                            false;

                    {
                        std::unique_lock<
                            std::shared_mutex
                        > lock(
                            _eventListenersMutex
                        );

                        std::shared_ptr<
                            EventListeners
                        > listeners =
                            CopyListenersForEventType(
                                eventType
                            );

                        firstListener =
                            listeners->empty();

                        listeners->push_back(
                            std::make_shared<
                                EventListenerContainer<
                                    IEvent
                                >
                            >(
                                std::move(
                                    callback
                                ),
                                handler.get(),
                                interest,
                                maximumTimeSinceDispatch,
                                std::move(
                                    customInterestCallback
                                )
                            )
                        );

                        _eventListeners[
                            eventType
                        ] = listeners;
                    }

                    if (firstListener) {
                        OnListenerRegistered(
                            eventType
                        );
                    }

                    return
                        EventListenerHandlePtr(
                            handler.release()
                        );
                }


                template<typename EventType>
                EventListenerHandlePtr
                RegisterListener(
                    std::function<
                        void(
                            EventType*,
                            EventDispatchMethod,
                            EventPriority
                        )
                    > callback,
                    EventListenerInterest
                        interest =
                            EventListenerInterest::
                                All,
                    EventTime
                        maximumTimeSinceDispatch =
                            EventTime(0),
                    std::function<
                        bool(EventType*)
                    >
                        customInterestCallback =
                            nullptr
                ) {
                    const std::type_index
                        eventType(
                            typeid(EventType)
                        );

                    std::unique_ptr<
                        EventListenerHandle
                    > handler(
                        new EventListenerHandle(
                            eventType,
                            this
                        )
                    );

                    std::function<
                        bool(IEvent*)
                    >
                        erasedInterest =
                            nullptr;

                    if (
                        customInterestCallback !=
                        nullptr
                    ) {
                        erasedInterest =
                            [
                                customInterestCallback
                            ](
                                IEvent* event
                            ) {
                                EventType*
                                    typedEvent =
                                        dynamic_cast<
                                            EventType*
                                        >(event);

                                return
                                    typedEvent !=
                                        nullptr &&
                                    customInterestCallback(
                                        typedEvent
                                    );
                            };
                    }

                    bool
                        firstListener =
                            false;

                    {
                        std::unique_lock<
                            std::shared_mutex
                        > lock(
                            _eventListenersMutex
                        );

                        std::shared_ptr<
                            EventListeners
                        > listeners =
                            CopyListenersForEventType(
                                eventType
                            );

                        firstListener =
                            listeners->empty();

                        listeners->push_back(
                            std::make_shared<
                                EventListenerContainer<
                                    EventType
                                >
                            >(
                                std::move(
                                    callback
                                ),
                                handler.get(),
                                interest,
                                maximumTimeSinceDispatch,
                                std::move(
                                    erasedInterest
                                )
                            )
                        );

                        _eventListeners[
                            eventType
                        ] = listeners;
                    }

                    if (firstListener) {
                        OnListenerRegistered(
                            eventType
                        );
                    }

                    return
                        EventListenerHandlePtr(
                            handler.release()
                        );
                }


                void UnregisterListener(
                    std::type_index eventType,
                    IEventListenerHandle*
                        handler
                ) override {
                    bool
                        removedLast =
                            false;

                    {
                        std::unique_lock<
                            std::shared_mutex
                        > lock(
                            _eventListenersMutex
                        );

                        const auto found =
                            _eventListeners.find(
                                eventType
                            );

                        if (
                            found ==
                                _eventListeners.end() ||
                            !found->second
                        ) {
                            return;
                        }

                        auto listeners =
                            std::make_shared<
                                EventListeners
                            >(
                                *found->second
                            );

                        for (
                            auto it =
                                listeners->begin();
                            it !=
                                listeners->end();
                            ++it
                        ) {
                            if (
                                (*it)->
                                    GetListenerHandler() ==
                                handler
                            ) {
                                static_cast<
                                    EventListenerHandle*
                                >(
                                    handler
                                )->ForceUnregister();

                                listeners->erase(
                                    it
                                );

                                break;
                            }
                        }

                        removedLast =
                            listeners->empty();

                        if (removedLast) {
                            _eventListeners.erase(
                                found
                            );
                        } else {
                            _eventListeners[
                                eventType
                            ] = listeners;
                        }
                    }

                    if (removedLast) {
                        OnListenerUnregistered(
                            eventType
                        );
                    }
                }


                void ProcessEvent(
                    IEvent* event,
                    EventDispatchMethod
                        dispatchMethod,
                    EventPriority priority
                ) {
                    EventListenersSnapshot
                        listeners;

                    {
                        std::shared_lock<
                            std::shared_mutex
                        > lock(
                            _eventListenersMutex
                        );

                        const auto found =
                            _eventListeners.find(
                                std::type_index(
                                    typeid(*event)
                                )
                            );

                        if (
                            found ==
                                _eventListeners.end() ||
                            !found->second
                        ) {
                            return;
                        }

                        listeners =
                            found->second;
                    }

                    for (
                        const auto&
                            listener :
                        *listeners
                    ) {
                        listener->ProcessEvent(
                            event,
                            dispatchMethod,
                            priority
                        );
                    }
                }
        };

    }

}
