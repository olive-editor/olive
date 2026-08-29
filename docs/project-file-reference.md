# Oak Video Editor Project File Reference / Oak 视频编辑器项目文件格式参考

This document describes Oak Video Editor's XML project format (`.ove`) as implemented in the current codebase. It is intended to be detailed enough to implement a compatible reader/writer.

本文件描述 Oak 视频编辑器当前代码库实现的 XML 项目格式（`.ove`），详细程度足以支持兼容的读写实现。

> Source of truth / 权威源码：`app/node/project.cpp`, `app/node/node.cpp`, `app/node/value.*`, `app/node/keyframe.*`, `app/node/project/serializer/*`, `app/node/project/footage/footage.cpp`, `app/node/output/viewer/viewer.cpp`, `app/node/output/track/track.cpp`, `app/node/group/group.cpp`, `app/window/mainwindow/mainwindowlayoutinfo.cpp`.

---

## 1. Root Document / 根文档

```xml
<olive version="230220" url="/path/to/project.ove">
  ...
</olive>
```

- `version`: serializer version in `YYMMDD` format. The latest serializer is `230220` (`ProjectSerializer230220`).
  - `version`：序列化器版本，格式为 `YYMMDD`。当前最新为 `230220`（`ProjectSerializer230220`）。
- `url`: optional source path.
  - `url`：可选的源文件路径。

---

## 2. Project Container / 项目容器

For full saves, the serializer writes a project container:

完整保存时，序列化器会写入一个项目容器：

```xml
<project>
  <project>...</project>   <!-- actual project data / 实际项目数据 -->
  <layout>...</layout>     <!-- UI layout / 用户界面布局 -->
</project>
```

See `ProjectSerializer230220::Save()` and `Load()` in `app/node/project/serializer/serializer230220.cpp`.

---

## 3. Project Data (`Project::Save` / `Project::Load`) / 项目数据

```xml
<project version="1">
  <uuid>...</uuid>
  <plugins>...</plugins>
  <nodes>...</nodes>
  <settings>...</settings>
</project>
```

### 3.1 `uuid`
- QUuid string identifying the project.
- 项目 UUID 字符串。

### 3.2 `plugins` / 插件列表

List of OpenFX plugins referenced by nodes. This block is loaded **before** `<nodes>` so that plugin nodes can be registered.

列出节点引用的 OpenFX 插件。该块在 `<nodes>` 之前加载，以便注册插件节点。

```xml
<plugins>
  <plugin id="com.vendor.Plugin" major="1" minor="2"
          bundle="/path/to/Plugin.ofx.bundle"
          file="/path/to/Plugin.ofx.bundle/Contents/MacOS/Plugin" />
</plugins>
```

Attributes / 属性：
- `id`: OFX plugin identifier (matches node `id`).
  - OFX 插件标识符（与节点 `id` 一致）。
- `major` / `minor`: OFX plugin version.
  - OFX 插件版本。
- `bundle`: bundle directory path (preferred).
  - 包目录路径（优先使用）。
- `file`: plugin binary path (fallback).
  - 插件二进制路径（回退）。

Loading behavior / 加载行为：
- If `<plugins>` exists, each `bundle` (or `file` if `bundle` is empty) is added to the OFX plugin path, the cache scans the paths, and plugin nodes are registered before `<nodes>` is parsed.
- 如果存在 `<plugins>`，则把每个 `bundle`（若 `bundle` 为空则使用 `file`）加入 OFX 插件路径，扫描缓存，并在解析 `<nodes>` 前注册插件节点。

### 3.3 `nodes` / 节点图

The node graph. Each `<node>` is written by `Node::Save()` and read by `Node::Load()`.

节点图。每个 `<node>` 由 `Node::Save()` 写入、`Node::Load()` 读取。

```xml
<nodes version="1">
  <node version="1" id="node.id" ptr="123456">
    <label>Optional Label</label>
    <color>3</color>
    <input>...</input>
    <links>...</links>
    <connections>...</connections>
    <hints>...</hints>
    <context>...</context>
    <caches>...</caches>
    <custom>...</custom>
  </node>
</nodes>
```

