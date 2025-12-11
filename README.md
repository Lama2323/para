# Game Server Simulation - Task Pipeline Demo

This project simulates a game server architecture using a task-based parallelism model (pipelining). It benchmarks the performance difference between a sequential approach and a concurrent task-based approach using a custom thread pool.

## Requirements

- C++17 compatible compiler (e.g., g++, clang++, MSVC)
- Windows (for the commands below) or Linux (commands will vary slightly)

## Build

You can compile the project using `g++` directly. Run the following command in the project root:

```powershell
g++ -std=c++17 -O2 -Isrc -pthread src/main.cpp src/game/match.cpp src/game/game_server.cpp src/client/client.cpp -o game_server.exe
```

## Run

To start the simulation benchmark:

```powershell
.\game_server.exe
```

## Stop Process (Kill)

If you need to forcefully stop the running game server process(es) without closing your terminal window, or if it hangs:

```powershell
taskkill /F /IM game_server.exe
```

## Project Structure

- `src/main.cpp`: Entry point and benchmark logic.
- `src/game/`: Game server logic and match processing.
- `src/client/`: Client simulation and input generation.
- `src/scheduler/`: Thread pool and task scheduling system.
- `src/common/`: Shared types and data structures.
