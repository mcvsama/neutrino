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
#include <neutrino/scope_exit.h>
#include <neutrino/stdexcept.h>
#include <neutrino/work_performer.h>

// Standard:
#include <atomic>
#include <cstddef>
#include <functional>
#include <memory>
#include <semaphore>
#include <sstream>
#include <stdexcept>
#include <string>
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


static int
increment_argument (int const value)
{
	return value + 1;
}


TestRegistration tests ([]{
	describe ("Neutrino", "WorkPerformer", []{
		it ("executes 100'000 non-trivial tasks", []{
			constexpr int kTasks = 100'000;
			constexpr int kResult = 1337;

			auto delayed_return = [](int thing_to_return) noexcept {
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

		it ("executes successive batches submitted by concurrent producers", []{
			constexpr std::size_t kProducers = 4;
			constexpr std::size_t kBatches = 8;
			constexpr std::size_t kBatchTasks = 64;
			std::atomic<std::size_t> executed { 0 };
			WorkPerformer wp (4, g_null_logger);
			std::vector<std::future<void>> producers;
			std::promise<void> start;
			auto start_future = start.get_future().share();
			ScopeExit release_producers ([&start] { start.set_value(); });

			for (std::size_t producer = 0; producer < kProducers; ++producer)
			{
				producers.emplace_back (std::async (std::launch::async, [&, producer, start = start_future] {
					start.get();
					std::vector<std::future<std::size_t>> results;
					results.reserve (kBatchTasks);

					for (std::size_t batch = 0; batch < kBatches; ++batch)
					{
						results.clear();
						auto const first = (producer * kBatches + batch) * kBatchTasks;

						for (std::size_t task = 0; task < kBatchTasks; ++task)
						{
							results.emplace_back (wp.submit ([&executed, index = first + task] {
								++executed;
								return index;
							}));
						}

						for (std::size_t task = 0; task < kBatchTasks; ++task)
						{
							test_asserts::verify ("submitted task completes", results[task].wait_for (2s) == std::future_status::ready);
							test_asserts::verify_equal ("each task returns its own index", results[task].get(), first + task);
						}
					}
				}));
			}

			release_producers();
			for (auto& producer: producers)
				producer.get();

			test_asserts::verify_equal ("every submitted task executes exactly once", executed.load(), kProducers * kBatches * kBatchTasks);
		});

		it ("copies lvalue arguments before queued tasks execute", []{
			WorkPerformer wp (1, g_null_logger);
			std::binary_semaphore gate (0);
			auto blocker = wp.submit ([&gate] { gate.acquire(); });

			int value = 123;
			auto copied = wp.submit ([](int const& argument) { return argument; }, value);
			std::packaged_task<int (int)> task ([](int argument) { return argument; });
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
			auto referenced = wp.submit ([](int& argument) { return ++argument; }, std::ref (value));
			std::packaged_task<int (int&)> task ([](int& argument) { return ++argument; });
			auto packaged = wp.submit (std::move (task), std::ref (value));
			auto const_referenced = wp.submit ([](int const& argument) { return argument; }, std::cref (value));
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
			auto moved = wp.submit ([](std::unique_ptr<int> value) { return *value; }, std::move (argument));
			std::packaged_task<int (std::unique_ptr<int>)> task ([](std::unique_ptr<int> value) { return *value; });
			auto packaged = wp.submit (std::move (task), std::make_unique<int> (456));

			test_asserts::verify ("ownership is transferred from the caller", !argument);
			test_asserts::verify_equal ("callable receives the moved argument", moved.get(), 123);
			test_asserts::verify_equal ("packaged task receives the moved argument", packaged.get(), 456);
		});

		it ("propagates callable argument move exceptions and keeps workers running", []{
			WorkPerformer wp (1, g_null_logger);
			auto result = wp.submit ([](ThrowOnWorkerMove) { return 123; }, ThrowOnWorkerMove());

			test_asserts::verify_throws<ArgumentMoveError> ("future reports the argument move exception", [&result] {
				result.get();
			});
			test_asserts::verify_equal ("worker executes subsequent tasks", wp.submit ([] { return 456; }).get(), 456);
		});

		it ("propagates packaged task argument move exceptions and keeps workers running", []{
			WorkPerformer wp (1, g_null_logger);
			std::packaged_task<int (ThrowOnWorkerMove)> task ([](ThrowOnWorkerMove) { return 123; });
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

		it ("executes callables that can only be invoked as rvalues", []{
			struct RvalueOnly
			{
				std::unique_ptr<int> result;

				std::unique_ptr<int>
				operator()() &&
					{ return std::move (this->result); }
			};

			WorkPerformer wp (1, g_null_logger);
			auto result = wp.submit (RvalueOnly { std::make_unique<int> (123) });
			test_asserts::verify_equal ("rvalue-only callable transfers its result", *result.get(), 123);
		});

		it ("preserves the value category when invoking callables", []{
			struct Qualified
			{
				int
				operator() (int const value) &
					{ return value + 1; }

				long
				operator() (int const value) &&
					{ return value + 2; }
			};

			WorkPerformer wp (1, g_null_logger);
			Qualified callable;
			auto lvalue = wp.submit (callable, 123);
			auto rvalue = wp.submit (Qualified(), 123);
			auto referenced = wp.submit (std::ref (callable), 123);
			static_assert (std::is_same_v<decltype (lvalue), std::future<int>>);
			static_assert (std::is_same_v<decltype (rvalue), std::future<long>>);
			test_asserts::verify_equal ("lvalue overload is selected", lvalue.get(), 124);
			test_asserts::verify_equal ("rvalue overload is selected", rvalue.get(), 125L);
			test_asserts::verify_equal ("explicit reference selects the lvalue overload", referenced.get(), 124);
		});

		it ("preserves const qualification when invoking callables", []{
			struct Qualified
			{
				int
				operator()() &
					{ return 1; }

				long
				operator()() const&
					{ return 2; }

				long long
				operator()() const&&
					{ return 3; }
			};

			WorkPerformer wp (1, g_null_logger);
			Qualified const callable;
			auto lvalue = wp.submit (callable);
			auto rvalue = wp.submit (std::move (callable));
			auto referenced = wp.submit (std::cref (callable));
			static_assert (std::is_same_v<decltype (lvalue), std::future<long>>);
			static_assert (std::is_same_v<decltype (rvalue), std::future<long long>>);
			test_asserts::verify_equal ("const lvalue overload is selected", lvalue.get(), 2L);
			test_asserts::verify_equal ("const rvalue overload is selected", rvalue.get(), 3LL);
			test_asserts::verify_equal ("explicit const reference selects the const overload", referenced.get(), 2L);
		});

		it ("executes function references and function pointers", []{
			WorkPerformer wp (1, g_null_logger);
			auto pointer = &increment_argument;
			auto function = wp.submit (increment_argument, 123);
			auto lvalue_pointer = wp.submit (pointer, 456);
			auto rvalue_pointer = wp.submit (&increment_argument, 789);
			test_asserts::verify_equal ("function reference is invoked", function.get(), 124);
			test_asserts::verify_equal ("lvalue function pointer is invoked", lvalue_pointer.get(), 457);
			test_asserts::verify_equal ("rvalue function pointer is invoked", rvalue_pointer.get(), 790);
		});

		it ("preserves void, reference and move-only results", []{
			WorkPerformer wp (1, g_null_logger);
			int value = 123;
			auto void_result = wp.submit ([](int) { }, value);
			auto reference_result = wp.submit ([](int& argument) -> int& { return argument; }, std::ref (value));
			auto move_only_result = wp.submit ([](int argument) { return std::make_unique<int> (argument); }, value);

			std::packaged_task<void (int)> void_task ([](int) { });
			std::packaged_task<int& (int&)> reference_task ([](int& argument) -> int& { return argument; });
			std::packaged_task<std::unique_ptr<int> (int)> move_only_task ([](int argument) { return std::make_unique<int> (argument); });
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
			auto throwing = [](int) -> int { throw std::runtime_error ("task body"); };
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

		it ("cancels queued dependencies before joining workers and rejects new tasks", []{
			std::ostringstream log;
			LoggerOutput output (log);
			Logger logger (output);
			std::binary_semaphore first_started (0);
			std::binary_semaphore second_started (0);
			std::binary_semaphore release_second (0);
			std::promise<std::future<int>> deliver_dependency;
			auto delivery = deliver_dependency.get_future();
			std::optional<WorkPerformer> wp (std::in_place, 2, logger);
			auto* performer = &*wp;

			auto first = wp->submit ([&, performer, delivery = std::move (delivery)]() mutable {
				ScopeExit release ([&release_second] { release_second.release(); });
				first_started.release();
				auto dependency = delivery.get();

				if (dependency.wait_for (2s) != std::future_status::ready)
					throw std::runtime_error ("queued dependency was not cancelled during shutdown");

				try {
					return dependency.get();
				}
				catch (std::future_error const&)
				{
					test_asserts::verify_throws<InvalidCall> ("new tasks are rejected during shutdown", [performer] {
						performer->submit ([] { });
					});
					throw;
				}
			});
			first_started.acquire();

			auto second = wp->submit ([&] {
				second_started.release();
				release_second.acquire();
			});
			second_started.acquire();

			deliver_dependency.set_value (wp->submit ([] { return 123; }));

			wp.reset();
			second.get();

			try {
				first.get();
				test_asserts::verify ("queued dependency is cancelled", false);
			}
			catch (std::future_error const& e)
			{
				test_asserts::verify ("queued dependency reports a broken promise", e.code() == std::future_errc::broken_promise);
			}

			test_asserts::verify ("abandoned task count is logged", log.str().find ("with 1 tasks never started") != std::string::npos);
		});

		it ("rejects scheduling changes from a running task during shutdown", []{
			std::binary_semaphore started (0);
			std::promise<void> cancelled;
			auto cancellation = cancelled.get_future();
			std::optional<WorkPerformer> wp (std::in_place, 1, g_null_logger);
			auto* performer = &*wp;
			wp->set (ThreadScheduler::Other, 0);

			auto running = wp->submit ([performer, &started, cancellation = std::move (cancellation)] {
				started.release();

				if (cancellation.wait_for (2s) != std::future_status::ready)
					throw std::runtime_error ("queued task was not cancelled during shutdown");

				test_asserts::verify_throws<InvalidCall> ("scheduling changes are rejected during shutdown", [performer] {
					performer->set (ThreadScheduler::Other, 0);
				});
			});
			started.acquire();

			// Cancelling this task destroys the captured promise and wakes the worker:
			wp->submit ([cancelled = std::move (cancelled)] { });
			wp.reset();
			running.get();
		});

		it ("releases cancelled task captures and arguments while their futures are retained", []{
			for (bool const packaged: { false, true })
				for (bool const as_argument: { false, true })
				{
					std::binary_semaphore started (0);
					std::promise<void> cancelled;
					auto cancellation = cancelled.get_future();
					std::optional<WorkPerformer> wp (std::in_place, 1, g_null_logger);
					auto running = wp->submit ([&started, cancellation = std::move (cancellation)]() mutable {
						started.release();

						if (cancellation.wait_for (2s) != std::future_status::ready)
							return false;

						try {
							cancellation.get();
							return false;
						}
						catch (std::future_error const& e)
						{
							return e.code() == std::future_errc::broken_promise;
						}
					});
					started.acquire();

					std::future<void> queued;

					if (as_argument)
					{
						auto satisfy = [](std::promise<void> promise) { promise.set_value(); };

						if (packaged)
						{
							std::packaged_task<void (std::promise<void>)> task (satisfy);
							queued = wp->submit (std::move (task), std::move (cancelled));
						}
						else
							queued = wp->submit (satisfy, std::move (cancelled));
					}
					else
					{
						auto satisfy = [cancelled = std::move (cancelled)]() mutable { cancelled.set_value(); };

						if (packaged)
							queued = wp->submit (std::packaged_task<void()> (std::move (satisfy)));
						else
							queued = wp->submit (std::move (satisfy));
					}

					wp.reset();
					test_asserts::verify ("cancellation releases the promise before joining the worker", running.get());

					try {
						queued.get();
						test_asserts::verify ("queued task is cancelled", false);
					}
					catch (std::future_error const& e)
					{
						test_asserts::verify ("retained future reports a broken promise", e.code() == std::future_errc::broken_promise);
					}
				}
		});

		it ("cancels all queued results before destroying captured objects", []{
			std::binary_semaphore started (0);
			std::promise<void> release_worker;
			auto release = release_worker.get_future();
			std::shared_future<int> later;
			bool later_cancelled = false;
			std::optional<WorkPerformer> wp (std::in_place, 1, g_null_logger);
			auto running = wp->submit ([&started, release = std::move (release)] {
				started.release();
				release.wait();
			});
			started.acquire();

			ScopeExit on_discard ([&] {
				if (later.valid() && later.wait_for (2s) == std::future_status::ready)
				{
					try {
						later.get();
					}
					catch (std::future_error const& e)
					{
						later_cancelled = e.code() == std::future_errc::broken_promise;
					}
				}
				release_worker.set_value();
			});
			auto first = wp->submit ([on_discard = std::move (on_discard)] { });
			later = wp->submit ([] { return 123; }).share();

			wp.reset();
			running.get();
			test_asserts::verify ("captured destructor observes cancellation of the later task", later_cancelled);

			try {
				first.get();
				test_asserts::verify ("first queued task is cancelled", false);
			}
			catch (std::future_error const& e)
			{
				test_asserts::verify ("first future reports a broken promise", e.code() == std::future_errc::broken_promise);
			}
		});

		it ("destroys abandoned tasks outside the queue lock", []{
			std::binary_semaphore started (0);
			std::promise<bool> observe_destruction;
			auto observation = observe_destruction.get_future();
			std::optional<WorkPerformer> wp (std::in_place, 1, g_null_logger);
			auto* performer = &*wp;
			auto running = wp->submit ([&started, observation = std::move (observation)]() mutable {
				started.release();
				return observation.wait_for (2s) == std::future_status::ready && observation.get();
			});
			started.acquire();

			ScopeExit on_discard ([&observe_destruction, performer] {
				observe_destruction.set_value (performer->queued_tasks() == 0);
			});
			// The callback must be destroyed during cancellation, outside the queue lock:
			wp->submit ([on_discard = std::move (on_discard)] { });

			wp.reset();
			test_asserts::verify ("task destructor can inspect the queue during shutdown", running.get());
		});

		it ("handles failures while preparing the shutdown log message", []{
			struct ThrowingTag: public LoggerTagProvider
			{
				std::optional<std::string>
				logger_tag() const override
					{ throw std::runtime_error ("tag failure"); }
			};

			ThrowingTag tag;
			Logger logger;
			logger.set_logger_tag_provider (tag);
			std::binary_semaphore started (0);
			std::binary_semaphore release (0);
			std::optional<WorkPerformer> wp (std::in_place, 1, logger);
			auto running = wp->submit ([&] {
				started.release();
				release.acquire();
			});
			started.acquire();

			ScopeExit on_discard ([&release] { release.release(); });
			wp->submit ([on_discard = std::move (on_discard)] { });
			wp.reset();
			running.get();
		});

		it ("abandons not-started tasks when destructed", []{
			auto sleeper = [](auto time) {
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