Attributes / 属性：
- `id`: node type identifier. For OpenFX nodes, this equals the OFX plugin identifier.
  - 节点类型标识符。对于 OpenFX 节点，等于 OFX 插件标识符。
- `ptr`: numeric pointer ID used to resolve connections and context positions.
  - 数字指针 ID，用于解析连接和上下文位置。
- `version`: currently `1`.
  - 当前为 `1`。

### 3.4 `settings` / 项目设置

Project settings stored as key/value text elements.

以键值文本元素保存的项目设置。

Known keys / 已知键：
- `cachesetting`
- `customcachepath`
- `colorconfigfilename`
- `defaultinputcolorspace`
- `colorreferencespace`
- `root` — pointer id of the root `Folder` node, resolved after all nodes are loaded.
  - `root`：根 `Folder` 节点的指针 ID，在所有节点加载完成后解析。

---

## 4. Node Serialization (`Node::Save` / `Node::Load`) / 节点序列化

### 4.1 `label`
User-visible node label.
用户可见的节点标签。

### 4.2 `color`
Override color index (integer), only written if not `-1`.
覆盖颜色索引（整数），只有不为 `-1` 时才写出。

### 4.3 `input` / 输入

Each input is serialized as:

每个输入序列化为：

```xml
<input id="InputId">
  <primary>...</primary>
  <subelements count="N">
    <element>...</element>
  </subelements>
</input>
```

- `primary`: element `-1` (the main input).
  - `primary`：元素 `-1`（主输入）。
- `subelements`: array elements (if the input is an array). `count` is the array size.
  - `subelements`：数组元素（如果输入是数组）。`count` 为数组大小。

#### 4.3.1 Immediate Values (`primary` / `element`) / 立即值

Each immediate block contains:

每个立即值块包含：

```xml
<keyframing>0|1</keyframing>
<standard>
  <track>...</track>
  ...
</standard>
<keyframes>
  <track>
    <key ...>...</key>
  </track>
</keyframes>
<csinput>...</csinput>
<csdisplay>...</csdisplay>
<csview>...</csview>
<cslook>...</cslook>
```

- `keyframing`: whether the input is keyframed (only written if the input is keyframable).
  - `keyframing`：输入是否启用关键帧（仅当输入可关键帧化时写出）。
- `standard`: default/static values (one `<track>` per keyframe track).
  - `standard`：默认值/静态值（每个关键帧轨道一个 `<track>`）。
- `keyframes`: only written if `keyframing` is true.
  - `keyframes`：仅在 `keyframing` 为 true 时写出。
- `cs*`: only written for `kColor` inputs; stored as input properties `col_input`, `col_display`, `col_view`, `col_look`.
  - `cs*`：仅对 `kColor` 输入写出；保存为输入属性 `col_input`、`col_display`、`col_view`、`col_look`。

#### 4.3.2 Track Count / 轨道数量

Track count is determined by `NodeValue::get_number_of_keyframe_tracks()`:

轨道数量由 `NodeValue::get_number_of_keyframe_tracks()` 决定：

| Type / 类型 | Tracks / 轨道数 |
| --- | --- |
| `kVec2` | 2 |
| `kVec3` | 3 |
| `kVec4` | 4 |
| `kColor` | 4 |
| `kBezier` | 6 |
| other / 其他 | 1 |

#### 4.3.3 Standard Value Encoding / 标准值编码

Values are written with `NodeValue::ValueToString()` and read with `NodeValue::StringToValue()`.

值通过 `NodeValue::ValueToString()` 写入，通过 `NodeValue::StringToValue()` 读取。

String encodings / 字符串编码：
- `kVec2`: `x:y`
- `kVec3`: `x:y:z`
- `kVec4`: `x:y:z:w`
- `kColor`: `r:g:b:a`
- `kBezier`: `x:y:cp1x:cp1y:cp2x:cp2y`
- `kRational`: `num/den` (see `rational::toString()`)
- `kInt`: integer as text / 整数字符串
- `kBinary`: Base64
- `kText`, `kFont`, `kFile`, `kCombo`, `kStrCombo`: string
- `kTexture`, `kSamples`, `kNone`: no text / 无文本

