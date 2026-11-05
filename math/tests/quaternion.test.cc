/* vim:ts=4
 *
 * Copyleft 2026  Michał Gawron
 * Marduk Unix Labs, http://mulabs.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Visit http://www.gnu.org/licenses/gpl-3.0.html for more information on licensing.
 */

// Neutrino:
#include <neutrino/math/math.h>
#include <neutrino/test/test_asserts.h>
#include <neutrino/test/test_suite.h>

// Standard:
#include <array>
#include <concepts>
#include <cstddef>
#include <limits>
#include <type_traits>


namespace neutrino::test {
namespace {

using Quaternion = math::Quaternion<double>;
using Rotation = math::Quaternion<double, void, void, true>;


template<class Q>
	concept AddAssignable = requires (Q& a, Q const& b) { a += b; };

template<class Q>
	concept SubtractAssignable = requires (Q& a, Q const& b) { a -= b; };

template<class Q>
	concept ScalarMultiplyAssignable = requires (Q& a) { a *= 2.0; };

template<class Q>
	concept ScalarDivideAssignable = requires (Q& a) { a /= 2.0; };

template<class A, class B>
	concept HasDotProduct = requires (A const& a, B const& b) { math::dot_product (a, b); };

template<class Q, class Factor>
	concept MultiplyAssignable = requires (Q& q, Factor const& factor) { q *= factor; };

template<class Q, class Factor>
	concept DivideAssignable = requires (Q& q, Factor const& factor) { q /= factor; };

template<class Q>
	concept InvertibleInPlace = requires (Q& q) { q.invert(); };

template<class A, class B>
	concept HasProduct = requires (A const& a, B const& b) { a * b; };

template<class A, class B>
	concept HasQuotient = requires (A const& a, B const& b) { a / b; };

template<class A, class B>
	concept HasSum = requires (A const& a, B const& b) { a + b; };

template<class A, class B>
	concept HasDifference = requires (A const& a, B const& b) { a - b; };

template<class Target, class Q>
	concept HasComponentCast = requires (Q const& q) { math::static_components_cast<Target> (q); };

template<class Target, class Q>
	concept HasNarrowCast = requires (Q const& q) { math::narrow_cast<Target> (q); };


template<si::UnitConcept Unit, std::floating_point Value>
	static void
	verify_physical_inverse()
	{
		using Quantity = si::Quantity<Unit, Value>;
		using InverseQuantity = decltype (1 / Quantity (1));
		struct Target: math::CoordinateSystemBase { };
		struct Source: math::CoordinateSystemBase { };
		using Q = math::Quaternion<Quantity, Target, Source>;
		using Inverse = math::Quaternion<InverseQuantity, Source, Target>;
		auto const epsilon = Value (64) * std::numeric_limits<Value>::epsilon();

		for (auto const scale: {
			Value (1), 
			std::numeric_limits<Value>::max(),
			std::numeric_limits<Value>::max() / Value (8),
			std::numeric_limits<Value>::min(),
			std::numeric_limits<Value>::min() / Value (8),
		}) {
			Q const original { Quantity (scale), Quantity (-scale), Quantity (scale), Quantity (-scale) };
			auto const inverse = original.inverted();
			static_assert (std::is_same_v<decltype (inverse), Inverse const>);
			static_assert (std::is_same_v<typename Q::InverseScalar, InverseQuantity>);
			auto const expected = InverseQuantity (Value (0.25) / scale);
			std::array<InverseQuantity, 4> const expected_components { expected, expected, -expected, expected };

			for (std::size_t i = 0; i < expected_components.size(); ++i)
			{
				test_asserts::verify_equal_with_epsilon ("physical inverse components have the expected relative magnitude",
														 inverse.components()[i] / expected_components[i], Value (1), epsilon);
			}

			math::Quaternion<Value, Target, Target> const target_identity (math::identity);
			math::Quaternion<Value, Source, Source> const source_identity (math::identity);
			static_assert (std::is_same_v<decltype (original * inverse), std::remove_const_t<decltype (target_identity)>>);
			static_assert (std::is_same_v<decltype (inverse * original), std::remove_const_t<decltype (source_identity)>>);
			test_asserts::verify_equal_with_epsilon ("physical right inverse gives dimensionless identity", original * inverse, target_identity, epsilon);
			test_asserts::verify_equal_with_epsilon ("physical left inverse gives dimensionless identity", inverse * original, source_identity, epsilon);
			test_asserts::verify_equal_with_epsilon ("physical self-division gives dimensionless identity", original / original, target_identity, epsilon);
		}
	}


template<math::Scalar S, bool IsRotation>
	static void
	verify_extreme_normalization()
	{
		using Value = si::decay_quantity_t<S>;
		struct Target: math::CoordinateSystemBase { };
		struct Source: math::CoordinateSystemBase { };
		using Q = math::Quaternion<S, Target, Source, IsRotation>;
		auto const epsilon = S (Value (32) * std::numeric_limits<Value>::epsilon());
		auto const half = S (Value (0.5));
		auto const diagonal = S (Value (1) / std::sqrt (Value (2)));
		std::array<Q, 3> const expected {{
			{ half, -half, half, -half },
			{ S (0), diagonal, -diagonal, S (0) },
			{ S (0), S (0), S (0), S (-1) },
		}};

		for (auto const scale: {
			std::numeric_limits<Value>::max(),
			std::numeric_limits<Value>::min(),
			std::numeric_limits<Value>::min() / Value (8),
			std::numeric_limits<Value>::denorm_min(),
		}) {
			std::array<Q, 3> const originals {{
				{ S (scale), S (-scale), S (scale), S (-scale) },
				{ S (0), S (scale), S (-scale), S (0) },
				{ S (0), S (0), S (0), S (-scale) },
			}};

			for (std::size_t i = 0; i < originals.size(); ++i)
			{
				auto quaternion = originals[i];
				auto const normalized = quaternion.normalized();

				static_assert (std::is_same_v<decltype (normalized), Q const>);
				test_asserts::verify_equal ("normalizing a copy preserves the source", quaternion, originals[i]);
				test_asserts::verify_equal_with_epsilon ("extreme components normalize to the expected values", normalized, expected[i], epsilon);
				test_asserts::verify_equal_with_epsilon ("normalized copy has unit norm", normalized.norm(), S (1), epsilon);

				quaternion.normalize();
				test_asserts::verify_equal_with_epsilon ("in-place normalization gives the expected components", quaternion, expected[i], epsilon);
				test_asserts::verify_equal_with_epsilon ("in-place normalization gives unit norm", quaternion.norm(), S (1), epsilon);
			}
		}
	}


template<math::Scalar S>
	static void
	verify_extreme_division()
	{
		using Value = si::decay_quantity_t<S>;
		struct Target: math::CoordinateSystemBase { };
		struct Source: math::CoordinateSystemBase { };
		struct Intermediate: math::CoordinateSystemBase { };
		using Numerator = math::Quaternion<S, Target, Intermediate>;
		using Divisor = math::Quaternion<S, Source, Intermediate>;
		using Quotient = math::Quaternion<Value, Target, Source>;
		using ScalarQuotient = math::Quaternion<Value, Intermediate, Source>;
		auto const epsilon = Value (64) * std::numeric_limits<Value>::epsilon();

		for (auto const scale: {
			std::numeric_limits<Value>::denorm_min(),
			std::numeric_limits<Value>::denorm_min() * Value (8),
			std::numeric_limits<Value>::min() / Value (8),
			std::numeric_limits<Value>::min(),
			std::numeric_limits<Value>::max() / Value (8),
			std::numeric_limits<Value>::max()
		}) {
			Numerator const a { S (scale), S (-scale), S (scale), S (-scale) };
			Divisor const b { S (scale), S (scale), S (-scale), S (-scale) };
			static_assert (std::is_same_v<decltype (a / b), Quotient>);
			static_assert (std::is_same_v<decltype (S (scale) / b), ScalarQuotient>);
			test_asserts::verify_equal_with_epsilon ("extreme division preserves composition order and coordinate tags", a / b,
													 Quotient (Value (0), Value (0), Value (1), Value (0)), epsilon);
			test_asserts::verify_equal_with_epsilon ("extreme scalar division cancels the common magnitude", S (scale) / b,
													 ScalarQuotient (Value (0.25), Value (-0.25), Value (0.25), Value (0.25)), epsilon);

			math::Quaternion<S> q { S (scale), S (0), S (0), S (0) };
			math::Quaternion<Value> const identity (math::identity);
			test_asserts::verify_equal ("real self-division does not require a representable inverse", q / q, identity);
			test_asserts::verify_equal ("real scalar division does not require a representable inverse", S (scale) / q, identity);

			if constexpr (std::floating_point<S>)
			{
				q /= q;
				test_asserts::verify_equal ("aliased compound division handles the smallest subnormal", q, identity);
			}
		}
	}


static TestRegistration tests ([]{
	describe ("Neutrino", "math", "Quaternion", []{
		it ("default-constructs rotations as identity", []{
			auto const verify_type = []<std::floating_point Value> {
				struct Target: math::CoordinateSystemBase { };
				struct Source: math::CoordinateSystemBase { };
				using Q = math::Quaternion<Value, Target, Source, true>;
				using Inverse = math::Quaternion<Value, Source, Target, true>;
				using Matrix = math::SquareMatrix<Value, 3, Target, Source>;
				constexpr Q q;
				constexpr Q value_initialized {};
				constexpr std::array<Q, 2> rotations {};
				constexpr Q identity (math::identity);
				static_assert (q == identity && value_initialized == identity);
				static_assert (rotations[0] == identity && rotations[1] == identity);
				static_assert (q == Q (math::unit));
				static_assert (q.inverted() == Inverse (math::identity));
				static_assert (Matrix (q) == Matrix (math::identity));
				static_assert (!std::is_constructible_v<Q, math::ZeroInitializer>);

				test_asserts::verify_equal ("default rotation has unit norm", q.norm(), Value (1));
				test_asserts::verify_equal ("default rotation produces the identity matrix", Matrix (q), Matrix (math::identity));
				math::Vector<Value, 3, Source> const vector { Value (2), Value (-3), Value (4) };
				math::Vector<Value, 3, Target> const expected { Value (2), Value (-3), Value (4) };
				test_asserts::verify_equal ("default rotation preserves vector components", q * vector, expected);
				math::Quaternion<Value, Source, Source, true> const delta { Value (0.5), Value (-0.5), Value (0.5), Value (0.5) };
				test_asserts::verify_equal ("default rotation is neutral under composition", q * delta,
											Q (delta.w(), delta.x(), delta.y(), delta.z()));

				math::Quaternion<Value, void, void, true> untagged;
				test_asserts::verify ("untagged default rotation has identity components", untagged.components() == identity.components());
			};

			verify_type.operator()<float>();
			verify_type.operator()<double>();
			verify_type.operator()<long double>();
		});

		it ("retains zero initialization for general quaternions", []{
			auto const verify_type = []<math::Scalar Scalar> {
				using Q = math::Quaternion<Scalar>;
				constexpr Q q;
				constexpr Q value_initialized {};
				constexpr Q zero (math::zero);
				static_assert (std::is_constructible_v<Q, math::ZeroInitializer>);
				static_assert (q == zero && value_initialized == zero);
				static_assert (zero == Q (Scalar (0), Scalar (0), Scalar (0), Scalar (0)));
				test_asserts::verify_equal ("general default quaternion has zero norm", q.norm(), Scalar (0));
			};

			verify_type.operator()<float>();
			verify_type.operator()<double>();
			verify_type.operator()<long double>();
			verify_type.operator()<si::Length>();
		});

		it ("copies quaternion components and preserves the unit-norm flag when changing coordinate tags", []{
			auto const verify_type = []<std::floating_point Value, bool IsRotation> {
				struct OldTarget: math::CoordinateSystemBase { };
				struct OldSource: math::CoordinateSystemBase { };
				struct NewTarget: math::CoordinateSystemBase { };
				struct NewSource: math::CoordinateSystemBase { };
				using Original = math::Quaternion<Value, OldTarget, OldSource, IsRotation>;
				using Result = math::Quaternion<Value, NewTarget, NewSource, IsRotation>;

				constexpr auto component = IsRotation ? Value (0.5) : Value (1);
				constexpr Original original { component, component, -component, -component };
				constexpr auto explicit_tags = math::coordinate_system_cast<NewTarget, NewSource, OldTarget, OldSource> (original);
				constexpr auto deduced_tags = math::coordinate_system_cast_any<NewTarget, NewSource> (original);

				static_assert (std::is_same_v<decltype (explicit_tags), Result const>);
				static_assert (std::is_same_v<decltype (deduced_tags), Result const>);
				static_assert (explicit_tags.components() == original.components());
				static_assert (deduced_tags == explicit_tags);

				auto source = original;
				auto copy = math::coordinate_system_cast<NewTarget, NewSource, OldTarget, OldSource> (source);
				auto deduced_copy = math::coordinate_system_cast_any<NewTarget, NewSource> (source);

				static_assert (std::is_same_v<decltype (copy), Result>);
				static_assert (std::is_same_v<decltype (deduced_copy), Result>);

				copy.w() = -copy.w();
				deduced_copy.w() = -deduced_copy.w();
				test_asserts::verify_equal ("changing either quaternion copy leaves the source unchanged", source, original);

				source.x() = -source.x();
				test_asserts::verify_equal ("explicit-tag quaternion copy has independent storage", copy.x(), component);
				test_asserts::verify_equal ("deduced-tag quaternion copy has independent storage", deduced_copy.x(), component);
			};

			verify_type.operator()<float, false>();
			verify_type.operator()<double, false>();
			verify_type.operator()<float, true>();
			verify_type.operator()<double, true>();
		});

		it ("preserves physical quaternion units when copying to different coordinate tags", []{
			auto const verify_type = []<std::floating_point Value> {
				struct Space: math::CoordinateSystemBase { };
				using Length = si::Quantity<si::Meter, Value>;

				constexpr math::Quaternion<Length> original { Length (1), Length (2), Length (3), Length (4) };
				constexpr auto explicit_tags = math::coordinate_system_cast<Space, Space, void, void> (original);
				constexpr auto deduced_tags = math::coordinate_system_cast_any<Space, Space> (original);

				static_assert (std::is_same_v<decltype (explicit_tags), math::Quaternion<Length, Space> const>);
				static_assert (std::is_same_v<decltype (deduced_tags), math::Quaternion<Length, Space> const>);
				static_assert (explicit_tags.components() == original.components());

				test_asserts::verify_equal ("explicit and deduced coordinate tags preserve physical components", explicit_tags, deduced_tags);
			};

			verify_type.operator()<float>();
			verify_type.operator()<double>();
		});

		it ("keeps coordinate-cast quaternion results alive after temporary sources expire", []{
			struct Space: math::CoordinateSystemBase { };
			auto const& explicit_tags = math::coordinate_system_cast<Space, Space, void, void> (Quaternion { 1.0, 2.0, 3.0, 4.0 });
			auto const& deduced_tags = math::coordinate_system_cast_any<Space, Space> (Rotation { math::identity });

			test_asserts::verify_equal ("temporary general quaternion's scalar component is copied", explicit_tags.w(), 1.0);
			test_asserts::verify_equal ("temporary general quaternion's imaginary component is copied", explicit_tags.z(), 4.0);
			test_asserts::verify_equal ("temporary rotation quaternion is copied", deduced_tags.w(), 1.0);
			test_asserts::verify_equal ("temporary rotation quaternion retains its unit norm", deduced_tags.norm(), 1.0);
		});

		it ("deduces quaternion dot products and computes all four component products", []{
			auto const verify_type = []<std::floating_point Value> {
				constexpr math::Quaternion<Value> a { Value (1), Value (2), Value (3), Value (4) };
				constexpr math::Quaternion<Value> b { Value (5), Value (6), Value (7), Value (8) };
				static_assert (std::is_same_v<decltype (math::dot_product (a, b)), Value>);
				static_assert (math::dot_product (a, b) == Value (70));
				static_assert (noexcept (math::dot_product (a, b)));

				test_asserts::verify_equal ("dot product sums all four component products", math::dot_product (a, b), Value (70));
				test_asserts::verify_equal ("dot product is symmetric", math::dot_product (b, a), Value (70));
				test_asserts::verify_equal ("self dot product is the squared norm", math::dot_product (a, a), Value (30));
			};

			verify_type.operator()<float>();
			verify_type.operator()<double>();
			verify_type.operator()<long double>();
			constexpr math::Quaternion<float> a { 1.0f, 2.0f, 3.0f, 4.0f };
			constexpr Quaternion b { 5.0, 6.0, 7.0, 8.0 };
			static_assert (std::is_same_v<decltype (math::dot_product (a, b)), double>);
			static_assert (math::dot_product (a, b) == 70.0);
			test_asserts::verify_equal ("mixed scalar types retain the promoted result type", math::dot_product (a, b), 70.0);
		});

		it ("widens quaternion components through direct and implicit conversion", []{
			using Float = math::Quaternion<float>;
			using Double = math::Quaternion<double>;
			using LongDouble = math::Quaternion<long double>;

			static_assert (std::is_convertible_v<Float, Double>);
			static_assert (std::is_constructible_v<Double, Float>);
			static_assert (std::is_convertible_v<Double, LongDouble>);
			static_assert (!std::is_convertible_v<Double, Float>);
			static_assert (!std::is_constructible_v<Float, Double>);
			static_assert (!std::is_assignable_v<Float&, Double>);

			constexpr Float original { 1.25f, -2.5f, 3.75f, -4.0f };
			constexpr Double direct { original };
			constexpr Double implicit = original;
			constexpr auto cast = math::static_components_cast<double> (original);
			constexpr Double expected { 1.25, -2.5, 3.75, -4.0 };

			static_assert (direct == expected && implicit == expected && cast == expected);
			static_assert (std::same_as<Float::Retyped<double>, Double>);

			Double assigned;
			assigned = original;
			test_asserts::verify_equal ("widening assignment preserves every component", assigned, expected);
		});

		it ("requires explicit component conversion for mixed addition and subtraction", []{
			using Float = math::Quaternion<float>;
			using Double = math::Quaternion<double>;
			using FloatRotation = math::Quaternion<float, void, void, true>;

			static_assert (!HasSum<Float, Double> && !HasSum<Double, Float>);
			static_assert (!HasDifference<Float, Double> && !HasDifference<Double, Float>);
			static_assert (!HasSum<FloatRotation, Rotation> && !HasDifference<FloatRotation, Rotation>);
			static_assert (HasSum<Double, Rotation> && HasSum<Rotation, Double>);
			static_assert (HasDifference<Double, Rotation> && HasDifference<Rotation, Double>);

			constexpr Float a { 1.0f, 2.0f, 3.0f, 4.0f };
			constexpr Double b { 5.0, 6.0, 7.0, 8.0 };
			constexpr auto sum = Double { a } + b;
			constexpr auto difference = b - Double { a };

			static_assert (sum == Double { 6.0, 8.0, 10.0, 12.0 });
			static_assert (difference == Double { 4.0, 4.0, 4.0, 4.0 });
			test_asserts::verify_equal ("explicit widening enables addition", sum, Double { 6.0, 8.0, 10.0, 12.0 });
			test_asserts::verify_equal ("explicit widening enables subtraction", difference, Double { 4.0, 4.0, 4.0, 4.0 });
		});

		it ("offers explicit narrowing by component type and by quaternion type", []{
			using Float = math::Quaternion<float>;

			constexpr Quaternion original { 1.0 + 0x1p-30, -2.0 - 0x1p-30, 3.5, -4.25 };
			constexpr Float expected { 1.0f, -2.0f, 3.5f, -4.25f };
			constexpr auto by_component = math::narrow_cast<float> (original);
			constexpr auto by_quaternion = math::narrow_cast<Float> (original);
			constexpr auto cast = math::static_components_cast<float> (original);

			static_assert (std::same_as<decltype (by_component), Float const>);
			static_assert (std::same_as<decltype (by_quaternion), Float const>);
			static_assert (by_component == expected && by_quaternion == expected && cast == expected);
			static_assert (!HasComponentCast<int, Quaternion>);
			static_assert (!HasNarrowCast<int, Quaternion>);
			test_asserts::verify_equal ("ordinary quaternions retain their magnitude during narrowing", by_component, expected);
		});

		it ("preserves coordinate tags and rotation flags during component conversion", []{
			struct Target: math::CoordinateSystemBase { };
			struct Source: math::CoordinateSystemBase { };

			using Float = math::Quaternion<float, Target, Source>;
			using Double = math::Quaternion<double, Target, Source>;
			using FloatRotation = math::Quaternion<float, Target, Source, true>;
			using DoubleRotation = math::Quaternion<double, Target, Source, true>;
			using Reversed = math::Quaternion<double, Source, Target>;

			static_assert (std::is_convertible_v<Float, Double>);
			static_assert (std::is_convertible_v<FloatRotation, DoubleRotation>);
			static_assert (!std::is_convertible_v<Float, Reversed>);
			static_assert (!std::is_constructible_v<Reversed, Float>);
			static_assert (!std::is_convertible_v<Float, DoubleRotation>);
			static_assert (!std::is_constructible_v<DoubleRotation, Float>);
			static_assert (!std::is_constructible_v<FloatRotation, DoubleRotation>);
			static_assert (!HasNarrowCast<FloatRotation, Double>);
			static_assert (!HasNarrowCast<Reversed, Float>);
			static_assert (!HasNarrowCast<math::Quaternion<float>, Double>);
			static_assert (!HasNarrowCast<math::Matrix<float, 1, 4, Target, Source>, Double>);

			constexpr FloatRotation original { 0.5f, -0.5f, 0.5f, -0.5f };
			constexpr DoubleRotation widened = original;
			constexpr auto narrowed = math::narrow_cast<FloatRotation> (widened);
			static_assert (narrowed == original);
			static_assert (std::same_as<decltype (math::static_components_cast<double> (original)), DoubleRotation>);
			test_asserts::verify_equal ("rotation conversion preserves its orientation", narrowed, original);

			// Widening must preserve even components too small to affect the stored norm.
			constexpr FloatRotation small { 1.0f, 0x1p-100f, 0.0f, 0.0f };
			constexpr DoubleRotation precise { small };
			static_assert (precise.w() == 1.0 && precise.x() == 0x1p-100);
		});

		it ("renormalizes unit quaternions after narrowing", []{
			using FloatRotation = math::Quaternion<float, void, void, true>;

			// A unit rotation whose rounded float components have a squared norm below one.
			Rotation const original { 0.60664059157134187, 0.0063060067383209567, -0.58022523741867793, -0.5434023378670978 };
			auto const by_component = math::narrow_cast<float> (original);
			auto const by_quaternion = math::narrow_cast<FloatRotation> (original);
			auto const cast = math::static_components_cast<float> (original);

			static_assert (std::same_as<decltype (by_component), FloatRotation const>);
			test_asserts::verify_equal ("both narrowing overloads agree", by_component, by_quaternion);
			test_asserts::verify_equal ("component cast follows the same normalization policy", by_component, cast);
			test_asserts::verify_equal_with_epsilon ("narrowing corrects rounding-induced norm drift", by_component.squared_norm(), 1.0f, 0.5f * std::numeric_limits<float>::epsilon());

			math::Vector<double, 3> const vector { 1.0, 2.0, 3.0 };
			test_asserts::verify_equal_with_epsilon ("narrowing preserves the represented rotation", by_component * vector, original * vector, 1e-6);
		});

		it ("uses scalar conversion rules for SI quaternion components", []{
			using Meter = si::Quantity<si::Meter>;
			using Kilometer = si::Quantity<si::Kilometer>;
			using Meters = math::Quaternion<Meter>;
			using Kilometers = math::Quaternion<Kilometer>;

			static_assert (std::is_convertible_v<Kilometers, Meters>);
			static_assert (!HasComponentCast<si::Time, Meters>);
			static_assert (!HasNarrowCast<math::Quaternion<si::Time>, Meters>);
			static_assert (!HasSum<Meters, Kilometers> && !HasDifference<Meters, Kilometers>);

			constexpr Kilometers original { Kilometer (1), Kilometer (-2), Kilometer (3), Kilometer (-4) };
			constexpr Meters converted { original };
			constexpr auto cast = math::static_components_cast<Meter> (original);
			static_assert (converted.w() == Meter (1000) && converted.x() == Meter (-2000));
			static_assert (converted.y() == Meter (3000) && converted.z() == Meter (-4000));
			static_assert (cast == converted);
			test_asserts::verify_equal ("explicit SI conversion retains physical components", math::narrow_cast<Kilometers> (converted), original);
		});

		it ("retains the stored unit-norm guarantee when changing physical component units", []{
			using MeterRotation = math::Quaternion<si::Quantity<si::Meter>, void, void, true>;
			using KilometerRotation = math::Quaternion<si::Quantity<si::Kilometer>, void, void, true>;
			using Meter = MeterRotation::Scalar;
			using Kilometer = KilometerRotation::Scalar;
			MeterRotation const original { Meter (0.5), Meter (-0.5), Meter (0.5), Meter (-0.5) };
			KilometerRotation const converted = original;
			test_asserts::verify_equal ("changing stored units preserves a rotation's stored unit norm", converted.norm(), Kilometer (1));
			test_asserts::verify_equal ("unit conversion keeps the represented orientation", converted.w(), Kilometer (0.5));
		});

		it ("supports dot products of rotation and general quaternions", []{
			constexpr Rotation rotation { 0.5, 0.5, 0.5, 0.5 };
			constexpr Quaternion general { 1.0, 2.0, 3.0, 4.0 };

			static_assert (math::dot_product (rotation, rotation) == 1.0);
			static_assert (math::dot_product (rotation, general) == 5.0);
			static_assert (math::dot_product (general, rotation) == 5.0);
			test_asserts::verify_equal ("rotation self dot product is one", math::dot_product (rotation, rotation), 1.0);
			test_asserts::verify_equal ("mixed rotation flags are deduced", math::dot_product (rotation, general), 5.0);
			test_asserts::verify_equal ("opposite rotation components have negative dot product", math::dot_product (rotation, -rotation), -1.0);
		});

		it ("preserves physical dimensions and requires matching coordinate tags for dot products", []{
			using namespace si::literals;
			struct Target: math::CoordinateSystemBase { };
			struct Source: math::CoordinateSystemBase { };
			using LengthQuaternion = math::Quaternion<si::Length, Target, Source>;
			using TimeQuaternion = math::Quaternion<si::Time, Target, Source>;
			static_assert (HasDotProduct<LengthQuaternion, TimeQuaternion>);
			static_assert (!HasDotProduct<LengthQuaternion, math::Quaternion<si::Length, Source, Target>>);
			static_assert (!HasDotProduct<LengthQuaternion, math::Quaternion<si::Length, Target, Target>>);
			constexpr LengthQuaternion a { 1_m, 2_m, 3_m, 4_m };
			constexpr LengthQuaternion b { 5_m, 6_m, 7_m, 8_m };
			constexpr TimeQuaternion times { 5_s, 6_s, 7_s, 8_s };
			constexpr auto area = math::dot_product (a, b);
			constexpr auto length_time = math::dot_product (a, times);
			static_assert (std::is_same_v<decltype (area), si::Area const>);
			static_assert (std::is_same_v<decltype (length_time), decltype (1_m * 1_s) const>);
			static_assert (area == 70_m * 1_m);
			static_assert (length_time == 70_m * 1_s);

			test_asserts::verify_equal ("length dot product has area units", area, 70_m * 1_m);
			test_asserts::verify_equal ("mixed physical dimensions are retained", length_time, 70_m * 1_s);
		});

		it ("compares equal floating-point components at compile time and runtime", []{
			auto const verify_type = []<std::floating_point Value> {
				constexpr math::Quaternion<Value> a { Value (1), Value (2), Value (3), Value (4) };
				constexpr math::Quaternion<Value> b { Value (1), Value (2), Value (3), Value (4) };
				static_assert (a == b);
				static_assert (!(a != b));

				test_asserts::verify ("equal components compare equal", a == b);
				test_asserts::verify ("inequality is false for equal components", !(a != b));
			};

			verify_type.operator()<float>();
			verify_type.operator()<double>();
			verify_type.operator()<long double>();
		});

		it ("detects a difference in each of the four quaternion components", []{
			Quaternion const original { 1.0, 2.0, 3.0, 4.0 };

			for (std::size_t index = 0; index < original.components().size(); ++index)
			{
				auto components = original.components();
				components[index] += 1.0;
				Quaternion const changed (components);

				test_asserts::verify ("changed component makes equality false", !(original == changed));
				test_asserts::verify ("changed component makes inequality true", original != changed);
				test_asserts::verify ("equality is symmetric", !(changed == original));
				test_asserts::verify ("inequality is symmetric", changed != original);
			}
		});

		it ("compares physical-quantity components with coordinate-system tags", []{
			using namespace si::literals;
			struct Target: math::CoordinateSystemBase { };
			struct Source: math::CoordinateSystemBase { };
			using PhysicalQuaternion = math::Quaternion<si::Length, Target, Source>;
			constexpr PhysicalQuaternion a { 1_m, 2_m, 3_m, 4_m };
			constexpr PhysicalQuaternion b { 1_m, 2_m, 3_m, 4_m };
			constexpr PhysicalQuaternion different { 1_m, 2_m, 3_m, 5_m };
			static_assert (a == b);
			static_assert (!(a != b));
			static_assert (!(a == different));
			static_assert (a != different);

			test_asserts::verify ("equal physical components compare equal", a == b);
			test_asserts::verify ("different physical components compare unequal", a != different);
		});

		it ("compares rotation quaternion components rather than represented rotations", []{
			constexpr Rotation q { 0.5, 0.5, 0.5, 0.5 };
			constexpr Rotation same { 0.5, 0.5, 0.5, 0.5 };
			constexpr auto opposite = -q;
			static_assert (q == same);
			static_assert (!(q != same));
			static_assert (!(q == opposite));
			static_assert (q != opposite);

			test_asserts::verify ("identical rotation components compare equal", q == same);
			test_asserts::verify ("opposite components compare unequal", q != opposite);
		});

		it ("preserves floating-point equality rules for signed zero and NaN", []{
			Quaternion const zero { 0.0, 0.0, 0.0, 0.0 };
			Quaternion const negative_zero { -0.0, -0.0, -0.0, -0.0 };
			Quaternion const with_nan { 1.0, std::numeric_limits<double>::quiet_NaN(), 3.0, 4.0 };

			test_asserts::verify ("signed zeros compare equal", zero == negative_zero);
			test_asserts::verify ("signed zeros do not compare unequal", !(zero != negative_zero));
			test_asserts::verify ("a NaN component prevents equality even with itself", !(with_nan == with_nan));
			test_asserts::verify ("a NaN component makes inequality true even with itself", with_nan != with_nan);
		});

		it ("discards the unit-norm guarantee for addition, subtraction and scalar arithmetic", []{
			constexpr Rotation q { 0.5, 0.5, 0.5, 0.5 };

			static_assert (std::is_same_v<decltype (q + q), Quaternion>);
			static_assert (std::is_same_v<decltype (q - q), Quaternion>);
			static_assert (std::is_same_v<decltype (q * 2.0), Quaternion>);
			static_assert (std::is_same_v<decltype (2.0 * q), Quaternion>);
			static_assert (std::is_same_v<decltype (q / 2.0), Quaternion>);
			static_assert (std::is_same_v<decltype (2.0 / q), Quaternion>);
			static_assert (std::is_same_v<decltype (q + Quaternion (q)), Quaternion>);
			static_assert (std::is_same_v<decltype (Quaternion (q) - q), Quaternion>);

			test_asserts::verify_equal ("addition doubles the norm", (q + q).norm(), 2.0);
			test_asserts::verify_equal ("self-subtraction has zero norm", (q - q).norm(), 0.0);
		});

		it ("deduces component types for mixed floating-point and integer factors", []{
			constexpr math::Quaternion<float> q { 1.0f, 2.0f, 3.0f, 4.0f };
			constexpr Quaternion doubled { 2.0, 4.0, 6.0, 8.0 };
			constexpr Quaternion halved { 0.5, 1.0, 1.5, 2.0 };
			static_assert (std::is_same_v<decltype (q * 2.0), Quaternion>);
			static_assert (std::is_same_v<decltype (2.0 * q), Quaternion>);
			static_assert (std::is_same_v<decltype (q / 2.0), Quaternion>);
			static_assert (std::is_same_v<decltype (2.0 / q), Quaternion>);
			static_assert (std::is_same_v<decltype (q * 2), math::Quaternion<float>>);
			static_assert (std::is_same_v<decltype (2 * q), math::Quaternion<float>>);
			static_assert (std::is_same_v<decltype (q / 2), math::Quaternion<float>>);
			static_assert (std::is_same_v<decltype (2 / q), math::Quaternion<float>>);
			static_assert (q * 2.0 == doubled && 2.0 * q == doubled && q / 2.0 == halved);
			static_assert ((q * 2).w() == 2.0f && (q / 2).z() == 2.0f);
			test_asserts::verify_equal_with_epsilon ("mixed scalar quotient agrees with inverse multiplication", 2.0 / q, 2.0 * q.inverted(), 1e-7);

			struct NotAScalar { };
			static_assert (!HasProduct<Quaternion, NotAScalar> && !HasProduct<NotAScalar, Quaternion>);
			static_assert (!HasQuotient<Quaternion, NotAScalar> && !HasQuotient<NotAScalar, Quaternion>);
		});

		it ("derives quaternion product and quotient units from their components", []{
			using namespace si::literals;
			struct Target: math::CoordinateSystemBase { };
			struct Source: math::CoordinateSystemBase { };
			using LengthQ = math::Quaternion<si::Length, Target, Source>;
			constexpr LengthQ q { 1_m, 2_m, 3_m, 4_m };
			constexpr auto scaled = q * 2;
			constexpr auto area = q * 2_m;
			constexpr auto dimensionless = q / 2_m;
			constexpr auto inverse = q.inverted();
			using InverseLength = decltype (1 / 1_m);
			static_assert (std::is_same_v<decltype (scaled), LengthQ const>);
			static_assert (std::is_same_v<decltype (q * 2.0), LengthQ>);
			static_assert (std::is_same_v<decltype (2.0 * q), LengthQ>);
			static_assert (std::is_same_v<decltype (q / 2), LengthQ>);
			static_assert (std::is_same_v<decltype (area), math::Quaternion<si::Area, Target, Source> const>);
			static_assert (std::is_same_v<decltype (dimensionless), math::Quaternion<double, Target, Source> const>);
			static_assert (std::is_same_v<decltype (inverse), math::Quaternion<InverseLength, Source, Target> const>);
			static_assert (std::is_same_v<decltype (2_m / q), math::Quaternion<double, Source, Target>>);
			static_assert (std::is_same_v<decltype (2 / q), math::Quaternion<InverseLength, Source, Target>>);
			static_assert (scaled.w() == 2_m && area.z() == 8_m * 1_m && dimensionless.x() == 1.0);
			static_assert (q / 2 == q / 2.0 && 2 * q == q * 2);
			test_asserts::verify_equal_with_epsilon ("division converts kilometer factors", (q / 2_km).w(), 0.0005, 1e-15);
			test_asserts::verify_equal ("left physical multiplication preserves result units", 2_m * q, area);
			test_asserts::verify_equal_with_epsilon ("a physical numerator cancels inverse units", 2_m / q, inverse * 2_m, 1e-14);
			test_asserts::verify_equal_with_epsilon ("mixed dimensions retain their product units", (q * 2_s).z(), 8_m * 1_s, 1e-14_m * 1_s);

			math::Vector<si::Time, 3, Source> const vector { -2_s, 3_s, 5_s };
			math::Quaternion<double, Target, Source> const dimensionless_q { 1.0, 2.0, 3.0, 4.0 };
			static_assert (std::is_same_v<decltype (q * vector), math::Vector<si::Time, 3, Target>>);
			test_asserts::verify_equal_with_epsilon ("physical quaternion units cancel when rotating a vector", q * vector, dimensionless_q * vector, 1e-12_s);

			constexpr math::Quaternion<double, Target, Source, true> rotation { 0.5, 0.5, 0.5, 0.5 };
			static_assert (std::is_same_v<decltype (rotation * 2_m), LengthQ>);
			static_assert (std::is_same_v<decltype (rotation / 2_m), math::Quaternion<InverseLength, Target, Source>>);
			static_assert (std::is_same_v<decltype (2_m / rotation), math::Quaternion<si::Length, Source, Target>>);

			using FloatLengthQ = math::Quaternion<si::Quantity<si::Meter, float>>;
			static_assert (!HasProduct<FloatLengthQ, si::Time> && !HasQuotient<FloatLengthQ, si::Time>);
		});

		it ("permits compound scalar operations only when result units can be assigned back", []{
			using namespace si::literals;
			using LengthQ = math::Quaternion<si::Length>;
			static_assert (MultiplyAssignable<LengthQ, double> && MultiplyAssignable<LengthQ, int>);
			static_assert (DivideAssignable<LengthQ, double> && DivideAssignable<LengthQ, int>);
			static_assert (!MultiplyAssignable<LengthQ, si::Length> && !DivideAssignable<LengthQ, si::Length>);
			static_assert (!MultiplyAssignable<LengthQ, LengthQ> && !DivideAssignable<LengthQ, LengthQ>);
			static_assert (!MultiplyAssignable<Rotation, int> && !DivideAssignable<Rotation, int>);
			static_assert (!InvertibleInPlace<LengthQ>);
			static_assert (InvertibleInPlace<Quaternion> && InvertibleInPlace<Rotation>);
			struct Target: math::CoordinateSystemBase { };
			struct Source: math::CoordinateSystemBase { };
			static_assert (!InvertibleInPlace<math::Quaternion<double, Target, Source>>);
			LengthQ const original { 1_m, 2_m, 3_m, 4_m };
			auto q = original;
			q *= 2;
			q /= 2.0;
			test_asserts::verify_equal ("compound scaling preserves physical components", q, original);

			Quaternion multiply_alias { 2.0, 3.0, 4.0, 5.0 };
			multiply_alias *= multiply_alias.w();
			test_asserts::verify_equal ("compound multiplication reads an aliased factor before assigning", multiply_alias, Quaternion { 4.0, 6.0, 8.0, 10.0 });
			Quaternion divide_alias { 2.0, 4.0, 6.0, 8.0 };
			divide_alias /= divide_alias.w();
			test_asserts::verify_equal ("compound division reads an aliased divisor before assigning", divide_alias, Quaternion { 1.0, 2.0, 3.0, 4.0 });
		});

		it ("inverts physical quaternion units at extreme magnitudes", []{
			verify_physical_inverse<si::Meter, float>();
			verify_physical_inverse<si::Meter, double>();
			verify_physical_inverse<si::Meter, long double>();
			verify_physical_inverse<si::Kilometer, float>();
			verify_physical_inverse<si::Kilometer, double>();
			verify_physical_inverse<si::Kilometer, long double>();
		});

		it ("preserves physical inverse values across unit scales", []{
			using namespace si::literals;
			math::Quaternion<si::Length> const meters { 1_m, 2_m, 3_m, 4_m };
			using Kilometers = si::Quantity<si::Kilometer>;
			math::Quaternion<Kilometers> const kilometers { Kilometers (0.001), Kilometers (0.002), Kilometers (0.003), Kilometers (0.004) };
			auto const meter_inverse = meters.inverted();
			auto const kilometer_inverse = kilometers.inverted();
			using InverseLength = decltype (1 / 1_m);

			for (std::size_t i = 0; i < 4; ++i)
			{
				test_asserts::verify_equal_with_epsilon ("inverse values are independent of stored unit scales",
														 meter_inverse.components()[i], InverseLength (kilometer_inverse.components()[i]), InverseLength (1e-14));
			}

			test_asserts::verify_equal_with_epsilon ("mixed-unit quaternion and inverse give identity", meters * kilometer_inverse, Quaternion (math::identity), 1e-14);
		});

		it ("assigns compatible scaled result units without discarding dimensions", []{
			using namespace si::literals;
			using Percent = si::Quantity<si::ScaledUnit<si::Dimensionless, std::ratio<1, 100>>>;
			using LengthQ = math::Quaternion<si::Length>;
			static_assert (MultiplyAssignable<LengthQ, Percent> && DivideAssignable<LengthQ, Percent>);
			LengthQ const original { 1_m, 2_m, 3_m, 4_m };
			auto length = original;
			length *= Percent (50);
			test_asserts::verify_equal ("compound scaling converts the resulting length unit", length, original / 2);
			length /= Percent (50);
			test_asserts::verify_equal ("compound division converts the resulting length unit", length, original);

			using PercentQ = math::Quaternion<Percent>;
			static_assert (InvertibleInPlace<PercentQ>);
			PercentQ q { Percent (50), Percent (50), Percent (50), Percent (50) };
			auto const inverse = q.inverted();
			static_assert (std::is_same_v<typename decltype (inverse)::Scalar, decltype (1 / Percent (1))>);
			q.invert();
			test_asserts::verify_equal ("in-place inversion converts inverse dimensionless unit scales", q.w(), Percent (50));
			test_asserts::verify_equal ("in-place inversion retains inverse signs", q.x(), Percent (-50));
			test_asserts::verify_equal_with_epsilon ("dimensionless inverse has the correct physical value", q.w() / Percent (inverse.w()), 1.0, 1e-14);
		});

		it ("correctly inverts results of norm-changing arithmetic on rotations", []{
			Rotation const q { 0.5, 0.5, 0.5, 0.5 };
			std::array<double, 4> const identity { 1.0, 0.0, 0.0, 0.0 };

			for (auto result: { q + q, q - (-q), q * 2.0, 2.0 * q, q / 2.0, 2.0 / q })
			{
				auto const inverse = result.inverted();
				test_asserts::verify ("right inverse gives identity", (result * inverse).components() == identity);
				test_asserts::verify ("left inverse gives identity", (inverse * result).components() == identity);
				result.invert();
				test_asserts::verify ("in-place inversion agrees", result.components() == inverse.components());
			}
		});

		it ("automatically divides ordinary values and values around the fast-path range boundaries", []{
			auto const verify_type = []<std::floating_point Value> {
				using Q = math::Quaternion<Value>;
				auto const epsilon = Value (64) * std::numeric_limits<Value>::epsilon();
				auto const lower = std::ldexp (Value (1), std::numeric_limits<Value>::min_exponent / 8);
				auto const upper = std::ldexp (Value (1), std::numeric_limits<Value>::max_exponent / 8);
				Q const expected = Q (Value (1), Value (23), Value (3), Value (1)) / Value (18);
				Q const scalar_expected = Q (Value (2), Value (1), Value (-3), Value (2)) / Value (9);

				for (auto const scale: {
					Value (1), lower / Value (2), lower,
					std::nextafter (lower, Value (0)),
					std::nextafter (lower, std::numeric_limits<Value>::infinity()),
					upper / Value (4),
					std::nextafter (upper / Value (4), std::numeric_limits<Value>::infinity()),
					upper / Value (2),
				}) {
					Q const a { scale, Value (2) * scale, Value (3) * scale, Value (4) * scale };
					Q const b { Value (2) * scale, -scale, Value (3) * scale, Value (-2) * scale };
					test_asserts::verify_equal_with_epsilon ("automatic division is continuous across the range boundaries",
															 a / b, expected, epsilon);
					test_asserts::verify_equal_with_epsilon ("automatic scalar division is continuous across the range boundaries",
															 Value (2) * scale / b, scalar_expected, epsilon);
					auto compound = a;
					compound /= b;
					test_asserts::verify_equal_with_epsilon ("compound division uses automatic dispatch", compound, expected, epsilon);
				}
			};

			verify_type.operator()<float>();
			verify_type.operator()<double>();
			verify_type.operator()<long double>();
			constexpr Quaternion a { 1.0, 2.0, 3.0, 4.0 };
			constexpr Quaternion b { 2.0, -1.0, 3.0, -2.0 };
			static_assert ((a / b).w() == 1.0 / 18.0);
			static_assert ((2.0 / b).w() == 2.0 / 9.0);
		});

		it ("retries strongly cancelling ordinary-range sums with the scaled algorithm", []{
			auto const verify_type = []<std::floating_point Value> {
				using Q = math::Quaternion<Value>;
				auto const large = std::ldexp (Value (1), std::numeric_limits<Value>::max_exponent / 8 - 2);
				auto const small = std::ldexp (Value (1), std::numeric_limits<Value>::min_exponent / 8 + 1);
				Q const a { large, small, -large, Value (0) };
				Q const b { Value (1), Value (1), Value (1), Value (0) };
				auto const epsilon = Value (64) * std::numeric_limits<Value>::epsilon();
				test_asserts::verify_equal_with_epsilon ("small nonzero components survive strong cancellation",
														 (a / b).w() / (small / Value (3)), Value (1), epsilon);
			};

			verify_type.operator()<float>();
			verify_type.operator()<double>();
			verify_type.operator()<long double>();
		});

		it ("accounts for mixed types and unit conversions in automatic division", []{
			using FloatLength = si::Quantity<si::Meter, float>;
			using Micrometers = si::Quantity<si::ScaledUnit<si::Meter, std::ratio<1, 1000000>>, float>;
			math::Quaternion<FloatLength> const meters { FloatLength (1), FloatLength (2), FloatLength (3), FloatLength (4) };
			math::Quaternion<Micrometers> const micrometers { Micrometers (2), Micrometers (-1), Micrometers (3), Micrometers (-2) };
			math::Quaternion<float> const expected = math::Quaternion<float> (1.0f, 23.0f, 3.0f, 1.0f) / 18.0f * 1000000.0f;
			test_asserts::verify_equal_with_epsilon ("large unit conversions select the scaled path",
													 meters / micrometers, expected, 1.0f);
			test_asserts::verify_equal_with_epsilon ("scalar division also handles large unit conversions",
													 (FloatLength (2) / micrometers).w(), 2000000.0f / 9.0f, 1.0f);

			math::Quaternion<float> const a { 1.0f, 2.0f, 3.0f, 4.0f };
			Quaternion const b { 2.0, -1.0, 3.0, -2.0 };
			static_assert (std::is_same_v<decltype (a / b), Quaternion>);
			test_asserts::verify_equal_with_epsilon ("mixed ordinary inputs retain the promoted type and values",
													 a / b, Quaternion (1.0, 23.0, 3.0, 1.0) / 18.0, 1e-14);
		});

		it ("divides finite quaternions even when their inverses overflow", []{
			verify_extreme_division<float>();
			verify_extreme_division<double>();
			verify_extreme_division<long double>();
			verify_extreme_division<si::Quantity<si::Meter, float>>();
			verify_extreme_division<si::Quantity<si::Meter, double>>();
			verify_extreme_division<si::Quantity<si::Meter, long double>>();
			verify_extreme_division<si::Quantity<si::Kilometer, double>>();
		});

		it ("retains small quotient components across widely separated magnitudes", []{
			auto const verify_type = []<std::floating_point Value> {
				using Q = math::Quaternion<Value>;
				auto const epsilon = Value (64) * std::numeric_limits<Value>::epsilon();
				auto const large = std::scalbn (Value (1), std::numeric_limits<Value>::max_exponent / 2 - 4);
				auto const small = std::numeric_limits<Value>::min() / Value (8);
				auto const divisor = Value (1) / large;
				Q const numerator { large, -large, small, Value (0) };
				Q const real_divisor { divisor, Value (0), Value (0), Value (0) };
				auto const quotient = numerator / real_divisor;
				test_asserts::verify_equal ("large quotient components remain finite", quotient.w(), large / divisor);
				test_asserts::verify_equal ("small numerator components survive division", quotient.y(), small / divisor);

				Q const cancelling_divisor { divisor, divisor, divisor, Value (0) };
				auto const cancelled = numerator / cancelling_divisor;
				test_asserts::verify_equal_with_epsilon ("small terms survive cancellation of large products",
					cancelled.w() / ((small / divisor) / Value (3)), Value (1), epsilon);

				auto const exponent = std::numeric_limits<Value>::max_exponent;
				auto const a = std::scalbn (Value (1), exponent - 4);
				auto const b = std::scalbn (Value (1), exponent / 4);
				auto const tiny = std::scalbn (Value (1), -exponent + 4);
				auto const result = Q (a, Value (0), Value (0), Value (0)) / Q (b, tiny, Value (0), Value (0));
				test_asserts::verify_equal ("small divisor components contribute to representable quotient components",
					result.x(), -std::scalbn (Value (1), -2 * (exponent / 4)));
			};

			verify_type.operator()<float>();
			verify_type.operator()<double>();
			verify_type.operator()<long double>();
		});

		it ("preserves mixed scalar types, units and constexpr evaluation in scaled division", []{
			constexpr auto tiny = std::numeric_limits<double>::denorm_min();
			constexpr Quaternion q { tiny, tiny, -tiny, -tiny };
			static_assert (q / q == Quaternion (math::identity));
			static_assert (tiny / q == Quaternion (0.25, -0.25, 0.25, 0.25));
			constexpr auto huge = std::numeric_limits<double>::max();
			constexpr Quaternion max { huge, huge, -huge, -huge };
			static_assert (max / max == Quaternion (math::identity));

			auto const float_tiny = std::numeric_limits<float>::denorm_min();
			math::Quaternion<float> const float_q { float_tiny, 0.0f, 0.0f, 0.0f };
			Quaternion const double_q { double (float_tiny), 0.0, 0.0, 0.0 };
			static_assert (std::is_same_v<decltype (double_q / float_q), Quaternion>);
			test_asserts::verify_equal ("mixed scalar quotient avoids an overflowing float inverse", double_q / float_q, Quaternion (math::identity));
			test_asserts::verify_equal ("mixed scalar numerator avoids an overflowing float inverse", double (float_tiny) / float_q, Quaternion (math::identity));

			using Kilometers = si::Quantity<si::Kilometer>;
			using Speed = decltype (si::Length (1) / si::Time (1));
			math::Quaternion<si::Length> const meters { si::Length (tiny), si::Length (0), si::Length (0), si::Length (0) };
			math::Quaternion<Kilometers> const kilometers { Kilometers (tiny), Kilometers (0), Kilometers (0), Kilometers (0) };
			math::Quaternion<si::Time> const seconds { si::Time (tiny), si::Time (0), si::Time (0), si::Time (0) };
			static_assert (std::is_same_v<decltype (meters / seconds), math::Quaternion<Speed>>);
			test_asserts::verify_equal_with_epsilon ("dimensionless quotients include the unit-scale conversion", (meters / kilometers).w(), 0.001, 1e-17);
			test_asserts::verify_equal_with_epsilon ("reverse unit conversion remains finite", (kilometers / meters).w(), 1000.0, 1e-11);
			test_asserts::verify_equal ("quotients of different dimensions retain their unit", (meters / seconds).w(), Speed (1));
			test_asserts::verify_equal ("physical scalar numerators retain the quotient unit", (si::Length (tiny) / seconds).w(), Speed (1));
		});

		it ("preserves division behavior for zero and non-finite arguments", []{
			Quaternion const zero (math::zero);
			Quaternion const finite { 1.0, 2.0, 3.0, 4.0 };
			test_asserts::verify_equal ("zero divided by a finite quaternion stays zero", zero / finite, zero);

			for (auto const& result: {
				finite / zero,
				1 / zero,
				finite / Quaternion (std::numeric_limits<double>::infinity(), 0.0, 0.0, 0.0),
				Quaternion (std::numeric_limits<double>::quiet_NaN(), 0.0, 0.0, 0.0) / finite,
			}) {
				for (auto const component: result.components())
					test_asserts::verify ("undefined quotients retain NaN components", std::isnan (component));
			}
		});

		it ("inverts finite quaternions whose component squares overflow or underflow", []{
			auto const verify_type = []<std::floating_point Value> {
				using Q = math::Quaternion<Value>;
				auto const epsilon = Value (64) * std::numeric_limits<Value>::epsilon();
				Q const identity (math::identity);

				for (auto const scale: {
					std::numeric_limits<Value>::max(),
					std::numeric_limits<Value>::max() / Value (8),
					std::numeric_limits<Value>::min() * Value (8),
					std::numeric_limits<Value>::min(),
					std::numeric_limits<Value>::min() / Value (8)
				}) {
					Q const original { scale, -scale, scale, -scale };
					auto const inverse = original.inverted();
					auto const expected = Value (0.25) / scale;
					std::array<Value, 4> const expected_components { expected, expected, -expected, expected };

					for (std::size_t i = 0; i < expected_components.size(); ++i)
					{
						test_asserts::verify_equal_with_epsilon ("inverse components have the expected relative magnitude",
																 inverse.components()[i] / expected_components[i], Value (1), epsilon);
					}

					test_asserts::verify_equal_with_epsilon ("right inverse gives identity", original * inverse, identity, epsilon);
					test_asserts::verify_equal_with_epsilon ("left inverse gives identity", inverse * original, identity, epsilon);
					test_asserts::verify_equal_with_epsilon ("quaternion division gives identity", original / original, identity, epsilon);

					auto in_place = original;
					in_place.invert();
					test_asserts::verify_equal ("in-place inversion agrees", in_place, inverse);
				}
			};

			verify_type.operator()<float>();
			verify_type.operator()<double>();
			verify_type.operator()<long double>();
		});

		it ("preserves small inverse components when rescaling a small quaternion", []{
			auto const tiny = std::numeric_limits<double>::denorm_min();
			Quaternion const original { 0.5, 0.5, 0.5, tiny };
			auto const inverse = original.inverted();
			test_asserts::verify_equal ("representable subnormal inverse component survives", inverse.z(), -tiny);

			for (auto const scale: { 1e200, 1e-200 })
			{
				Quaternion const real { scale, 0.0, 0.0, 0.0 };
				test_asserts::verify_equal_with_epsilon ("real quaternion inverse has the expected magnitude",
														 real.inverted().w() / (1.0 / scale), 1.0, 1e-14);

				math::Vector<double, 3> const vector { 1.0, 2.0, 3.0 };
				test_asserts::verify_equal_with_epsilon ("real quaternion leaves vectors unchanged", real * vector, vector, 1e-14);
			}
		});

		it ("inverts asymmetric quaternions at extreme scales", []{
			auto const verify_type = []<std::floating_point Value> {
				using Q = math::Quaternion<Value>;
				auto const epsilon = Value (64) * std::numeric_limits<Value>::epsilon();

				for (auto const scale: { std::numeric_limits<Value>::max(), std::numeric_limits<Value>::min() / Value (2) })
				{
					Q const original { scale, -scale / Value (2), scale / Value (4), -scale / Value (8) };
					auto const inverse = original.inverted();
					// 1 + (1/2)^2 + (1/4)^2 + (1/8)^2 = 85/64.
					auto const expected = (Value (64) / Value (85)) / scale;
					std::array<Value, 4> const expected_components { expected, expected / Value (2), -expected / Value (4), expected / Value (8) };

					for (std::size_t i = 0; i < expected_components.size(); ++i)
					{
						test_asserts::verify_equal_with_epsilon ("asymmetric inverse has the expected relative components",
																 inverse.components()[i] / expected_components[i], Value (1), epsilon);
					}

					test_asserts::verify_equal_with_epsilon ("asymmetric right inverse gives identity", original * inverse, Q (math::identity), epsilon);
					test_asserts::verify_equal_with_epsilon ("asymmetric left inverse gives identity", inverse * original, Q (math::identity), epsilon);
				}
			};

			verify_type.operator()<float>();
			verify_type.operator()<double>();
			verify_type.operator()<long double>();
		});

		it ("preserves inverse coordinate tags and constexpr evaluation", []{
			struct Target: math::CoordinateSystemBase { };
			struct Source: math::CoordinateSystemBase { };
			using TaggedQuaternion = math::Quaternion<double, Target, Source>;

			constexpr TaggedQuaternion q { 0.5, 0.5, -0.5, -0.5 };
			constexpr auto inverse = q.inverted();

			static_assert (std::is_same_v<decltype (inverse), math::Quaternion<double, Source, Target> const>);
			static_assert ((q * inverse).w() == 1.0);
			static_assert ((inverse * q).w() == 1.0);
		});

		it ("preserves the unit-norm guarantee for unary operations, composition and inversion", []{
			Rotation q { 0.5, 0.5, 0.5, 0.5 };
			static_assert (std::is_same_v<decltype (+q), Rotation>);
			static_assert (std::is_same_v<decltype (-q), Rotation>);
			static_assert (std::is_same_v<decltype (~q), Rotation>);
			static_assert (std::is_same_v<decltype (q.conjugated()), Rotation>);
			static_assert (std::is_same_v<decltype (q.inverted()), Rotation>);
			static_assert (std::is_same_v<decltype (q * q), Rotation>);
			static_assert (std::is_same_v<decltype (q / q), Rotation>);
			static_assert (std::is_same_v<decltype (q * Quaternion (q)), Quaternion>);
			static_assert (std::is_same_v<decltype (Quaternion (q) / q), Quaternion>);
			auto const original = q;
			q *= original;
			q /= original;
			test_asserts::verify ("composition and division cancel", q.components() == original.components());
			q.invert();
			test_asserts::verify ("rotation inverse is its conjugate", q.components() == original.conjugated().components());
		});

		it ("right-composes tagged quaternions in place within their source space", []{
			struct World: math::CoordinateSystemBase { };
			struct Body: math::CoordinateSystemBase { };
			using Orientation = math::Quaternion<double, World, Body, true>;
			using BodyRotation = math::Quaternion<float, Body, Body, true>;
			using WorldRotation = math::Quaternion<double, World, World, true>;
			using General = math::Quaternion<double, World, Body>;
			using BodyQuaternion = math::Quaternion<double, Body, Body>;

			static_assert (MultiplyAssignable<Orientation, BodyRotation> && DivideAssignable<Orientation, BodyRotation>);
			static_assert (!MultiplyAssignable<Orientation, Orientation> && !DivideAssignable<Orientation, Orientation>);
			static_assert (!MultiplyAssignable<Orientation, WorldRotation> && !DivideAssignable<Orientation, WorldRotation>);
			static_assert (!MultiplyAssignable<Orientation, BodyQuaternion> && !DivideAssignable<Orientation, BodyQuaternion>);
			static_assert (MultiplyAssignable<General, BodyRotation> && DivideAssignable<General, BodyRotation>);
			static_assert (MultiplyAssignable<General, BodyQuaternion> && DivideAssignable<General, BodyQuaternion>);

			constexpr Orientation original { 0.0, 1.0, 0.0, 0.0 };
			constexpr BodyRotation delta { 0.0f, 0.0f, 1.0f, 0.0f };
			constexpr auto product = [=]{ auto q = original; q *= delta; return q; }();
			constexpr auto quotient = [=]{ auto q = original; q /= delta; return q; }();
			static_assert (product == Orientation (0.0, 0.0, 0.0, 1.0));
			static_assert (quotient == Orientation (0.0, 0.0, 0.0, -1.0));
			static_assert (product == original * delta && quotient == original / delta);

			auto orientation = original;
			static_assert (std::is_same_v<decltype (orientation *= delta), Orientation&>);
			static_assert (std::is_same_v<decltype (orientation /= delta), Orientation&>);
			test_asserts::verify ("compound multiplication returns the original object", &(orientation *= delta) == &orientation);
			test_asserts::verify_equal ("body rotation composes on the right", orientation, product);
			test_asserts::verify ("compound division returns the original object", &(orientation /= delta) == &orientation);
			test_asserts::verify_equal ("division undoes the body rotation", orientation, original);

			General general (original);
			general *= delta;
			general /= delta;
			test_asserts::verify_equal ("general quaternions accept rotation operands", general, General (original));

			General const initial { 1.0, 2.0, 3.0, 4.0 };
			BodyQuaternion const factor { 2.0, -1.0, 3.0, -2.0 };
			general = initial;
			general *= factor;
			test_asserts::verify_equal ("general multiplication preserves composition order", general, General (3.0, -15.0, 9.0, 15.0));
			general = initial;
			general /= factor;
			test_asserts::verify_equal_with_epsilon ("general division uses the full inverse", general, General (1.0, 23.0, 3.0, 1.0) / 18, 1e-14);
		});

		it ("preserves physical units in tagged compound quaternion operations", []{
			struct World: math::CoordinateSystemBase { };
			struct Body: math::CoordinateSystemBase { };
			using LengthQ = math::Quaternion<si::Length, World, Body>;
			using BodyLengthQ = math::Quaternion<si::Length, Body, Body>;
			using Percent = si::Quantity<si::ScaledUnit<si::Dimensionless, std::ratio<1, 100>>>;
			using PercentQ = math::Quaternion<Percent, Body, Body>;
			using FloatPercentQ = math::Quaternion<si::Quantity<typename Percent::Unit, float>, Body, Body>;

			static_assert (MultiplyAssignable<LengthQ, PercentQ> && DivideAssignable<LengthQ, PercentQ>);
			static_assert (!MultiplyAssignable<LengthQ, BodyLengthQ> && !DivideAssignable<LengthQ, BodyLengthQ>);
			static_assert (!MultiplyAssignable<LengthQ, FloatPercentQ> && !DivideAssignable<LengthQ, FloatPercentQ>);
			LengthQ const original { si::Length (1), si::Length (2), si::Length (3), si::Length (4) };
			PercentQ const factor { Percent (0), Percent (50), Percent (0), Percent (0) };
			auto q = original;
			q *= factor;
			test_asserts::verify_equal ("product converts its scaled unit back to length", q,
										LengthQ (si::Length (-1), si::Length (0.5), si::Length (2), si::Length (-1.5)));
			q /= factor;
			test_asserts::verify_equal_with_epsilon ("division converts reciprocal scales back to length", q, original, si::Length (1e-14));
		});

		it ("evaluates compound quaternion operations before overwriting an aliased operand", []{
			Quaternion const original { 1.0, 2.0, 3.0, 4.0 };
			auto q = original;
			q *= q;
			test_asserts::verify_equal ("self multiplication uses all original components", q, original * original);
			q = original;
			q /= q;
			test_asserts::verify_equal_with_epsilon ("self division gives identity", q, Quaternion (math::identity), 1e-14);
			Rotation rotation { 0.5, 0.5, 0.5, 0.5 };
			rotation *= rotation;
			rotation /= rotation;
			test_asserts::verify_equal ("self composition preserves rotation semantics", rotation, Rotation (math::identity));
		});

		it ("rejects compound assignments that would invalidate a rotation's unit-norm guarantee", []{
			static_assert (!AddAssignable<Rotation>);
			static_assert (!SubtractAssignable<Rotation>);
			static_assert (!ScalarMultiplyAssignable<Rotation>);
			static_assert (!ScalarDivideAssignable<Rotation>);
			static_assert (AddAssignable<Quaternion>);
			static_assert (SubtractAssignable<Quaternion>);
			static_assert (ScalarMultiplyAssignable<Quaternion>);
			static_assert (ScalarDivideAssignable<Quaternion>);
		});

		it ("allows discarding the unit-norm guarantee without implicitly introducing it", []{
			static_assert (std::is_convertible_v<Rotation, Quaternion>);
			static_assert (!std::is_convertible_v<Quaternion, Rotation>);
			Rotation const q { 0.5, 0.5, 0.5, 0.5 };
			Quaternion result = q;
			result += q;
			result -= q;
			result *= 2.0;
			result /= 2.0;
			test_asserts::verify ("general arithmetic retains the expected components", result.components() == q.components());
		});

		it ("supports normalization while preserving the quaternion type", []{
			Rotation rotation { 1.0, 1.0, 1.0, 1.0 };
			Quaternion general { 1.0, 1.0, 1.0, 1.0 };
			static_assert (std::is_same_v<decltype (rotation.normalized()), Rotation>);
			static_assert (std::is_same_v<decltype (general.normalized()), Quaternion>);
			std::array<double, 4> const expected { 0.5, 0.5, 0.5, 0.5 };
			test_asserts::verify ("normalized rotation is unit length", rotation.normalized().components() == expected);
			test_asserts::verify ("normalized general quaternion is unit length", general.normalized().components() == expected);
			rotation.normalize();
			general.normalize();
			test_asserts::verify ("rotation normalizes in place", rotation.components() == expected);
			test_asserts::verify ("general quaternion normalizes in place", general.components() == expected);
		});

		it ("normalizes finite quaternions with overflowing or subnormal norms", []{
			verify_extreme_normalization<float, false>();
			verify_extreme_normalization<double, false>();
			verify_extreme_normalization<long double, false>();
			verify_extreme_normalization<float, true>();
			verify_extreme_normalization<double, true>();
			verify_extreme_normalization<long double, true>();
		});

		it ("normalizes physical quaternions at extreme stored magnitudes", []{
			verify_extreme_normalization<si::Quantity<si::Meter, float>, false>();
			verify_extreme_normalization<si::Quantity<si::Meter, double>, false>();
			verify_extreme_normalization<si::Quantity<si::Meter, long double>, false>();
			verify_extreme_normalization<si::Quantity<si::Kilometer, float>, false>();
			verify_extreme_normalization<si::Quantity<si::Kilometer, double>, false>();
			verify_extreme_normalization<si::Quantity<si::Kilometer, long double>, false>();
		});

		it ("preserves normalization behavior for zero and non-finite components", []{
			auto const nan = std::numeric_limits<double>::quiet_NaN();
			auto const inf = std::numeric_limits<double>::infinity();

			for (auto const& original: { Quaternion (math::zero), Quaternion { 1.0, nan, 2.0, 3.0 } })
			{
				for (auto const component: original.normalized().components())
					test_asserts::verify ("undefined normalization gives NaN components", std::isnan (component));
			}

			for (auto const& original: { Quaternion { inf, nan, 2.0, -3.0 }, Quaternion { nan, inf, 2.0, -3.0 } })
			{
				auto const normalized = original.normalized();
				test_asserts::verify ("infinite component divided by an infinite norm gives NaN", std::isnan (normalized.w()) && std::isnan (normalized.x()));
				test_asserts::verify_equal ("finite components divided by an infinite norm give zero", normalized.y(), 0.0);
				test_asserts::verify ("negative finite components preserve the zero sign", normalized.z() == 0.0 && std::signbit (normalized.z()));
			}
		});

		it ("normalizes physical quaternions while preserving units, scalar types and coordinate tags", []{
			auto const verify_type = []<si::UnitConcept Unit, std::floating_point Value> {
				using Quantity = si::Quantity<Unit, Value>;
				struct Target: math::CoordinateSystemBase { };
				struct Source: math::CoordinateSystemBase { };
				using PhysicalQuaternion = math::Quaternion<Quantity, Target, Source>;
				auto const epsilon = Quantity (Value (32) * std::numeric_limits<Value>::epsilon());
				std::array<PhysicalQuaternion, 2> const originals {{
					{ Quantity (3), Quantity (4), Quantity (0), Quantity (0) },
					{ Quantity (0), Quantity (0), Quantity (-3), Quantity (-4) },
				}};
				std::array<PhysicalQuaternion, 2> const expected {{
					{ Quantity (Value (0.6)), Quantity (Value (0.8)), Quantity (0), Quantity (0) },
					{ Quantity (0), Quantity (0), Quantity (Value (-0.6)), Quantity (Value (-0.8)) },
				}};

				for (std::size_t index = 0; index < originals.size(); ++index)
				{
					auto quaternion = originals[index];
					auto const normalized = quaternion.normalized();

					static_assert (std::is_same_v<decltype (normalized), PhysicalQuaternion const>);
					static_assert (std::is_same_v<decltype (normalized.norm()), Quantity>);

					test_asserts::verify_equal ("normalized copy leaves the source unchanged", quaternion, originals[index]);
					test_asserts::verify_equal_with_epsilon ("normalized copy has the expected physical components", normalized, expected[index], epsilon);
					test_asserts::verify_equal_with_epsilon ("normalized copy has norm one in its original unit", normalized.norm(), Quantity (1), epsilon);

					quaternion.normalize();
					test_asserts::verify_equal_with_epsilon ("in-place normalization has the expected physical components", quaternion, expected[index], epsilon);
					test_asserts::verify_equal_with_epsilon ("in-place normalization has norm one in its original unit", quaternion.norm(), Quantity (1), epsilon);
				}
			};

			verify_type.operator()<si::Meter, float>();
			verify_type.operator()<si::Meter, double>();
			verify_type.operator()<si::Kilometer, float>();
			verify_type.operator()<si::Kilometer, double>();
		});

		it ("rotates vectors across the floating-point exponent range", []{
			auto const verify = []<std::floating_point Value> {
				using Q = math::Quaternion<Value>;
				using V = math::Vector<Value, 3>;

				auto const epsilon = Value (64) * std::numeric_limits<Value>::epsilon();
				std::array<Value, 5> const scales {
					std::numeric_limits<Value>::denorm_min(),
					std::numeric_limits<Value>::min(),
					Value (1),
					std::numeric_limits<Value>::max() / Value (8),
					std::numeric_limits<Value>::max(),
				};

				for (auto const qs: scales)
				{
					for (auto const vs: scales)
					{
						V const v { vs, -vs, vs };
						std::array<Q, 4> const rotations {{
							{ qs, Value (0), Value (0), Value (0) }, { qs, qs, qs, qs },
							{ Value (0), qs, Value (0), Value (0) }, { qs, qs, Value (0), Value (0) }
						}};
						std::array<V, 4> const expected {{ v, { vs, vs, -vs }, { vs, vs, -vs }, { vs, -vs, -vs } }};

						for (std::size_t r = 0; r < rotations.size(); ++r)
						{
							auto const actual = rotations[r] * v;

							for (std::size_t i = 0; i < 3; ++i)
							{
								test_asserts::verify_equal_with_epsilon ("rotation preserves finite components at extreme magnitudes",
																		 actual[i] / expected[r][i], Value (1), epsilon);
							}
						}
					}
				}
			};

			verify.operator()<float>();
			verify.operator()<double>();
			verify.operator()<long double>();
		});

		it ("preserves small vector components alongside large components", []{
			auto const verify = []<std::floating_point Value, bool UnitRotation> {
				using Q = math::Quaternion<Value, void, void, UnitRotation>;
				using V = math::Vector<Value, 3>;

				auto const large = std::numeric_limits<Value>::max();
				auto const small = std::numeric_limits<Value>::denorm_min();
				V const v { large, small, -small };
				test_asserts::verify_equal ("identity preserves the whole vector", Q (math::identity) * v, v);
				test_asserts::verify_equal ("cyclic rotation preserves tiny components",
											Q { Value (0.5), Value (0.5), Value (0.5), Value (0.5) } * v, V { -small, large, small });
			};

			verify.operator()<float, false>();
			verify.operator()<double, false>();
			verify.operator()<long double, false>();
			verify.operator()<float, true>();
			verify.operator()<double, true>();
			verify.operator()<long double, true>();
		});

		it ("retains contributions from widely separated quaternion components", []{
			using V = math::Vector<double, 3>;
			Quaternion const q { 1e200, 1e-200, 0.0, 0.0 };
			auto const result = q * V { 0.0, 0.0, 1e300 };
			test_asserts::verify_equal ("the x component remains zero", result[0], 0.0);
			test_asserts::verify_equal_with_epsilon ("a tiny rotation has a representable effect on a large vector", result[1] / -2e-100, 1.0, 1e-14);
			test_asserts::verify_equal_with_epsilon ("the z component retains its scale", result[2] / 1e300, 1.0, 1e-14);

			Rotation const unit { 1.0, std::numeric_limits<double>::denorm_min(), 0.0, 0.0 };
			auto const unit_result = unit * V { 0.0, 0.0, 1e300 };
			test_asserts::verify_equal_with_epsilon ("a subnormal unit quaternion component still contributes",
													 unit_result[1] / (-2.0 * std::numeric_limits<double>::denorm_min() * 1e300), 1.0, 1e-14);
		});

		it ("recomputes cancellation inside rotation coefficients", []{
			using V = math::Vector<double, 3>;
			// All inputs fit the fast range, but 1 + x*x - 1 would discard x*x.
			constexpr Quaternion q { 1.0, 0x1p-60, 1.0, 0.0 };
			constexpr auto rotated = q * V { 1.0, 0.0, 0.0 };
			static_assert (rotated[0] == 0x1p-121);
			test_asserts::verify_equal ("small squared term survives cancellation", rotated[0], 0x1p-121);
		});

		it ("rotates correctly on both sides of the fast arithmetic bounds", []{
			auto const verify = []<std::floating_point Value> {
				using Q = math::Quaternion<Value>;
				using V = math::Vector<Value, 3>;

				auto const lower = std::ldexp (Value (1), std::numeric_limits<Value>::min_exponent / 8);
				auto const upper = std::ldexp (Value (1), std::numeric_limits<Value>::max_exponent / 8);
				auto const epsilon = Value (64) * std::numeric_limits<Value>::epsilon();

				for (auto const qs: { lower / Value (2), lower, Value (1), upper / Value (4), upper })
				{
					for (auto const vs: { lower / Value (2), lower, Value (1), upper / Value (4), upper })
					{
						auto const result = Q { qs, Value (2) * qs, Value (3) * qs, Value (4) * qs } * V { vs, Value (2) * vs, Value (3) * vs };
						// The exact rotation of (1, 2, 3) by (1, 2, 3, 4) is (9/5, 2, 13/5).
						V const expected { Value (9) / Value (5), Value (2), Value (13) / Value (5) };

						for (std::size_t i = 0; i < 3; ++i)
							test_asserts::verify_equal_with_epsilon ("both arithmetic paths agree with the exact rotation", result[i] / vs, expected[i], epsilon);
					}
				}
			};

			verify.operator()<float>();
			verify.operator()<double>();
			verify.operator()<long double>();
		});

		it ("preserves rotation result types, physical units and coordinate tags", []{
			struct Target: math::CoordinateSystemBase { };
			struct Source: math::CoordinateSystemBase { };

			auto const verify = []<class SQ, class SV> {
				using Q = math::Quaternion<SQ, Target, Source>;
				using V = math::Vector<SV, 3, Source>;
				using ResultScalar = decltype ((SQ (1) * SV (1)) * (1 / SQ (1)));
				using Result = math::Vector<ResultScalar, 3, Target>;
				using Value = si::decay_quantity_t<SQ>;

				V const v { SV (1), SV (2), SV (3) };
				Result const expected { ResultScalar (SV (3)), ResultScalar (SV (1)), ResultScalar (SV (2)) };

				for (auto const scale: { Value (1), std::numeric_limits<Value>::denorm_min(), std::numeric_limits<Value>::max() })
				{
					auto const result = Q { SQ (scale), SQ (scale), SQ (scale), SQ (scale) } * v;
					static_assert (std::is_same_v<decltype (result), Result const>);

					for (std::size_t i = 0; i < 3; ++i)
						test_asserts::verify_equal_with_epsilon ("unit conversions preserve the physical vector", result[i] / expected[i], 1.0, 1e-6);
				}
			};

			verify.operator()<float, double>();
			verify.operator()<double, float>();
			verify.operator()<double, si::Length>();
			verify.operator()<si::Quantity<si::Kilometer, double>, si::Length>();
			verify.operator()<si::Quantity<si::Meter, float>, double>();
			verify.operator()<si::Length, decltype (1 / si::Quantity<si::Kilometer, double> (1))>();
		});

		it ("applies robust rotation to every matrix column", []{
			struct Target: math::CoordinateSystemBase { };
			struct Intermediate: math::CoordinateSystemBase { };
			struct Source: math::CoordinateSystemBase { };

			using Q = math::Quaternion<double, Target, Intermediate>;
			using M = math::Matrix<double, 2, 3, Intermediate, Source>;
			using Result = math::Matrix<double, 2, 3, Target, Source>;

			M const matrix { 1e300, 1e-300, 2e300, 2e-300, 3e300, 3e-300 };
			Result const expected { 3e300, 3e-300, 1e300, 1e-300, 2e300, 2e-300 };
			auto const result = Q { 1e200, 1e200, 1e200, 1e200 } * matrix;
			static_assert (std::is_same_v<decltype (result), Result const>);

			for (std::size_t i = 0; i < expected.components().size(); ++i)
				test_asserts::verify_equal_with_epsilon ("matrix columns retain their scales", result.components()[i] / expected.components()[i], 1.0, 1e-14);
		});

		it ("preserves rotation behavior for zero and non-finite inputs", []{
			using V = math::Vector<double, 3>;

			auto const nan = std::numeric_limits<double>::quiet_NaN();
			auto const inf = std::numeric_limits<double>::infinity();

			for (auto const q: { Quaternion (math::zero), Quaternion { inf, 0.0, 0.0, 0.0 }, Quaternion { 1.0, nan, 0.0, 0.0 } })
			{
				auto const result = q * V { 1.0, 2.0, 3.0 };

				for (auto const component: result.components())
					test_asserts::verify ("undefined rotations yield NaN", std::isnan (component));
			}

			for (auto const value: { nan, inf })
			{
				auto const result = Quaternion (math::identity) * V { value, 2.0, 3.0 };

				for (auto const component: result.components())
					test_asserts::verify ("non-finite vectors retain Hamilton-product propagation", std::isnan (component));
			}
		});

		it ("constructs unit quaternions from rotation matrices", []{
			for (auto const q: { Rotation (math::identity), Rotation { 0.0, 1.0, 0.0, 0.0 },
								Rotation { 0.0, 0.0, 1.0, 0.0 }, Rotation { 0.0, 0.0, 0.0, 1.0 } })
			{
				math::SquareMatrix<double, 3> const matrix (q);
				Rotation const reconstructed (matrix);
				test_asserts::verify_equal ("reconstructed rotation has unit norm", reconstructed.norm(), 1.0);
				test_asserts::verify ("rotation is preserved", reconstructed.components() == q.components());
			}
		});

		it ("preserves coordinate-system tags when discarding the unit-norm guarantee", []{
			struct Target: math::CoordinateSystemBase { };
			struct Source: math::CoordinateSystemBase { };
			math::Quaternion<double, Target, Source, true> const q { 0.5, 0.5, 0.5, 0.5 };
			using General = math::Quaternion<double, Target, Source>;
			static_assert (std::is_same_v<decltype (q + q), General>);
			static_assert (std::is_same_v<decltype (q - q), General>);
			static_assert (std::is_same_v<decltype (q * 2.0), General>);
			static_assert (std::is_same_v<decltype (q / 2.0), General>);
			auto const doubled = q + q;
			static_assert (std::is_same_v<decltype (doubled.inverted()), math::Quaternion<double, Source, Target>>);
			std::array<double, 4> const identity { 1.0, 0.0, 0.0, 0.0 };
			test_asserts::verify ("tagged inverse gives identity", (doubled * doubled.inverted()).components() == identity);
		});
	});
});

} // namespace
} // namespace neutrino::test
