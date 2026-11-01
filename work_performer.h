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
#include <neutrino/synchronized.h>
#include <neutrino/thread.h>

// Standard:
#include <cstddef>
#include <functional>
#include <future>
#include <optional>
#include <queue>
#include <semaphore>
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

	// Dtor
	~WorkPerformer();

	/**
	 * Set scheduling parameter for all threads.
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
	 */
	template<class Result, class ...TaskArgs, class ...Args>
		std::future<Result>
		submit (std::packaged_task<Result (TaskArgs...)>&&, Args&&...);

	/**
	 * Submit new task to execute.
	 * Arguments are copied or moved into the queue. Use std::ref/std::cref to pass references.
	 * Referenced objects must remain alive until the task finishes.
	 * Exceptions during argument transfer or execution on a worker are stored in the returned future.
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
	};

	using TaskQueue = std::queue<std::unique_ptr<AbstractTask>>;

  private:
	/**
	 * Queue a task whose argument transfers are already covered by its exception handling.
	 */
	template<class Result>
		std::future<Result>
		enqueue (std::packaged_task<Result()>&&);

	/**
	 * Signal worker threads to stop, wake them, and wait for them to finish.
	 * Also used to clean up after partial construction.
	 */
	void
	stop_and_join();

	/**
	 * Function executed by all threads.
	 * Waits for new tasks or exits when _terminating is true.
	 */
	void
	thread();

  private:
	Logger							_logger;
	std::atomic<bool>				_terminating { false };
	Synchronized<TaskQueue> mutable	_tasks;
	std::counting_semaphore<>		_tasks_semaphore;
	std::vector<std::thread>		_threads;
};


inline std::size_t
WorkPerformer::threads_number() const noexcept
{
	return _threads.size();
}


template<class Result>
	inline std::future<Result>
	WorkPerformer::enqueue (std::packaged_task<Result()>&& task)
	{
		using PackagedTask = std::packaged_task<Result()>;

		struct ConcreteTask: public AbstractTask
		{
			explicit
			ConcreteTask (PackagedTask&& pt):
				packaged_task (std::move (pt))
			{ }

			void
			operator()() override
				{ this->packaged_task(); }

			PackagedTask packaged_task;
		};

		std::future<Result> future = task.get_future();
		_tasks->emplace (std::make_unique<ConcreteTask> (std::move (task)));
		_tasks_semaphore.release();
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

		return enqueue (std::packaged_task<Result()> (std::move (invoke)));
	}


template<class Function, class ...Args>
	inline std::future<std::invoke_result_t<Function&&, std::decay_t<Args>...>>
	WorkPerformer::submit (Function&& function, Args&&... args)
	{
		using Result = std::invoke_result_t<Function&&, std::decay_t<Args>...>;
		auto invoke = [function = std::forward<Function> (function),
					   args = std::tuple<std::decay_t<Args>...> (std::forward<Args> (args)...)]() mutable -> decltype(auto)
		{
			return std::apply (function, std::move (args));
		};

		return enqueue (std::packaged_task<Result()> (std::move (invoke)));
	}

} // namespace neutrino

#endif