Special cases / 特例：
- `kVideoParams` / `kAudioParams` are nested objects (see sections 5 and 6).
  - `kVideoParams` / `kAudioParams` 为嵌套对象（见第 5、6 节）。
- `kSubtitleParams` is **skipped on load** to avoid overwriting subtitle data.
  - `kSubtitleParams` 在加载时**被跳过**，以避免覆盖字幕数据。

#### 4.3.4 Keyframes (`NodeKeyframe::save` / `NodeKeyframe::load`) / 关键帧

```xml
<key input="InputId" time="num/den" type="0"
     inhandlex="0" inhandley="0"
     outhandlex="0" outhandley="0">value</key>
```

Attributes / 属性：
- `input`: input id.
  - 输入 ID。
- `time`: rational time (`rational::toString()`).
  - 有理数时间（`rational::toString()`）。
- `type`: integer enum `NodeKeyframe::Type` (`0` = linear, etc.).
  - 整数枚举 `NodeKeyframe::Type`（`0` 为线性等）。
- `inhandlex`, `inhandley`, `outhandlex`, `outhandley`: bezier handle coordinates.
  - 贝塞尔手柄坐标。

Text value uses `NodeValue::ValueToString(data_type, value, true)`.
文本值使用 `NodeValue::ValueToString(data_type, value, true)`。

### 4.4 `links` / 链接

Block-to-block link list (for timeline items):

块到块链接列表（用于时间线项目）：

```xml
<links>
  <link>ptr</link>
</links>
```

### 4.5 `connections` / 连接

Input/output connections:

输入/输出连接：

```xml
<connections>
  <connection input="InputId" element="-1">
    <output>ptr</output>
  </connection>
</connections>
```

- `output` is the serialized pointer (`ptr`) of the output node.
- `output` 为输出节点的序列化指针（`ptr`）。

### 4.6 `hints` (Value Hints) / 值提示

Value hints are per-input UI hints stored by `Node::ValueHint::save()` / `load()`:

值提示是按输入保存的 UI 提示，由 `Node::ValueHint::save()` / `load()` 读写：

```xml
<hints>
  <hint input="InputId" element="-1" version="1">
    <types>
      <type>0</type>
    </types>
    <index>0</index>
    <tag>...</tag>
  </hint>
</hints>
```

- `types`: list of allowed `NodeValue::Type` integers.
  - `types`：允许的 `NodeValue::Type` 整数列表。
- `index`: hint index.
  - `index`：提示索引。
- `tag`: hint tag string.
  - `tag`：提示标签字符串。

### 4.7 `context` (Node positions in contexts) / 上下文中的节点位置

```xml
<context>
  <node ptr="other_node_ptr">
    <x>0</x>
    <y>0</y>
    <expanded>0|1</expanded>
  </node>
</context>
```

- Stores the position and expanded state of other nodes inside this node's context (e.g. inside a NodeGroup).
- 保存其他节点在该节点上下文（例如 NodeGroup 内部）中的位置和展开状态。

### 4.8 `caches` / 缓存 UUID

Node cache UUIDs:

节点缓存 UUID：

```xml
<caches>
  <audio>uuid</audio>
  <video>uuid</video>
  <thumb>uuid</thumb>
  <waveform>uuid</waveform>
</caches>
```

### 4.9 `custom` / 自定义节点数据

Default `Node::SaveCustom()` writes nothing. Specific node subclasses may override.

默认 `Node::SaveCustom()` 不写任何内容。具体子类可重写。

---

## 5. Node-specific Custom Data / 节点专属自定义数据

### 5.1 `Footage` (`app/node/project/footage/footage.cpp`)

```xml
<custom>
  <timestamp>1740000000</timestamp>
  <proxy enabled="1" state="ready" stream="0" preset="1">/path/to/proxy.mp4</proxy>
  <proxy enabled="1" state="ready" stream="0" preset="1" custom="1" pwidth="960" pheight="540" pcrf="20" ppreset="fast" pext="mov" paudio="0">/path/to/proxy.mov</proxy>
  <sourcestarttime source="timecode">1/25</sourcestarttime>
  <viewer>...</viewer>
</custom>
```

