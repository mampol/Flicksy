# Flicksy v0.3.1.0

Flicksy lets you use your smartphone's flick input
to enter text on a Windows PC.

No dedicated mobile app is required.
Just connect through your web browser.

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

### server
<img src="images/flicksy_server.png" width="400">

### client
<img src="images/flicksy_client.png" width="300">

## Roadmap

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


Visual Studio project files are not included.
Please create a new C++17 Win32 project and add the source files manually.
