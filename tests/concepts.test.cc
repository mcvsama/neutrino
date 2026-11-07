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
#include <neutrino/concepts.h>

// Standard:
#include <string>
#include <utility>


namespace neutrino::test {
namespace {

struct Base { };
struct Derived: Base { };
struct PrivateDerived: private Base { };
struct Left: Base { };
struct Right: Base { };
struct AmbiguousDerived: Left, Right { };


struct ReferenceConversion
{
	int& value;

	constexpr
	operator int&() const noexcept
		{ return this->value; }
};


struct ValueConversion
{
	operator int() const;
};


struct ExplicitReferenceConversion
{
	explicit
	operator int&() const;
};


static constexpr int const&
borrow_int (NonTemporaryReference<int> auto&& value)
{
	return value;
}


template<class Arg>
	concept CanBorrowInt = requires (Arg&& arg) { borrow_int (std::forward<Arg> (arg)); };


static_assert (CanBorrowInt<int&>);
static_assert (CanBorrowInt<int const&>);
static_assert (!CanBorrowInt<int>);
static_assert (!CanBorrowInt<int const>);
static_assert (!CanBorrowInt<int&&>);
static_assert (!CanBorrowInt<int const&&>);
static_assert (!CanBorrowInt<int volatile&>);
static_assert (!CanBorrowInt<double&>);

static_assert (NonTemporaryReference<int&, int&>);
static_assert (!NonTemporaryReference<int const&, int&>);
static_assert (NonTemporaryReference<int const&, int const&>);
static_assert (!NonTemporaryReference<int&, int&&>);

static_assert (NonTemporaryReference<Derived&, Base>);
static_assert (NonTemporaryReference<Derived const&, Base>);
static_assert (NonTemporaryReference<Derived&, Base&>);
static_assert (NonTemporaryReference<Derived const&, Base const&>);
static_assert (!NonTemporaryReference<Derived const&, Base&>);
static_assert (!NonTemporaryReference<Derived, Base>);
static_assert (!NonTemporaryReference<PrivateDerived&, Base>);
static_assert (!NonTemporaryReference<AmbiguousDerived&, Base>);

static_assert (CanBorrowInt<ReferenceConversion&>);
static_assert (!CanBorrowInt<ValueConversion&>);
static_assert (!CanBorrowInt<ExplicitReferenceConversion&>);
static_assert (NonTemporaryReference<std::string&, std::string>);
static_assert (NonTemporaryReference<std::string const&, std::string>);
static_assert (!NonTemporaryReference<std::string, std::string>);
static_assert (!NonTemporaryReference<char const (&)[4], std::string>);
static_assert (!NonTemporaryReference<char const*&, std::string>);


static constexpr bool
preserves_reference_identity()
{
	int value = 1;
	int const const_value = 2;
	ReferenceConversion conversion { value };
	return &borrow_int (value) == &value &&
		&borrow_int (const_value) == &const_value &&
		&borrow_int (conversion) == &value;
}


static_assert (preserves_reference_identity());

} // namespace
} // namespace neutrino::test
