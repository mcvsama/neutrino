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
#include <ostream>
#include <stdexcept>
#include <streambuf>


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
