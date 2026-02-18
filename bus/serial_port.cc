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

// Local:
#include "serial_port.h"

// Neutrino:
#include <neutrino/numeric.h>
#include <neutrino/string.h>

// System:
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>

// Standard:
#include <cstddef>
#include <format>
#include <map>
#include <utility>


namespace neutrino {

SerialPort::SerialPort (DataReadyCallback data_ready, FailureCallback failure):
	_data_ready (std::move (data_ready)),
	_failure (std::move (failure))
{ }


SerialPort::SerialPort (SerialPort&& other) noexcept
{
	*this = std::move (other);
}


SerialPort::~SerialPort()
{
	close();
}


SerialPort&
SerialPort::operator= (SerialPort&& other) noexcept
{
	_owned = std::move (other._owned);
	_logger = other._logger;
	_configuration = other._configuration;
	_data_ready = other._data_ready;
	_failure = other._failure;
	_notifier = std::move (other._notifier);
	_device = other._device;
	_good = other._good;
	_error = other._error;
	_read_failure_count = other._read_failure_count;
	_max_read_failure_count = other._max_read_failure_count;
	_write_failure_count = other._write_failure_count;
	_max_write_failure_count = other._max_write_failure_count;
	_input_buffer = std::move (other._input_buffer);
	_output_buffer = std::move (other._output_buffer);
	return *this;
}


void
SerialPort::set_max_read_failures (unsigned int number)
{
	_max_read_failure_count = number;
}


void
SerialPort::set_max_write_failures (unsigned int number)
{
	_max_write_failure_count = number;
}


void
SerialPort::write (Blob const& data)
{
	if (!_good)
		throw WriteException ("can't write to serial port - device is closed");

	_output_buffer.append (data.begin(), data.end());

	int written = ::write (_device, _output_buffer.data(), _output_buffer.size());

	if (written == -1)
	{
		_logger << _log_prefix << "Write error " << strerror (errno) << std::endl;

		if (errno != EAGAIN && errno != EWOULDBLOCK)
		{
			_logger << _log_prefix << "Write failure (could not write " << _output_buffer.size() << " bytes)." << std::endl;
			_write_failure_count++;

			if (_write_failure_count > _max_write_failure_count)
				notify_failure ("multiple write failures");
		}
		else
			_logger << _log_prefix << "Write failure: would block." << std::endl;
	}
	else if (written < static_cast<int> (_output_buffer.size()))
	{
		_logger << _log_prefix << "Write buffer overrun." << std::endl;

		_output_buffer.erase (_output_buffer.begin(), _output_buffer.begin() + written);
	}
	else
	{
		_output_buffer.clear();
		_write_failure_count = 0;
	}
}


void
SerialPort::write (std::string const& data)
{
	_output_buffer.append (data.begin(), data.end());
	flush_async();
}


void
SerialPort::flush_async()
{
	write (Blob());
}


void
SerialPort::flush()
{
	::fsync (_device);
}


bool
SerialPort::open()
{
	_logger << _log_prefix << "Opening device " << _configuration._device_path << " at " << configuration().baud_rate() << std::endl;
	close();
	_device = ::open (_configuration._device_path.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);

	if (_device < 0)
	{
		auto es = strerror (errno);
		_error = es;
		_good = false;
		_logger << _log_prefix << "Could not open device file " << _configuration._device_path << ": " << es << std::endl;
	}
	else
	{
		if (set_device_options())
		{
			_logger << _log_prefix << "Open at " << configuration().baud_rate() << "." << std::endl;
			_good = true;
			_notifier = std::make_unique<QSocketNotifier> (_device, QSocketNotifier::Read, this);
			_notifier->setEnabled (true);
			QObject::connect (_notifier.get(), SIGNAL (activated (int)), this, SLOT (read()));
		}
		else
			_logger << _log_prefix << "Failed to set device parameters." << std::endl;
	}

	return good();
}


void
SerialPort::close()
{
	if (_owned)
	{
		_notifier.reset();

		if (_device)
		{
			::close (_device);
			_device = 0;
			_good = false;
			_error.clear();
			_read_failure_count = 0;
			_write_failure_count = 0;
			_input_buffer.clear();
			_output_buffer.clear();
		}
	}
}


speed_t
SerialPort::termios_baud_rate (unsigned int baud_rate)
{
	static std::map<unsigned int, speed_t> const baud_rates_map {
		{ 50u, B50 },
		{ 75u, B75 },
		{ 110u, B110 },
		{ 134u, B134 },
		{ 150u, B150 },
		{ 200u, B200 },
		{ 300u, B300 },
		{ 600u, B600 },
		{ 1200u, B1200 },
		{ 1800u, B1800 },
		{ 2400u, B2400 },
		{ 4800u, B4800 },
		{ 9600u, B9600 },
		{ 19200u, B19200 },
		{ 38400u, B38400 },
		{ 57600u, B57600 },
		{ 115200u, B115200 },
		{ 230400u, B230400 },
		{ 460800u, B460800 },
		{ 500000u, B500000 },
		{ 576000u, B576000 },
		{ 921600u, B921600 },
		{ 1000000u, B1000000 },
		{ 1152000u, B1152000 },
		{ 1500000u, B1500000 },
		{ 2000000u, B2000000 },
		{ 2500000u, B2500000 },
		{ 3000000u, B3000000 },
		{ 3500000u, B3500000 },
		{ 4000000u, B4000000 },
	};

	auto c = baud_rates_map.find (baud_rate);

	if (c == baud_rates_map.end())
		c = baud_rates_map.upper_bound (baud_rate);

	if (c == baud_rates_map.end())
		return 0;

	return c->second;
}


speed_t
SerialPort::termios_baud_rate (std::string_view const baud_rate_str)
{
	return parse<speed_t> (baud_rate_str);
}


void
SerialPort::read()
{
	if (!_good)
		return;

	std::string buffer;

	bool err = false;
	bool exc = Exception::catch_and_log (_logger, [&]{
		// Read as much as possible:
		for (;;)
		{
			std::string::size_type prev_size = buffer.size();
			std::string::size_type try_read = 4096;
			buffer.resize (prev_size + try_read);
			auto sn = ::read (_device, &buffer[prev_size], try_read);

			if (sn < 0)
			{
				if (errno == EAGAIN || errno == EWOULDBLOCK)
				{
					// Nothing to read (read would block)
					buffer.resize (prev_size);
					_logger << _log_prefix << "Nothing to read (read would block)." << std::endl;
					break;
				}
				else
				{
					_logger << _log_prefix << "Error while reading from serial port: " << strerror (errno) << std::endl;
					err = true;
					break;
				}
			}
			else
			{
				auto n = neutrino::to_unsigned (sn);

				buffer.resize (prev_size + n);

				if (n == 0)
				{
					_logger << _log_prefix << "Read failure (0 bytes read by read())." << std::endl;
					_read_failure_count++;
					if (_read_failure_count > _max_read_failure_count)
						notify_failure ("multiple read failures");
				}

				if (n < try_read)
					break;
			}
		}
	});

	if (exc || err)
		notify_failure ("read()");

	if (!buffer.empty())
	{
		_input_buffer.append (buffer.begin(), buffer.end());
		if (_data_ready)
			_data_ready();
	}
}


bool
SerialPort::set_device_options()
{
	std::string parity_str = "none";

	if (_configuration._parity == Parity::Odd)
		parity_str = "odd";
	else if (_configuration._parity == Parity::Even)
		parity_str = "even";

	_logger << _log_prefix << "Setting baud rate: " << _configuration._baud_rate << ", data bits: " << _configuration._data_bits
			<< ", parity: " << parity_str << ", stop bits: " << _configuration._stop_bits << std::endl;

	termios options;
	bzero (&options, sizeof (options));

	// Min characters to be read:
	options.c_cc[VMIN] = _configuration._vmin;
	// Time to wait for data (tenths of seconds):
	options.c_cc[VTIME] = _configuration._vtime;
	// Set output and local modes to defaults:
	options.c_cflag = CREAD | CLOCAL;

	switch (_configuration._data_bits)
	{
		case 5: options.c_cflag |= CS5; break;
		case 6: options.c_cflag |= CS6; break;
		case 7: options.c_cflag |= CS7; break;
		case 8: options.c_cflag |= CS8; break;
	}

	if (_configuration._stop_bits == 2)
		options.c_cflag |= CSTOPB;

	switch (_configuration._parity)
	{
		case Parity::None:	break;
		case Parity::Odd:	options.c_cflag |= PARENB | PARODD; break;
		case Parity::Even:	options.c_cflag |= PARENB; break;
	}

	if (_configuration._rtscts)
		options.c_cflag |= CRTSCTS;

	options.c_iflag = IGNPAR;
	options.c_oflag = 0;
	options.c_lflag = 0;

	auto const baud_rate_const = termios_baud_rate (_configuration._baud_rate);
	cfsetispeed (&options, baud_rate_const);
	cfsetospeed (&options, baud_rate_const);

	tcflush (_device, TCIOFLUSH);

	if (tcsetattr (_device, TCSAFLUSH, &options) != 0)
	{
		_logger << _log_prefix << "Could not setup serial port: " << _configuration._device_path << ": " << strerror (errno) << std::endl;
		return false;
	}

	if (tcflow (_device, TCOON | TCION) != 0)
	{
		_logger << _log_prefix << "Could not enable flow: tcflow(): " << _configuration._device_path << ": " << strerror (errno) << std::endl;
		return false;
	}

	return true;
}


void
SerialPort::notify_failure (std::string const& message)
{
	_error = message;
	_logger << _log_prefix << "Failure detected: " << message << std::endl;

	if (_failure)
		_failure();

	close();
}

} // namespace neutrino
