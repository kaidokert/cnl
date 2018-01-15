
//          Copyright John McFarlane 2015 - 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file ../LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

/// \file
/// \brief essential definitions related to the `cnl::overflow_integer` type

#if !defined(CNL_OVERFLOW_INT_H)
#define CNL_OVERFLOW_INT_H 1

#include "fixed_point.h"
#include "overflow.h"

#include "bits/number_base.h"

/// compositional numeric library
namespace cnl {
    ////////////////////////////////////////////////////////////////////////////////
    // forward-declarations

    template<class Rep, class OverflowTag>
    class overflow_integer;

    namespace _integer_impl {
        ////////////////////////////////////////////////////////////////////////////////
        // cnl::_integer_impl::is_overflow_int - trait to identify cnl::overflow_integer<>

        template<class T>
        struct is_overflow_int
                : std::false_type {
        };

        template<class Rep, class OverflowTag>
        struct is_overflow_int<overflow_integer<Rep, OverflowTag>>
                : std::true_type {
        };

        ////////////////////////////////////////////////////////////////////////////////
        // cnl::_integer_impl::are_integer_class_operands - basically identifies
        // operands that should go into a function defined here; filters out fixed-point

        template<class Lhs, class Rhs>
        struct are_integer_class_operands {
            static constexpr int integer_class = is_overflow_int<Lhs>::value + is_overflow_int<Rhs>::value;
            static constexpr int integer_or_float = _impl::is_integer_or_float<Lhs>::value + _impl::is_integer_or_float<Rhs>::value;
            static constexpr bool value = (integer_class >= 1) && (integer_or_float == 2);
        };

        ////////////////////////////////////////////////////////////////////////////////
        // cnl::_integer_impl::common_type

        template<class, class, class = void>
        struct common_type;

        // given two overflow_integer<>, produces the type that is best suited to both of them
        template<class LhsRep, class RhsRep, class OverflowTag>
        struct common_type<
                overflow_integer<LhsRep, OverflowTag>,
                overflow_integer<RhsRep, OverflowTag>> {
            using type = overflow_integer<
                    typename std::common_type<LhsRep, RhsRep>::type,
                    OverflowTag>;
        };

        // given a overflow_integer<> and a built-in integer type,
        // generates a overflow_integer<> type that is as big as both of them (or as close as possible)
        template<class LhsRep, class LhsOverflowTag, class RhsInteger>
        struct common_type<
                overflow_integer<LhsRep, LhsOverflowTag>, RhsInteger,
                _impl::enable_if_t<
                        !_integer_impl::is_overflow_int<RhsInteger>::value && std::is_integral<RhsInteger>::value>> {
            using type = typename cnl::overflow_integer<typename std::common_type<LhsRep, RhsInteger>::type, LhsOverflowTag>;
        };

        // given a overflow_integer<> and a floating-point type,
        // generates a floating-point type that is as big as both of them (or as close as possible)
        template<class LhsRep, class LhsOverflowTag, class Float>
        struct common_type<
                overflow_integer<LhsRep, LhsOverflowTag>, Float,
                _impl::enable_if_t<std::is_floating_point<Float>::value>> {
            using type = typename std::common_type<LhsRep, Float>::type;
        };

        // when first type is not overflow_integer<> and second type is, reverse the order
        template<class Lhs, class RhsRep, class RhsOverflowTag>
        struct common_type<Lhs, overflow_integer<RhsRep, RhsOverflowTag>>
                : common_type<overflow_integer<RhsRep, RhsOverflowTag>, Lhs> {
        };
    }

    ////////////////////////////////////////////////////////////////////////////////
    // cnl::overflow_integer<>

    // an integer which can be customized to react in different ways to overflow;
    // currently doesn't correctly detect overflow from operators
    template<class Rep = int, class OverflowTag = throwing_overflow_tag>
    class overflow_integer : public _impl::number_base<overflow_integer<Rep, OverflowTag>, Rep> {
        using _base = _impl::number_base<overflow_integer<Rep, OverflowTag>, Rep>;
    public:
        ////////////////////////////////////////////////////////////////////////////////
        // types

