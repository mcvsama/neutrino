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


static TestRegistration tests ([]{
	describe ("Neutrino", "math", "Quaternion", []{
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
