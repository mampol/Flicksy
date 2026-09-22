# Flicksy v0.4.1.0

### English

Flicksy lets you use your smartphone's flick input
to enter text on a Windows PC.

No dedicated mobile app is required.
Just connect through your web browser.

<p align="center">• • •</p>

### 日本語

Flicksy は、スマートフォンのフリック入力を
Windows PC の文字入力として利用するためのツールです。

スマホ側に専用アプリは不要で、
ブラウザから接続します。

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

## How to use

### English

1. Start `Flicksy.exe` on your Windows PC.
2. Click `START` to start the local server.
3. Make sure your smartphone and PC are connected to the **same network**.
4. Scan the QR code shown in Flicksy with your smartphone.
5. Open the displayed page in your browser.
6. Enter text on your smartphone and send it to the PC.
7. The text is entered into the currently active window on your PC.

**Note:** When `Start server automatically` is enabled, Flicksy starts in the system tray on the next launch without showing the main window.

<p align="center">• • •</p>

### 日本語

1. Windows PCで Flicksy.exe を起動します。
2. START を押してサーバーを開始します。
3. PCとスマートフォンを**同じネットワークに接続**します。
4. Flicksyに表示されたQRコードをスマートフォンで読み取ります。
5. ブラウザでFlicksyのページを開きます。
6. スマートフォンで文字を入力し、PCへ送信します。
7. 入力データはアクティブなウィンドウへ入力されます。

**Note:** 「Start server automatically」をチェックすると次回起動時にタスクトレイにのみ常駐し、メインウィンドウは表示されません

## Status

Currently under development.

### Security / セキュリティ
- Added token-based authentication for client requests.
- クライアントからの操作要求にトークン認証を追加しました。

## Screenshot

### server
<img src="images/flicksy_server.png" width="400">

### client
<img src="images/flicksy_client.png" width="250">

## Roadmap

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
