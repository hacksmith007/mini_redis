# Mini Redis

Mini Redis is a lightweight Redis-compatible server simulator written in modern C++.
It implements basic Redis commands, persistence, and periodic background tasks to demonstrate how a simple in-memory key-value store can be built and extended.

## Features

- TCP-based Redis-like server running on port `8080`
- Supports commands:
  - `SET key value`
  - `SETEX key ttl value`
  - `GET key`
  - `DEL key`
  - `compact` (compacts the append-only file)
- Persistent storage through AOF and snapshotting
- Background scheduler for periodic expiry cleanup and snapshot persistence
- Configurable logging via `config.txt` using `REDIS_LOG_LEVEL`

## Project Layout

- `Server/server.cpp` - main server entrypoint, socket handling, and command loop
- `Store/store.cpp` - in-memory store, persistence, and AOF handling
- `Parser/parser.cpp` - command parsing and dispatch
- `Logging/RedisLogger.cpp` - logging implementation with log-level control
- `Scheduler/scheduler.cpp` - simple periodic task scheduler
- `Utility/` - shared helpers and support utilities
- `include/` - common headers and shared interfaces

## Build Instructions

From the `mini_redis` folder, run:

```bash
cmake --preset mini_redis_cmake_preset
cmake --build --preset mini_redis_build_preset
```

This generates the `mini_redis` executable in the build directory.

## Run

Start the server from the `mini_redis` folder or the generated build directory:

```bash
./mini_redis
```

The server listens on `localhost:8080` and accepts a single client connection.

## Usage

You can connect to the server using a TCP client such as `nc`:

```bash
nc localhost 8080
```

Then issue supported commands:

```text
SET mykey hello world
GET mykey
DEL mykey
compact
exit
```

Type `exit` to close the client connection.

## Configuration

The server reads `REDIS_LOG_LEVEL` from `config.txt` to control log verbosity.

Example entry:

```text
REDIS_LOG_LEVEL=DEBUG
```

## Notes

- The current implementation is single-client only.
- Periodic tasks include snapshot persistence and expiry cleanup every 5 seconds.
- Logging is recorded in `redis.log` and `redis_error.log`.
