# Telegram CLI

A full-featured Telegram client for Linux terminals, built in C++ using **TDLib** and **FTXUI**.

```
╔══════════════════════════════════════════════════════════════════════╗
║  TGCLI  | [●] Connected  |  YourName                             ║
╠═══════════╦══════════════════════════════════════════╦═════════════╣
║  Chats    ║  Alice                          14:32   ║  Info       ║
║───────────║  Hey! How are you?                      ║─────────────║
║  Alice  2 ║                                         ║  Private    ║
║  Bob      ║  You                            14:33   ║  Chat       ║
║  Group A  ║  I'm good! Working on tgcli             ║             ║
║  Chan B   ║                                         ║  Actions:   ║
║           ║  Alice                          14:35   ║  :stars     ║
║           ║  Cool! Show me when it's done           ║  :gifts     ║
║           ╠══════════════════════════════════════════╣             ║
║           ║  > Type a message...                    ║             ║
╚═══════════╩══════════════════════════════════════════╩═════════════╝
```

## Features

- **Full Telegram messaging** — send, receive, reply, forward, edit, delete
- **Real-time updates** — messages appear instantly via TDLib push
- **exteraGram badges** — supporter/developer checkmarks from exteraGram API
- **Telegram Stars** — view balance, transaction history
- **Gifts & NFTs** — browse, send, convert to NFT, transfer collectibles
- **Premium detection** — shows premium badge next to usernames
- **3 color themes** — Dark, Nord, Gruvbox
- **Nerd Font icons** — JetBrains Mono Nerd Font symbols throughout
- **Keyboard-first** — full navigation without a mouse

## Requirements

- Linux (Ubuntu/Debian, Arch, Fedora)
- C++20 compiler (GCC 10+ or Clang 12+)
- CMake 3.14+
- libcurl
- JetBrains Mono Nerd Font (for proper icon display)

## Installation

### Method 1: Install Pre-built Binary (Recommended)
Once a release is published on GitHub, you can install the latest version system-wide using our script:

```bash
curl -sL https://raw.githubusercontent.com/JettaXP/Telegram-CLI/main/install.sh | sudo bash
```

After installation, just run:
```bash
tgcli
```

### Method 2: Build from Source

```bash
# 1. Install system dependencies
chmod +x scripts/install_deps.sh
./scripts/install_deps.sh

# 2. Build TDLib (one-time, takes ~20 min)
chmod +x scripts/build_tdlib.sh
./scripts/build_tdlib.sh

# 3. Build Telegram CLI
mkdir -p build && cd build
cmake .. -DTd_DIR=$(pwd)/../deps/td-install/lib/cmake/Td
make -j$(nproc)

# 4. Install system-wide
sudo make install

# 5. Run from anywhere
tgcli
```

## Keyboard Shortcuts

| Key | Action |
|-----|--------|
| `j` / `↓` | Navigate down in chat list |
| `k` / `↑` | Navigate up in chat list |
| `Enter` | Select chat / Send message |
| `/` | Toggle search in chat list |
| `F2` | Toggle info panel |
| `r` | Reply to last message |
| `e` | Edit last outgoing message |
| `d` | Delete last message |
| `f` | Forward last message |
| `PgUp/PgDn` | Scroll message history |
| `Esc` | Cancel reply/edit |
| `Ctrl+C` | Quit |

*Mouse is also supported for scrolling the chat list and message history!*

## Commands

Type in the message input bar (prefix with `:`):

| Command | Action |
|---------|--------|
| `:settings` | Information about config location |
| `:stars` | Toggle Stars balance panel |
| `:gifts` | Toggle Gifts & NFT panel |
| `:info` | Toggle info panel |
| `:theme dark` | Switch to Dark theme |
| `:theme nord` | Switch to Nord theme |
| `:theme gruvbox` | Switch to Gruvbox theme |
| `:react ❤️` | React to last message |
| `:logout` | Log out of Telegram |
| `:quit` | Exit the app |

## Media Support

- **Images & Photos:** To view images directly in your terminal, use a terminal emulator that supports advanced graphics protocols (like **Kitty**, **WezTerm**, or **iTerm2**).
- **Videos:** Video playback inside the terminal is possible using ASCII/block character rendering via **Chafa**. Make sure `chafa` is installed on your system.
*(Note: Full media integration is currently in experimental development.)*

## exteraGram Integration

This client integrates with the [exteraGram](https://exteragram.app) API to show supporter and developer badges next to usernames. The badges are fetched from `https://api.exteragram.app/api/v1/profiles` and cached locally.

- **Supporters** get a  (checkmark) badge in cyan
- **Developers** get a  (wrench) badge in gold

## Configuration

Config file: `~/.config/tgcli/config.ini`

```ini
# Telegram CLI Configuration
api_id=YOUR_API_ID
api_hash=YOUR_API_HASH
theme=dark
```

## Project Structure

```
tgcli/
├── CMakeLists.txt
├── scripts/
│   ├── install_deps.sh
│   └── build_tdlib.sh
├── src/
│   ├── main.cpp
│   ├── app/
│   │   ├── App.hpp/cpp        # Main app orchestrator
│   │   ├── Config.hpp/cpp     # Configuration management
│   │   ├── State.hpp          # Global reactive state
│   │   └── exteraGram.hpp/cpp # exteraGram API integration
│   ├── telegram/
│   │   ├── Client.hpp/cpp     # TDLib wrapper
│   │   ├── Auth.hpp/cpp       # Authentication flow
│   │   ├── Messages.hpp/cpp   # Messaging operations
│   │   ├── Stars.hpp/cpp      # Stars & payments
│   │   └── Gifts.hpp/cpp      # Gifts & NFT collectibles
│   └── ui/
│       ├── AuthScreen.hpp/cpp # Login screen
│       ├── StatusBar.hpp/cpp  # Top status bar
│       ├── ChatList.hpp/cpp   # Left sidebar
│       ├── ChatView.hpp/cpp   # Message history
│       ├── InputBar.hpp/cpp   # Message compose
│       ├── StarsPanel.hpp/cpp # Stars balance view
│       ├── GiftsPanel.hpp/cpp # Gifts/NFT browser
│       └── InfoPanel.hpp/cpp  # Chat info sidebar
└── README.md
```

## Tech Stack

| Component | Library |
|-----------|---------|
| Telegram API | [TDLib](https://github.com/tdlib/td) — official Telegram C++ library |
| Terminal UI | [FTXUI](https://github.com/ArthurSonzogni/FTXUI) — modern declarative TUI |
| JSON | [nlohmann/json](https://github.com/nlohmann/json) — header-only JSON |
| HTTP | libcurl — for exteraGram API |
| Build | CMake 3.14+ |

## License

MIT