        using rep = Rep;
        using overflow_tag = OverflowTag;

        ////////////////////////////////////////////////////////////////////////////////
        // functions

        constexpr overflow_integer() = delete;

        template<class RhsRep, class RhsOverflowTag>
        constexpr overflow_integer(overflow_integer<RhsRep, RhsOverflowTag> const& rhs)
                :overflow_integer(_impl::to_rep(rhs))
        {
        }

        template<class Rhs, _impl::enable_if_t<!_integer_impl::is_overflow_int<Rhs>::value, int> dummy = 0>
        constexpr overflow_integer(Rhs const& rhs)
                :_base(convert<rep>(overflow_tag{}, rhs))
        {
        }

        /// constructor taking an integral constant
        template<CNL_IMPL_CONSTANT_VALUE_TYPE Value>
        constexpr overflow_integer(constant<Value>)
                : _base(static_cast<rep>(Value))
        {
            static_assert(Value <= numeric_limits<rep>::max(), "initialization by out-of-range value");
            static_assert(!numeric_limits<decltype(Value)>::is_signed || Value >= numeric_limits<rep>::lowest(), "initialization by out-of-range value");
        }

        template<class T>
        constexpr explicit operator T() const
        {
            return static_cast<T>(_impl::to_rep(*this));
        }
    };

    ////////////////////////////////////////////////////////////////////////////////
    // cnl::_impl::set_rep<overflow_integer<>>

    namespace _impl {
        template<class Rep, class OverflowTag>
        struct get_rep<overflow_integer<Rep, OverflowTag>> {
            using type = Rep;
        };

        template<class OldRep, class OverflowTag, class NewRep>
        struct set_rep<overflow_integer<OldRep, OverflowTag>, NewRep> {
            using type = overflow_integer<NewRep, OverflowTag>;
        };
    }

    ////////////////////////////////////////////////////////////////////////////////
    // numeric traits

    template<class Rep, class OverflowTag>
    struct digits<overflow_integer<Rep, OverflowTag>> : digits<Rep> {
    };

    template<class Rep, class OverflowTag, _digits_type MinNumBits>
    struct set_digits<overflow_integer<Rep, OverflowTag>, MinNumBits> {
        using type = overflow_integer<set_digits_t<Rep, MinNumBits>, OverflowTag>;
    };

    template<class Rep, class OverflowTag, class Value>
    struct from_value<overflow_integer<Rep, OverflowTag>, Value> {
        using type = overflow_integer<Value, OverflowTag>;
    };

    template<class Rep, class OverflowTag>
    struct scale<overflow_integer<Rep, OverflowTag>> {
        using value_type = overflow_integer<Rep, OverflowTag>;
        constexpr auto operator()(value_type const& i, int base, int exp) const
        -> decltype(_impl::to_rep(i) * _num_traits_impl::pow<value_type>(base, exp)) {
            return (exp < 0)
                   ? _impl::to_rep(i) / _num_traits_impl::pow<value_type>(base, -exp)
                   : _impl::to_rep(i) * _num_traits_impl::pow<value_type>(base, exp);
        }
    };

    ////////////////////////////////////////////////////////////////////////////////
    // cnl::make_overflow_int

