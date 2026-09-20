# Flicksy

Flicksy は、スマートフォンのフリック入力を 
Windows PC の文字入力として利用するためのツールです。

スマホ側に専用アプリは不要で、ブラウザから接続します。

## Features

- スマホ標準IMEをそのまま利用
- ローカルHTTP経由で文字列送信
- QRコード接続
- Enter / Tab / Esc / BS
- ↑ / ↓ / ← / →
- SendInputによる文字入力
- Clipboard経由入力にも対応予定

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

現在開発中です。

## 今後は、

- メインウィンドウUI
- ログ表示
- タスクトレイ常駐
- Dark / Lightテーマ
- SendInput / Clipboard切替

などを実装予定です。

## Development
- C++17
- Win32 API
- Visual Studio 2022
- HTML / JavaScript

## Dependencies

- [QR-Code-generator](https://github.com/nayuki/QR-Code-generator)
- [cpp-httplib](https://github.com/yhirose/cpp-httplib)

Both libraries are licensed under the MIT License.
See `THIRD_PARTY_LICENSES.txt` for details.

## License

This project is licensed under the MIT License.
