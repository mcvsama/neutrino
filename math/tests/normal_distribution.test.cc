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
#include <neutrino/math/normal_distribution.h>
#include <neutrino/test/test_asserts.h>
#include <neutrino/test/test_suite.h>

// Standard:
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <random>
#include <stdexcept>
#include <type_traits>


namespace neutrino::test {
namespace {

class CountingGenerator: private std::mt19937
{
  public:
	using std::mt19937::mt19937;
	using std::mt19937::result_type;
	using std::mt19937::min;
	using std::mt19937::max;

	std::size_t calls { 0 };

	result_type
	operator()()
	{
		++calls;
		return std::mt19937::operator()();
	}
};

static_assert (std::uniform_random_bit_generator<CountingGenerator>);


template<class Value>
	static void
	verify_constant_distribution (Value const mean)
	{
		using Number = si::decay_quantity_t<Value>;
		using Distribution = math::NormalDistribution<Value>;

		for (auto const zero: { Number (0), -Number (0) })
		{
			std::array<Distribution, 2> distributions {
				Distribution (mean, Value (zero)),
				Distribution (math::NormalVariable<Value> (mean, Value (zero))),
			};

			for (auto& distribution: distributions)
			{
				CountingGenerator generator (123);
				static_assert (std::is_same_v<decltype (distribution (generator)), Value>);
				static_assert (std::is_same_v<decltype (distribution.mean()), Value>);
				static_assert (std::is_same_v<decltype (distribution.stddev()), Value>);
				test_asserts::verify_equal ("constant distribution retains its exact mean", distribution.mean(), mean);
				test_asserts::verify_equal ("dummy distribution's deviation is not exposed", distribution.stddev(), Value (0));

				for (std::size_t draw = 0; draw < 16; ++draw)
					test_asserts::verify_equal ("zero deviation always produces the exact mean", distribution (generator), mean);

				distribution.reset();
				test_asserts::verify_equal ("reset preserves the constant value", distribution (generator), mean);
				test_asserts::verify_equal ("constant draws and reset do not advance the generator", generator.calls, std::size_t (0));
			}
		}
	}


template<class Value>
	static void
	verify_positive_distribution()
	{
		Value const mean (7.25);
		Value const stddev (0.5);
		math::NormalDistribution<Value> direct (mean, stddev);
		math::NormalDistribution<Value> from_variable { math::NormalVariable<Value> (mean, stddev) };
		std::normal_distribution<> reference (si::value (mean), si::value (stddev));
		CountingGenerator direct_generator (123);
		CountingGenerator variable_generator (123);
		CountingGenerator reference_generator (123);
		test_asserts::verify_equal ("positive distribution retains the mean", direct.mean(), mean);
		test_asserts::verify_equal ("positive distribution retains the deviation", direct.stddev(), stddev);

		for (std::size_t draw = 0; draw < 31; ++draw)
		{
			Value const expected = Value (1) * reference (reference_generator);
			test_asserts::verify_equal ("direct constructor preserves the existing sample sequence", direct (direct_generator), expected);
			test_asserts::verify_equal ("NormalVariable constructor preserves the existing sample sequence", from_variable (variable_generator), expected);
		}

		direct.reset();
		from_variable.reset();
		reference.reset();
		Value const expected = Value (1) * reference (reference_generator);
		test_asserts::verify_equal ("reset matches the underlying distribution", direct (direct_generator), expected);
		test_asserts::verify_equal ("reset also works after construction from NormalVariable", from_variable (variable_generator), expected);
		test_asserts::verify_equal ("direct constructor preserves generator consumption", direct_generator.calls, reference_generator.calls);
		test_asserts::verify_equal ("NormalVariable constructor preserves generator consumption", variable_generator.calls, reference_generator.calls);
		test_asserts::verify ("positive deviation uses the generator", direct_generator.calls > 0);
	}


static TestRegistration tests ([]{
	describe ("Neutrino", "math", "NormalDistribution", []{
		it ("treats zero and negative-zero deviations as constant floating-point distributions", []{
			auto const verify_type = []<class Value> {
				for (auto const mean: { Value (0), Value (7.25), Value (-3.5) })
					verify_constant_distribution (mean);
			};
			verify_type.operator()<float>();
			verify_type.operator()<double>();
			verify_type.operator()<long double>();
		});

		it ("preserves physical units and scalar types for constant distributions", []{
			using namespace si::literals;
			verify_constant_distribution (293.15_K);
			verify_constant_distribution (1000_Pa);
			verify_constant_distribution (si::Quantity<si::Meter, float> (7.25f));
			verify_constant_distribution (si::Quantity<si::Kilometer> (-3.5));
		});

		it ("preserves constant means beyond the precision and range of double", []{
			verify_constant_distribution (std::nextafter (1.0L, 2.0L));
			verify_constant_distribution (std::numeric_limits<long double>::max());
			verify_constant_distribution (-std::numeric_limits<long double>::max());
			verify_constant_distribution (si::Quantity<si::Meter, long double> (std::numeric_limits<long double>::max()));
		});

		it ("preserves positive-deviation samples, reset behavior and generator consumption", []{
			verify_positive_distribution<float>();
			verify_positive_distribution<double>();
			verify_positive_distribution<long double>();
			verify_positive_distribution<si::Temperature>();
			verify_positive_distribution<si::Pressure>();
			verify_positive_distribution<si::Quantity<si::Meter, float>>();
			verify_positive_distribution<si::Quantity<si::Kilometer>>();
		});

		it ("copies cached samples and both constant and random distribution states", []{
			using Distribution = math::NormalDistribution<double>;
			Distribution source (2.0, 0.5);
			std::mt19937 generator (123);
			static_cast<void> (source (generator));
			Distribution copied (source);
			Distribution assigned (99.0, 0.0);
			assigned = source;
			auto copied_generator = generator;
			auto assigned_generator = generator;

			for (std::size_t draw = 0; draw < 8; ++draw)
			{
				auto const expected = source (generator);
				test_asserts::verify_equal ("copy construction preserves distribution state", copied (copied_generator), expected);
				test_asserts::verify_equal ("assignment replaces a constant distribution's state", assigned (assigned_generator), expected);
			}

			copied = Distribution (-3.0, 0.0);
			assigned = copied;
			CountingGenerator counting_generator (123);
			test_asserts::verify_equal ("assignment can replace a random distribution with a constant", assigned (counting_generator), -3.0);
			test_asserts::verify_equal ("copied constant state reports zero deviation", assigned.stddev(), 0.0);
			test_asserts::verify_equal ("copied constant state never uses the generator", counting_generator.calls, std::size_t (0));
		});

		it ("rejects negative deviations and non-finite parameters through both constructors", []{
			auto const verify_type = []<class Value> {
				using Number = si::decay_quantity_t<Value>;
				using Distribution = math::NormalDistribution<Value>;

				for (auto const deviation: { Number (-1), std::numeric_limits<Number>::quiet_NaN(), std::numeric_limits<Number>::infinity(), -std::numeric_limits<Number>::infinity() })
				{
					test_asserts::verify_throws<std::invalid_argument> ("direct constructor rejects invalid deviation", [&] {
						Distribution distribution (Value (0), Value (deviation));
					});
					test_asserts::verify_throws<std::invalid_argument> ("NormalVariable constructor rejects invalid deviation", [&] {
						Distribution distribution { math::NormalVariable<Value> (Value (0), Value (deviation)) };
					});
				}

				for (auto const mean: { std::numeric_limits<Number>::quiet_NaN(), std::numeric_limits<Number>::infinity(), -std::numeric_limits<Number>::infinity() })
					for (auto const deviation: { Number (0), Number (1) })
					{
						test_asserts::verify_throws<std::invalid_argument> ("direct constructor requires a finite mean even for zero deviation", [&] {
							Distribution distribution { Value (mean), Value (deviation) };
						});
						test_asserts::verify_throws<std::invalid_argument> ("NormalVariable constructor requires a finite mean", [&] {
							Distribution distribution { math::NormalVariable<Value> (Value (mean), Value (deviation)) };
						});
					}
			};
			verify_type.operator()<float>();
			verify_type.operator()<double>();
			verify_type.operator()<si::Temperature>();
			verify_type.operator()<si::Pressure>();
		});

		it ("checks positive-deviation parameters before narrowing to the underlying double distribution", []{
			using Distribution = math::NormalDistribution<long double>;

			if constexpr (std::numeric_limits<long double>::max_exponent > std::numeric_limits<double>::max_exponent)
			{
				auto const max = std::numeric_limits<long double>::max();
				test_asserts::verify_throws<std::invalid_argument> ("oversized finite mean is rejected before conversion", [max] {
					Distribution distribution (max, 1.0L);
				});
				test_asserts::verify_throws<std::invalid_argument> ("oversized finite deviation is rejected before conversion", [max] {
					Distribution distribution (0.0L, max);
				});
			}

			if constexpr (std::numeric_limits<long double>::min_exponent - std::numeric_limits<long double>::digits <
						  std::numeric_limits<double>::min_exponent - std::numeric_limits<double>::digits)
			{
				test_asserts::verify_throws<std::invalid_argument> ("positive deviation cannot narrow to zero", [] {
					Distribution distribution (0.0L, std::numeric_limits<long double>::denorm_min());
				});
			}
		});
	});
});

} // namespace
} // namespace neutrino::test
