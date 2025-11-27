# XRay Monitor

Lightweight XRay monitor daemon with Telegram integration. So far **vless** protocol only.

## Conditions of Working

For the monitor to work, the following must be satisfied:

* The XRay server configuration file must define an access log file (key: `log.access`)
* In the XRay server configuration file, each user must have an `email` field specified (key:  `inbounds[protocol=vless].settings.clients[email]`)
* The monitor must be run under the same user account as the xray service.

> [! IMPORTANT]
> The XRay server configuration file by default is here `/usr/local/etc/xray/config.json`, but if is it don't so, set option `--xray-config-path` or `-c`!

## Command Line Options

| Value | Desc | Required | Default |
| --- | --- | --- | --- |
| --help, -h | Show help message | - | - |
| --version, -v | Just show version | - | - |
| --xray-config-path, -c | XRay config file path | - | `/usr/local/etc/xray/config.json` |
| --log-level, -l | Log level (trace, debug, info, warning, error, fatal) | - | info |
| --log-filepath | Log file path | - | - |
| --interval, -i | Server log file polling interval in seconds | - | 10 |
| --telegram-token, -t | Telegram bot token. If not specified - no to be notifications | - | - |
| --telegram-channel | Telegram channel ID | - | - |

## System Requiremts:

* Ubuntu 20.04+
* Debian 11+
* CentOS 8+ / AlmaLinux 8+
* Most modern Linux x86_64 systems

You may require only `libstdc++6`, but usually it already exist.

## Set Up on Ubuntu with Systemd

* Copy release binary in `/usr/local/bin`
* Make systemd file ([example](xray-monitor.service)) in `/etc/systemd/system`
* Type command line options in it (line what starts with `ExecStart=`)
* `systemctl daemon-reload`
* `systemctl start xray-monitor`

## Dev Requiremts

* C++17
* cmake
* Boost 1.83: program_options, json, log