- `<timestamp>`: media modification timestamp (epoch milliseconds).
  - `<timestamp>`：媒体修改时间戳（Unix 毫秒）。
- `<proxy>`: proxy media state.
  - `<proxy>`：代理媒体状态。
  - Attributes / 属性：
    - `enabled`: `0` or `1`.
    - `enabled`：`0` 或 `1`。
    - `state`: string from `ProxyManager::ProxyStateToString()` (e.g. `missing`, `ready`, `generating`, `failed`).
    - `state`：`ProxyManager::ProxyStateToString()` 返回的字符串（如 `missing`、`ready`、`generating`、`failed`）。
    - `stream`: video stream index used for the proxy.
    - `stream`：代理使用的视频流索引。
    - `preset`: proxy preset version.
    - `preset`：代理预设版本。
    - `custom` (optional): `1` when the footage uses per-footage custom proxy parameters instead of the global settings.
    - `custom`（可选）：为 `1` 表示该素材使用独立的自定义代理参数，而不是全局设置。
    - `pwidth`, `pheight` (optional, requires `custom="1"`): custom proxy dimensions.
    - `pwidth`、`pheight`（可选，需 `custom="1"`）：自定义代理分辨率。
    - `pcrf` (optional): custom x264 CRF value.
    - `pcrf`（可选）：自定义 x264 CRF 值。
    - `ppreset` (optional): custom x264 preset name.
    - `ppreset`（可选）：自定义 x264 预设名称。
    - `pext` (optional): custom proxy container extension (e.g. `mp4`, `mov`).
    - `pext`（可选）：自定义代理容器扩展名（如 `mp4`、`mov`）。
    - `paudio` (optional): `1` if the proxy includes audio streams, `0` for video-only. Proxies generated with audio store the video stream at index 0 followed by the source audio streams in source order.
    - `paudio`（可选）：`1` 表示代理包含音频流，`0` 表示仅视频。包含音频的代理将视频流放在索引 0，其后按源顺序跟随音频流。
  - Text content: proxy file path (may be empty if `enabled` is true but proxy is not yet generated).
  - 文本内容：代理文件路径（如果 `enabled` 为 true 但代理尚未生成，则可能为空）。
- `<sourcestarttime>`: source start time offset.
  - `<sourcestarttime>`：源起始时间偏移。
  - `source` attribute: source identifier (e.g. `timecode`, `bwf_time_reference`, or `manual` when entered by the user).
  - `source` 属性：源标识符（如 `timecode`、`bwf_time_reference`，或用户手动输入时的 `manual`）。
  - Text: rational `numerator/denominator`.
  - 文本：有理数 `numerator/denominator`。
- `<viewer>`: see `ViewerOutput` below.
  - `<viewer>`：见下文 `ViewerOutput`。

### 5.2 `ViewerOutput` (`app/node/output/viewer/viewer.cpp`)

Used by `Footage`, `Sequence`, etc.

`Footage`、`Sequence` 等使用。

```xml
<viewer>
  <workarea version="1">
    <enabled>0|1</enabled>
    <in>num/den</in>
    <out>num/den</out>
  </workarea>
  <markers>
    <marker name="Marker Name" in="num/den" out="num/den" color="0" />
  </markers>
</viewer>
```

- `<workarea>`: render/export work area.
  - `<workarea>`：渲染/导出工作区。
- `<markers>`: timeline markers.
  - `<markers>`：时间线标记。
  - Marker attributes / 标记属性：
    - `name`: marker name.
    - `name`：标记名称。
    - `in` / `out`: rational time range.
    - `in` / `out`：有理数时间范围。
    - `color`: color index.
    - `color`：颜色索引。

### 5.3 `Track` (`app/node/output/track/track.cpp`)

```xml
<custom>
  <height>48</height>
</custom>
```

- `<height>`: track height in the timeline.
  - `<height>`：时间线中轨道的显示高度。

### 5.4 `NodeGroup` (`app/node/group/group.cpp`)

