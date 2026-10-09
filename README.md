# fcitx5-enhint

在 fcitx5 候选词的**上方**显示对应中文词的英文释义。

```
wo
I, me        to hold        to lie            <-- 新增的英文提示
1. 我         2. 握          3. 卧             <-- 候选词
```

释义来自本地 [CC-CEDICT](https://cc-cedict.org/) 词典，打字时**无需联网**。

[English](README.en.md) · **中文**

---

## 功能

* 为每个候选词在**上方单独一行**显示英文释义。
* 可在**原版 fcitx5** 上运行 —— 就是普通插件，不需要修改/重编 fcitx5。
* 本地词典（CC-CEDICT）查询，离线、即时、不联网。
* 可配置释义数量、分隔符，以及可选的开关热键。
* 某个候选没有释义时，仍保留空行，保证所有候选垂直对齐。

## 原理

* 插件挂接核心的 `Instance::OutputFilter` 信号 —— 和简繁转换用的是同一个钩子。
* 对每个候选词的文本查询 CC-CEDICT，把英文释义作为**新增的一行**前置。
* fcitx5 的 `classicui` 模块本身支持多行候选文本，会逐行垂直渲染，于是形成上面
  的效果。

因为它只用到通用输出过滤器，所以能在原版 fcitx5（>= 5.1.20）上工作，可作为普通
插件发行。

## 依赖

* fcitx5 >= 5.1.20（多行候选渲染）
* fcitx5-chinese-addons —— 提供 `pinyin` 输入法
* `classicui` 用户界面（默认候选窗）
* CC-CEDICT 词典（见[词典](#词典)）

## 安装

### 方式 A —— 系统安装（推荐）

```sh
cmake -B build -G Ninja
cmake --build build
sudo cmake --install build      # 安装到 /usr/lib/fcitx5 与 /usr/share/fcitx5
fcitx5 -r
```

可在编译时指定词典，一并装到系统：

```sh
cmake -B build -G Ninja -DENHINT_CEDICT=/path/to/cedict_ts.u8
sudo cmake --install build
```

### 方式 B —— 用户级安装（免 root）

```sh
cmake -B build -G Ninja
cmake --build build
mkdir -p ~/.local/lib/fcitx5 ~/.local/share/fcitx5/addon
cp build/src/libenhint.so ~/.local/lib/fcitx5/
cp build/src/enhint.conf  ~/.local/share/fcitx5/addon/
```

`~/.local/lib/fcitx5` **默认不在** fcitx5 的搜索路径里，需要用
`FCITX_ADDON_DIRS` 告诉它。注意该变量会**覆盖**默认路径，因此必须同时带上
`/usr/lib/fcitx5`。

对由 systemd 启动的会话（大多数 Wayland 环境），用 `environment.d` 持久化，并重启
服务：

```sh
cat > ~/.config/environment.d/95-fcitx5-enhint.conf <<'EOF'
FCITX_ADDON_DIRS=$HOME/.local/lib/fcitx5:/usr/lib/fcitx5
EOF
# 让当前会话立即生效：
systemctl --user set-environment \
  FCITX_ADDON_DIRS="$HOME/.local/lib/fcitx5:/usr/lib/fcitx5"
systemctl --user restart 'app-org.fcitx.Fcitx5@autostart.service'
```

如果你的 fcitx5 是由 shell 或合成器配置启动的，就在对应位置设置同一个变量，例如：

```sh
FCITX_ADDON_DIRS="$HOME/.local/lib/fcitx5:/usr/lib/fcitx5" fcitx5 -r
```

## 词典

把 CC-CEDICT 下载到用户数据目录：

```sh
tools/fetch-cedict.sh
# 生成 ~/.local/share/fcitx5/enhint/cedict_ts.u8
```

插件按以下顺序查找（命中即止）：

1. `~/.local/share/fcitx5/enhint/cedict_ts.u8`
2. `~/.local/share/fcitx5/enhint/cedict.u8`
3. `~/.local/share/fcitx5/enhint/cedict.txt`
4. `<prefix>/share/fcitx5/enhint/` 下的同名文件

找不到词典时插件保持静默并打印一条警告。词典在**后台线程**解析，不会拖慢 fcitx5
启动。

## 配置

配置文件：`~/.config/fcitx5/conf/enhint.conf`

```ini
[Enhint]
Enabled=True
MaxGlosses=2
Separator=", "
SkipPrograms=
Hotkey=
```

| 键             | 默认值 | 说明                                               |
|----------------|--------|----------------------------------------------------|
| `Enabled`      | `True` | 是否显示英文提示。                                 |
| `MaxGlosses`   | `2`    | 每个候选显示的释义条数（1–8）。                    |
| `Separator`    | `", "` | 多条释义之间的分隔符。                             |
| `SkipPrograms` | 空     | 不装饰的程序名列表（见下）。                       |
| `Hotkey`       | 空     | 可选开关热键，例如 `Control+Alt+E`。               |

开关动作也可以添加到输入法托盘/菜单。

## 自带候选窗的客户端

有些程序**自己画候选窗**，而不是让 fcitx5 画。它们声明 `ClientSideInputPanel`
能力，会原样收到候选文本；其中少数在选中时会直接插入该文本，导致英文提示被写进
输入框。

如果某个程序出现这种情况，把它的程序名加进 `SkipPrograms`：

```ini
SkipPrograms=某个程序
```

程序名可以从 fcitx5 日志里看到 —— 插件会为每个使用自带候选窗的程序打印一行日志。

## 故障排查

* **完全没有提示** —— 用 `fcitx5 -r --verbose default=4` 查看是否有
  `Loaded addon enhint`。若没有，说明插件库不在搜索路径（见安装）或
  `enhint.conf` 没找到。
* **`Could not locate library libenhint.so`** —— 用户级安装时未设置
  `FCITX_ADDON_DIRS`，或库没装好。
* **`no CC-CEDICT dictionary found`** —— 执行 `tools/fetch-cedict.sh`。
* **提示出现在错误的程序里 / 被输入进去** —— 把该程序加入 `SkipPrograms`。

## 开发

```sh
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

单独测试词典解析：直接编译 `src/cedict.cpp` 即可。

## 目录结构

```
CMakeLists.txt          顶层构建
src/CMakeLists.txt      插件目标、addon 配置、词典安装
src/enhint.{h,cpp}      fcitx5 插件（配置 + 输出过滤器）
src/cedict.{h,cpp}      精简版 CC-CEDICT 读取
src/enhint.conf.in      addon 元数据模板
tools/fetch-cedict.sh   CC-CEDICT 下载脚本
```

## 许可

* 插件源码：**LGPL-2.1-or-later**。
* CC-CEDICT：**CC BY-SA 4.0** —— 本仓库不再分发其内容。
