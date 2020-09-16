//  Copyright (c) 2020 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// hpxinspect:nodeprecatedname:hpx::traits::is_callable

#pragma once

#include <hpx/config/constexpr.hpp>
#include <hpx/basic_execution/operation_state.hpp>
#include <hpx/basic_execution/receiver.hpp>
#include <hpx/functional/tag_invoke.hpp>

#include <exception>
#include <type_traits>
#include <utility>

namespace hpx { namespace basic_execution {
#if defined(DOXYGEN)
    /// connect is a customization point object.
    /// From some subexpression `s` and `r`, let `S` be the type such that `decltype((s))`
    /// is `S` and let `R` be the type such that `decltype((r))` is `R`. The result of
    /// the expression `hpx::basic_execution::connect(s, r)` is then equivalent to:
    ///     * `s.connect(r)`, if that expression is valid and returns a type
    ///       satisfying the `operation_state`
    ///       (\see hpx::basic_execution::traits::is_operation_state)
    ///       and if `S` satifies the `sender` concept.
    ///     * `s.connect(r)`, if that expression is valid and returns a type
    ///       satisfying the `operation_state`
    ///       (\see hpx::basic_execution::traits::is_operation_state)
    ///       and if `S` satifies the `sender` concept.
    ///       Overload resolution is performed in a context that include the declaration
    ///       `void connect();`
    ///     * Otherwise: TODO once executor is in place...
    ///     * Otherwise, the expression is ill-formed.
    ///
    /// The customization is implemented in terms of `hpx::function::tag_invoke`
    template <typename S, typename R>
    void connect(S&& s, R&& r);
#endif

    namespace traits {
        /// A sender is a type that is describing an asynchronous operation. The
        /// operation itself might not have started yet. In order to get the result
        /// of this asynchronous operation, a sender needs to be connected to a
        /// receiver with the corresponding value, error and done channels:
        ///     * `hpx::basic_execution::connect`
        ///
        /// In addition, `hpx::basic_execution::traits::sender_traits ` needs to
        /// be specialized in some form.
        ///
        /// A sender's destructor shall not block pending completion of submitted
        /// operations.
        template <typename Sender>
        struct is_sender;

        /// \see is_sender
        template <typename Sender, typename Receiver>
        struct is_sender_to;

        /// `sender_traits` expose the different value and error types exposed
        /// by a sender. This can be either specialized directly for user defined
        /// sender types or embedded value_types, error_types and sends_done
        /// inside the sender type can be provided.
        template <typename Sender>
        struct sender_traits;

        template <typename Sender>
        struct sender_traits<Sender volatile> : sender_traits<Sender>
        {
        };
        template <typename Sender>
        struct sender_traits<Sender const> : sender_traits<Sender>
        {
        };
        template <typename Sender>
        struct sender_traits<Sender&> : sender_traits<Sender>
        {
        };
        template <typename Sender>
        struct sender_traits<Sender&&> : sender_traits<Sender>
        {
        };
    }    // namespace traits

    HPX_INLINE_CONSTEXPR_VARIABLE struct connect_t
    {
#define HPX_BASIC_EXECUTION_CONNECT_EXPRESSION                                 \
    hpx::functional::tag_invoke(                                               \
        *this, std::forward<Sender>(sender), std::forward<Receiver>(rcv))
        template <typename Sender, typename Receiver>
        constexpr HPX_FORCEINLINE auto tag_invoke_impl(
            std::true_type /* is tag invocable */, Sender&& sender,
            Receiver&& rcv) const
            noexcept(noexcept(HPX_BASIC_EXECUTION_CONNECT_EXPRESSION))
                -> decltype(HPX_BASIC_EXECUTION_CONNECT_EXPRESSION)
        {
            return HPX_BASIC_EXECUTION_CONNECT_EXPRESSION;
        }
#undef HPX_BASIC_EXECUTION_CONNECT_EXPRESSION

#define HPX_BASIC_EXECUTION_CONNECT_EXPRESSION                                 \
    std::forward<Sender>(sender).connect(std::forward<Receiver>(rcv))
        template <typename Sender, typename Receiver>
        constexpr HPX_FORCEINLINE auto tag_invoke_impl(
            std::false_type /* is not tag invocable */, Sender&& sender,
            Receiver&& rcv) const
            noexcept(noexcept(HPX_BASIC_EXECUTION_CONNECT_EXPRESSION))
                -> decltype(HPX_BASIC_EXECUTION_CONNECT_EXPRESSION)
        {
            return HPX_BASIC_EXECUTION_CONNECT_EXPRESSION;
        }
#undef HPX_BASIC_EXECUTION_CONNECT_EXPRESSION

#define HPX_BASIC_EXECUTION_CONNECT_EXPRESSION                                 \
    tag_invoke_impl(IsTagInvocable{}, std::forward<Sender>(sender),            \
        std::forward<Receiver>(rcv))
        template <typename Sender, typename Receiver,
            typename IsTagInvocable = hpx::functional::is_tag_invocable<
                connect_t, Sender&&, Receiver&&>>
        constexpr HPX_FORCEINLINE auto operation_select(
            std::true_type /* is sender */, Sender&& sender,
            Receiver&& rcv) const
            noexcept(noexcept(HPX_BASIC_EXECUTION_CONNECT_EXPRESSION))
                -> decltype(HPX_BASIC_EXECUTION_CONNECT_EXPRESSION)
        {
            return HPX_BASIC_EXECUTION_CONNECT_EXPRESSION;
        }
#undef HPX_BASIC_EXECUTION_CONNECT_EXPRESSION

        //         template <typename Sender, typename Receiver>
        //         constexpr HPX_FORCEINLINE auto operation_select(
        //             std::false_type /* is not sender */, Sender&& sender,
        //             Receiver&& rcv) const -> void
        //         {
        //             static_assert(traits::is_sender<Sender>::value, "Implement me...");
        //         }

#define HPX_BASIC_EXECUTION_CONNECT_EXPRESSION                                 \
    operation_select(                                                          \
        IsSender{}, std::forward<Sender>(sender), std::forward<Receiver>(rcv))
        template <typename Sender, typename Receiver,
            typename IsSender = traits::is_sender<Sender>>
        constexpr HPX_FORCEINLINE auto operator()(
            Sender&& sender, Receiver&& rcv) const
            noexcept(noexcept(HPX_BASIC_EXECUTION_CONNECT_EXPRESSION))
                -> decltype(HPX_BASIC_EXECUTION_CONNECT_EXPRESSION)
        {
            static_assert(
                hpx::basic_execution::traits::is_operation_state_v<decltype(
                    HPX_BASIC_EXECUTION_CONNECT_EXPRESSION)>,
                "hpx::basic_execution::connect needs to return a type "
                "saisfying the operation_state concept");
            return HPX_BASIC_EXECUTION_CONNECT_EXPRESSION;
        }
#undef HPX_BASIC_EXECUTION_CONNECT_EXPRESSION
    } connect;

