# fcitx5-enhint

Show the English meaning of each Chinese candidate **above** the candidate in
the fcitx5 candidate window.

```
wo
I, me        to hold        to lie            <-- English hints (new)
1. 我         2. 握          3. 卧             <-- candidates
```

The hint is looked up **offline** from [CC-CEDICT](https://cc-cedict.org/), so
no network access is needed while typing.

**English** · [中文](README.md)

---

## Features

* English hint for every candidate, rendered on its own line above the Chinese
  word.
* Works on **stock fcitx5** — it is an ordinary addon, no patched fcitx5 needed.
* Offline dictionary lookup (CC-CEDICT), instant and privacy friendly.
* Configurable number of glosses, separator, and an optional toggle hotkey.
* Keeps candidates vertically aligned even when a candidate has no translation.

## How it works

* The addon connects to the core `Instance::OutputFilter` signal, the same hook
  used by the Simplified/Traditional converter.
* For the text of each candidate it looks up CC-CEDICT and prepends the English
  gloss as an extra line.
* fcitx5's `classicui` module already renders multi-line candidate text stacked
  vertically, which produces the layout shown above.

Because it only hooks the generic output filter, it keeps working on stock
fcitx5 (>= 5.1.20) and can ship as a normal addon package.

## Requirements

* fcitx5 >= 5.1.20 (multi-line candidate rendering)
* fcitx5-chinese-addons — provides the `pinyin` input method
* The `classicui` user interface (the default candidate window)
* CC-CEDICT (see [Dictionary](#dictionary))

## Installation

### Option A — system-wide (recommended)

```sh
cmake -B build -G Ninja
cmake --build build
sudo cmake --install build      # installs to /usr/lib/fcitx5 + /usr/share/fcitx5
fcitx5 -r
```

Configure a dictionary path at build time so it is installed system wide:

```sh
cmake -B build -G Ninja -DENHINT_CEDICT=/path/to/cedict_ts.u8
sudo cmake --install build
```

### Option B — user-local (no root)

Install the library and addon config into your home:

```sh
cmake -B build -G Ninja
cmake --build build
mkdir -p ~/.local/lib/fcitx5 ~/.local/share/fcitx5/addon
cp build/src/libenhint.so ~/.local/lib/fcitx5/
cp build/src/enhint.conf  ~/.local/share/fcitx5/addon/
```

`~/.local/lib/fcitx5` is **not** searched by default, so tell fcitx5 about it
through `FCITX_ADDON_DIRS`. Note that this variable **replaces** the default
path, so you must include `/usr/lib/fcitx5` as well.

For a session started by systemd (most Wayland setups), create an
`environment.d` drop-in and restart the service:

```sh
cat > ~/.config/environment.d/95-fcitx5-enhint.conf <<'EOF'
FCITX_ADDON_DIRS=$HOME/.local/lib/fcitx5:/usr/lib/fcitx5
EOF
# apply to the current session:
systemctl --user set-environment \
  FCITX_ADDON_DIRS="$HOME/.local/lib/fcitx5:/usr/lib/fcitx5"
systemctl --user restart 'app-org.fcitx.Fcitx5@autostart.service'
```

If fcitx5 is started from a shell or a compositor config instead, set the same
variable there, e.g.

```sh
FCITX_ADDON_DIRS="$HOME/.local/lib/fcitx5:/usr/lib/fcitx5" fcitx5 -r
```

## Dictionary

Download CC-CEDICT into the user data directory:

```sh
tools/fetch-cedict.sh
# writes ~/.local/share/fcitx5/enhint/cedict_ts.u8
```

The addon searches these locations (first match wins):

1. `~/.local/share/fcitx5/enhint/cedict_ts.u8`
2. `~/.local/share/fcitx5/enhint/cedict.u8`
3. `~/.local/share/fcitx5/enhint/cedict.txt`
4. the same file names under `<prefix>/share/fcitx5/enhint/`

If no dictionary is found, the addon stays idle and logs a warning. The
dictionary is parsed on a background thread, so it does not slow down fcitx5
startup.

## Configuration

Config file: `~/.config/fcitx5/conf/enhint.conf`

```ini
[Enhint]
Enabled=True
MaxGlosses=2
Separator=", "
SkipPrograms=
Hotkey=
```

| Key            | Default | Description                                                              |
|----------------|---------|--------------------------------------------------------------------------|
| `Enabled`      | `True`  | Show the hints.                                                          |
| `MaxGlosses`   | `2`     | Number of English glosses per candidate (1–8).                           |
| `Separator`    | `", "`  | String inserted between glosses.                                         |
| `SkipPrograms` | empty   | Program names that must not be decorated (see below).                    |
| `Hotkey`       | empty   | Optional key to toggle the hints, e.g. `Control+Alt+E`.                  |

The toggle action can also be added to the input method tray/menu.

## Client-side candidate windows

Some applications draw their **own** candidate window instead of letting fcitx5
draw it. They declare the `ClientSideInputPanel` capability and receive the
candidate text verbatim. A few of them insert that text directly on selection,
which would leak the English hint into the input.

If that happens in a specific application, add its program name to
`SkipPrograms`:

```ini
SkipPrograms=some-app
```

You can find the program name from the fcitx5 log; the addon logs one line per
program that uses a client-side candidate window.

## Troubleshooting

* **No hints at all** — check `fcitx5 -r --verbose default=4` output for
  `Loaded addon enhint`. If it is missing, the addon library is not on the addon
  path (see installation) or `enhint.conf` is not found.
* **`Could not locate library libenhint.so`** — `FCITX_ADDON_DIRS` is not set
  (user-local install) or the library was not installed.
* **`no CC-CEDICT dictionary found`** — run `tools/fetch-cedict.sh`.
* **Hints appear in the wrong application / get typed in** — add that program
  to `SkipPrograms`.

## Development

```sh
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

The dictionary parser can be tested standalone by compiling `src/cedict.cpp`.

## Project layout

```
CMakeLists.txt          top level build
src/CMakeLists.txt      addon target, addon conf, dictionary install
src/enhint.{h,cpp}      the fcitx5 addon (config + output filter)
src/cedict.{h,cpp}      minimal CC-CEDICT reader
src/enhint.conf.in      addon metadata template
tools/fetch-cedict.sh   downloader for CC-CEDICT
```

## License

* Addon source: **LGPL-2.1-or-later**.
* CC-CEDICT: **CC BY-SA 4.0** — not bundled with this repository.