```xml
<custom>
  <inputpassthroughs>
    <inputpassthrough>
      <node>ptr</node>
      <input>InputId</input>
      <element>0</element>
      <id>PassthroughId</id>
      <name>Display Name</name>
      <flags>0</flags>
      <type>float</type>
      <default>...</default>
      <properties>
        <property>
          <key>...</key>
          <value>...</value>
        </property>
      </properties>
    </inputpassthrough>
  </inputpassthroughs>
  <outputpassthrough>ptr</outputpassthrough>
</custom>
```

- `<inputpassthrough>`: maps an inner node's input to a group-level passthrough input.
  - `<inputpassthrough>`：将内部节点输入映射到组级别的透传输入。
  - Child elements / 子元素：
    - `<node>`: pointer id of the inner node.
      - `<node>`：内部节点的指针 ID。
    - `<input>`: inner input id.
      - `<input>`：内部输入 ID。
    - `<element>`: element index.
      - `<element>`：元素索引。
    - `<id>`: passthrough input id on the group.
      - `<id>`：组上的透传输入 ID。
    - `<name>`: display name.
      - `<name>`：显示名称。
    - `<flags>`: input flags (integer bitmask).
      - `<flags>`：输入标志（整数位掩码）。
    - `<type>`: `NodeValue` type name (see `NodeValue::GetDataTypeName()`).
      - `<type>`：`NodeValue` 类型名称（见 `NodeValue::GetDataTypeName()`）。
    - `<default>`: default value encoded with `NodeValue::ValueToString(type, ..., false)`.
      - `<default>`：使用 `NodeValue::ValueToString(type, ..., false)` 编码的默认值。
    - `<properties>`: arbitrary key/value property pairs.
      - `<properties>`：任意键值属性对。
- `<outputpassthrough>`: pointer id of the node that provides the group's output.
  - `<outputpassthrough>`：提供组输出的节点指针 ID。

---

## 6. `VideoParams` / 视频参数

Serialized inside `<standard><track>` for inputs of type `kVideoParams`.

在 `kVideoParams` 类型输入的 `<standard><track>` 中序列化。

```xml
<width>...</width>
<height>...</height>
<depth>...</depth>
<timebase>num/den</timebase>
<format>int</format>
<channelcount>int</channelcount>
<pixelaspectratio>num/den</pixelaspectratio>
<interlacing>int</interlacing>
<divider>int</divider>
<enabled>0|1</enabled>
<x>float</x>
<y>float</y>
<streamindex>int</streamindex>
<videotype>int</videotype>
<framerate>num/den</framerate>
<starttime>int64</starttime>
<duration>int64</duration>
<premultipliedalpha>0|1</premultipliedalpha>
<colorspace>string</colorspace>
<colorrange>int</colorrange>
```

- `format`: `PixelFormat::Format` integer (`U8`, `U10`, `U16`, `F16`, `F32`).
  - `format`：`PixelFormat::Format` 整数（`U8`、`U10`、`U16`、`F16`、`F32`）。
- `interlacing`: `VideoParams::Interlacing` integer.
  - `interlacing`：`VideoParams::Interlacing` 整数。
- `videotype`: `VideoParams::Type` integer.
  - `videotype`：`VideoParams::Type` 整数。
- `colorrange`: `VideoParams::ColorRange` integer (`0` = limited, `1` = full).
  - `colorrange`：`VideoParams::ColorRange` 整数（`0` 为 limited，`1` 为 full）。

---

## 7. `AudioParams` / 音频参数

Serialized inside `<standard><track>` for inputs of type `kAudioParams`.

在 `kAudioParams` 类型输入的 `<standard><track>` 中序列化。

```xml
<samplerate>int</samplerate>
<channellayout>uint64</channellayout>
<format>string</format>
<enabled>0|1</enabled>
<streamindex>int</streamindex>
<duration>int64</duration>
<timebase>num/den</timebase>
```

- `format`: `SampleFormat::to_string()` value.
  - `format`：`SampleFormat::to_string()` 值。

---

