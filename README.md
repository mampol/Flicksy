# Flicksy

Flicksy は、スマートフォンのフリック入力を 
Windows PC の文字入力として利用するためのツールです。

スマホ側に専用アプリは不要で、ブラウザから接続します。

## Structure

```text
Smartphone
    ↓
Browser
    ↓
HTTP
    ↓
Flicksy
    ↓
SendInput / Clipboard
    ↓
Windows Application
```

## Status

Currently under development.

## Screenshot

server
<img src="images/flicksy_server.png">

client
<img src="images/flicksy_client.png">

## Roadmap

- System tray support
- Dark mode
- Improved logging
- Startup behavior improvements
- Better mobile UI

## Development

- C++17
- Win32 API
- Visual Studio 2022
- HTML / JavaScript

## Dependencies

- [cpp-httplib](https://github.com/yhirose/cpp-httplib) - MIT License
- [QR-Code-generator](https://github.com/nayuki/QR-Code-generator) - MIT License
- [toml++](https://github.com/marzer/tomlplusplus) - MIT License

All third-party libraries listed above are licensed under the MIT License.
See `THIRD_PARTY_LICENSES.txt` for details.

## License

This project is licensed under the MIT License.
