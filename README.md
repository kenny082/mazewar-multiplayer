# MazeWar Multiplayer Server

A concurrent multiplayer game server built in **C**, using **POSIX threads**, **sockets**, and **semaphores** to support real-time communication between clients in a grid-based maze environment.

## Features

- Supports up to **26 concurrent players**, each represented by a unique avatar
- Real-time movement, chat, and laser-based interactions across a shared maze
- Custom **binary protocol** for efficient client-server communication
- Thread-safe state management using **mutexes** and **fine-grained locking**
- Modular architecture:
  - `server.c`, `player.c`, `protocol.c`, `maze.c`, `client_registry.c`, `main.c`

## Technologies Used

- **C (C99)**
- **POSIX threads** & **semaphores**
- **TCP sockets**
- **Makefile** for build automation

## Architecture Overview

- **server.c** — Initializes TCP socket, accepts client connections, and spawns handler threads
- **protocol.c** — Parses packets, formats messages, and routes commands
- **player.c / maze.c** — Manages in-game logic and player/maze state
- **client_registry.c** — Tracks active players and maintains synchronization
- **main.c** — Entry point, handles lifecycle and signal-based shutdown

## Setup & Usage

### Build

```bash
make
```

### Additional Targets

```bash
make all     # Builds all binaries
make clean   # Removes compiled binaries
make debug   # Builds with debug symbols (for gdb)
```

These can be combined as needed:

```bash
make clean all
make debug
```

### Run

After building the project, follow these steps to run the game:

#### 1. Server Setup

Launch the server on a valid port (1024–65535):

```bash
bin/mazewar -p 3333
```

#### 2. Client Joining (up to 26 clients)

Clients can connect using:

```bash
util/gclient -p 3333 [-a <Avatar>] [-u <Username>]
```

Example:

```bash
util/gclient -p 3333 -a Z -u Kenny
```

- `-a` specifies the avatar character (one letter)
- `-u` sets the player’s username

#### 3. Play the Game

- All clients can move, chat, and fire lasers in real time
- The server handles synchronization, broadcasting, and protocol coordination
- Can be hosted locally or over a LAN/internet
