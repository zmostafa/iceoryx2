// Copyright (c) 2024 Contributors to the Eclipse Foundation
//
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
//
// This program and the accompanying materials are made available under the
// terms of the Apache Software License 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0, or the MIT license
// which is available at https://opensource.org/licenses/MIT.
//
// SPDX-License-Identifier: Apache-2.0 OR MIT

#ifndef IOX2_SERVICE_BUILDER_PUBLISH_SUBSCRIBE_HPP
#define IOX2_SERVICE_BUILDER_PUBLISH_SUBSCRIBE_HPP

#include "iox/builder_addendum.hpp"
#include "iox/expected.hpp"
#include "iox2/attribute_specifier.hpp"
#include "iox2/attribute_verifier.hpp"
#include "iox2/internal/iceoryx2.hpp"
#include "iox2/internal/service_builder_publish_subscribe_internal.hpp"
#include "iox2/payload_info.hpp"
#include "iox2/port_factory_publish_subscribe.hpp"
#include "iox2/service_builder_publish_subscribe_error.hpp"
#include "iox2/service_type.hpp"

#include <typeinfo>

namespace iox2 {
/// Builder to create new [`MessagingPattern::PublishSubscribe`] based [`Service`]s
template <typename Payload, typename UserHeader, ServiceType S>
class ServiceBuilderPublishSubscribe {
    /// If the [`Service`] is created, it defines the [`Alignment`] of the payload for the service. If
    /// an existing [`Service`] is opened it requires the service to have at least the defined
    /// [`Alignment`]. If the Payload [`Alignment`] is greater than the provided [`Alignment`]
    /// then the Payload [`Alignment`] is used.
    IOX_BUILDER_OPTIONAL(uint64_t, payload_alignment);

    /// If the [`Service`] is created, defines the overflow behavior of the service. If an existing
    /// [`Service`] is opened it requires the service to have the defined overflow behavior.
    IOX_BUILDER_OPTIONAL(bool, enable_safe_overflow);

    /// If the [`Service`] is created it defines how many [`crate::sample::Sample`] a
    /// [`crate::port::subscriber::Subscriber`] can borrow at most in parallel. If an existing
    /// [`Service`] is opened it defines the minimum required.
    IOX_BUILDER_OPTIONAL(uint64_t, subscriber_max_borrowed_samples);

    /// If the [`Service`] is created it defines the maximum history size a
    /// [`crate::port::subscriber::Subscriber`] can request on connection. If an existing
    /// [`Service`] is opened it defines the minimum required.
    IOX_BUILDER_OPTIONAL(uint64_t, history_size);

    /// If the [`Service`] is created it defines how many [`crate::sample::Sample`] a
    /// [`crate::port::subscriber::Subscriber`] can store in its internal buffer. If an existing
    /// [`Service`] is opened it defines the minimum required.
    IOX_BUILDER_OPTIONAL(uint64_t, subscriber_max_buffer_size);

    /// If the [`Service`] is created it defines how many [`crate::port::subscriber::Subscriber`] shall
    /// be supported at most. If an existing [`Service`] is opened it defines how many
    /// [`crate::port::subscriber::Subscriber`] must be at least supported.
    IOX_BUILDER_OPTIONAL(uint64_t, max_subscribers);

    /// If the [`Service`] is created it defines how many [`crate::port::publisher::Publisher`] shall
    /// be supported at most. If an existing [`Service`] is opened it defines how many
    /// [`crate::port::publisher::Publisher`] must be at least supported.
    IOX_BUILDER_OPTIONAL(uint64_t, max_publishers);

    /// If the [`Service`] is created it defines how many [`Node`](crate::node::Node)s shall
    /// be able to open it in parallel. If an existing [`Service`] is opened it defines how many
    /// [`Node`](crate::node::Node)s must be at least supported.
    IOX_BUILDER_OPTIONAL(uint64_t, max_nodes);

  public:
    /// Sets the user header type of the [`Service`].
    template <typename NewHeader>
    auto user_header() && -> ServiceBuilderPublishSubscribe<Payload, NewHeader, S>&&;

    /// If the [`Service`] exists, it will be opened otherwise a new [`Service`] will be
    /// created.
    auto open_or_create() && -> iox::expected<PortFactoryPublishSubscribe<S, Payload, UserHeader>,
                                              PublishSubscribeOpenOrCreateError>;

    /// If the [`Service`] exists, it will be opened otherwise a new [`Service`] will be
    /// created. It defines a set of attributes. If the [`Service`] already exists all attribute
    /// requirements must be satisfied otherwise the open process will fail. If the [`Service`]
    /// does not exist the required attributes will be defined in the [`Service`].
    auto open_or_create_with_attributes(const AttributeVerifier& required_attributes) && -> iox::
        expected<PortFactoryPublishSubscribe<S, Payload, UserHeader>, PublishSubscribeOpenOrCreateError>;

