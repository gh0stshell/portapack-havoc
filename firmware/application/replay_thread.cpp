/*
 * Copyright (C) 2016 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include "replay_thread.hpp"

#include "baseband_api.hpp"
#include "buffer_exchange.hpp"

// DEBUG:
#include "hackrf_gpio.hpp"
using namespace hackrf::one;

struct BasebandReplay {
	BasebandReplay(ReplayConfig* const config) {
		baseband::replay_start(config);
	}

	~BasebandReplay() {
		baseband::replay_stop();
	}
};

// ReplayThread ///////////////////////////////////////////////////////////

ReplayThread::ReplayThread(
	std::unique_ptr<stream::Reader> reader,
	size_t read_size,
	size_t buffer_count,
	std::function<void()> success_callback,
	std::function<void(File::Error)> error_callback
) : config { read_size, buffer_count },
	reader { std::move(reader) },
	success_callback { std::move(success_callback) },
	error_callback { std::move(error_callback) }
{
	// Need significant stack for FATFS
	thread = chThdCreateFromHeap(NULL, 1024, NORMALPRIO + 10, ReplayThread::static_fn, this);
}

ReplayThread::~ReplayThread() {
	if( thread ) {
		chThdTerminate(thread);
		chThdWait(thread);
		thread = nullptr;
	}
}

msg_t ReplayThread::static_fn(void* arg) {
	auto obj = static_cast<ReplayThread*>(arg);
	const auto error = obj->run();
	if( error.is_valid() && obj->error_callback ) {
		obj->error_callback(error.value());
	} else {
		if( obj->success_callback ) {
			obj->success_callback();
		}
	}
	return 0;
}

Optional<File::Error> ReplayThread::run() {
	BasebandReplay replay { &config };
	BufferExchange buffers { &config };

	while( !chThdShouldTerminate() ) {
		auto buffer = buffers.get();
		/*auto read_result = reader->read(buffer->data(), buffer->size());
		if( read_result.is_error() ) {
			return read_result.error();
		}
		buffers.put(buffer);*/
		chThdSleep(50);		// DEBUG
		//led_tx.toggle();
	}

	return { };
}
