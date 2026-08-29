# Oak Video Editor 项目文件参考手册（详细版）

本文档基于当前源代码实现，描述 Oak Video Editor 的 XML 项目文件格式，目标是足够详细以实现兼容读写器。

> 代码来源：`app/node/project.cpp`、`app/node/node.cpp`、`app/node/value.*`、`app/node/keyframe.*`、`app/node/project/serializer/*`。

## 1. 根元素

```xml
<olive version="230220" url="/path/to/project.ove">
  ...
</olive>
```

- `version`：序列化版本号（`YYMMDD`）。
- `url`：可选的项目文件路径。

## 2. 项目容器

完整保存时，会有 `project` 容器：

```xml
<project>
  <project>...</project>
  <layout>...</layout>
</project>
```

- 内层 `<project>`：项目数据。
- `<layout>`：界面布局（`MainWindowLayoutInfo::fromXml`）。

## 3. 项目数据（`Project::Save`）

```xml
<project version="1">
  <uuid>...</uuid>
  <plugins>...</plugins>
  <nodes>...</nodes>
  <settings>...</settings>
</project>
```

### 3.1 `uuid`
项目 UUID（QUuid 字符串）。

### 3.2 `plugins`
项目中使用的 OpenFX 插件列表，用于加载节点前补充插件搜索路径。

```xml
<plugins>
  <plugin id="com.vendor.Plugin" major="1" minor="2"
          bundle="/path/to/Plugin.ofx.bundle"
          file="/path/to/Plugin.ofx.bundle/Contents/MacOS/Plugin" />
</plugins>
```

属性：
- `id`：OFX 插件标识符（与节点 `id` 相同）。
- `major` / `minor`：插件版本。
- `bundle`：插件 bundle 目录路径（优先使用）。
- `file`：插件二进制路径（备用）。

加载策略：
- 若存在 `<plugins>`，先把 `bundle`（或 `file`）加入 OFX 搜索路径并扫描，然后注册插件节点，再进入 `<nodes>` 解析。

### 3.3 `nodes`
节点图，节点由 `Node::Save()` 写出：

```xml
<nodes version="1">
  <node version="1" id="node.id" ptr="123456">
    <label>...</label>
    <color>...</color>
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

关键属性：
- `id`：节点类型标识。OpenFX 节点为插件标识符。
- `ptr`：序列化指针 ID，用于恢复连接与位置。
- `version`：当前为 `1`。

### 3.4 `settings`
项目设置，键值对形式保存：

已知键：
- `cachesetting`
- `customcachepath`
- `colorconfigfilename`
- `defaultinputcolorspace`
- `colorreferencespace`
- `root`

## 4. 节点序列化（`Node::Save` / `Node::Load`）

### 4.1 `label`
节点显示名。

### 4.2 `color`
节点覆盖颜色（整数索引）。

### 4.3 `input`
每个输入：

```xml
<input id="InputId">
  <primary>...</primary>
  <subelements count="N">
    <element>...</element>
  </subelements>
</input>
```

- `primary`：主元素（element = -1）。
- `subelements`：数组输入，`count` 为数组长度。

#### 4.3.1 立即值结构（`primary` / `element`）

```xml
<keyframing>0|1</keyframing>
<standard>
  <track>...</track>
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

- `keyframing`：是否启用关键帧（仅在输入可关键帧时写出）。
- `standard`：默认值（每条 track 一份）。
- `keyframes`：仅当 `keyframing=1` 时写出。
- `cs*`：仅用于 `kColor`，保存色彩管理信息。

#### 4.3.2 Track 数量
由 `NodeValue::get_number_of_keyframe_tracks()` 决定：

| 类型 | Track 数量 |
| --- | --- |
| kVec2 | 2 |
| kVec3 | 3 |
| kVec4 | 4 |
| kColor | 4 |
| kBezier | 6 |
| 其他 | 1 |

#### 4.3.3 标准值编码
由 `NodeValue::ValueToString()` 写出，`NodeValue::StringToValue()` 读入：

- `kVec2`: `x:y`
- `kVec3`: `x:y:z`
- `kVec4`: `x:y:z:w`
- `kColor`: `r:g:b:a`
- `kBezier`: `x:y:cp1x:cp1y:cp2x:cp2y`
- `kRational`: `num/den`
- `kInt`: 整数文本
- `kBinary`: Base64
- `kText` / `kFont` / `kFile` / `kCombo` / `kStrCombo`: 纯文本
- `kTexture` / `kSamples` / `kNone`: 无文本

特殊情况：
- `kVideoParams` / `kAudioParams` 以子对象形式保存（见第 5/6 节）。
- `kSubtitleParams` 在加载时被跳过（避免覆盖实际字幕数据）。

#### 4.3.4 关键帧（`NodeKeyframe::save`）

```xml
<key input="InputId" time="num/den" type="0" inhandlex="0" inhandley="0" outhandlex="0" outhandley="0">value</key>
```

- `input`：输入 ID。
- `time`：理性时间。
- `type`：关键帧类型枚举值。
- `inhandlex` / `inhandley` / `outhandlex` / `outhandley`：贝塞尔控制点。

文本值使用 `NodeValue::ValueToString(data_type, value, true)`。

### 4.4 `links`
节点间的“块”链接：

```xml
<links>
  <link>ptr</link>
</links>
```

### 4.5 `connections`
输入/输出连接：

```xml
<connections>
  <connection input="InputId" element="-1">
    <output>ptr</output>
  </connection>
</connections>
```

### 4.6 `hints`（输入提示）

```xml
<hints>
  <hint input="InputId" element="-1" version="1">
    <types>
      <type>...</type>
    </types>
    <index>0</index>
    <tag>...</tag>
  </hint>
</hints>
```

### 4.7 `context`（节点位置）

```xml
<context>
  <node ptr="other_node_ptr">
    <x>0</x>
    <y>0</y>
    <expanded>0|1</expanded>
  </node>
</context>
```

### 4.8 `caches`

```xml
<caches>
  <audio>uuid</audio>
  <video>uuid</video>
  <thumb>uuid</thumb>
  <waveform>uuid</waveform>
</caches>
```

### 4.9 `custom`
节点自定义内容，默认实现为空；各子类可覆盖。

## 5. VideoParams
`kVideoParams` 输入以子对象保存：

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

## 6. AudioParams
`kAudioParams` 输入以子对象保存：

```xml
<samplerate>int</samplerate>
<channellayout>uint64</channellayout>
<format>string</format>
<enabled>0|1</enabled>
<streamindex>int</streamindex>
<duration>int64</duration>
<timebase>num/den</timebase>
```

## 7. 部分保存
序列化器支持写出部分数据：

- `<markers>`
- `<keyframes>`
- `<nodes>`（子集）

## 8. OpenFX 插件兼容

- OpenFX 节点 `id` 等于插件标识符。
- `<plugins>` 记录插件路径，加载时会先扫描并注册插件节点。
- 插件缺失时节点无法实例化并被跳过。

## 9. 版本兼容

- 根元素 `version` 决定使用哪个序列化器。
- 若缺少对应版本，会报 `kProjectTooNew` 或 `kProjectTooOld`。