    namespace traits {
        namespace detail {
            template <typename Sender>
            constexpr bool specialized(...)
            {
                return true;
            }

            template <typename Sender>
            constexpr bool specialized(
                typename hpx::basic_execution::traits::sender_traits<
                    Sender>::__unspecialized*)
            {
                return false;
            }
        }    // namespace detail

        template <typename Sender>
        struct is_sender
          : std::integral_constant<bool,
                std::is_move_constructible<
                    typename std::decay<Sender>::type>::value &&
                    detail::specialized<Sender>(nullptr)>
        {
        };

        template <typename Sender>
        constexpr bool is_sender_v = is_sender<Sender>::value;

        namespace detail {
            template <bool IsSenderReceiver, typename Sender, typename Receiver>
            struct is_sender_to_impl;

            template <typename Sender, typename Receiver>
            struct is_sender_to_impl<false, Sender, Receiver> : std::false_type
            {
            };

            template <typename Sender, typename Receiver>
            struct is_sender_to_impl<true, Sender, Receiver>
              : std::integral_constant<bool,
                    hpx::traits::is_callable<hpx::basic_execution::connect_t(
                        Sender&&, Receiver&&)>::value ||
                        hpx::traits::is_callable<hpx::basic_execution::
                                connect_t(Sender&&, Receiver&)>::value ||
                        hpx::traits::is_callable<hpx::basic_execution::
                                connect_t(Sender&&, Receiver const&)>::value ||
                        hpx::traits::is_callable<hpx::basic_execution::
                                connect_t(Sender&, Receiver&&)>::value ||
                        hpx::traits::is_callable<hpx::basic_execution::
                                connect_t(Sender&, Receiver&)>::value ||
                        hpx::traits::is_callable<hpx::basic_execution::
                                connect_t(Sender&, Receiver const&)>::value ||
                        hpx::traits::is_callable<hpx::basic_execution::
                                connect_t(Sender const&, Receiver&&)>::value ||
                        hpx::traits::is_callable<hpx::basic_execution::
                                connect_t(Sender const&, Receiver&)>::value ||
                        hpx::traits::is_callable<
                            hpx::basic_execution::connect_t(
                                Sender const&, Receiver const&)>::value>
            {
            };
        }    // namespace detail

        template <typename Sender, typename Receiver>
        struct is_sender_to
          : detail::is_sender_to_impl<is_sender_v<Sender> &&
                    is_receiver_v<Receiver>,
                Sender, Receiver>
        {
        };

        namespace detail {
            template <typename... As>
            struct tuple_mock;
            template <typename... As>
            struct variant_mock;

            template <typename Sender>
            constexpr bool has_value_types(
                typename Sender::template value_types<tuple_mock,
                    variant_mock>*)
            {
                return true;
            }

            template <typename Sender>
            constexpr bool has_value_types(...)
            {
                return false;
            }

            template <typename Sender>
            constexpr bool has_error_types(
                typename Sender::template error_types<variant_mock>*)
            {
                return true;
            }

            template <typename Sender>
            constexpr bool has_error_types(...)
            {
                return false;
            }

            template <typename Sender>
            constexpr bool has_sends_done(decltype(Sender::sends_done)*)
            {
                return true;
            }

            template <typename Sender>
            constexpr bool has_sends_done(...)
            {
                return false;
            }

            template <typename Sender>
            struct has_sender_types
              : std::integral_constant<bool,
                    has_value_types<Sender>(nullptr) &&
                        has_error_types<Sender>(nullptr) &&
                        has_sends_done<Sender>(nullptr)>
            {
            };

            template <bool HasSenderTraits, typename Sender>
            struct sender_traits_base;

            template <typename Sender>
            struct sender_traits_base<true /* HasSenderTraits */, Sender>
            {
                template <template <class...> class Tuple,
                    template <class...> class Variant>
                using value_types =
                    typename Sender::template value_types<Tuple, Variant>;

                template <template <class...> class Variant>
                using error_types =
                    typename Sender::template error_types<Variant>;

                static constexpr bool sends_done = Sender::sends_done;
            };

            template <typename Sender>
            struct sender_traits_base<false /* HasSenderTraits */, Sender>
            {
                // TODO: fix once executors are there...
                using __unspecialized = void;
            };
        }    // namespace detail

        template <typename Sender>
        struct sender_traits
          : detail::sender_traits_base<detail::has_sender_types<Sender>::value,
                Sender>
        {
        };
    }    // namespace traits

}}    // namespace hpx::basic_execution
