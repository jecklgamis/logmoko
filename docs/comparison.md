# Comparison with Other C/C++ Logging Frameworks

| Feature | logmoko | spdlog | quill | fmtlog | g3log | zlog | log.c | log4c | syslog |
|---|---|---|---|---|---|---|---|---|---|
| **Language** | C | C++ | C++ | C++ | C++ | C | C | C | C (POSIX) |
| **I/O model** | Async | Async | Async | Async | Async | Sync | Sync | Sync | Kernel IPC |
| **Named loggers** | Yes | Yes | Yes | No (global) | No (LogWorker) | Yes (categories) | No (global) | Yes (categories) | No |
| **Log levels** | TRACE–OFF (6) | trace–off (7) | trace–off (7) | DBG–OFF (5) | DEBUG/INFO/WARN/FATAL (4+) | DEBUG–FATAL (6) | TRACE–FATAL (6) | TRACE–FATAL (7) | emerg–debug (8) |
| **File output** | Yes | Yes | Yes | Yes | Yes | Yes | Yes | Yes | No |
| **Console output** | Yes | Yes | Yes | No | No | No | Yes (stderr) | Yes | No |
| **Syslog output** | Yes | Yes | No | No | No | Yes | No | Yes | Yes (is syslog) |
| **UDP/socket output** | Yes ¹ | No | No | No | No | No | No | No | No |
| **Log rotation** | Yes | Yes | No | No | No | Yes | No | Yes | No |
| **Custom format** | Yes (fn ptr) | Yes (pattern) | Yes (pattern) | No | No | Yes (config) | No | Yes (config) | No |
| **Config file** | Yes (INI) | No | No | No | No | Yes (zlog.conf) | No | Yes (XML/.props) | No |
| **Multiple sinks per logger** | Yes | Yes | Yes | No | No | Yes | No | Yes | No |
| **Drop counter** | Yes | Yes | No | No | No | N/A | N/A | N/A | N/A |
| **Header-only** | No | Optional | No | Yes (vendored) | No | No | Yes (vendored) | No | N/A |
| **Crash/fatal handler** | No | No | No | No | Yes | No | No | No | No |
| **Re-initializable** | Yes | Yes | Yes | Yes | Yes | Yes | Yes | No | Yes |
| **Transport encryption** | Yes (AES-256-GCM) | N/A | N/A | N/A | N/A | N/A | N/A | N/A | N/A |

¹ UDP socket handler with optional AES-256-GCM encryption.

See [benchmark.md](benchmark.md) for performance comparisons.
