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
#include <neutrino/test/test_asserts.h>
#include <neutrino/test/test_suite.h>

// Neutrino:
#include <neutrino/work_performer.h>

// Standard:
#include <cstddef>
#include <functional>
#include <memory>
#include <semaphore>
#include <stdexcept>
#include <thread>
#include <type_traits>


namespace neutrino::test {
namespace {

using namespace std::chrono_literals;

Logger g_null_logger;


struct ArgumentMoveError
{ };


class ThrowOnWorkerMove: private Noncopyable
{
  public:
	ThrowOnWorkerMove() = default;

	ThrowOnWorkerMove (ThrowOnWorkerMove&&);

  private:
	std::thread::id const _submitting_thread { std::this_thread::get_id() };
};


ThrowOnWorkerMove::ThrowOnWorkerMove (ThrowOnWorkerMove&& other):
	_submitting_thread (other._submitting_thread)
{
	if (std::this_thread::get_id() != _submitting_thread)
		throw ArgumentMoveError();
}


TestRegistration tests ([]{
	describe ("Neutrino", "WorkPerformer", []{
		it ("executes 100'000 non-trivial tasks", []{
			constexpr int kTasks = 100'000;
			constexpr int kResult = 1337;

			auto delayed_return = [] (int thing_to_return) noexcept {
				for (volatile int i = 0; i < 1'000; )
				{
					i = i + 1;
					continue;
				}

				return thing_to_return;
			};

			std::optional<WorkPerformer> wp (std::in_place, 8, g_null_logger);
			std::vector<std::future<int>> futures;

			for (std::size_t i = 0; i < kTasks; ++i)
				futures.emplace_back (wp->submit (delayed_return, kResult));

			// Wait for all tasks to finish:
			while (wp->queued_tasks() > 0)
				std::this_thread::sleep_for (0.1s);

			// Destroy the work performer:
			wp.reset();

			bool all_are_correct = true;

			for (auto& future: futures)
				if (future.get() != kResult)
					all_are_correct = false;

			test_asserts::verify ("tasks are executed and return values correctly", all_are_correct);
		});

		it ("copies lvalue arguments before queued tasks execute", []{
			WorkPerformer wp (1, g_null_logger);
			std::binary_semaphore gate (0);
			auto blocker = wp.submit ([&gate] { gate.acquire(); });

			int value = 123;
			auto copied = wp.submit ([] (int const& argument) { return argument; }, value);
			std::packaged_task<int (int)> task ([] (int argument) { return argument; });
			auto packaged = wp.submit (std::move (task), value);
			value = 456;

			gate.release();
			blocker.get();
			auto const copied_result = copied.get();
			auto const packaged_result = packaged.get();
			test_asserts::verify_equal ("callable receives the submitted value", copied_result, 123);
			test_asserts::verify_equal ("packaged task receives the submitted value", packaged_result, 123);
		});

		it ("passes arguments by reference only when explicitly requested", []{
			WorkPerformer wp (1, g_null_logger);
			std::binary_semaphore gate (0);
			auto blocker = wp.submit ([&gate] { gate.acquire(); });

			int value = 123;
			auto referenced = wp.submit ([] (int& argument) { return ++argument; }, std::ref (value));
			std::packaged_task<int (int&)> task ([] (int& argument) { return ++argument; });
			auto packaged = wp.submit (std::move (task), std::ref (value));
			auto const_referenced = wp.submit ([] (int const& argument) { return argument; }, std::cref (value));
			value = 456;

			gate.release();
			blocker.get();
			auto const referenced_result = referenced.get();
			auto const packaged_result = packaged.get();
			auto const const_referenced_result = const_referenced.get();
			test_asserts::verify_equal ("callable modifies the original argument", referenced_result, 457);
			test_asserts::verify_equal ("packaged task modifies the original argument", packaged_result, 458);
			test_asserts::verify_equal ("const reference observes the original argument", const_referenced_result, 458);
			test_asserts::verify_equal ("changes remain visible to the caller", value, 458);
		});

		it ("moves non-copyable arguments into queued tasks", []{
			WorkPerformer wp (1, g_null_logger);
			auto argument = std::make_unique<int> (123);
			auto moved = wp.submit ([] (std::unique_ptr<int> value) { return *value; }, std::move (argument));
			std::packaged_task<int (std::unique_ptr<int>)> task ([] (std::unique_ptr<int> value) { return *value; });
			auto packaged = wp.submit (std::move (task), std::make_unique<int> (456));

			test_asserts::verify ("ownership is transferred from the caller", !argument);
			test_asserts::verify_equal ("callable receives the moved argument", moved.get(), 123);
			test_asserts::verify_equal ("packaged task receives the moved argument", packaged.get(), 456);
		});

		it ("propagates callable argument move exceptions and keeps workers running", []{
			WorkPerformer wp (1, g_null_logger);
			auto result = wp.submit ([] (ThrowOnWorkerMove) { return 123; }, ThrowOnWorkerMove());

			test_asserts::verify_throws<ArgumentMoveError> ("future reports the argument move exception", [&result] {
				result.get();
			});
			test_asserts::verify_equal ("worker executes subsequent tasks", wp.submit ([] { return 456; }).get(), 456);
		});

		it ("propagates packaged task argument move exceptions and keeps workers running", []{
			WorkPerformer wp (1, g_null_logger);
			std::packaged_task<int (ThrowOnWorkerMove)> task ([] (ThrowOnWorkerMove) { return 123; });
			auto result = wp.submit (std::move (task), ThrowOnWorkerMove());

			test_asserts::verify_throws<ArgumentMoveError> ("future reports the argument move exception", [&result] {
				result.get();
			});
			test_asserts::verify_equal ("worker executes subsequent tasks", wp.submit ([] { return 456; }).get(), 456);
		});

		it ("propagates invocation errors from completed nullary packaged tasks", []{
			WorkPerformer wp (1, g_null_logger);
			std::packaged_task<int()> task ([] { return 123; });
			task();
			auto result = wp.submit (std::move (task));
			auto next_result = wp.submit (std::packaged_task<int()> ([] { return 456; }));

			// Wait for the worker to attempt the completed task and then run another task:
			auto const next_value = next_result.get();
			try {
				result.get();
				test_asserts::verify ("future reports the invocation error", false);
			}
			catch (std::future_error const& e)
			{
				test_asserts::verify ("promise is already satisfied", e.code() == std::future_errc::promise_already_satisfied);
			}
			test_asserts::verify_equal ("worker executes subsequent nullary packaged tasks", next_value, 456);
		});

		it ("preserves void, reference and move-only results", []{
			WorkPerformer wp (1, g_null_logger);
			int value = 123;
			auto void_result = wp.submit ([] (int) { }, value);
			auto reference_result = wp.submit ([] (int& argument) -> int& { return argument; }, std::ref (value));
			auto move_only_result = wp.submit ([] (int argument) { return std::make_unique<int> (argument); }, value);

			std::packaged_task<void (int)> void_task ([] (int) { });
			std::packaged_task<int& (int&)> reference_task ([] (int& argument) -> int& { return argument; });
			std::packaged_task<std::unique_ptr<int> (int)> move_only_task ([] (int argument) { return std::make_unique<int> (argument); });
			auto packaged_void_result = wp.submit (std::move (void_task), value);
			auto packaged_reference_result = wp.submit (std::move (reference_task), std::ref (value));
			auto packaged_move_only_result = wp.submit (std::move (move_only_task), value);

			void_result.get();
			packaged_void_result.get();
			test_asserts::verify ("callable preserves reference identity", &reference_result.get() == &value);
			test_asserts::verify ("packaged task preserves reference identity", &packaged_reference_result.get() == &value);
			test_asserts::verify_equal ("callable transfers ownership of its result", *move_only_result.get(), value);
			test_asserts::verify_equal ("packaged task transfers ownership of its result", *packaged_move_only_result.get(), value);
		});

		it ("propagates exceptions thrown by task bodies", []{
			WorkPerformer wp (1, g_null_logger);
			auto throwing = [] (int) -> int { throw std::runtime_error ("task body"); };
			auto result = wp.submit (throwing, 123);
			std::packaged_task<int (int)> task (throwing);
			auto packaged_result = wp.submit (std::move (task), 456);

			test_asserts::verify_throws<std::runtime_error> ("callable exception reaches its future", [&result] {
				result.get();
			});
			test_asserts::verify_throws<std::runtime_error> ("packaged task exception reaches its future", [&packaged_result] {
				packaged_result.get();
			});
		});

		it ("abandons not-started tasks when destructed", []{
			auto sleeper = [] (auto time) {
				std::this_thread::sleep_for (time);
				return 0;
			};

			std::optional<WorkPerformer> wp (std::in_place, 1, g_null_logger);
			auto v1 = wp->submit (sleeper, 0.01s);
			auto v2 = wp->submit (sleeper, 0.3s);

			wp.reset();

			try {
				v2.get();
				test_asserts::verify ("exception is thrown", false);
			}
			catch (std::future_error& e)
			{
				test_asserts::verify ("promise is broken", e.code() == std::future_errc::broken_promise);
			}
		});
	});
});

} // namespace
} // namespace neutrino::test
