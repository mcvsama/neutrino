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


static TestRegistration tests ([]{
	describe ("Neutrino", "math", "Quaternion", []{
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
