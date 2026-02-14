/* vim:ts=4
 *
 * Copyleft 2025  Michał Gawron
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

// Neutrino:
#include <neutrino/scope_exit.h>

// Standard:
#include <cstddef>
#include <thread>
#include <type_traits>


namespace neutrino::test {
namespace {

AutoTest t1 ("neutrino::ScopeExit", []{
	// `ScopeExit e1` wasn't constructed with the callback, so we don't know what callback type will be assigned in the future.
	// Therefore we need type-erasing std::function to handle that.
	{
		ScopeExit e1;
		test_asserts::verify ("e1 uses std::function<void()>", std::is_same_v<decltype (e1)::Callback, std::function<void()>>);
	}

	// With `e2` and `e3` the `ScopeExit` is created with a callback, so it can hold the type of the callback directly
	// in the class without overhead of std::function.

	{
		ScopeExit e2 ([]{ [[maybe_unused]] volatile int k = 0; k = 1; });
		test_asserts::verify ("e2 doesn't use std::function<void()>", !std::is_same_v<decltype (e2)::Callback, std::function<void()>>);
	}

	{
		volatile int k = 0;
		ScopeExit e3 ([&]{ k = 1; });
		test_asserts::verify ("e3 doesn't use std::function<void()>", !std::is_same_v<decltype (e3)::Callback, std::function<void()>>);
	}
});

} // namespace
} // namespace neutrino::test
