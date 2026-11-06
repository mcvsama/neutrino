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
#include <neutrino/crypto/secure.h>
#include <neutrino/test/test_asserts.h>
#include <neutrino/test/test_suite.h>

// Standard:
#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>


namespace neutrino::test {
namespace {

/**
 * Inspect the original object's backing storage after destruction without accessing a destroyed
 * object or freed allocation. Check an eight-byte marker so a zeroed string terminator alone
 * cannot hide the remaining bytes of a twelve-byte small-string secret.
 */
template<class Container, bool Wrapped, class Operation>
	static void
	verify_inline_secret_erased (Operation const& operation)
	{
		using Source = std::conditional_t<Wrapped, Secure<Container>, Container>;
		alignas (Source) std::array<unsigned char, sizeof (Source)> storage {};
		std::array<unsigned char, 8> marker;
		marker.fill (0x5a);
		auto const contains_marker = [&] {
			return std::search (storage.begin(), storage.end(), marker.begin(), marker.end()) != storage.end();
		};
		auto const destroy = [](Source* source) {
			std::destroy_at (source);
		};
		std::unique_ptr<Source, decltype (destroy)> source (
			std::construct_at (reinterpret_cast<Source*> (storage.data()), std::size_t (12), typename Container::value_type (0x5a)),
			destroy
		);
		// Implementations without SSO still run the operation and its functional assertions.
		bool const inline_secret = contains_marker();
		operation (*source);
		source.reset();

		if (inline_secret)
			test_asserts::verify ("the original object's storage contains no remaining inline secret", !contains_marker());
	}


static TestRegistration tests ([]{
	describe ("Neutrino", "crypto", "Secure", []{
		it ("reports container constructor availability accurately", []{
			auto const verify = []<class Container> {
				using Secret = Secure<Container>;
				using Value = typename Container::value_type;

				static_assert (!std::constructible_from<Secret, int>);
				static_assert (!std::is_constructible_v<Secret, int>);
				static_assert (!std::constructible_from<Secret, void*>);
				static_assert (!std::constructible_from<Secret, Container&, void*>);
				static_assert (!std::constructible_from<Secret, int, int, int>);
				static_assert (std::default_initializable<Secret>);
				static_assert (std::constructible_from<Secret, Container&>);
				static_assert (std::constructible_from<Secret, Container const&>);
				static_assert (std::constructible_from<Secret, Container&&>);
				static_assert (std::constructible_from<Secret, Secret&>);
				static_assert (std::constructible_from<Secret, Secret const&>);
				static_assert (std::constructible_from<Secret, Secret&&>);
				static_assert (std::constructible_from<Secret, Secret const&&>);

				Value const bytes[] { Value (0x5a), Value (0x4b), Value (0x39) };
				Secret const from_pointer (bytes, std::size_t (3));
				Secret const from_range (std::begin (bytes), std::end (bytes));
				test_asserts::verify_equal ("pointer and length construction preserves the bytes", *from_pointer, Container (bytes, 3));
				test_asserts::verify_equal ("iterator range construction remains available", *from_range, *from_pointer);
			};

			verify.operator()<Blob>();
			verify.operator()<std::string>();
		});

		it ("moves empty, short and long secrets while clearing and preserving the source for reuse", []{
			auto const verify = []<class Container> {
				using Secret = Secure<Container>;

				static_assert (!std::is_nothrow_move_constructible_v<Secret>);
				static_assert (!std::is_nothrow_move_assignable_v<Secret>);
				static_assert (std::is_nothrow_destructible_v<Secret>);

				for (auto const size: { std::size_t (0), std::size_t (1), std::size_t (12), std::size_t (32), std::size_t (4096) })
				{
					Container const expected (size, typename Container::value_type (0x5a));
					Container container = expected;
					Secret from_container (std::move (container));
					test_asserts::verify_equal ("consuming a container preserves its value", *from_container, expected);
					test_asserts::verify ("the consumed container is empty", container.empty());

					Secret source (expected);
					Secret destination (std::move (source));
					test_asserts::verify_equal ("moving a wrapper preserves its value", *destination, expected);
					test_asserts::verify ("the moved-from wrapper is empty", source->empty());

					source = Secret (std::size_t (3), typename Container::value_type (0x7f));
					test_asserts::verify_equal ("the moved-from wrapper remains reusable", source->size(), std::size_t (3));
					test_asserts::verify_equal ("reusing the source does not affect the destination", *destination, expected);

					for (auto const previous_size: { std::size_t (0), std::size_t (12), std::size_t (4096) })
					{
						Secret assigned (previous_size, typename Container::value_type (0x7f));
						Secret replacement (expected);
						auto& result = (assigned = std::move (replacement));
						test_asserts::verify ("move assignment returns the destination", &result == &assigned);
						test_asserts::verify_equal ("move assignment replaces the full value", *assigned, expected);
						test_asserts::verify ("move assignment clears the source", replacement->empty());
					}
				}
			};

			verify.operator()<Blob>();
			verify.operator()<std::string>();
		});

		it ("preserves copy behavior and values during copy and move self-assignment", []{
			auto const verify = []<class Container> {
				for (auto const size: { std::size_t (0), std::size_t (12), std::size_t (4096) })
				{
					Container const expected (size, typename Container::value_type (0x5a));
					Secure<Container> source (expected);
					Secure<Container> copied (source);
					Secure<Container> assigned;
					assigned = source;
					test_asserts::verify_equal ("copy construction preserves the value", *copied, expected);
					test_asserts::verify_equal ("copy assignment preserves the value", *assigned, expected);
					test_asserts::verify_equal ("copying retains the source", *source, expected);

					auto& alias = source;
					source = std::as_const (alias);
					test_asserts::verify_equal ("copy self-assignment preserves the secret", *source, expected);

					source = std::move (alias);
					test_asserts::verify_equal ("move self-assignment preserves the secret", *source, expected);
				}
			};

			verify.operator()<Blob>();
			verify.operator()<std::string>();
		});

		it ("erases inline secrets after move construction and assignment", []{
			auto const verify = []<class Container> {
				verify_inline_secret_erased<Container, true> ([](Secure<Container>& source) {
					Secure<Container> destination (std::move (source));
					test_asserts::verify_equal ("move construction retains the secret", *destination, Container (12, typename Container::value_type (0x5a)));
					test_asserts::verify ("move construction clears the original size", source->empty());
				});
				verify_inline_secret_erased<Container, true> ([](Secure<Container>& source) {
					Secure<Container> destination (std::size_t (4096), typename Container::value_type (0x7f));
					destination = std::move (source);
					test_asserts::verify_equal ("move assignment retains the secret", *destination, Container (12, typename Container::value_type (0x5a)));
					test_asserts::verify ("move assignment clears the original size", source->empty());
				});
			};

			verify.operator()<Blob>();
			verify.operator()<std::string>();
		});

		it ("erases inline container secrets including allocator and substring construction", []{
			auto const verify = []<class Container> {
				verify_inline_secret_erased<Container, false> ([](Container& source) {
					Secure<Container> destination (std::move (source));
					test_asserts::verify_equal ("container construction retains the secret", *destination, Container (12, typename Container::value_type (0x5a)));
					test_asserts::verify ("container construction clears the source", source.empty());
				});
				verify_inline_secret_erased<Container, false> ([](Container& source) {
					Secure<Container> destination (std::move (source), source.get_allocator());
					test_asserts::verify_equal ("allocator construction retains the secret", *destination, Container (12, typename Container::value_type (0x5a)));
					test_asserts::verify ("allocator construction clears the source", source.empty());
				});
				verify_inline_secret_erased<Container, false> ([](Container& source) {
					Secure<Container> destination (std::move (source), std::size_t (2), std::size_t (4));
					test_asserts::verify_equal ("substring construction retains the selected bytes", *destination, Container (4, typename Container::value_type (0x5a)));
					test_asserts::verify ("substring construction clears the whole source", source.empty());
				});
			};

			verify.operator()<Blob>();
			verify.operator()<std::string>();
		});

		it ("erases inline secrets at destruction and when replacing a value with a shorter secret", []{
			auto const verify = []<class Container> {
				verify_inline_secret_erased<Container, true> ([](Secure<Container>&) { });
				verify_inline_secret_erased<Container, true> ([](Secure<Container>& secret) {
					Secure<Container> const replacement (std::size_t (2), typename Container::value_type (0x7f));
					secret = replacement;
				});
				verify_inline_secret_erased<Container, true> ([](Secure<Container>& secret) {
					Secure<Container> replacement (std::size_t (2), typename Container::value_type (0x7f));
					secret = std::move (replacement);
				});
			};

			verify.operator()<Blob>();
			verify.operator()<std::string>();
		});
	});
});

} // namespace
} // namespace neutrino::test
