# UtauPocket Synth Core

简体中文 ｜ [English](README.md)

面向嵌入式 Linux（ARM）与桌面平台的**实时歌声合成核心**。

它把 UTAU 风格的声库（录音样本 + `oto.ini`）离线分析成 **LLSM（Low Level Speech Model，低层语音模型）** 参数帧，运行时由 MIDI 事件驱动，实时合成为连续滑音、弯音、颤音都可控的歌声。

与采样回放的合成器不同，变调不是重采样：LLSM 把语音分解为「声门源 + 声道滤波器」，改变音高时保留声道包络，因此不会出现"花栗鼠"效应。

> 本文档由代码与实测整理。若文档与代码不一致，以代码为准。

---

## 目录

- [1. 特性](#1-特性)
- [2. 架构与工作原理](#2-架构与工作原理)
- [3. 目录结构](#3-目录结构)
- [4. 构建](#4-构建)
- [5. 运行播放器](#5-运行播放器)
- [6. 作为库嵌入](#6-作为库嵌入)
- [7. 制作声库](#7-制作声库)
- [8. 文件格式](#8-文件格式)
- [9. MIDI 控制映射](#9-midi-控制映射)
- [10. 交叉编译](#10-交叉编译)
- [11. 已知限制](#11-已知限制)
- [12. 常见问题](#12-常见问题)
- [13. 许可证](#13-许可证)
- [附录 A：README.md 原文副本](#appendix-a)

---

## 1. 特性

- **参数化变调**：基于 LLSM 的源-滤波器分解，变调保持音色（共振峰）不变。
- **兼容 UTAU 声库制作习惯**：沿用 `oto.ini` 的五个时间点（offset / consonant / cutoff / preutterance / overlap），并保留前置段、辅音段、元音循环段（往返循环）与重叠段交叉淡化。
- **实时演奏**：串口 MIDI（UART）输入，块处理输出，适合低算力设备。
- **单音 + 后进先出**：按新键滑音到新音高，松开自动回到仍按住的音。
- **双调制系统**：FM（音高）与 AM（音量）各有一组 ADSR + 正弦 LFO。
- **可调音色**：Rd 乘子、唇半径、气声（非周期）增益、合成带宽乘子。
- **C ABI**：只有 5 个导出符号，易于被 C/C++/Python（ctypes）/其他语言调用。
- **两种精度**：`float`（默认，体积小、快）或 `double` 编译期切换。
- **静态库 + 动态库**：`.a` 与 `.so`（带 version script，只导出公共 API）。
- **交叉编译友好**：Makefile 原生支持 `CROSS_COMPILE`，已验证 aarch64 / armhf / armel。
- **配套离线工具**：`wav2llsm`（单样本分析）+ `utau2llsm`（图形化批量转换与循环点分析）。

---

## 2. 架构与工作原理

### 2.1 分层视图

```
┌────────────────────────────────────────────────────────────────────┐
│ 应用层   apps/player          Linux: tinyalsa + 串口 MIDI          │
│                               Windows: PortAudio/ASIO + RtMidi     │
│          utils/utau2llsm      Python 预览（ctypes 调 .so）         │
├────────────────────────────────────────────────────────────────────┤
│ 接口层   include/utau_pocket.h —— 5 个 C 函数                       │
│          init / close / synth / push_midi_evt / tune 回调           │
├────────────────────────────────────────────────────────────────────┤
│ 控制层   SynthCtrl（音高·包络·LFO·滑音）                            │
│          VocalStateMachine（音素状态机·交叉淡化）                   │
│          LLSMDatabase（索引·校验·加载）                             │
│          SynthTables / HopTables（查表）                            │
├────────────────────────────────────────────────────────────────────┤
│ 合成层   libllsm2 实时合成（llsm_rtsynth_buffer_*）                 │
│          帧变换：F0 覆写 / Rd 乘子 / 相位平移                       │
├────────────────────────────────────────────────────────────────────┤
│ 基础层   ciglet（FFT·中值滤波） · llsm_file_utils（.LS2 读写）      │
│          arduino_midi_library（MIDI 字节流解析）                    │
└────────────────────────────────────────────────────────────────────┘
```

### 2.2 模块职责与状态

| 模块 | 源文件 | 职责 | 主要状态 |
|---|---|---|---|
| `CoreState` | `src/utau_pocket.cpp` | 实例生命周期、hop 渲染、双 voice 混音、故障处理 | `hop_buffer`、`phase`、`omega`、`volume`、`fault` |
| `SynthCtrl` | `src/synth_ctrl.cpp` | MIDI 音符/CC → 音高与音量；滑音、弯音、主/FM/AM 三组包络与 LFO | `currentF0`、`currentVol`、3×ADSR、2×SineLFO |
| `NoteTracker` | `src/note_tracker.cpp` | 单音后进先出音符栈 | `prev[]`、`next[]`、`topNote` |
| `VocalStateMachine` | `src/vocal_state_machine.cpp` | 双 voice 音素推进与交叉淡化 | `State`、`frame_q16`、`gain/target` |
| `ADSR` | `src/adsr.cpp` | 线性 A/P/D/S/R 包络 | `state`、`val`、各段步长 |
| `SineLFO` | `src/sine_lfo.cpp` | 1024 点查表正弦 LFO | `phase`、`phase_inc` |
| `LLSMDatabase` | `src/llsm_database.cpp` | 解析索引、加载 `.LS2`、参数一致性校验、帧预处理 | `records[32768]`、`opt_s`、`conf` |
| `SynthTables` | `src/lookup_tables.cpp` | 频率表、弯音表、正弦表、CC 乘子表 | 静态数组，进程内只初始化一次 |
| MIDI 解析 | `lib/arduino_midi_library` + `src/midi_input.h` | 字节流 → 回调（OMNI，逐字节解析） | `MemoryTransport` |

### 2.3 模块依赖

```
apps/player ───────────────► utau_pocket.h（C ABI）
utau2llsm（Python）─ctypes──► libutau_pocket.so
                                   │
                 ┌─────────────────┴──────────────────┐
                 ▼                                    ▼
            SynthCtrl                        VocalStateMachine
          ┌────┼─────────┐                    ┌────────┴────────┐
          ▼    ▼         ▼                    ▼                 ▼
     NoteTracker SynthTables           LLSMDatabase      llsmrt（实时合成）
          │         │                        │                 │
          ▼         ▼                        ▼                 ▼
     ADSR / SineLFO  freq/bend/sine     llsm_file_utils     ciglet（FFT/DSP）
                                             │
                                             ▼
                                          .LS2 文件

wav2llsm ──► libpyin（F0）──► libllsm2（分析）──► llsm_file_utils
                └── libgvps（Viterbi 路径搜索）
```

### 2.4 LLSM 两层模型

libllsm2 把语音拆成两层：

| 层 | 内容 | 作用 |
|---|---|---|
| **Layer 0** | 谐波幅度/相位（`LLSM_FRAME_HM`）、噪声 PSD（`LLSM_FRAME_NM`）、残差 PSD | 谐波 + 噪声（HNM）表示，负责"像不像原声" |
| **Layer 1** | 声道包络 `VTMAGN`、声门参数 `Rd`、声源相位 `VSPHSE` | 源-滤波器解释，负责"能不能干净地改" |

关键帧字段：

| 索引 | 名称 | 含义 |
|---|---|---|
| 0 | `LLSM_FRAME_F0` | 基频 |
| 1 | `LLSM_FRAME_HM` | 谐波模型 |
| 2 | `LLSM_FRAME_NM` | 噪声模型 |
| 10 | `LLSM_FRAME_RD` | 声门参数 Rd |
| 11 | `LLSM_FRAME_VTMAGN` | 声道幅度响应（dB） |
| 12 | `LLSM_FRAME_VSPHSE` | 声源谐波相位 |
| 4 / 6 / 7 | `UTAU_FRAME_RD_ORIGINAL` / `F0_ORIGINAL` / `PHASE_SHIFTED` | **本项目私有**：原始值备份与相位记账 |

前三个私有字段是这套实现的关键细节：每次渲染都从"原始 F0 / 原始 Rd"重新计算，而不是在上一帧的结果上再改一次，所以连续变调不会累积漂移；`PHASE_SHIFTED` 记录已经施加过的相位偏移，保证拼接处相位连续、不产生抵消。

正因为变调只改 F0 与 Rd、**不动 `VTMAGN`**，音高变了而共振峰不变——这就是第 1 节所说的"不会花栗鼠化"。

### 2.5 一次 hop 的处理

`hop` 由声库的 `thop` 决定，默认 `hop = round(fs·thop) = 128`（44.1 kHz 下约 2.9 ms），`max_hop = ceil(thop·fs) + 4 = 132`。

1. **控制更新**（`SynthCtrl::update`）
   - 三组 ADSR 各自推进：主包络控音量，FM 包络控音高调制，AM 包络控音量调制。
   - 滑音：`currentPitch` 从 `startPitch` 线性逼近 `targetPitch`，步数来自 CC5。
   - 基频：`freqTable[⌊p⌋] × bendTable[frac] × bendTable[bendIndex]`——小数音高与弯音共用一张表，因此支持微分音滑音。
   - FM：`f0 = baseFreq × (1 + sin(2πφ)·fmEnv·fmDepth)`，`fmDepth ≤ 1/6`。
   - AM：`vol = mainEnv × (1 − amEnv·amDepth·(0.5 − 0.5·sin(2πφ)))`。
2. **音素推进**（`VocalStateMachine::update`）
   - 两个 voice 各自按 Q16 定点步进；速度来自 CC116（辅音）/ CC117（元音）的乘子表。
   - 若处于交叉淡化，按已推进帧数占 `ovl` 的比例更新两者增益。
3. **帧变换**（`tune`）
   - `ratio = f0_original / f0`（原始 F0 无效时取 1），供混音时补偿谐波幅度。
   - `rd = rd_original × rdMultiplier`。
   - 相位平移 `phase − PHASE_SHIFTED`，随后写回 `PHASE_SHIFTED = phase`。
4. **合成与混音**
   - 每帧送入 `llsm_rtsynth_buffer_feed`，再逐样本 `llsm_rtsynth_buffer_fetch_decomposed` 取（周期 `ap`、非周期 `an`）。
   - 混音：`out = (ap·ratio_A + an·apGain)·gainA + (bp·ratio_B + bn·apGain)·gainB`，再乘主音量。
   - 增益、交叉淡化、气声增益都用 `inv_length[length]` 做逐样本线性斜坡，避免台阶噪声。
5. **相位累积**：`phase = (ω·f0 + phase) mod 2π`，`ω = 2π·thop`，保证 hop 之间相位连续。
6. **输出**：结果留在 `hop_buffer`，`utau_pocket_synth` 按调用者的 `blk_size` 切片拷出；一次调用可能跨多个 hop。

### 2.6 音素状态机与时序

每个音素在 `oto.ini` 里被切成四段，运行时对应五个状态：

```
IDLE ──Program Change──► CONSONANT ──► VOWEL_START ──► VOWEL_LOOP_FWD
                            │              │                │  ▲
                            │              │                ▼  │
                            └──────────────┴──────────► VOWEL_LOOP_BWD
```

| 状态 | 帧区间 | 说明 |
|---|---|---|
| `CONSONANT` | `0 … con` | 辅音段，一次性通过 |
| `VOWEL_START` | `con … vowel_lfrm` | 进入元音前的过渡 |
| `VOWEL_LOOP_FWD/BWD` | `vowel_lfrm … vowel_rfrm` | 在循环区间内往返（ping-pong） |

- 帧位置是 **Q16 定点**（`frame_q16`），折返时用 `% (2·width)` 处理任意越界，包括"步长大于循环长度"的退化情况。
- 推进速度：`cc_multiplier_lut_q16[v]`，即 `0.2 × 5^(v/64)`——CC 值 0 → 0.2×，64 → 1.0×，127 → 约 4.88×。
- 换音素时 `current` 与 `fading` 互换，淡化长度为 `ovl` 帧；`flag` 的 bit0 决定叠加方向（对应 UTAU 负 overlap 的语义）。淡化结束且旧 voice 增益归零后才置为 `IDLE`，保证最后一段斜坡不会被截断。

### 2.7 生命周期、线程与实时预算

**初始化**（`utau_pocket_init`）：解析索引 → 逐个加载 `.LS2` 并校验 → 读取 `fs` 与 `thop` → 预分配 `hop_buffer`（`max_hop`）与两个 `llsm_rtsynth_buffer`（`2×max_hop`）→ 注册 MIDI 回调。成功返回采样率，失败返回负错误码且不留下半初始化实例。

**渲染**（`utau_pocket_synth`）：渲染路径只做计算，不做文件 I/O，也不做大块分配；一次调用处理的 hop 数由 `blk_size` 决定，工作量有界。

**线程模型**：全局单实例，无锁，所有调用必须串行；`utau_pocket_config.h` 检测到 pthread / OpenMP / FFTW 线程会直接 `#error`。

**故障处理**：任何渲染错误都会把当前整块清零并置 `fault`，之后所有调用返回 `LLSM_ERR_INVALID_STATE`，必须 `close` 后重新 `init`——宁可静音，不输出半截脏数据。

**延迟**：约 `一个 hop（≈2.9 ms）+ blk_size + period_count × period_size`。播放器默认 `128 + 2×128`，即约 8.7 ms 的缓冲。

**内存**：索引表 `32768 × 32 B ≈ 1 MB` 常驻；所有 `.LS2` 的帧数据在 `init` 时**全部读入内存**（实测 2 秒 44.1 kHz 样本约 1 MB），因此声库条目越多、单样本越长，常驻内存越高。

---

## 3. 目录结构

| 路径 | 说明 |
|---|---|
| `include/` | 公共头文件（`utau_pocket.h` 是唯一需要暴露给调用方的头） |
| `src/` | 核心实现：控制、状态机、包络、LFO、查表、声库加载 |
| `lib/libllsm2/` | LLSM 分析与合成（GPL-3.0-or-later，本地有修改） |
| `lib/libpyin/` | 概率化 YIN，F0 提取（分析期使用） |
| `lib/libgvps/` | 通用 Viterbi 路径搜索（被 pyin 使用） |
| `lib/ciglet/` | DSP 基础库：FFT（Ooura）、中值滤波、频谱工具 |
| `lib/llsm_file_utils/` | `.LS2` cache 文件读写 |
| `lib/arduino_midi_library/` | MIDI 1.0 字节流解析（仅用接收部分） |
| `utils/wav2llsm/` | 单样本分析工具（wav → `.LS2`） |
| `utils/utau2llsm/` | 声库转换 / 编辑 GUI（Python + Tkinter） |
| `apps/player/` | 播放器：Linux 用 tinyalsa + 串口，Windows 用 PortAudio + RtMidi |
| `extras/new_geping.csv` | 别名表：音素别名 → bank/prog 字符 |
| `Makefile` | Linux/GNU make 构建 |
| `UtauPocket.sln` | Windows / Visual Studio 构建 |

---

## 4. 构建

### 4.1 依赖

| 平台 | 需要 |
|---|---|
| Linux | 支持 C11 与 C++14 的编译器（GCC/Clang）、GNU make；运行时只需 `libm`（播放器另需 `libdl`） |
| Windows | Visual Studio（打开 `UtauPocket.sln`）；PortAudio / RtMidi 已内置在 `apps/player/vendor/` |

tinyalsa 的源码与头文件随仓库提供，不需要额外安装 ALSA 开发包（内核 uapi 头文件由 libc 开发包带入）。

### 4.2 常用目标

```bash
make -j"$(nproc)"            # all：静态库 + 动态库 + wav2llsm + 播放器
make static                  # build/float/lib/libutau_pocket.a
make shared                  # build/float/lib/libutau_pocket.so
make utils                   # build/float/bin/wav2llsm
make player                  # build/float/bin/utau_pocket（仅 Linux）
make clean                   # 删除当前 BUILD_DIR
```

产物默认位于 `build/float/`（`PRECISION=double` 时为 `build/double/`）。

`make clean` 只删除当前的 `BUILD_DIR`；多目标构建时请带上相同的 `BUILD_DIR=...` 再 clean。

### 4.3 构建变量

| 变量 | 默认 | 说明 |
|---|---|---|
| `PRECISION` | `float` | `float` 或 `double`，决定 `FP_TYPE` 与默认输出目录 |
| `CROSS_COMPILE` | 空 | 交叉编译前缀，如 `aarch64-linux-gnu-` |
| `BUILD_DIR` | `build/$(PRECISION)` | 输出目录，多目标构建时建议显式指定 |
| `OPT` | `-O3` | 优化与额外编译选项 |
| `CC` / `CXX` / `AR` | 由 `CROSS_COMPILE` 推导 | 显式设置时优先 |
| `LDFLAGS` | 空 | 链接选项（静态链接可传 `-static`） |

> 注意：`PRECISION` 会改变数据精度，**库与应用必须用同一精度编译**。`include/utau_pocket_config.h` 也要求全工程统一。

### 4.4 Windows

用 Visual Studio 打开 `UtauPocket.sln`，包含四个工程：

- `utau_pocket`：核心 DLL
- `wav2llsm`：分析工具
- `utau_pocket_player`：播放器（PortAudio + RtMidi）
- `portaudio_static`：PortAudio 静态库

---

## 5. 运行播放器

```bash
# Linux：从串口读 MIDI，从 tinyalsa 输出
utau_pocket oto.db -s /dev/ttyS3 -b 31250 -c 2 -p 128

# Windows：列出 MIDI 输入 → 选择 → 播放
utau_pocket_player.exe oto.db -l
utau_pocket_player.exe oto.db -m 0
```

### 5.1 参数

| 参数 | 平台 | 默认 | 说明 |
|---|---|---|---|
| `-p, --period-size N` | 全平台 | 128 | 每块样本数 |
| `-n, --period-count N` | 全平台 | 2 | PCM 周期数（Windows 上作为延迟提示） |
| `-c, --channels N` | 全平台 | 2 | 输出声道数，单声道复制到各声道 |
| `-r, --rate N` | 全平台 | 声库采样率 | **必须与声库一致**，程序不做重采样 |
| `-D, --card N` | Linux | 0 | tinyalsa 声卡号 |
| `-d, --device N` | Linux | 0 | tinyalsa PCM 设备号 |
| `-s, --serial PATH` | Linux | `/dev/ttyS3` | 串口 MIDI 设备 |
| `-b, --baud N` | Linux | 115200 | 串口波特率，支持 31250（MIDI DIN 标准速率） |
| `-l, --list-midi` | Windows | — | 列出 MIDI 输入，不需要声库 |
| `-m, --midi-in N` | Windows | 自动 | 选择 MIDI 输入序号 |

Linux 下访问串口通常需要把用户加入 `dialout` 组：

```bash
sudo usermod -aG dialout "$USER"   # 重新登录后生效
```

播放器收到 MIDI **Tune Request（F6）** 时会向串口回写一个 `'A'` 字节，可用于与上位机做握手/同步。

---

## 6. 作为库嵌入

### 6.1 最小示例

```c
#include "utau_pocket.h"
#include <stdio.h>

int main(void)
{
    /* 第二个参数是每次 synth 要产出的样本数 */
    int fs = utau_pocket_init("oto.db", 128);
    if (fs <= 0) {
        fprintf(stderr, "加载声库失败：LLSM 错误 %d\n", fs);
        return 1;
    }

    const uint8_t bank[] = { 0xB0, 32, 'O' };  /* CC32：选音色库 */
    const uint8_t prog[] = { 0xC0, 'a' };      /* Program Change：选音素 */
    const uint8_t on[]   = { 0x90, 60, 100 };  /* Note On：中央 C */
    utau_pocket_push_midi_evt(bank, sizeof bank);
    utau_pocket_push_midi_evt(prog, sizeof prog);
    utau_pocket_push_midi_evt(on,   sizeof on);

    float buf[128];
    for (int i = 0; i < 200; ++i) {
        if (utau_pocket_synth(buf) != 128) break;
        /* buf 中是 128 个归一化浮点样本（未裁剪），交给音频设备 */
    }

    utau_pocket_close();
    return 0;
}
```

链接（核心是 C++ 实现，用 C 编译器链接静态库时需要补 `-lstdc++`）：

```bash
g++ app.c -Iinclude -Lbuild/float/lib -lutau_pocket -lm -o app
# 或动态库：
gcc app.c -Iinclude -Lbuild/float/lib -lutau_pocket -lm -o app
```

> 示例按 `PRECISION=float` 编写。若用 `double` 构建，`buf` 要改成 `double` 数组，且应用与库必须使用同一精度。

### 6.2 API

| 函数 | 说明 |
|---|---|
| `int utau_pocket_init(const TCHAR *db_path, int blk_size)` | 加载声库，成功返回采样率，失败返回负错误码；不留下半初始化实例 |
| `void utau_pocket_close(void)` | 释放实例，幂等 |
| `int utau_pocket_synth(FP_TYPE *buf)` | 写满 `blk_size` 个样本，返回 `blk_size`；失败时整块清零并进入故障态，需重新 init |
| `int utau_pocket_push_midi_evt(const uint8_t *evt, size_t len)` | 同步消费 MIDI 字节流，跨包的不完整消息会被保留 |
| `void utau_pocket_set_tune_request_callback(cb, user_data)` | 注册 Tune Request（F6）通知；回调内**不得**再调核心 API |

调用约束：

- **全局单实例**；所有调用必须串行，不可递归。
- `push_midi_evt` 不排队、不带时间戳；事件影响下一个尚未生成的 hop，已经缓冲的音频不会被改写。
- 输出是未裁剪的归一化浮点，削波由调用方负责。
- 核心强制单线程：`utau_pocket_config.h` 会在检测到 pthread / OpenMP / FFTW 线程时直接 `#error`。

---

## 7. 制作声库

### 7.1 `wav2llsm`：单样本分析

```bash
wav2llsm input.WAV -o out.LS2 -p 128 -n 2048 -H 400 -f 50 -F 800
```

| 参数 | 默认 | 说明 |
|---|---|---|
| `-b, --bias` | 2.0 | 清浊判定偏置（pyin） |
| `-f, --fmin` / `-F, --fmax` | 50 / 800 | F0 搜索范围（Hz） |
| `-h, --hop` | 128 | 帧移（样本数），决定 `thop` |
| `-n, --fft` | 2048 | 分析 FFT 大小 |
| `-p, --psd` | 128 | 噪声 PSD 维数 |
| `-H, --har` / `-e, --har_e` | 400 / 5 | 最大谐波数 / 噪声包络谐波数 |
| `-s, --spec` / `-B, --bap` | 64 / 5 | 频谱包络阶数 / 频带非周期性阶数 |
| `-t, --trange` | 24 | 允许的最大跳变距离 |
| `-o, --output` | 同名 `.LS2` | 输出路径 |

成功时 **stdout 每行输出一帧的 `f0,rd`**，供编辑器做循环点分析使用（不是调试输出，管道模式下请保留）。

### 7.2 `utau2llsm`：批量转换与编辑

```bash
pip install -r utils/utau2llsm/requirements.txt
python utils/utau2llsm/main.py
```

- 依赖：`numpy`、`Pillow`、Tkinter（随 Python 提供）；`sounddevice` 可选，仅实时试听需要。
- 需要能找到 `wav2llsm`：放在脚本旁、`build/float/bin/`、或 `dist/linux-x86_64/float/`，也可用环境变量 `UTAU_WAV2LLSM` 指定。
- 输入：UTAU 声库目录（必须含 `oto.ini`）+ 别名 CSV（如 `extras/new_geping.csv`）。
- 流程：解析 `oto.ini`（兼容 UTF-8 / Shift-JIS / GBK / CP437）→ 按 offset/overlap/cutoff 裁剪 → 多进程调用 `wav2llsm` → 生成 `<bank><prog>.LS2` 与 `oto.db` 索引。
- 循环点：用 ContentVec（768 维自监督特征）+ F0 + Rd 构造代价函数自动搜索，也可在时间线上手工拖拽。ContentVec 模型需自备并在界面里指定路径；没有模型时仍可手工标循环点。
- 编辑结果原子写入，改前自动备份为 `oto.db.bak`。
- 内置预览：用 `ctypes` 加载 `libutau_pocket`，边改边听。

---

## 8. 文件格式

### 8.1 `oto.db`（声库索引）

连续存放的 16 字节小端记录（`<8H`），每条对应一个音素：

| 字段 | 说明 |
|---|---|
| `midi_symbol` | 由 `bank` 与 `prog` 拼成：`prog` 在高字节、`bank` 在低字节，两者都是 ASCII 字符 |
| `ovl` | 重叠帧数（交叉淡化长度） |
| `pre` | 前置帧数 |
| `con` | 辅音段结束帧 |
| `vowel_lfrm` / `vowel_rfrm` | 元音循环段左右端点 |
| `nfrm` | 总帧数 |
| `flag` | bit0：重叠段方向取反 |

### 8.2 `.LS2`（LLSM cache 文件）

每个音素一个，文件名是 `<bank><prog>.LS2`，与 `oto.db` 同目录。内容为 libllsm2 的模型参数与逐帧数据（F0、谐波、噪声 PSD、Rd、声道包络等）。

加载时会校验：

- 各字段范围合法，`con ≤ vowel_lfrm < vowel_rfrm < nfrm`；
- 帧参数都是有限值；
- **同一 `oto.db` 内所有 `.LS2` 必须共享同一套分析参数**（`fs`、`thop`、`fnyq`、`lipradius`、`npsd`、`nchannel`、`nspec`、`maxnhar` 等）。

### 8.3 别名表 CSV

`extras/new_geping.csv`：

```csv
alias,midi_bank_char,midi_prog_char
a,O,a
ai,O,l
ba,B,a
```

左边是 `oto.ini` 里的别名，右边两个字符决定输出文件名与运行时的 `CC32 + Program Change` 组合。

---

## 9. MIDI 控制映射

所有通道都响应（OMNI）。Note On velocity=0 视作 Note Off。

| CC | 作用 | 说明 |
|---|---|---|
| 7 | 主音量 | `v/127` |
| 5 | 滑音时间 | 长度 = `v+1` ticks |
| 73 / 76 / 75 / 79 / 72 | 主 ADSR：A / Peak / D / S / R | A、D、R 为 `v+1`；Peak 是攻击顶点电平 |
| 102–106 | FM ADSR：A / P / D / S / R | 音高调制包络 |
| 107 | FM LFO 速率 | |
| 108 | FM 深度 | 上限 1/6（约 ±16.7%） |
| 109–113 | AM ADSR：A / P / D / S / R | 音量调制包络 |
| 114 | AM LFO 速率 | |
| 115 | AM 深度 | 上限 1.0 |
| 116 | 辅音段推进速度 | `0.2 × 5^(v/64)`：0→0.2×，64→1.0×，127→约 4.88× |
| 117 | 元音段推进速度 | 同上 |
| 32 | 音色库选择 | **必须先发 CC32，再发 Program Change**，否则换音无效 |
| 118 | 奈奎斯特频率乘子 | 改变合成带宽 |
| 119 | 气声（非周期）增益 | |
| 20 | Rd 乘子 | 影响音色明暗/紧松 |
| 21 | 唇半径 | 声道模型参数 |
| 120 | All Sound Off | 立即静音 |
| 123 | All Notes Off | 逐个释放 |

另外：

- **Pitch Bend**：±2 半音，14 bit，中心 8192。
- **Tune Request（F6）**：触发已注册的回调（播放器会向串口回 `'A'`）。

---

## 10. 交叉编译

Makefile 直接支持 `CROSS_COMPILE`：

```bash
# aarch64 Linux
make -j"$(nproc)" static shared utils \
     CROSS_COMPILE=aarch64-linux-gnu- BUILD_DIR=build/aarch64

# armhf（hard-float，32 位）
make -j"$(nproc)" static shared utils \
     CROSS_COMPILE=arm-linux-gnueabihf- BUILD_DIR=build/armhf

# armel（soft-float，32 位）
make -j"$(nproc)" static shared utils \
     CROSS_COMPILE=arm-linux-gnueabi- BUILD_DIR=build/armel
```

已验证的目标（GCC 13.3，Ubuntu 24.04 交叉包）：

| 目标 | 说明 |
|---|---|
| `aarch64-linux-gnu` | 64 位 ARM，默认产出 PIE |
| `arm-linux-gnueabihf` | 32 位硬浮点，默认 `-march=armv7-a+fp` |
| `arm-linux-gnueabi` | 32 位软浮点，默认 `-march=armv5te`，非 PIE |
| `armv7-a + NEON` | `OPT="-O2 -march=armv7-a -mfpu=neon-vfpv4 -mfloat-abi=hard"` |
| `armv6`（树莓派 1 / Zero） | `OPT="-O2 -march=armv6zk -marm -mfpu=vfp -mfloat-abi=hard"` |
| `PRECISION=double` | 逐采样精度切换，输出体积约为 float 的两倍 |
| 静态链接 | `LDFLAGS="-static"`，产出无 `NEEDED` 的自包含二进制 |

在 x86_64 上用 qemu-user 跑 ARM 产物时，需要指定目标 sysroot：

```bash
qemu-aarch64-static -L /usr/aarch64-linux-gnu ./build/aarch64/bin/wav2llsm in.wav -o out.LS2
qemu-arm-static     -L /usr/arm-linux-gnueabihf ./build/armhf/bin/wav2llsm in.wav -o out.LS2
```

两个容易踩的点：

1. **armv6 + 硬浮点必须加 `-marm`**。ARMv6 默认走 Thumb-1，而 Thumb-1 不支持硬浮点 ABI，否则报
   `sorry, unimplemented: Thumb-1 'hard-float' VFP ABI`。
2. **交叉 sysroot 会抬高 ELF 的 CPU 属性**。用 Ubuntu 的 armhf 工具链编 armv6 时，项目目标文件是 `Tag_CPU_arch: v6KZ`，但链接后因为混入 armv7 的 libc，最终二进制会标成 `v7`。要真正跑在树莓派 1/Zero 上，必须换用 armv6 的 sysroot（如 Raspbian 的）。

---

## 11. 已知限制

- **单实例、单线程**：核心是全局单例，禁止并发调用。
- **单音**：内部两个 voice 只用于换音素时的交叉淡化，不构成复音；`NoteTracker` 是后进先出的单音栈。
- **不做重采样**：输出采样率等于声库采样率，播放器会拒绝不匹配的 `-r`。
- **声库必须同构**：一个 `oto.db` 内的所有 `.LS2` 必须共享同一套分析参数。
- **不支持实时分析**：libllsm2 只支持离线分析，`wav2llsm` 必须事先跑完。
- **平台实现分裂**：Linux 用 tinyalsa + 串口，Windows 用 PortAudio/ASIO + RtMidi，两套代码路径。
- **动态链接依赖目标机运行库**：用较新发行版编出的二进制可能要求较新的 glibc（例如 Ubuntu 24.04 需要 `GLIBC_2.34`）；对老目标系统请换旧 sysroot 或静态链接。
- **项目处于早期阶段**：仓库目前只有一次提交，原 `README.md` 尚标注 TBC，接口与文件格式仍可能调整。

---

## 12. 常见问题

**Q：`Cannot load voicebank: LLSM error N`？**
依次检查：`oto.db` 路径是否正确、同目录下 `<bank><prog>.LS2` 是否齐全、所有声库参数是否一致、`nfrm` 是否与文件实际帧数相符。

**Q：qemu 报 `Could not open '/lib/ld-linux-aarch64.so.1'`？**
加 `-L <sysroot>`，例如 `-L /usr/aarch64-linux-gnu`。

**Q：目标机上报 `GLIBC_2.34 not found`？**
构建环境的 glibc 比目标机新。用目标系统对应的旧 sysroot 重新编译，或改用静态链接 / musl 工具链。

**Q：播放器没有声音？**
确认 `/dev/snd` 存在且有权限，`-D`/`-d` 指定的声卡与设备正确，采样率与声库一致。

**Q：串口打不开？**
检查设备路径（默认 `/dev/ttyS3`）、波特率，以及当前用户是否在 `dialout` 组。

**Q：换音素没反应？**
Program Change 之前必须先用 CC32 选 bank，且 bank/prog 要在 `oto.db` 里存在。

---

## 13. 许可证

本项目以 **GNU General Public License v3.0** 发布（见 `LICENSE`）。

第三方组件及其许可证清单、本地修改状态见 `README.md` 的 "Third-party components and licensing inventory" 一节；为便于中文读者查阅，本文末尾[附录 A](#appendix-a)逐字收录了同一份清单。其中 **libllsm2 为 GPL-3.0-or-later**，因此与其链接的作品也需要遵循 GPL 兼容的许可。

`apps/player/vendor/` 下的 PortAudio、RtMidi、tinyalsa、ASIO SDK 等各自保留原许可证，分发时请一并保留其声明文件。

---

<a id="appendix-a"></a>

## 附录 A：README.md 原文副本

> 以下内容逐字复制自仓库根目录的 [`README.md`](README.md)（英文原文，未翻译），用于在中文文档内保留同一份第三方组件与许可证清单。
> 原文的项目简介目前仍是 `TBC`；其一级标题与语言切换链接未收录，章节标题层级做了降级处理，其余内容逐字保留。

TBC

### Third-party components and licensing inventory

| Component / local path | Upstream | Included licensing information | Status / remaining work |
| --- | --- | --- | --- |
| `lib/libllsm2` | [Sleepwalking/libllsm2](https://github.com/Sleepwalking/libllsm2) | `LICENSE`; source headers specify GPL-3.0-or-later | Header version 2.1.0; locally modified. License matches upstream master as checked on 15 Oct 2026. Record the exact imported commit and local changes. |
| `lib/arduino_midi_library` | [FortySevenEffects/arduino_midi_library](https://github.com/FortySevenEffects/arduino_midi_library) | `LICENSE` and MIT source headers | Header version 5.0.0; locally modified. License matches upstream master as checked on 15 Oct 2026. Record the imported commit and changes. |
| `lib/ciglet` | [Sleepwalking/ciglet](https://github.com/Sleepwalking/ciglet) | BSD-3-Clause text in `ciglet.c` and `ciglet.h` | Record the imported revision and changes; add a standalone copy of its original license. External files below have separate notices. |
| `lib/libpyin` | [Sleepwalking/libpyin](https://github.com/Sleepwalking/libpyin) | BSD-3-Clause source headers | Record the imported revision and changes; add a standalone copy of its original license. |
| `lib/libgvps` | [Sleepwalking/libgvps](https://github.com/Sleepwalking/libgvps) | BSD-3-Clause `LICENSE` | License matches upstream master as checked on 15 Oct 2026. Record the imported revision and changes. |
| `apps/player/vendor/rtmidi` | [thestk/rtmidi](https://github.com/thestk/rtmidi) | MIT-style `LICENSE`, including a non-binding request | Included version 6.0.0. License matches upstream master as checked on 15 Oct 2026; record the exact source revision. |
| `apps/player/vendor/portaudio` | [PortAudio/portaudio](https://github.com/PortAudio/portaudio) | MIT-style `LICENSE.txt` and per-file notices | Included source subset is documented as 19.7.0. License matches upstream master as checked on 15 Oct 2026; record the exact source revision and any changes. |
| `apps/player/vendor/tinyalsa` | [tinyalsa/tinyalsa](https://github.com/tinyalsa/tinyalsa) | `LICENSE` points to BSD-style `NOTICE`; source files carry additional copyright notices | `LICENSE` and `NOTICE` match upstream master as checked on 15 Oct 2026. Keep both files; record the imported revision and subset. |
| `apps/player/vendor/asiosdk` | [Steinberg ASIO SDK](https://www.steinberg.net/developers/asiosdk-open/) | Dual Steinberg proprietary / GPLv3 licenses in `LICENSE.txt` and `common/LICENSE.txt`; separate licensing and trademark documents | Both license files match the official 2.3.4 distribution dated 15 Oct 2025. The intended integration path is GPLv3. Preserve embedded notices and review separate asset/trademark terms. This is not a claim that 2.3.4 is the latest SDK release. |
| `lib/ciglet/external/fastapprox-all.h` | Paul Mineiro, [fastapprox](https://github.com/etheory/fastapprox) | Embedded BSD-style notices | Preserve all embedded notices and confirm the exact source revision. |
| `lib/ciglet/external/fftsg_h.c` | [Takuya Ooura FFT](https://www.kurims.kyoto-u.ac.jp/~ooura/fft.html) | Embedded permission to use, copy, modify and distribute, with a request to reference the package when modified | Preserve the original notice; identify the imported release and changes. |
| `lib/ciglet/external/fast_median.c` | Nicolas Devillard / algorithms credited in the file | Embedded permissive statement and routine-specific provenance comments | Retain the complete comments; verify provenance before assigning a standardized license identifier. |
| `lib/ciglet/external/wavfile.c` | Masanori Morise / [WORLD](https://github.com/mmorise/World), with Kanru Hua modifications | Copyright and modification history; no complete standalone grant in this file | Identify the original WORLD revision and include the applicable license. Do not infer authorization solely from the copyright header. |
| `include/optparse.h` | [skeeto/optparse](https://github.com/skeeto/optparse) | Embedded public-domain dedication | Preserve the notice and confirm the imported revision. |