    /// Opens an existing [`Service`].
    auto open() && -> iox::expected<PortFactoryPublishSubscribe<S, Payload, UserHeader>, PublishSubscribeOpenError>;

    /// Opens an existing [`Service`] with attribute requirements. If the defined attribute
    /// requirements are not satisfied the open process will fail.
    auto open_with_attributes(const AttributeVerifier& required_attributes) && -> iox::
        expected<PortFactoryPublishSubscribe<S, Payload, UserHeader>, PublishSubscribeOpenError>;

    /// Creates a new [`Service`].
    auto create() && -> iox::expected<PortFactoryPublishSubscribe<S, Payload, UserHeader>, PublishSubscribeCreateError>;

    /// Creates a new [`Service`] with a set of attributes.
    auto create_with_attributes(
        const AttributeSpecifier& attributes) && -> iox::expected<PortFactoryPublishSubscribe<S, Payload, UserHeader>,
                                                                  PublishSubscribeCreateError>;

  private:
    template <ServiceType>
    friend class ServiceBuilder;

    explicit ServiceBuilderPublishSubscribe(iox2_service_builder_h handle);

    void set_parameters();

    template <typename PayloadType>
    auto get_payload_type_name() -> internal::FromCustomizedPayloadTypeName<PayloadType>;

    template <typename PayloadType>
    auto get_payload_type_name() -> internal::FromNonSlice<PayloadType>;

    template <typename PayloadType>
    auto get_payload_type_name() -> internal::FromSliceWithCustomizedInnerPayloadTypeName<PayloadType>;

    template <typename PayloadType>
    auto get_payload_type_name() -> internal::FromSliceWithoutCustomizedInnerPayloadTypeName<PayloadType>;

    template <typename UserHeaderType>
    auto get_user_header_type_name() ->
        typename std::enable_if_t<internal::HasUserHeaderTypeNameMember<UserHeaderType>::value, const char*>;

    template <typename UserHeaderType>
    auto get_user_header_type_name() ->
        typename std::enable_if_t<!internal::HasUserHeaderTypeNameMember<UserHeaderType>::value, const char*>;

