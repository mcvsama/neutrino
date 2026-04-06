/* vim:ts=4
 *
 * Copyleft 2008…2013  Michał Gawron
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
#include <neutrino/test/auto_test.h>
#include <neutrino/numeric.h>
#include <neutrino/si/si.h>

// Standard:
#include <cstddef>


namespace neutrino::test {
namespace {

AutoTest t1 ("Neutrino: trapezoid_integral()", []{
	auto int1 = trapezoid_integral (static_cast<double (*)(double)> (std::sin), { -5.3, +12.0 }, 1e-5);

	test_asserts::verify_equal_with_epsilon ("integrated sin() is correct", int1, -0.28948, 1e-5);
});


AutoTest g2 ("Neutrino: renormalize()", []{
	using namespace si::literals;

	// Should compile:
	[[maybe_unused]] float x = renormalize (1_rad, Range { 0_deg, 90_deg }, Range { 0.0f, 1.0f });
	[[maybe_unused]] float y = renormalize (1_rad, { 0_deg, 90_deg }, { 0.0f, 1.0f });
});

} // namespace
} // namespace neutrino::test
