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
#include <neutrino/noncopyable.h>
#include <neutrino/nonmovable.h>
#include <neutrino/value_or_ptr.h>

// Standard:
#include <cstddef>
#include <string>
#include <thread>
#include <type_traits>


namespace neutrino::test {
namespace {

class TestDefaultConstructible:
	private neutrino::Noncopyable,
	private Nonmovable
{
  public:
	TestDefaultConstructible()
	{ }

	TestDefaultConstructible (int, std::string)
	{ }

  public:
	std::string test_string = "default";
};


class TestNonCM:
	private neutrino::Noncopyable,
	private Nonmovable
{
  public:
	TestNonCM (int const test_int, std::string const& test_string):
		test_int (test_int),
		test_string (test_string)
	{ }

  public:
	int			test_int;
	std::string	test_string;
};


AutoTest t1 ("neutrino::ValueOrPtr: default-constructible", []{
	using V = ValueOrPtr<TestDefaultConstructible>;

	auto vp = V();
	test_asserts::verify_equal ("vp uses internal value after construction", vp->test_string, "default");

	auto t = TestDefaultConstructible();
	t.test_string = "external";
	vp = &t;
	test_asserts::verify_equal ("vp uses external value after assigning t", vp->test_string, "external");

	vp = nullptr;
	test_asserts::verify_equal ("vp uses internal value after assigning nullptr", vp->test_string, "default");
});


AutoTest t2 ("neutrino::ValueOrPtr: non-copyable, non-movable", []{
	using V = ValueOrPtr<TestNonCM, int, std::string>;

	auto vp = V (std::in_place, 1, "s1");
	test_asserts::verify_equal ("vp uses internal value (1) after construction", vp->test_int, 1);
	test_asserts::verify_equal ("vp uses internal value (\"s1\") after construction", vp->test_string, "s1");

	auto t = TestNonCM (2, "s2");
	vp = &t;
	test_asserts::verify_equal ("vp uses external value (2)", vp->test_int, 2);
	test_asserts::verify_equal ("vp uses external value (\"s2\")", vp->test_string, "s2");

	vp = nullptr;
	test_asserts::verify_equal ("vp uses internal value (1) after assigning nullptr", vp->test_int, 1);
	test_asserts::verify_equal ("vp uses internal value (\"s1\") after assigning nullptr", vp->test_string, "s1");
});

} // namespace
} // namespace neutrino::test