## 8. Main Window Layout (`MainWindowLayoutInfo`) / 主窗口布局

The `<layout>` element stores the state of the main window (open folders, sequences, panel data, and Qt window state).

`<layout>` 元素保存主窗口状态（打开的文件夹、序列、面板数据和 Qt 窗口状态）。

```xml
<layout version="1">
  <folders>
    <folder>ptr</folder>
  </folders>
  <timeline>
    <sequence>ptr</sequence>
  </timeline>
  <viewers>
    <viewer>ptr</viewer>
  </viewers>
  <data>
    <panel id="PanelId">
      <option name="key">value</option>
    </panel>
  </data>
  <state>base64</state>
</layout>
```

- `<folders>`: pointer ids of open project folders.
  - `<folders>`：打开的项目文件夹指针 ID。
- `<timeline>`: pointer ids of open sequences.
  - `<timeline>`：打开的序列指针 ID。
- `<viewers>`: same as `<timeline>` in this implementation.
  - `<viewers>`：当前实现与 `<timeline>` 相同。
- `<data>`: per-panel persistent data.
  - `<data>`：每个面板的持久化数据。
- `<state>`: base64-encoded Qt window/toolbar/dock state.
  - `<state>`：Base64 编码的 Qt 窗口/工具栏/停靠状态。

---

## 9. Partial Documents / 部分文档

`ProjectSerializer230220` can emit three kinds of partial documents for copy/paste and interchange:

`ProjectSerializer230220` 可输出三种部分文档，用于复制/粘贴和交换：

### 9.1 `markers`

```xml
<markers version="1">
  <marker name="..." in="num/den" out="num/den" color="0" />
</markers>
```

### 9.2 `keyframes`

```xml
<keyframes version="1">
  <node id="NodeId">
    <input id="InputId">
      <element id="0">
        <track id="0">
          <key ...>...</key>
        </track>
      </element>
    </input>
  </node>
</keyframes>
```

### 9.3 `nodes` / `timeline`

For copy/paste of nodes (`kOnlyNodes`) the root is `<nodes>`; for clip paste (`kOnlyClips`) the root is `<timeline>`.

复制/粘贴节点（`kOnlyNodes`）时根为 `<nodes>`；剪辑粘贴（`kOnlyClips`）时根为 `<timeline>`。

```xml
<nodes version="1">
  <node id="NodeId" ptr="..." items="ptr1,ptr2">
    ...
  </node>
  <properties>
    <node ptr="...">
      <key>value</key>
    </node>
  </properties>
</nodes>
```

- `items`: comma-separated pointer ids of items that depend on this node; used to avoid duplicating shared dependencies.
  - `items`：依赖该节点的项目指针 ID 逗号列表，用于避免重复共享依赖。
- `<properties>`: opaque string key/value pairs attached to nodes during copy/paste.
  - `<properties>`：复制/粘贴时附加到节点的任意字符串键值对。

---

## 10. Versioning / 版本管理

- Root `<olive version="...">` selects the serializer.
  - 根元素 `<olive version="...">` 选择序列化器。
- Newer files may be rejected with `kProjectTooNew` if no matching serializer exists.
  - 如果没有匹配的序列化器，较新的文件可能以 `kProjectTooNew` 拒绝加载。
- Each major block (`project`, `nodes`, `workarea`, etc.) has its own internal `version` attribute for future expansion.
  - 每个主要块（`project`、`nodes`、`workarea` 等）都有自己的内部 `version` 属性，便于未来扩展。

---

## 11. OpenFX Node Compatibility Notes / OpenFX 节点兼容性说明

- OpenFX nodes are identified by the OFX plugin identifier (`Plugin::getIdentifier()`).
  - OpenFX 节点由 OFX 插件标识符（`Plugin::getIdentifier()`）标识。
- The `<plugins>` list ensures Oak Video Editor can locate external plugins before instantiating nodes.
  - `<plugins>` 列表确保 Oak 在实例化节点前能够定位外部插件。
- If a plugin cannot be found at load time, the node cannot be instantiated and will be skipped.
  - 如果加载时找不到插件，则无法实例化该节点，会被跳过。
