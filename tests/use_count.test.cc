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
#include <neutrino/use_count.h>
#include <neutrino/test/test_asserts.h>
#include <neutrino/test/test_suite.h>

// Standard:
#include <atomic>
#include <barrier>
#include <cstddef>
#include <thread>
#include <type_traits>
#include <vector>


namespace neutrino::test {
namespace {

struct IncompleteUseCountOwner;


struct UseCountOwner
{
	UseCount count { this };
};


static_assert (std::is_default_constructible_v<UseCount>);
static_assert (std::is_constructible_v<UseCount, std::nullptr_t>);
static_assert (!std::is_convertible_v<std::nullptr_t, UseCount>);
static_assert (!std::is_copy_constructible_v<UseCount>);
static_assert (!std::is_copy_assignable_v<UseCount>);
static_assert (!std::is_move_constructible_v<UseCount>);
static_assert (!std::is_move_assignable_v<UseCount>);
static_assert (std::is_copy_constructible_v<UseToken>);
static_assert (std::is_copy_assignable_v<UseToken>);


static TestRegistration tests ([]{
	describe ("Neutrino", "UseCount", []{
		it ("supports counters without an owner", []{
			auto verify_counter = [](UseCount& count) {
				test_asserts::verify_equal ("the initial count is zero", count.get(), std::size_t { 0 });

				{
					UseToken token (count);
					test_asserts::verify_equal ("a token adds a dependency", count.get(), std::size_t { 1 });
				}

				test_asserts::verify_equal ("destroying the token releases its dependency", count.get(), std::size_t { 0 });
			};

			UseCount count;
			UseCount null_owner (nullptr);
			verify_counter (count);
			verify_counter (null_owner);
		});

		it ("supports owners with incomplete types", []{
			UseCount count (static_cast<IncompleteUseCountOwner*> (nullptr));

			{
				UseToken token (count);
				test_asserts::verify_equal ("a token adds a dependency", count.get(), std::size_t { 1 });
			}

			test_asserts::verify_equal ("destroying the token releases its dependency", count.get(), std::size_t { 0 });
		});

		it ("preserves zero when a decrement is rejected", []{
			UseCountOwner owner;
			test_asserts::verify_equal ("the initial count is zero", owner.count.get(), std::size_t { 0 });
			test_asserts::verify_throws<InvalidCall> ("decrementing zero throws", [&]{ --owner.count; });
			test_asserts::verify_equal ("the rejected decrement leaves zero unchanged", owner.count.get(), std::size_t { 0 });

			++owner.count;
			test_asserts::verify_equal ("increment adds a dependency", owner.count.get(), std::size_t { 1 });
			--owner.count;
			test_asserts::verify_equal ("decrement removes the dependency", owner.count.get(), std::size_t { 0 });
		});

		it ("balances copied and reassigned tokens", []{
			UseCountOwner first;
			UseCountOwner second;

			{
				UseToken original (first.count);
				UseToken copy (original);
				UseToken reassigned (second.count);
				test_asserts::verify_equal ("copying adds a dependency", first.count.get(), std::size_t { 2 });
				reassigned = original;
				original = original;
				test_asserts::verify_equal ("assignment transfers the dependency", first.count.get(), std::size_t { 3 });
				test_asserts::verify_equal ("assignment releases the previous counter", second.count.get(), std::size_t { 0 });
			}

			test_asserts::verify_equal ("destroying tokens releases all dependencies", first.count.get(), std::size_t { 0 });
		});

		it ("counts tokens created and destroyed concurrently", []{
			UseCountOwner owner;

			{
				UseToken const source (owner.count);
				constexpr std::size_t thread_count = 4;
				constexpr std::size_t tokens_per_thread = 2000;
				std::barrier phase (static_cast<std::ptrdiff_t> (thread_count + 1));
				std::vector<std::jthread> workers;
				workers.reserve (thread_count);

				for (std::size_t thread = 0; thread < thread_count; ++thread)
				{
					workers.emplace_back ([&]{
						std::vector<UseToken> tokens;
						tokens.reserve (tokens_per_thread);
						phase.arrive_and_wait();

						for (std::size_t i = 0; i < tokens_per_thread; ++i)
						{
							tokens.emplace_back (source);
							UseToken temporary (owner.count);
							temporary = tokens.back();
							static_cast<void> (owner.count.get());
						}

						phase.arrive_and_wait();
						phase.arrive_and_wait();
					});
				}

				phase.arrive_and_wait();
				phase.arrive_and_wait();
				auto const observed_count = owner.count.get();
				phase.arrive_and_wait();
				workers.clear();

				test_asserts::verify_equal ("all live tokens are counted", observed_count, thread_count * tokens_per_thread + 1);
				test_asserts::verify_equal ("joined workers leave only the source token", owner.count.get(), std::size_t { 1 });
			}

			test_asserts::verify_equal ("the final token releases its dependency", owner.count.get(), std::size_t { 0 });
		});

		it ("allows only one concurrent decrement of a count of one", []{
			UseCountOwner owner;
			++owner.count;
			constexpr std::size_t thread_count = 8;
			std::barrier start (static_cast<std::ptrdiff_t> (thread_count));
			std::atomic<std::size_t> successful { 0 };
			std::atomic<std::size_t> rejected { 0 };
			std::vector<std::jthread> workers;
			workers.reserve (thread_count);

			for (std::size_t thread = 0; thread < thread_count; ++thread)
			{
				workers.emplace_back ([&]{
					start.arrive_and_wait();

					try {
						--owner.count;
						++successful;
					}
					catch (InvalidCall const&)
					{
						++rejected;
					}
				});
			}

			workers.clear();
			test_asserts::verify_equal ("exactly one decrement succeeds", successful.load(), std::size_t { 1 });
			test_asserts::verify_equal ("all remaining decrements are rejected", rejected.load(), thread_count - 1);
			test_asserts::verify_equal ("the count remains zero", owner.count.get(), std::size_t { 0 });
		});
	});
});

} // namespace
} // namespace neutrino::test