    template<class OverflowTag, class Rep>
    constexpr auto make_overflow_int(Rep const& value)
    -> overflow_integer<Rep, OverflowTag>
    {
        return value;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // binary arithmetic

    namespace _impl {
        ////////////////////////////////////////////////////////////////////////////////
        // arithmetc

        // for arithmetic operands with a common overflow tag
        template<class Operator, class Rep, class OverflowTag>
        struct unary_operator<Operator, overflow_integer<Rep, OverflowTag>> {
            constexpr auto operator()(overflow_integer<Rep, OverflowTag> const& operand) const
            -> decltype(overflow_integer<decltype(Operator()(to_rep(operand))), OverflowTag>(Operator()(to_rep(operand))))
            {
                return overflow_integer<decltype(Operator()(to_rep(operand))), OverflowTag>(Operator()(to_rep(operand)));
            }
        };

        // for arithmetic operands with a common overflow tag
        template<class OperatorTag, class LhsRep, class RhsRep, class OverflowTag>
        struct binary_operator<OperatorTag, overflow_integer<LhsRep, OverflowTag>, overflow_integer<RhsRep, OverflowTag>> {
            constexpr auto operator()(
                    overflow_integer<LhsRep, OverflowTag> const& lhs,
                    overflow_integer<RhsRep, OverflowTag> const& rhs) const
            -> decltype(make_overflow_int<OverflowTag>(_overflow_impl::binary_operator<OverflowTag, OperatorTag>()(cnl::_impl::to_rep(lhs), cnl::_impl::to_rep(rhs))))
            {
                return make_overflow_int<OverflowTag>(
                        _overflow_impl::binary_operator<OverflowTag, OperatorTag>()(cnl::_impl::to_rep(lhs), cnl::_impl::to_rep(rhs)));
            }
        };

        // for arithmetic operands with different policies
        template<class OperatorTag, class LhsRep, class LhsTag, class RhsRep, class RhsTag>
        struct binary_operator<OperatorTag, overflow_integer<LhsRep, LhsTag>, overflow_integer<RhsRep, RhsTag>> {
            constexpr auto operator()(
                    const overflow_integer<LhsRep, LhsTag>& lhs,
                    const overflow_integer<RhsRep, RhsTag>& rhs) const
            -> decltype(binary_operator<OperatorTag, overflow_integer<LhsRep, cnl::_impl::common_type_t<LhsTag, RhsTag>>, overflow_integer<LhsRep, cnl::_impl::common_type_t<LhsTag, RhsTag>>>()(lhs, rhs))
            {
                using common_tag = cnl::_impl::common_type_t<LhsTag, RhsTag>;
                return binary_operator<OperatorTag, overflow_integer<LhsRep, common_tag>, overflow_integer<LhsRep, common_tag>>()(lhs, rhs);
            }
        };

        // for comparison operands with a common overflow tag
        template<class OperatorTag, class LhsRep, class RhsRep, class OverflowTag>
        struct comparison_operator<OperatorTag,
                overflow_integer<LhsRep, OverflowTag>, overflow_integer<RhsRep, OverflowTag>> {
            constexpr auto operator()(
                    overflow_integer<LhsRep, OverflowTag> const& lhs,
                    overflow_integer<RhsRep, OverflowTag> const& rhs) const
            -> decltype(_overflow_impl::comparison_operator<OverflowTag, OperatorTag>()(cnl::_impl::to_rep(lhs), cnl::_impl::to_rep(rhs)))
            {
                return _overflow_impl::comparison_operator<OverflowTag, OperatorTag>()(cnl::_impl::to_rep(lhs), cnl::_impl::to_rep(rhs));
            }
        };

        // for arithmetic operands with different policies
        template<class OperatorTag, class LhsRep, class LhsTag, class RhsRep, class RhsTag>
        struct comparison_operator<OperatorTag, overflow_integer<LhsRep, LhsTag>, overflow_integer<RhsRep, RhsTag>> {
            constexpr auto operator()(
                    const overflow_integer<LhsRep, LhsTag>& lhs,
                    const overflow_integer<RhsRep, RhsTag>& rhs) const
            -> decltype(comparison_operator<OperatorTag, overflow_integer<LhsRep, cnl::_impl::common_type_t<LhsTag, RhsTag>>, overflow_integer<LhsRep, cnl::_impl::common_type_t<LhsTag, RhsTag>>>()(lhs, rhs))
            {
                using common_tag = cnl::_impl::common_type_t<LhsTag, RhsTag>;
                return comparison_operator<OperatorTag, overflow_integer<LhsRep, common_tag>, overflow_integer<LhsRep, common_tag>>()(lhs, rhs);
            }
        };
    }
}

namespace cnl {
    ////////////////////////////////////////////////////////////////////////////////
    // std::numeric_limits specialization for overflow_integer

