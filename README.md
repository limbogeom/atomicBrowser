# atomicBrowser

A lightweight, text-based terminal web browser written in C++. It features an interactive Graphical User Interface (GUI) rendered directly in the terminal using ANSI escape codes, a custom HTML lexer/tokenizer, a basic CSS engine supporting element blocking layout styles, and support for both remote network fetching and local offline HTML document rendering.

## Features

- **Local & Remote Browsing:** Smoothly resolves and renders both live web pages via network requests and local static files (supporting relative paths, absolute paths, and home directory `~/` expansion).
- **Custom CSS Engine:** Implements cascading styles supporting text alignment (`left`, `center`, `right`), text margins, text/background coloring, and structural block control via the `display` property (`block`, `inline`, `none`).
- **Dynamic HTML Parsing:** Features a custom lexical analyzer that processes tags, tokenizes web documents, and builds an in-memory Document Object Model (DOM) specialized for terminal viewports.
- **Interactive Terminal UI:** Navigable interface containing an interactive address bar overlay, focus shifting for input fields, hyperlinks, and form action hooks.

---

## Controls

Once the browser is running, use the following key bindings to navigate pages:

| Key | Description |
| :--- | :--- |
| `Tab` / `Down Arrow` / `Right Arrow` | Move focus forward to the next interactive element (Input, Link, Button, or Address Bar). |
| `Up Arrow` / `Left Arrow` | Move focus backward to the previous interactive element. |
| `Enter` | Trigger the focused action (Navigate link, submit form, or focus URL entry). |
| `Backspace` / `Delete` | Erase characters when typing inside text inputs or the address bar. |
| `B` / `b` | Navigate backward in history. |
| `F` / `f` | Navigate forward in history. |
| `Q` / `q` | Quit the application cleanly and restore terminal raw mode. |

---

## Supported HTML & CSS Specifications

### HTML Tags
- Headers: `<h1>`, `<h2>`, `<h3>` (pre-styled with explicit terminal color schemes)
- Structure: `<div>`, `<p>`, `<br>`, `<hr>`, `<blockquote>`
- Formatting: `<b>`, `<strong>`, `<i>`, `<em>`
- Interactive: `<form>`, `<input>`, `<button>`, `<a href="...">`
- Layout Tables: `<table>`, `<tr>`, `<th>`, `<td>`

### CSS Properties
- `color`: `red`, `green`, `blue`, `yellow`, `black`, `white`
- `background` / `bg`: Accepts standard ANSI terminal colors.
- `text-align`: `left`, `center`, `right`
- `margin-left` / `margin-right`: Indentation styling using padding integers.
- `display`: Handles `block` (forces text breaks), `inline` (flows continuously), and `none` (recursively hides elements and prevents their children from rendering).

---

## Examples of URL Inputs

You can type the following directly into the browser's address bar:

1. **Internal Sandbox Home:** `home` or leave blank to return to the interactive engine presentation page.
2. **Local Relative Files:** `test.html` or `./docs/index.html` (The browser automatically reads it relative to your working execution path).
3. **Absolute Local Paths:** `/home/user/workspace/page.html` or `C:/Users/Admin/Desktop/index.html`
4. **Live Websites:** `example.com` (Will automatically prepend `https://` protocols and attempt to pull socket packets over the web stream).

---

## Building and Running

### Prerequisites
- A modern C++ compiler supporting at least **C++17** or higher (`g++`, `clang++`, or MSVC).
- A Unix-like terminal environment (Linux, macOS, WSL) or a Windows console setup supporting standard ANSI escape codes.

### Compilation
Navigate to the root project folder containing your header and implementation files, then execute the compiler command:

```bash
g++ -std=c++17 main.cpp Browser.cpp HtmlLexer.cpp Terminal.cpp -o SimpleBrowser
