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
#include <neutrino/logger.h>
#include <neutrino/test/test_asserts.h>
#include <neutrino/test/test_suite.h>

// Standard:
#include <barrier>
#include <cstddef>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <streambuf>
#include <thread>
#include <vector>


namespace neutrino::test {
namespace {

struct LogWriteError
{ };


class ThrowingBuffer: public std::streambuf
{
  public:
	unsigned attempts { 0 };

  protected:
	// API of std::streambuf
	std::streamsize
	xsputn (char const*, std::streamsize) override;
};


std::streamsize
ThrowingBuffer::xsputn (char const*, std::streamsize)
{
	++this->attempts;
	throw LogWriteError();
}


TestRegistration tests ([]{
	describe ("Neutrino", "Logger", []{
		it ("supports concurrent copies sharing an output", []{
			std::ostringstream stream;
			LoggerOutput output (stream);
			Logger const source (output);
			constexpr std::size_t thread_count = 4;
			std::barrier start (static_cast<std::ptrdiff_t> (thread_count));
			std::vector<std::jthread> workers;
			workers.reserve (thread_count);

			for (std::size_t thread = 0; thread < thread_count; ++thread)
			{
				workers.emplace_back ([&]{
					start.arrive_and_wait();

					for (std::size_t i = 0; i < 2000; ++i)
					{
						Logger copy (source);
						Logger assigned;
						assigned = copy;
						assigned = source;
					}
				});
			}
		});

		it ("handles write failures when destroying a log block", []{
			ThrowingBuffer buffer;
			std::ostream stream (&buffer);
			stream.exceptions (std::ios::badbit | std::ios::failbit);
			LoggerOutput output (stream);
			output.set_timestamps_enabled (false);

			{
				LogBlock block (&output);
				block << "log entry\n";
			}

			test_asserts::verify_equal ("failed write does not escape destruction", buffer.attempts, 1u);
		});

		it ("propagates write failures from explicit flush calls", []{
			ThrowingBuffer buffer;
			std::ostream stream (&buffer);
			stream.exceptions (std::ios::badbit | std::ios::failbit);
			LoggerOutput output (stream);
			output.set_timestamps_enabled (false);
			LogBlock block (&output);
			block << "log entry\n";

			test_asserts::verify_throws<LogWriteError> ("explicit flush reports the original write error", [&block] {
				block.flush();
			});
			test_asserts::verify_equal ("explicit flush attempts to write the entry", buffer.attempts, 1u);
		});

		it ("preserves the active exception when a destructor fails to flush", []{
			ThrowingBuffer buffer;
			std::ostream stream (&buffer);
			stream.exceptions (std::ios::badbit | std::ios::failbit);
			LoggerOutput output (stream);
			output.set_timestamps_enabled (false);

			test_asserts::verify_throws<std::runtime_error> ("the original exception survives stack unwinding", [&output] {
				LogBlock block (&output);
				block << "log entry\n";
				throw std::runtime_error ("original exception");
			});
			test_asserts::verify_equal ("destructor attempts to write during stack unwinding", buffer.attempts, 1u);
		});
	});
});

} // namespace
} // namespace neutrino::test