    template<class Rep, class OverflowTag>
    struct numeric_limits<cnl::overflow_integer<Rep, OverflowTag>>
            : numeric_limits<cnl::_impl::number_base<cnl::overflow_integer<Rep, OverflowTag>, Rep>> {
        static constexpr bool is_integer = true;
    };

    template<class Rep, class OverflowTag>
    struct numeric_limits<cnl::overflow_integer<Rep, OverflowTag> const>
            : numeric_limits<cnl::_impl::number_base<cnl::overflow_integer<Rep, OverflowTag>, Rep>> {
        static constexpr bool is_integer = true;
    };
}

namespace std {
    // std::common_type<T, cnl::overflow_integer>
    template<
            class Lhs,
            class RhsRep, class RhsOverflowTag>
    struct common_type<
            Lhs,
            cnl::overflow_integer<RhsRep, RhsOverflowTag>>
            : cnl::_integer_impl::common_type<
                    Lhs,
                    cnl::overflow_integer<RhsRep, RhsOverflowTag>> {
    };

    // std::common_type<cnl::overflow_integer, T>
    template<
            class LhsRep, class LhsOverflowTag,
            class Rhs>
    struct common_type<
            cnl::overflow_integer<LhsRep, LhsOverflowTag>,
            Rhs>
            : cnl::_integer_impl::common_type<
                    cnl::overflow_integer<LhsRep, LhsOverflowTag>,
                    Rhs> {
    };

    // std::common_type<cnl::overflow_integer, cnl::fixed_point>
    template<
            class LhsRep, class LhsOverflowTag,
            class RhsRep, int RhsExponent>
    struct common_type<
            cnl::overflow_integer<LhsRep, LhsOverflowTag>,
            cnl::fixed_point<RhsRep, RhsExponent>>
            : std::common_type<
                    cnl::fixed_point<cnl::overflow_integer<LhsRep, LhsOverflowTag>, 0>,
                    cnl::fixed_point<RhsRep, RhsExponent>> {
    };

    // std::common_type<cnl::fixed_point, cnl::overflow_integer>
    template<
            class LhsRep, int LhsExponent,
            class RhsRep, class RhsOverflowTag>
    struct common_type<
            cnl::fixed_point<LhsRep, LhsExponent>,
            cnl::overflow_integer<RhsRep, RhsOverflowTag>>
            : std::common_type<
                    cnl::fixed_point<LhsRep, LhsExponent>,
                    cnl::fixed_point<cnl::overflow_integer<RhsRep, RhsOverflowTag>, 0>> {
    };
    
    // std::common_type<cnl::overflow, cnl::overflow>
    template<
            class LhsRep, class LhsOverflowTag,
            class RhsRep, class RhsOverflowTag>
    struct common_type<
            cnl::overflow_integer<LhsRep, LhsOverflowTag>,
            cnl::overflow_integer<RhsRep, RhsOverflowTag>>
            : cnl::_integer_impl::common_type<
                    cnl::overflow_integer<LhsRep, LhsOverflowTag>,
                    cnl::overflow_integer<RhsRep, RhsOverflowTag>> {
    };

    ////////////////////////////////////////////////////////////////////////////////
    // std::numeric_limits specialization for overflow_integer

    template<class Rep, class OverflowTag>
    struct numeric_limits<cnl::overflow_integer<Rep, OverflowTag>>
            : cnl::numeric_limits<cnl::overflow_integer<Rep, OverflowTag>> {
    };

    template<class Rep, class OverflowTag>
    struct numeric_limits<cnl::overflow_integer<Rep, OverflowTag> const>
            : cnl::numeric_limits<cnl::overflow_integer<Rep, OverflowTag>> {
    };
}

#endif  // CNL_OVERFLOW_INT_H