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

#ifndef NEUTRINO__WORK_PERFORMER_H__INCLUDED
#define NEUTRINO__WORK_PERFORMER_H__INCLUDED

// Neutrino:
#include <neutrino/logger.h>
#include <neutrino/noncopyable.h>
#include <neutrino/polymorphic.h>
#include <neutrino/stdexcept.h>
#include <neutrino/synchronized.h>
#include <neutrino/thread.h>

// Standard:
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <future>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>


namespace neutrino {

/**
 * WorkPerformer queues packaged tasks and executes them in the context of separate threads. It's a good
 * solution for CPU-intensive tasks.
 *
 * The threads are waiting for execution packaged tasks even if nothing is scheduled, so no time is lost
 * for creating new threads. Also avoids creating too many threads at the same time.
 *
 * It's not a good solution for packaged tasks that do IO, since they will block execution units (threads).
 */
class WorkPerformer: private Noncopyable
{
  public:
	// Ctor
	explicit
	WorkPerformer (std::size_t threads_number, Logger const&);

	/**
	 * Cancel queued tasks before waiting for running tasks to finish.
	 * Futures for cancelled tasks report std::future_errc::broken_promise.
	 * All queued results are cancelled before destroying any queued task's callable or arguments.
	 * Cancelled tasks release their stored callables and arguments even if their futures are retained.
	 *
	 * Must not run on any of this performer's worker threads. In particular, a task or the
	 * destruction of its captured objects must not release the last owning reference to this
	 * performer (for example, the last std::shared_ptr). The destructor joins all workers;
	 * attempting to join the current worker causes std::terminate().
	 */
	~WorkPerformer();

	/**
	 * Set scheduling parameter for all threads.
	 * Throws InvalidCall if the performer is stopping.
	 */
	void
	set (ThreadScheduler, int priority);

	/**
	 * Return number of threads created.
	 */
	std::size_t
	threads_number() const noexcept;

	/**
	 * Return number of tasks which haven't started execution.
	 */
	std::size_t
	queued_tasks() const noexcept;

	/**
	 * Submit new task to execute.
	 * Arguments are copied or moved into the queue. Use std::ref/std::cref to pass references.
	 * Referenced objects must remain alive until the task finishes.
	 * Exceptions during argument transfer or execution on a worker are stored in the returned future.
	 * Throws InvalidCall if the performer is stopping.
	 */
	template<class Result, class ...TaskArgs, class ...Args>
		std::future<Result>
		submit (std::packaged_task<Result (TaskArgs...)>&&, Args&&...);

	/**
	 * Submit new task to execute.
	 * The callable is invoked with its original const qualification and value category.
	 * Arguments are copied or moved into the queue. Use std::ref/std::cref to pass references.
	 * Referenced objects must remain alive until the task finishes.
	 * Exceptions during argument transfer or execution on a worker are stored in the returned future.
	 * Throws InvalidCall if the performer is stopping.
	 */
	template<class Function, class ...Args>
		std::future<std::invoke_result_t<Function&&, std::decay_t<Args>...>>
		submit (Function&& function, Args&&...);

  private:
	/**
	 * Type-erasing container for tasks to execute.
	 */
	struct AbstractTask: public Polymorphic
	{
		virtual void
		operator()() = 0;

		/**
		 * Abandon the result without destroying the stored callable or arguments.
		 */
		virtual void
		cancel() noexcept = 0;
	};

	using TaskQueue = std::deque<std::unique_ptr<AbstractTask>>;

	struct State
	{
		TaskQueue	tasks;
		bool		terminating { false };
	};

  private:
	/**
	 * Queue a nullary callable and capture invocation exceptions in the returned future.
	 * Keep ownership of the callable separate from the future's shared state.
	 */
	template<class Result, class Function>
		std::future<Result>
		enqueue (Function&&);

	/**
	 * Cancel all queued results, then destroy queued tasks outside the queue lock and join workers.
	 * Return the number of cancelled tasks.
	 * Also used to clean up after partial construction.
	 */
	std::size_t
	stop_and_join();

	/**
	 * Function executed by all threads.
	 * Waits for new tasks or exits when the state indicates termination.
	 */
	void
	thread();

  private:
	Logger						_logger;
	Synchronized<State>			_state;
	std::condition_variable		_tasks_condition;
	std::vector<std::thread>	_threads;
};


inline std::size_t
WorkPerformer::threads_number() const noexcept
{
	return _threads.size();
}


template<class Result, class Function>
	inline std::future<Result>
	WorkPerformer::enqueue (Function&& function)
	{
		using StoredFunction = std::decay_t<Function>;
		using PackagedTask = std::packaged_task<Result()>;

		struct ConcreteTask: public AbstractTask, private Noncopyable
		{
			explicit
			ConcreteTask (Function&& fn):
				function (std::forward<Function> (fn)),
				packaged_task ([this]() -> Result { return std::invoke (this->function); })
			{ }

			void
			operator()() override
				{ this->packaged_task(); }

			void
			cancel() noexcept override
				{ this->packaged_task = PackagedTask(); }

			StoredFunction function;
			// Only borrows function; ConcreteTask has a stable address and outlives the invocation.
			// Destroy this producer before function so cancellation reports broken_promise first.
			PackagedTask packaged_task;
		};

		auto queued_task = std::make_unique<ConcreteTask> (std::forward<Function> (function));
		std::future<Result> future = queued_task->packaged_task.get_future();
		{
			auto state = _state.lock();

			if (state->terminating)
				throw InvalidCall ("can't submit tasks while the work performer is stopping");

			state->tasks.emplace_back (std::move (queued_task));
		}
		_tasks_condition.notify_one();
		return future;
	}


template<class Result, class ...TaskArgs, class ...Args>
	inline std::future<Result>
	WorkPerformer::submit (std::packaged_task<Result (TaskArgs...)>&& task, Args&&... args)
	{
		auto future = task.get_future();
		auto invoke = [task = std::move (task),
					   args = std::tuple<std::decay_t<Args>...> (std::forward<Args> (args)...),
					   future = std::move (future)]() mutable -> Result
		{
			std::apply (task, std::move (args));
			// Forward exceptions captured by the original task to the returned future too:
			return future.get();
		};

		return enqueue<Result> (std::move (invoke));
	}


template<class Function, class ...Args>
	inline std::future<std::invoke_result_t<Function&&, std::decay_t<Args>...>>
	WorkPerformer::submit (Function&& function, Args&&... args)
	{
		using Result = std::invoke_result_t<Function&&, std::decay_t<Args>...>;
		auto invoke = [function = std::forward<Function> (function),
					   args = std::tuple<std::decay_t<Args>...> (std::forward<Args> (args)...)]() mutable -> decltype(auto)
		{
			return std::apply (std::forward_like<Function&&> (function), std::move (args));
		};

		return enqueue<Result> (std::move (invoke));
	}

} // namespace neutrino

#endif