    iox2_service_builder_pub_sub_h m_handle = nullptr;
};

template <typename Payload, typename UserHeader, ServiceType S>
inline ServiceBuilderPublishSubscribe<Payload, UserHeader, S>::ServiceBuilderPublishSubscribe(
    iox2_service_builder_h handle)
    : m_handle { iox2_service_builder_pub_sub(handle) } {
}

template <typename Payload, typename UserHeader, ServiceType S>
template <typename PayloadType>
inline auto ServiceBuilderPublishSubscribe<Payload, UserHeader, S>::get_payload_type_name()
    -> internal::FromCustomizedPayloadTypeName<PayloadType> {
    return PayloadType::PAYLOAD_TYPE_NAME;
}

// NOLINTBEGIN(readability-function-size) : template alternative is less readable
template <typename Payload, typename UserHeader, ServiceType S>
template <typename PayloadType>
inline auto ServiceBuilderPublishSubscribe<Payload, UserHeader, S>::get_payload_type_name()
    -> internal::FromNonSlice<PayloadType> {
    if (std::is_same_v<PayloadType, uint8_t>) {
        return "u8";
    }
    if (std::is_same_v<PayloadType, uint16_t>) {
        return "u16";
    }
    if (std::is_same_v<PayloadType, uint32_t>) {
        return "u32";
    }
    if (std::is_same_v<PayloadType, uint64_t>) {
        return "u64";
    }
    if (std::is_same_v<PayloadType, int8_t>) {
        return "i8";
    }
    if (std::is_same_v<PayloadType, int16_t>) {
        return "i16";
    }
    if (std::is_same_v<PayloadType, int32_t>) {
        return "i32";
    }
    if (std::is_same_v<PayloadType, int64_t>) {
        return "i64";
    }
    if (std::is_same_v<PayloadType, float>) {
        return "f32";
    }
    if (std::is_same_v<PayloadType, double>) {
        return "f64";
    }
    if (std::is_same_v<PayloadType, bool>) {
        return "bool";
    }
    return typeid(typename PayloadInfo<PayloadType>::ValueType).name();
}
// NOLINTEND(readability-function-size)

template <typename Payload, typename UserHeader, ServiceType S>
template <typename PayloadType>
inline auto ServiceBuilderPublishSubscribe<Payload, UserHeader, S>::get_payload_type_name()
    -> internal::FromSliceWithCustomizedInnerPayloadTypeName<PayloadType> {
    return PayloadType::ValueType::PAYLOAD_TYPE_NAME;
}

template <typename Payload, typename UserHeader, ServiceType S>
template <typename PayloadType>
inline auto ServiceBuilderPublishSubscribe<Payload, UserHeader, S>::get_payload_type_name()
    -> internal::FromSliceWithoutCustomizedInnerPayloadTypeName<PayloadType> {
    return get_payload_type_name<typename PayloadType::ValueType>();
}

template <typename Payload, typename UserHeader, ServiceType S>
template <typename UserHeaderType>
inline auto ServiceBuilderPublishSubscribe<Payload, UserHeader, S>::get_user_header_type_name() ->
    typename std::enable_if_t<internal::HasUserHeaderTypeNameMember<UserHeaderType>::value, const char*> {
    return UserHeaderType::USER_HEADER_TYPE_NAME;
}

template <typename Payload, typename UserHeader, ServiceType S>
template <typename UserHeaderType>
inline auto ServiceBuilderPublishSubscribe<Payload, UserHeader, S>::get_user_header_type_name() ->
    typename std::enable_if_t<!internal::HasUserHeaderTypeNameMember<UserHeaderType>::value, const char*> {
    if (std::is_void_v<UserHeader>) {
        return "()"; // no user header provided
    }
    return typeid(UserHeader).name();
}

template <typename Payload, typename UserHeader, ServiceType S>
inline void ServiceBuilderPublishSubscribe<Payload, UserHeader, S>::set_parameters() {
    m_enable_safe_overflow.and_then(
        [&](auto value) { iox2_service_builder_pub_sub_set_enable_safe_overflow(&m_handle, value); });
    m_subscriber_max_borrowed_samples.and_then(
        [&](auto value) { iox2_service_builder_pub_sub_set_subscriber_max_borrowed_samples(&m_handle, value); });
    m_history_size.and_then([&](auto value) { iox2_service_builder_pub_sub_set_history_size(&m_handle, value); });
    m_subscriber_max_buffer_size.and_then(
        [&](auto value) { iox2_service_builder_pub_sub_set_subscriber_max_buffer_size(&m_handle, value); });
    m_max_subscribers.and_then([&](auto value) { iox2_service_builder_pub_sub_set_max_subscribers(&m_handle, value); });
    m_max_publishers.and_then([&](auto value) { iox2_service_builder_pub_sub_set_max_publishers(&m_handle, value); });
    m_payload_alignment.and_then(
        [&](auto value) { iox2_service_builder_pub_sub_set_payload_alignment(&m_handle, value); });
    m_max_nodes.and_then([&](auto value) { iox2_service_builder_pub_sub_set_max_nodes(&m_handle, value); });

    using ValueType = typename PayloadInfo<Payload>::ValueType;
    auto type_variant = iox::IsSlice<Payload>::VALUE ? iox2_type_variant_e_DYNAMIC : iox2_type_variant_e_FIXED_SIZE;

    // payload type details
    const auto* payload_type_name = get_payload_type_name<Payload>();
    const auto payload_type_name_len = strlen(payload_type_name);
    const auto payload_type_size = sizeof(ValueType);
    const auto payload_type_align = alignof(ValueType);

    const auto payload_result = iox2_service_builder_pub_sub_set_payload_type_details(
        &m_handle, type_variant, payload_type_name, payload_type_name_len, payload_type_size, payload_type_align);

    if (payload_result != IOX2_OK) {
        IOX_PANIC("This should never happen! Implementation failure while setting the Payload-Type.");
    }

    // user header type details
    const auto header_layout = iox::Layout::from<UserHeader>();
    const auto* user_header_type_name = get_user_header_type_name<UserHeader>();
    const auto user_header_type_name_len = strlen(user_header_type_name);
    const auto user_header_type_size = header_layout.size();
    const auto user_header_type_align = header_layout.alignment();

    const auto user_header_result =
        iox2_service_builder_pub_sub_set_user_header_type_details(&m_handle,
                                                                  iox2_type_variant_e_FIXED_SIZE,
                                                                  user_header_type_name,
                                                                  user_header_type_name_len,
                                                                  user_header_type_size,
                                                                  user_header_type_align);

    if (user_header_result != IOX2_OK) {
        IOX_PANIC("This should never happen! Implementation failure while setting the User-Header-Type.");
    }
}

template <typename Payload, typename UserHeader, ServiceType S>
template <typename NewHeader>
inline auto ServiceBuilderPublishSubscribe<Payload, UserHeader, S>::
    user_header() && -> ServiceBuilderPublishSubscribe<Payload, NewHeader, S>&& {
    // required here since we just change the template header type but the builder structure stays the same
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return std::move(*reinterpret_cast<ServiceBuilderPublishSubscribe<Payload, NewHeader, S>*>(this));
}

template <typename Payload, typename UserHeader, ServiceType S>
inline auto ServiceBuilderPublishSubscribe<Payload, UserHeader, S>::open_or_create() && -> iox::
    expected<PortFactoryPublishSubscribe<S, Payload, UserHeader>, PublishSubscribeOpenOrCreateError> {
    set_parameters();

    iox2_port_factory_pub_sub_h port_factory_handle {};
    auto result = iox2_service_builder_pub_sub_open_or_create(m_handle, nullptr, &port_factory_handle);

    if (result == IOX2_OK) {
        return iox::ok(PortFactoryPublishSubscribe<S, Payload, UserHeader>(port_factory_handle));
    }

    return iox::err(iox::into<PublishSubscribeOpenOrCreateError>(result));
}

template <typename Payload, typename UserHeader, ServiceType S>
inline auto ServiceBuilderPublishSubscribe<Payload, UserHeader, S>::open() && -> iox::
    expected<PortFactoryPublishSubscribe<S, Payload, UserHeader>, PublishSubscribeOpenError> {
    set_parameters();

    iox2_port_factory_pub_sub_h port_factory_handle {};
    auto result = iox2_service_builder_pub_sub_open(m_handle, nullptr, &port_factory_handle);

    if (result == IOX2_OK) {
        return iox::ok(PortFactoryPublishSubscribe<S, Payload, UserHeader>(port_factory_handle));
    }

    return iox::err(iox::into<PublishSubscribeOpenError>(result));
}

template <typename Payload, typename UserHeader, ServiceType S>
inline auto ServiceBuilderPublishSubscribe<Payload, UserHeader, S>::create() && -> iox::
    expected<PortFactoryPublishSubscribe<S, Payload, UserHeader>, PublishSubscribeCreateError> {
    set_parameters();

    iox2_port_factory_pub_sub_h port_factory_handle {};
    auto result = iox2_service_builder_pub_sub_create(m_handle, nullptr, &port_factory_handle);

    if (result == IOX2_OK) {
        return iox::ok(PortFactoryPublishSubscribe<S, Payload, UserHeader>(port_factory_handle));
    }

    return iox::err(iox::into<PublishSubscribeCreateError>(result));
}

template <typename Payload, typename UserHeader, ServiceType S>
inline auto ServiceBuilderPublishSubscribe<Payload, UserHeader, S>::open_or_create_with_attributes(
    const AttributeVerifier&
        required_attributes) && -> iox::expected<PortFactoryPublishSubscribe<S, Payload, UserHeader>,
                                                 PublishSubscribeOpenOrCreateError> {
    set_parameters();

    iox2_port_factory_pub_sub_h port_factory_handle {};
    auto result = iox2_service_builder_pub_sub_open_or_create_with_attributes(
        m_handle, &required_attributes.m_handle, nullptr, &port_factory_handle);

    if (result == IOX2_OK) {
        return iox::ok(PortFactoryPublishSubscribe<S, Payload, UserHeader>(port_factory_handle));
    }

    return iox::err(iox::into<PublishSubscribeOpenOrCreateError>(result));
}

template <typename Payload, typename UserHeader, ServiceType S>
inline auto ServiceBuilderPublishSubscribe<Payload, UserHeader, S>::open_with_attributes(
    const AttributeVerifier&
        required_attributes) && -> iox::expected<PortFactoryPublishSubscribe<S, Payload, UserHeader>,
                                                 PublishSubscribeOpenError> {
    set_parameters();

    iox2_port_factory_pub_sub_h port_factory_handle {};
    auto result = iox2_service_builder_pub_sub_open_with_attributes(
        m_handle, &required_attributes.m_handle, nullptr, &port_factory_handle);

    if (result == IOX2_OK) {
        return iox::ok(PortFactoryPublishSubscribe<S, Payload, UserHeader>(port_factory_handle));
    }

    return iox::err(iox::into<PublishSubscribeOpenError>(result));
}

template <typename Payload, typename UserHeader, ServiceType S>
inline auto ServiceBuilderPublishSubscribe<Payload, UserHeader, S>::create_with_attributes(
    const AttributeSpecifier& attributes) && -> iox::expected<PortFactoryPublishSubscribe<S, Payload, UserHeader>,
                                                              PublishSubscribeCreateError> {
    set_parameters();

    iox2_port_factory_pub_sub_h port_factory_handle {};
    auto result = iox2_service_builder_pub_sub_create_with_attributes(
        m_handle, &attributes.m_handle, nullptr, &port_factory_handle);

    if (result == IOX2_OK) {
        return iox::ok(PortFactoryPublishSubscribe<S, Payload, UserHeader>(port_factory_handle));
    }

    return iox::err(iox::into<PublishSubscribeCreateError>(result));
}
} // namespace iox2

#endif
