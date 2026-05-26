<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="zh_CN">
<context>
    <name>AdvancedSettings</name>
    <message>
        <location filename="widgets/adv-settings/advancedsettings.ui" line="38"/>
        <source>Firmware flasher</source>
        <translation>固件刷写</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/advancedsettings.ui" line="59"/>
        <source>USB settings</source>
        <translation>USB选项</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/advancedsettings.ui" line="210"/>
        <source>USB exchange period</source>
        <translation>USB交换时间(原文: USB exchange period)</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/advancedsettings.ui" line="100"/>
        <source>VID</source>
        <translation>VID</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/advancedsettings.ui" line="77"/>
        <source>Device USB name</source>
        <translation>设备USB名称</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/advancedsettings.ui" line="224"/>
        <source> ms</source>
        <translation> 毫秒</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/advancedsettings.ui" line="152"/>
        <source>PID</source>
        <translation>PID</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/advancedsettings.ui" line="136"/>
        <location filename="widgets/adv-settings/advancedsettings.ui" line="188"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Some values may not work on some operating systems (PID 5750 on Ubuntu, for example). After choosing the wrong value, the controller needs to be reflashed with ST-link/UART.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;某些值在某些操作系统上可能无法使用（例如 Ubuntu 上的 PID 5750）。选择错误的值后，需要通过 ST-Link/UART 重新刷写控制器。&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/advancedsettings.ui" line="290"/>
        <source>Application settings</source>
        <translation>应用程序设置</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/advancedsettings.ui" line="358"/>
        <source>Restart for full changes</source>
        <translation>重启应用以进行完全更改</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/advancedsettings.ui" line="361"/>
        <source>Restart</source>
        <translation>重启应用</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/advancedsettings.ui" line="457"/>
        <source>Other settings</source>
        <translation>其他设置</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/advancedsettings.ui" line="544"/>
        <source>Font size</source>
        <translation>字体大小</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/advancedsettings.ui" line="266"/>
        <source>Remove name</source>
        <translation>删除命名</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/advancedsettings.ui" line="248"/>
        <source>Remove device name with selected VID/PID from registry</source>
        <translation>从注册表中删除已选择VID/PID的设备名称</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/advancedsettings.ui" line="478"/>
        <source>About</source>
        <translation>关于</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/advancedsettings.ui" line="311"/>
        <source>Languages</source>
        <translation>语言</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/advancedsettings.ui" line="247"/>
        <location filename="widgets/adv-settings/advancedsettings.ui" line="253"/>
        <location filename="widgets/adv-settings/advancedsettings.ui" line="265"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Sometimes Windows does not update the name in gaming devices and needs to be removed from the registry. This happens automatically when the config is written, but manual deletion may be required when the device is disconnected.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;有时Windows不会更新游戏设备中的名称,需要从注册表中删除。写入配置时会自动执行此操作,但在设备未连接时可能需要手动删除。&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/advancedsettings.cpp" line="102"/>
        <source>Show all connected devices</source>
        <translation>显示所有已连接的设备</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/advancedsettings.cpp" line="103"/>
        <source>Dump every detected FreeJoy device&apos;s USB identity (VID:PID, name, serial). Useful for diagnosing phantom PID conflicts.</source>
        <translation>转储每个检测到的 FreeJoy 设备的 USB 标识（VID:PID、名称、序列号）。用于诊断虚假的 PID 冲突。</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/advancedsettings.cpp" line="205"/>
        <source>&lt;br&gt;Built with Qt %1 (%2)&lt;br&gt;Fork source on &lt;a style=&quot;color: #03A9F4; text-decoration:none;&quot;
                                href=&quot;https://github.com/anpeaco/FreeJoyXConfiguratorQt&quot;&gt;GitHub&lt;/a&gt;;
                                upstream &lt;a style=&quot;color: #03A9F4; text-decoration:none;&quot;
                                href=&quot;https://github.com/FreeJoy-Team/FreeJoyConfiguratorQt&quot;&gt;FreeJoyConfiguratorQt&lt;/a&gt;.&lt;br&gt;
                                Released under GPLv3.&lt;br&gt;</source>
        <translation>&lt;br&gt;使用 Qt %1（%2）构建&lt;br&gt;分支源码见 &lt;a style=&quot;color: #03A9F4; text-decoration:none;&quot; href=&quot;https://github.com/anpeaco/FreeJoyXConfiguratorQt&quot;&gt;GitHub&lt;/a&gt;；上游 &lt;a style=&quot;color: #03A9F4; text-decoration:none;&quot; href=&quot;https://github.com/FreeJoy-Team/FreeJoyConfiguratorQt&quot;&gt;FreeJoyConfiguratorQt&lt;/a&gt;。&lt;br&gt;依据 GPLv3 发布。&lt;br&gt;</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/advancedsettings.cpp" line="213"/>
        <source>&lt;br&gt;See the upstream &lt;a style=&quot;color: #03A9F4; text-decoration:none;&quot;
                            href=&quot;https://github.com/FreeJoy-Team/FreeJoyWiki&quot;&gt;FreeJoy wiki&lt;/a&gt;
                            for detailed wiring and sensor instructions.</source>
        <translation>&lt;br&gt;有关详细的接线和传感器说明，请参阅上游 &lt;a style=&quot;color: #03A9F4; text-decoration:none;&quot; href=&quot;https://github.com/FreeJoy-Team/FreeJoyWiki&quot;&gt;FreeJoy wiki&lt;/a&gt;。</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/advancedsettings.cpp" line="216"/>
        <source>About %1 Configurator</source>
        <translation>关于 %1 Configurator</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/advancedsettings.cpp" line="293"/>
        <source>(unnamed)</source>
        <translation>(未命名)</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/advancedsettings.cpp" line="304"/>
        <source>This VID:PID is already used by: &lt;b&gt;%1&lt;/b&gt;. Pick a unique PID to avoid Windows OEMName cache collisions and DirectInput confusion.</source>
        <translation>此 VID:PID 已被占用：&lt;b&gt;%1&lt;/b&gt;。请选择唯一的 PID，以避免 Windows OEMName 缓存冲突和 DirectInput 混淆。</translation>
    </message>
</context>
<context>
    <name>Axes</name>
    <message>
        <location filename="widgets/axes/axes.ui" line="66"/>
        <source>GroupBox</source>
        <translation>群组框</translation>
    </message>
    <message>
        <location filename="widgets/axes/axes.ui" line="135"/>
        <source>Toggle prescaler, resolution, and axes-to-buttons sliders for this axis</source>
        <translation>切换此轴的预分频、分辨率和“轴转按钮”滑块</translation>
    </message>
    <message>
        <location filename="widgets/axes/axes.ui" line="138"/>
        <source>Show extended settings</source>
        <translation>展开更多选项</translation>
    </message>
    <message>
        <location filename="widgets/axes/axes.ui" line="170"/>
        <location filename="widgets/axes/axes.h" line="189"/>
        <source>Calibrate</source>
        <translation>校准</translation>
    </message>
    <message>
        <location filename="widgets/axes/axes.ui" line="187"/>
        <source>Reset calibration (Min / Center / Max back to factory defaults)</source>
        <translation>重置校准（Min / 中心 / Max 恢复出厂默认值）</translation>
    </message>
    <message>
        <location filename="widgets/axes/axes.ui" line="245"/>
        <source>Center</source>
        <translation>中心</translation>
    </message>
    <message>
        <location filename="widgets/axes/axes.ui" line="582"/>
        <source>Axes &gt; Buttons</source>
        <translation>轴 &gt; 按钮</translation>
    </message>
    <message>
        <location filename="widgets/axes/axes.ui" line="633"/>
        <source>Clear all buttons-from-axes for this axis</source>
        <translation>清除此轴的所有“轴生成按钮”</translation>
    </message>
    <message>
        <location filename="widgets/axes/axes.ui" line="654"/>
        <source>Source</source>
        <translation>源</translation>
    </message>
    <message>
        <location filename="widgets/axes/axes.ui" line="701"/>
        <source>Click then rotate any connected axis to set its source pin as this axis&apos;s source. Click again to cancel. Only works for pins already bound to some axis -- the firmware reports raw values per-axis, not per-pin.</source>
        <translation>点击后转动任意已连接的轴，将其源引脚设为此轴的源。再次点击可取消。仅适用于已绑定到某个轴的引脚 — 固件按轴报告原始值，而非按引脚。</translation>
    </message>
    <message>
        <location filename="widgets/axes/axes.ui" line="209"/>
        <source>Minimum</source>
        <translation>最小</translation>
    </message>
    <message>
        <location filename="widgets/axes/axes.ui" line="256"/>
        <source>Override the natural midpoint between Min and Max with a custom center value</source>
        <translation>用自定义中心值覆盖 Min 与 Max 之间的自然中点</translation>
    </message>
    <message>
        <location filename="widgets/axes/axes.ui" line="270"/>
        <source>Capture the current axis value as the new Center</source>
        <translation>将当前轴值设为新的中心</translation>
    </message>
    <message>
        <location filename="widgets/axes/axes.ui" line="308"/>
        <source>Maximum</source>
        <translation>最大</translation>
    </message>
    <message>
        <location filename="widgets/axes/axes.ui" line="369"/>
        <source>Raw</source>
        <translation>原生(Raw)数据</translation>
    </message>
    <message>
        <location filename="widgets/axes/axes.ui" line="391"/>
        <source>Out</source>
        <translation>处理后的数据</translation>
    </message>
    <message>
        <location filename="widgets/axes/axes.ui" line="533"/>
        <source>Inverted</source>
        <translation>反向</translation>
    </message>
    <message>
        <location filename="widgets/axes/axes.ui" line="555"/>
        <source>Output</source>
        <translation>输出</translation>
    </message>
    <message>
        <location filename="widgets/axes/axes.h" line="190"/>
        <source>Stop &amp;&amp; Save</source>
        <translation>停止并保存</translation>
    </message>
    <message>
        <location filename="widgets/axes/axes.h" line="284"/>
        <location filename="widgets/axes/axes.h" line="321"/>
        <source>None</source>
        <translation>无</translation>
    </message>
    <message>
        <location filename="widgets/axes/axes.h" line="285"/>
        <location filename="widgets/axes/axes.h" line="322"/>
        <source>Encoder</source>
        <translation>编码器</translation>
    </message>
    <message>
        <location filename="widgets/axes/axes.cpp" line="24"/>
        <source>Slider 1</source>
        <translation>滑条 1 (原文Slider 1)</translation>
    </message>
    <message>
        <location filename="widgets/axes/axes.cpp" line="25"/>
        <source>Slider 2</source>
        <translation>滑条 2 (原文Slider 2)</translation>
    </message>
    <message>
        <location filename="widgets/axes/axes.cpp" line="307"/>
        <source>Auto-sequence -- rotate any axis to assign its source, then the next axis arms automatically. Single-click, Esc, or click elsewhere to stop.</source>
        <translation>自动序列 — 转动任意轴以分配其源，然后下一个轴会自动就绪。单击、按 Esc 或点击别处即可停止。</translation>
    </message>
    <message>
        <location filename="widgets/axes/axes.cpp" line="308"/>
        <source>Armed -- rotate any connected axis to assign its source. Click again to cancel.</source>
        <translation>已就绪 — 转动任意已连接的轴以分配其源。再次点击可取消。</translation>
    </message>
    <message>
        <location filename="widgets/axes/axes.cpp" line="309"/>
        <source>Click then rotate any connected axis to set its source pin as this axis&apos;s source. Double-click to auto-sequence down the axes. Only works for pins already bound to some axis -- the firmware reports raw values per-axis, not per-pin.</source>
        <translation>点击后转动任意已连接的轴，将其源引脚设为此轴的源。双击可沿各轴自动序列。仅适用于已绑定到某个轴的引脚 — 固件按轴报告原始值，而非按引脚。</translation>
    </message>
    <message>
        <location filename="widgets/axes/axes.cpp" line="374"/>
        <source>Encoder %1</source>
        <translation>编码器 %1</translation>
    </message>
    <message>
        <location filename="widgets/axes/axes.cpp" line="470"/>
        <source>Also bound to %1</source>
        <translation>也绑定到 %1</translation>
    </message>
    <message>
        <location filename="widgets/axes/axes.cpp" line="473"/>
        <source>This source is also bound to: %1.
Two axes reading the same input will move together.</source>
        <translation>此源也绑定到：%1。
读取同一输入的两个轴会一起移动。</translation>
    </message>
    <message>
        <location filename="widgets/axes/axes.cpp" line="498"/>
        <source>No input source assigned. Select a Main Source above to enable HID output for this axis.</source>
        <translation>未分配输入源。请在上方选择一个主源以启用此轴的 HID 输出。</translation>
    </message>
</context>
<context>
    <name>AxesConfig</name>
    <message>
        <location filename="widgets/axes/axesconfig.ui" line="84"/>
        <source>Hidden axes:</source>
        <translation>隐藏轴:</translation>
    </message>
    <message>
        <location filename="widgets/axes/axesconfig.cpp" line="81"/>
        <source>Pin changes aren&apos;t on the device yet — auto-detect uses the device&apos;s current pins. Write Config to detect newly-assigned analog pins.</source>
        <translation>引脚更改尚未写入设备 — 自动检测使用设备当前的引脚。请写入配置以检测新分配的模拟引脚。</translation>
    </message>
</context>
<context>
    <name>AxesCurves</name>
    <message>
        <location filename="widgets/axes-curves/axescurves.ui" line="32"/>
        <source>X</source>
        <translation>X</translation>
    </message>
    <message>
        <location filename="widgets/axes-curves/axescurves.ui" line="58"/>
        <source>Y</source>
        <translation>Y</translation>
    </message>
    <message>
        <location filename="widgets/axes-curves/axescurves.ui" line="84"/>
        <source>Z</source>
        <translation>Z</translation>
    </message>
    <message>
        <location filename="widgets/axes-curves/axescurves.ui" line="110"/>
        <source>Slider 1</source>
        <translation>滑条 1 {1?}</translation>
    </message>
    <message>
        <location filename="widgets/axes-curves/axescurves.ui" line="120"/>
        <source>Slider 2</source>
        <translation>滑条 1 {2?}</translation>
    </message>
    <message>
        <location filename="widgets/axes-curves/axescurves.ui" line="42"/>
        <source>Rx</source>
        <translation>Rx</translation>
    </message>
    <message>
        <location filename="widgets/axes-curves/axescurves.ui" line="68"/>
        <source>Ry</source>
        <translation>Ry</translation>
    </message>
    <message>
        <location filename="widgets/axes-curves/axescurves.ui" line="94"/>
        <source>Rz</source>
        <translation>Rz</translation>
    </message>
</context>
<context>
    <name>AxesCurvesButton</name>
    <message>
        <location filename="widgets/axes-curves/axescurvesbutton.cpp" line="90"/>
        <source>not in use</source>
        <translation>未使用</translation>
    </message>
</context>
<context>
    <name>AxesCurvesConfig</name>
    <message>
        <location filename="widgets/axes-curves/axescurvesconfig.ui" line="35"/>
        <source>Curve profiles</source>
        <translation>曲线预设</translation>
    </message>
    <message>
        <location filename="widgets/axes-curves/axescurvesconfig.ui" line="82"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Curve profiles are saved in the application configuration.&lt;/p&gt;&lt;p&gt;&lt;span style=&quot; font-size:10pt; font-weight:600;&quot;&gt;Set &lt;/span&gt;&lt;span style=&quot; font-weight:600;&quot;&gt;-&lt;/span&gt; sets profile to the current value of the curve.&lt;/p&gt;&lt;p&gt;&lt;img src=&quot;:/Images/icons/lucide/rotate-ccw.svg&quot; height=&quot;16&quot;/&gt; - resets profile value.&lt;/p&gt;&lt;p&gt;Use &lt;span style=&quot; font-weight:600;&quot;&gt;CTRL&lt;/span&gt; for multiple selection.&lt;/p&gt;&lt;p&gt;You can &lt;span style=&quot; font-weight:600;&quot;&gt;drag&lt;/span&gt; curves with the &lt;span style=&quot; font-weight:600;&quot;&gt;mouse.&lt;/span&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;曲线配置保存在应用程序配置中。&lt;/p&gt;&lt;p&gt;&lt;span style=&quot; font-size:10pt; font-weight:600;&quot;&gt;设置 &lt;/span&gt;&lt;span style=&quot; font-weight:600;&quot;&gt;-&lt;/span&gt; 将配置设为曲线的当前值。&lt;/p&gt;&lt;p&gt;&lt;img src=&quot;:/Images/icons/lucide/rotate-ccw.svg&quot; height=&quot;16&quot;/&gt; - 重置配置值。&lt;/p&gt;&lt;p&gt;按住 &lt;span style=&quot; font-weight:600;&quot;&gt;CTRL&lt;/span&gt; 可多选。&lt;/p&gt;&lt;p&gt;可以用&lt;span style=&quot; font-weight:600;&quot;&gt;鼠标&lt;/span&gt;&lt;span style=&quot; font-weight:600;&quot;&gt;拖动&lt;/span&gt;曲线。&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location filename="widgets/axes-curves/axescurvesconfig.ui" line="98"/>
        <source>Toggle multi-select mode (same as holding Ctrl on the curve list)</source>
        <translation>切换多选模式（与在曲线列表中按住 Ctrl 相同）</translation>
    </message>
</context>
<context>
    <name>AxesCurvesProfiles</name>
    <message>
        <location filename="widgets/axes-curves/axescurvesprofiles.ui" line="55"/>
        <source>Save the current curve to the selected profile</source>
        <translation>将当前曲线保存到所选配置</translation>
    </message>
    <message>
        <location filename="widgets/axes-curves/axescurvesprofiles.ui" line="58"/>
        <source>Set</source>
        <translation>设置</translation>
    </message>
    <message>
        <location filename="widgets/axes-curves/axescurvesprofiles.ui" line="71"/>
        <source>Reset the selected profile to its default curve</source>
        <translation>将所选配置重置为其默认曲线</translation>
    </message>
    <message>
        <location filename="widgets/axes-curves/axescurvesprofiles.cpp" line="13"/>
        <source>Set current axis curve to this preset</source>
        <translation>将当前轴曲线保存为此预设</translation>
    </message>
    <message>
        <location filename="widgets/axes-curves/axescurvesprofiles.cpp" line="77"/>
        <source>Reset to Linear</source>
        <translation>重置为线性</translation>
    </message>
    <message>
        <location filename="widgets/axes-curves/axescurvesprofiles.cpp" line="95"/>
        <source>Reset to Linear Invert</source>
        <translation>重置为线性反转</translation>
    </message>
    <message>
        <location filename="widgets/axes-curves/axescurvesprofiles.cpp" line="124"/>
        <source>Reset to Exponent</source>
        <translation>重置为指数</translation>
    </message>
    <message>
        <location filename="widgets/axes-curves/axescurvesprofiles.cpp" line="153"/>
        <source>Reset to Exponent Invert</source>
        <translation>重置为指数反转</translation>
    </message>
    <message>
        <location filename="widgets/axes-curves/axescurvesprofiles.cpp" line="161"/>
        <source>Reset to Shape</source>
        <translation>重置为波形1</translation>
    </message>
    <message>
        <location filename="widgets/axes-curves/axescurvesprofiles.cpp" line="169"/>
        <source>Reset to Shape2</source>
        <translation>重置为波形2</translation>
    </message>
    <message>
        <location filename="widgets/axes-curves/axescurvesprofiles.cpp" line="177"/>
        <source>Reset to Pulse</source>
        <translation>重置为脉冲</translation>
    </message>
    <message>
        <location filename="widgets/axes-curves/axescurvesprofiles.cpp" line="185"/>
        <source>Reset to no Pulse</source>
        <translation>重置为无脉冲</translation>
    </message>
</context>
<context>
    <name>AxesExtended</name>
    <message>
        <location filename="widgets/axes/axesextended.ui" line="68"/>
        <source>Chanel/Encoder</source>
        <translation>通道/编码器</translation>
    </message>
    <message>
        <location filename="widgets/axes/axesextended.ui" line="87"/>
        <source>Function axis</source>
        <translation>作用轴</translation>
    </message>
    <message>
        <location filename="widgets/axes/axesextended.ui" line="104"/>
        <source>Prescaler %</source>
        <translation>预缩放 %</translation>
    </message>
    <message>
        <location filename="widgets/axes/axesextended.ui" line="114"/>
        <source>Resolution</source>
        <translation>分辨率</translation>
    </message>
    <message>
        <location filename="widgets/axes/axesextended.ui" line="196"/>
        <source>Button 3</source>
        <translation>按钮 3</translation>
    </message>
    <message>
        <location filename="widgets/axes/axesextended.ui" line="228"/>
        <source>Button 2</source>
        <translation>按钮 2</translation>
    </message>
    <message>
        <location filename="widgets/axes/axesextended.ui" line="241"/>
        <source>Button 1</source>
        <translation>按钮 1</translation>
    </message>
    <message>
        <location filename="widgets/axes/axesextended.ui" line="308"/>
        <source>Filter off</source>
        <translation>滤波关闭</translation>
    </message>
    <message>
        <location filename="widgets/axes/axesextended.ui" line="324"/>
        <source>Function</source>
        <translation>功能</translation>
    </message>
    <message>
        <location filename="widgets/axes/axesextended.ui" line="334"/>
        <source>Step div</source>
        <translation>步进分度</translation>
    </message>
    <message>
        <location filename="widgets/axes/axesextended.ui" line="372"/>
        <source>I2C address</source>
        <translation>I2C 地址</translation>
    </message>
    <message>
        <location filename="widgets/axes/axesextended.ui" line="401"/>
        <source> bits</source>
        <translation> 位(bits)</translation>
    </message>
    <message>
        <location filename="widgets/axes/axesextended.ui" line="408"/>
        <source>Offset</source>
        <translation>偏移</translation>
    </message>
    <message>
        <location filename="widgets/axes/axesextended.ui" line="470"/>
        <source>Deadband</source>
        <translation>死区</translation>
    </message>
    <message>
        <location filename="widgets/axes/axesextended.ui" line="480"/>
        <source>Dynamic deadband</source>
        <translation>动态死区</translation>
    </message>
    <message>
        <location filename="widgets/axes/axesextended.cpp" line="52"/>
        <location filename="widgets/axes/axesextended.cpp" line="78"/>
        <source>Filter</source>
        <translation>滤波</translation>
    </message>
    <message>
        <location filename="widgets/axes/axesextended.h" line="61"/>
        <source>None</source>
        <translation>无</translation>
    </message>
    <message>
        <location filename="widgets/axes/axesextended.h" line="62"/>
        <source>Plus</source>
        <translation>增加</translation>
    </message>
    <message>
        <location filename="widgets/axes/axesextended.h" line="63"/>
        <source>Minus</source>
        <translation>减小</translation>
    </message>
    <message>
        <location filename="widgets/axes/axesextended.h" line="64"/>
        <source>Equal</source>
        <translation>平衡</translation>
    </message>
    <message>
        <location filename="widgets/axes/axesextended.h" line="69"/>
        <location filename="widgets/axes/axesextended.h" line="79"/>
        <source>Function enable</source>
        <translation>功能启用</translation>
    </message>
    <message>
        <location filename="widgets/axes/axesextended.h" line="70"/>
        <location filename="widgets/axes/axesextended.h" line="80"/>
        <source>Prescale enable</source>
        <translation>预缩放启用</translation>
    </message>
    <message>
        <location filename="widgets/axes/axesextended.h" line="71"/>
        <location filename="widgets/axes/axesextended.h" line="81"/>
        <source>Center</source>
        <translation>中心</translation>
    </message>
    <message>
        <location filename="widgets/axes/axesextended.h" line="72"/>
        <location filename="widgets/axes/axesextended.h" line="82"/>
        <source>Reset</source>
        <translation>重置</translation>
    </message>
    <message>
        <location filename="widgets/axes/axesextended.h" line="73"/>
        <source>Down</source>
        <translation>下</translation>
    </message>
    <message>
        <location filename="widgets/axes/axesextended.h" line="74"/>
        <source>Up</source>
        <translation>上</translation>
    </message>
    <message>
        <location filename="widgets/axes/axesextended.h" line="87"/>
        <source>off</source>
        <translation>关闭</translation>
    </message>
    <message>
        <location filename="widgets/axes/axesextended.h" line="88"/>
        <source>level 1</source>
        <translation>1级</translation>
    </message>
    <message>
        <location filename="widgets/axes/axesextended.h" line="89"/>
        <source>level 2</source>
        <translation>2级</translation>
    </message>
    <message>
        <location filename="widgets/axes/axesextended.h" line="90"/>
        <source>level 3</source>
        <translation>3级</translation>
    </message>
    <message>
        <location filename="widgets/axes/axesextended.h" line="91"/>
        <source>level 4</source>
        <translation>4级</translation>
    </message>
    <message>
        <location filename="widgets/axes/axesextended.h" line="92"/>
        <source>level 5</source>
        <translation>5级</translation>
    </message>
    <message>
        <location filename="widgets/axes/axesextended.h" line="93"/>
        <source>level 6</source>
        <translation>6级</translation>
    </message>
    <message>
        <location filename="widgets/axes/axesextended.h" line="94"/>
        <source>level 7</source>
        <translation>7级</translation>
    </message>
</context>
<context>
    <name>AxesToButtonsSlider</name>
    <message>
        <location filename="widgets/axes/axestobuttonsslider.ui" line="26"/>
        <location filename="widgets/axes/axestobuttonsslider.ui" line="39"/>
        <location filename="widgets/axes/axestobuttonsslider.ui" line="52"/>
        <location filename="widgets/axes/axestobuttonsslider.ui" line="65"/>
        <location filename="widgets/axes/axestobuttonsslider.ui" line="78"/>
        <location filename="widgets/axes/axestobuttonsslider.ui" line="91"/>
        <location filename="widgets/axes/axestobuttonsslider.ui" line="104"/>
        <location filename="widgets/axes/axestobuttonsslider.ui" line="117"/>
        <location filename="widgets/axes/axestobuttonsslider.ui" line="130"/>
        <location filename="widgets/axes/axestobuttonsslider.ui" line="143"/>
        <location filename="widgets/axes/axestobuttonsslider.ui" line="156"/>
        <location filename="widgets/axes/axestobuttonsslider.ui" line="169"/>
        <source>TextLabel</source>
        <translation>文本标签</translation>
    </message>
</context>
<context>
    <name>ButtonConfig</name>
    <message>
        <location filename="widgets/buttons/buttonconfig.ui" line="41"/>
        <source>Logical buttons</source>
        <translation>逻辑按钮</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonconfig.ui" line="178"/>
        <source>Function</source>
        <translation>功能</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonconfig.ui" line="121"/>
        <source>Press timer</source>
        <translation>按动计时器</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonconfig.ui" line="99"/>
        <source>Delay timer</source>
        <translation>延时计时器</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonconfig.ui" line="77"/>
        <source>Shift</source>
        <translation></translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonconfig.ui" line="143"/>
        <source>Physical</source>
        <translation>实体</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonconfig.ui" line="200"/>
        <source>Operator</source>
        <translation>运算符</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonconfig.ui" line="222"/>
        <source>Source B</source>
        <translation>源 B</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonconfig.ui" line="244"/>
        <source>Invert this button&apos;s input (HIGH becomes pressed, LOW becomes released)</source>
        <translation>反转此按钮的输入（HIGH 视为按下，LOW 视为释放）</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonconfig.ui" line="335"/>
        <source>Disable this button slot</source>
        <translation>禁用此按钮槽</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonconfig.ui" line="390"/>
        <source>Clear every logical button slot back to defaults. Pin, axis, shift register and timer settings are not affected.</source>
        <oldsource>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Press a physical button for assigning it to a logical one.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</oldsource>
        <translation>将每个逻辑按钮槽清除为默认值。引脚、轴、移位寄存器和定时器设置不受影响。</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonconfig.ui" line="486"/>
        <source>Physical buttons</source>
        <translation>物理按钮</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonconfig.cpp" line="236"/>
        <source>Clear All Logical Buttons</source>
        <translation>清除所有逻辑按钮</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonconfig.cpp" line="237"/>
        <source>Reset every logical button slot back to defaults?

Function returns to Normal, physical button assignments, Source B, shift modifier, operator and per-row timers are all cleared. Pin Config, Axes, Shift Registers and the global Timer values are NOT affected.

This cannot be undone except by reading the config back from the device.</source>
        <translation>将每个逻辑按钮槽重置为默认值？

功能恢复为“普通”，物理按钮分配、源 B、Shift 修饰键、运算符和每行定时器都会被清除。引脚配置、轴、移位寄存器和全局定时器值不受影响。

除非从设备重新读取配置，否则无法撤销此操作。</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonconfig.cpp" line="275"/>
        <source>Matrix</source>
        <translation>矩阵</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonconfig.cpp" line="285"/>
        <source>Shift register %1</source>
        <translation>移位寄存器 %1</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonconfig.cpp" line="289"/>
        <source>Shift registers</source>
        <translation>移位寄存器</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonconfig.cpp" line="299"/>
        <source>Axis %1 to buttons</source>
        <translation>轴 %1 转按钮</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonconfig.cpp" line="303"/>
        <source>Axis-to-buttons</source>
        <translation>轴转按钮</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonconfig.cpp" line="306"/>
        <source>Direct</source>
        <translation>直接</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonconfig.cpp" line="417"/>
        <source>Other</source>
        <translation>其他</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonconfig.cpp" line="906"/>
        <source>Logical Buttons Cleared</source>
        <translation>逻辑按钮已清除</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonconfig.cpp" line="907"/>
        <source>The connection change removed the input(s) referenced by logical button(s) %1. Their physical button / Source B have been cleared. Please reassign before saving.</source>
        <translation>连接更改移除了逻辑按钮 %1 所引用的输入。它们的物理按钮 / 源 B 已被清除。请在保存前重新分配。</translation>
    </message>
</context>
<context>
    <name>ButtonLogical</name>
    <message>
        <location filename="widgets/buttons/buttonlogical.ui" line="59"/>
        <source>Drag to reorder</source>
        <translation>拖动以重新排序</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.ui" line="125"/>
        <source>Click then press a physical button to assign it to this slot. Double-click to auto-sequence down the rows. 5 second timeout.</source>
        <translation>点击后按下一个物理按钮，将其分配给此槽。双击可沿各行自动序列。5 秒超时。</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.ui" line="196"/>
        <source>Disable this button slot</source>
        <translation>禁用此按钮槽</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.ui" line="255"/>
        <source>Invert this button&apos;s input (HIGH becomes pressed, LOW becomes released)</source>
        <translation>反转此按钮的输入（HIGH 视为按下，LOW 视为释放）</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.ui" line="352"/>
        <source>Click then press a physical button to assign it to Source B. 5 second timeout.</source>
        <translation>点击后按下一个物理按钮，将其分配给源 B。5 秒超时。</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="190"/>
        <source>Timer 1</source>
        <translation>计时器 1</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="191"/>
        <source>Timer 2</source>
        <translation>计时器 2</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="192"/>
        <source>Timer 3</source>
        <translation>计时器 3</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="189"/>
        <location filename="widgets/buttons/buttonlogical.h" line="197"/>
        <location filename="widgets/buttons/buttonlogical.h" line="261"/>
        <source>-</source>
        <translation></translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="198"/>
        <source>Shift 1</source>
        <translation></translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="199"/>
        <source>Shift 2</source>
        <translation></translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="200"/>
        <source>Shift 3</source>
        <translation></translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="201"/>
        <source>Shift 4</source>
        <translation></translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="202"/>
        <source>Shift 5</source>
        <translation></translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="220"/>
        <source>Toggle switch ON</source>
        <translation>拨动开关 ON</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="219"/>
        <source>Toggle switch ON/OFF</source>
        <translation>拨动开关 ON/OFF</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="203"/>
        <source>Shift 6</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="204"/>
        <source>Shift 7</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="205"/>
        <source>Shift 8</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="213"/>
        <source>Normal</source>
        <translation>普通</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="214"/>
        <source>Double tap</source>
        <translation>双击</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="215"/>
        <source>Tap</source>
        <translation>点击</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="216"/>
        <source>Logic</source>
        <translation>逻辑</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="218"/>
        <source>Toggle</source>
        <translation>切换</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="221"/>
        <source>Toggle switch OFF</source>
        <translation>拨动开关 OFF</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="223"/>
        <source>POV1 Up</source>
        <translation>视角1 上</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="224"/>
        <source>POV1 Right</source>
        <translation>视角1 右</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="225"/>
        <source>POV1 Down</source>
        <translation>视角1 下</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="226"/>
        <source>POV1 Left</source>
        <translation>视角1 左</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="227"/>
        <source>POV1 Center</source>
        <translation>视角1 居中</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="228"/>
        <source>POV2 Up</source>
        <translation>视角2 上</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="229"/>
        <source>POV2 Right</source>
        <translation>视角2 右</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="230"/>
        <source>POV2 Down</source>
        <translation>视角2 下</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="231"/>
        <source>POV2 Left</source>
        <translation>视角2 左</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="232"/>
        <source>POV2 Center</source>
        <translation>视角2 居中</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="233"/>
        <source>POV3 Up</source>
        <translation>视角3 上</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="234"/>
        <source>POV3 Right</source>
        <translation>视角3 右</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="235"/>
        <source>POV3 Down</source>
        <translation>视角3 下</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="236"/>
        <source>POV3 Left</source>
        <translation>视角3 左</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="237"/>
        <source>POV3 Center</source>
        <translation type="unfinished">视角2 居中 {3 ?}</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="238"/>
        <source>POV4 Up</source>
        <translation>视角4 上</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="239"/>
        <source>POV4 Right</source>
        <translation>视角4 右</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="240"/>
        <source>POV4 Down</source>
        <translation>视角4 下</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="241"/>
        <source>POV4 Left</source>
        <translation>视角4 左</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="242"/>
        <source>POV4 Center</source>
        <translation type="unfinished">视角2 居中 {4 ?}</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="244"/>
        <source>Encoder A</source>
        <translation>编码器 A</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="245"/>
        <source>Encoder B</source>
        <translation>编码器 B</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="246"/>
        <source>Radio 1</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="247"/>
        <source>Radio 2</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="248"/>
        <source>Radio 3</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="249"/>
        <source>Radio 4</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="262"/>
        <source>AND</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="263"/>
        <source>OR</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="264"/>
        <source>NOR</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="265"/>
        <source>NAND</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="266"/>
        <source>XOR</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="267"/>
        <source>XNOR</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="250"/>
        <source>Sequential toggle</source>
        <translation>顺序拨动</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.h" line="251"/>
        <source>Sequential button</source>
        <translation>顺序按钮</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.cpp" line="414"/>
        <source>Disabled: gesture-managed slots are driven by the global tap and double-tap windows.</source>
        <translation>已禁用：由手势管理的槽由全局点击和双击时间窗控制。</translation>
    </message>
    <message>
        <location filename="widgets/buttons/buttonlogical.cpp" line="421"/>
        <source>Minimum-hold floor: guarantees the host sees the logical button high for at least this duration after the gesture fires. Minimum 20 ms.</source>
        <translation>最小保持下限：保证手势触发后，主机至少在此时长内看到逻辑按钮为高电平。最小 20 毫秒。</translation>
    </message>
</context>
<context>
    <name>ColorPicker</name>
    <message>
        <location filename="widgets/color/colorpicker.ui" line="46"/>
        <source>Red:</source>
        <translation>红：</translation>
    </message>
    <message>
        <location filename="widgets/color/colorpicker.ui" line="63"/>
        <source>Green:</source>
        <translation>绿：</translation>
    </message>
    <message>
        <location filename="widgets/color/colorpicker.ui" line="70"/>
        <source>Blue:</source>
        <translation>蓝：</translation>
    </message>
    <message>
        <location filename="widgets/color/colorpicker.ui" line="77"/>
        <source>Value:</source>
        <translation>值：</translation>
    </message>
</context>
<context>
    <name>ConfigToFile</name>
    <message>
        <location filename="configtofile.cpp" line="367"/>
        <source>Firmware version in config file doesn&apos;t match configurator version. Check settings before writing config.</source>
        <translation>配置文件中的固件版本与配置器版本不匹配。写入配置前请检查设置。</translation>
    </message>
    <message>
        <location filename="configtofile.cpp" line="368"/>
        <source>Pins B8, B9 reset! In this version I2C moved from pins B8, B9 to B10, B11. Check it!</source>
        <translation>引脚B8、B9已重新设置!在此版本中,I2C从引脚B8、B9移动到B10、B11。请重新检查引脚!</translation>
    </message>
    <message>
        <location filename="configtofile.cpp" line="369"/>
        <source>Firmware version!</source>
        <translation>固件版本！</translation>
    </message>
</context>
<context>
    <name>CurrentConfig</name>
    <message>
        <location filename="widgets/pins/currentconfig.ui" line="32"/>
        <source>Current Config</source>
        <translation>当前配置</translation>
    </message>
    <message>
        <location filename="widgets/pins/currentconfig.ui" line="86"/>
        <source>Axis sources:</source>
        <translation>轴数据源数量:</translation>
    </message>
    <message>
        <location filename="widgets/pins/currentconfig.ui" line="106"/>
        <source>Total LEDs:</source>
        <translation>LED数量总计:</translation>
    </message>
    <message>
        <location filename="widgets/pins/currentconfig.ui" line="116"/>
        <source>Buttons from matrix:</source>
        <translation>按钮矩阵上的按钮数量:</translation>
    </message>
    <message>
        <location filename="widgets/pins/currentconfig.ui" line="224"/>
        <source>Buttons from axes:</source>
        <translation>轴上的按钮数量:</translation>
    </message>
    <message>
        <location filename="widgets/pins/currentconfig.ui" line="310"/>
        <source>0</source>
        <translation>0</translation>
    </message>
    <message>
        <location filename="widgets/pins/currentconfig.ui" line="96"/>
        <source>Single buttons:</source>
        <translation>独立按钮数量:</translation>
    </message>
    <message>
        <location filename="widgets/pins/currentconfig.ui" line="76"/>
        <source>Columns of buttons:</source>
        <translation>按钮列数:</translation>
    </message>
    <message>
        <location filename="widgets/pins/currentconfig.ui" line="66"/>
        <source>Rows of buttons:</source>
        <translation>按钮行数:</translation>
    </message>
    <message>
        <location filename="widgets/pins/currentconfig.ui" line="126"/>
        <source>Total buttons:</source>
        <translation>按钮数量总计:</translation>
    </message>
    <message>
        <location filename="widgets/pins/currentconfig.ui" line="294"/>
        <source>Buttons from shift regs:</source>
        <translation>移位寄存器上的按钮数量:</translation>
    </message>
</context>
<context>
    <name>DebugWindow</name>
    <message>
        <location filename="widgets/debugwindow.ui" line="14"/>
        <source>Debug</source>
        <translation>调试信息</translation>
    </message>
    <message>
        <location filename="widgets/debugwindow.ui" line="32"/>
        <source>Debug packets speed within 5 seconds</source>
        <translation>5秒内调试数据包速度</translation>
    </message>
    <message>
        <location filename="widgets/debugwindow.ui" line="86"/>
        <source>Write log to file</source>
        <translation>将日志保存到文件</translation>
    </message>
    <message>
        <location filename="widgets/debugwindow.ui" line="93"/>
        <source>Write a timestamped marker line to the log so you can find this moment quickly when reviewing later</source>
        <translation>在日志中写入一行带时间戳的标记，以便稍后查看时快速找到此刻</translation>
    </message>
    <message>
        <location filename="widgets/debugwindow.ui" line="142"/>
        <source>Buttons log</source>
        <translation>按钮日志</translation>
    </message>
    <message>
        <location filename="widgets/debugwindow.ui" line="119"/>
        <source>within 5 seconds</source>
        <translation>5秒内</translation>
    </message>
    <message>
        <location filename="widgets/debugwindow.ui" line="122"/>
        <location filename="widgets/debugwindow.cpp" line="136"/>
        <source>0 ms</source>
        <translation>0 毫秒</translation>
    </message>
    <message>
        <location filename="widgets/debugwindow.ui" line="76"/>
        <source>0</source>
        <translation>0</translation>
    </message>
    <message>
        <location filename="widgets/debugwindow.ui" line="132"/>
        <source>Packets received:</source>
        <translation>已接收数据包:</translation>
    </message>
    <message>
        <location filename="widgets/debugwindow.ui" line="35"/>
        <source>Packets speed:</source>
        <translation>数据包速度:</translation>
    </message>
    <message>
        <location filename="widgets/debugwindow.ui" line="45"/>
        <source>Application log</source>
        <translation>应用程序日志</translation>
    </message>
    <message>
        <location filename="widgets/debugwindow.cpp" line="121"/>
        <source> ms</source>
        <translation> 毫秒</translation>
    </message>
</context>
<context>
    <name>Encoders</name>
    <message>
        <location filename="widgets/encoders/encoders.ui" line="60"/>
        <location filename="widgets/encoders/encoders.ui" line="99"/>
        <location filename="widgets/encoders/encoders.cpp" line="11"/>
        <source>Not defined</source>
        <translation>未定义</translation>
    </message>
    <message>
        <location filename="widgets/encoders/encoders.ui" line="73"/>
        <source>Encoder type</source>
        <translation>编码器类型</translation>
    </message>
    <message>
        <location filename="widgets/encoders/encoders.ui" line="86"/>
        <source>Input A</source>
        <translation>A相输入</translation>
    </message>
    <message>
        <location filename="widgets/encoders/encoders.ui" line="125"/>
        <source>Input B</source>
        <translation>B相输入</translation>
    </message>
    <message>
        <location filename="widgets/encoders/encoders.h" line="45"/>
        <source>Encoder 1x</source>
        <translation></translation>
    </message>
    <message>
        <location filename="widgets/encoders/encoders.h" line="46"/>
        <source>Encoder 2x</source>
        <translation></translation>
    </message>
    <message>
        <location filename="widgets/encoders/encoders.h" line="47"/>
        <source>Encoder 4x</source>
        <translation></translation>
    </message>
    <message>
        <location filename="widgets/encoders/encoders.cpp" line="48"/>
        <location filename="widgets/encoders/encoders.cpp" line="61"/>
        <source>Button #%1</source>
        <translation>按钮 #%1</translation>
    </message>
</context>
<context>
    <name>EncodersConfig</name>
    <message>
        <location filename="widgets/encoders/encodersconfig.ui" line="65"/>
        <source>Fast encoders</source>
        <translation>快速编码器</translation>
    </message>
    <message>
        <location filename="widgets/encoders/encodersconfig.ui" line="92"/>
        <source>Slow encoders</source>
        <translation>低分辨率编码器</translation>
    </message>
</context>
<context>
    <name>FastEncoder</name>
    <message>
        <location filename="widgets/encoders/fastencoder.ui" line="60"/>
        <source>Input A</source>
        <translation>输入 A</translation>
    </message>
    <message>
        <location filename="widgets/encoders/fastencoder.ui" line="73"/>
        <source>Input B</source>
        <translation>输入 B</translation>
    </message>
    <message>
        <location filename="widgets/encoders/fastencoder.ui" line="86"/>
        <source>Encoder type</source>
        <translation>编码器类型</translation>
    </message>
    <message>
        <location filename="widgets/encoders/fastencoder.ui" line="96"/>
        <source>Enable</source>
        <translation>启用</translation>
    </message>
    <message>
        <location filename="widgets/encoders/fastencoder.ui" line="99"/>
        <source>Enable this fast encoder. Maps both encoder pins (A and B) to FAST_ENCODER in Pin Config; clearing the checkbox releases them back to NOT_USED.</source>
        <translation>启用此快速编码器。在引脚配置中将两个编码器引脚（A 和 B）映射为 FAST_ENCODER；取消勾选会将它们释放回 NOT_USED。</translation>
    </message>
    <message>
        <location filename="widgets/encoders/fastencoder.ui" line="109"/>
        <location filename="widgets/encoders/fastencoder.ui" line="122"/>
        <location filename="widgets/encoders/fastencoder.cpp" line="10"/>
        <source>Not defined</source>
        <translation>未定义</translation>
    </message>
    <message>
        <location filename="widgets/encoders/fastencoder.h" line="68"/>
        <source>Encoder 1x</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="widgets/encoders/fastencoder.h" line="69"/>
        <source>Encoder 2x</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="widgets/encoders/fastencoder.h" line="70"/>
        <source>Encoder 4x</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>FlashConfirmationDialog</name>
    <message>
        <location filename="dialogs/flashconfirmationdialog.ui" line="14"/>
        <location filename="dialogs/flashconfirmationdialog.ui" line="28"/>
        <source>Confirm flash</source>
        <translation>确认刷写</translation>
    </message>
    <message>
        <location filename="dialogs/flashconfirmationdialog.ui" line="38"/>
        <source>Device</source>
        <translation>设备</translation>
    </message>
    <message>
        <location filename="dialogs/flashconfirmationdialog.ui" line="43"/>
        <source>Name:</source>
        <translation>名称：</translation>
    </message>
    <message>
        <location filename="dialogs/flashconfirmationdialog.ui" line="51"/>
        <source>Serial:</source>
        <translation>序列号：</translation>
    </message>
    <message>
        <location filename="dialogs/flashconfirmationdialog.ui" line="59"/>
        <location filename="dialogs/flashconfirmationdialog.ui" line="92"/>
        <source>Board:</source>
        <translation>开发板：</translation>
    </message>
    <message>
        <location filename="dialogs/flashconfirmationdialog.ui" line="67"/>
        <source>Current firmware:</source>
        <translation>当前固件：</translation>
    </message>
    <message>
        <location filename="dialogs/flashconfirmationdialog.ui" line="79"/>
        <source>Target firmware</source>
        <translation>目标固件</translation>
    </message>
    <message>
        <location filename="dialogs/flashconfirmationdialog.ui" line="84"/>
        <source>File:</source>
        <translation>文件：</translation>
    </message>
    <message>
        <location filename="dialogs/flashconfirmationdialog.ui" line="100"/>
        <source>Version:</source>
        <translation>版本：</translation>
    </message>
    <message>
        <location filename="dialogs/flashconfirmationdialog.ui" line="120"/>
        <source>The configurator will:</source>
        <translation>配置器将：</translation>
    </message>
    <message>
        <location filename="dialogs/flashconfirmationdialog.ui" line="133"/>
        <source>Approximately 30 seconds. Do not unplug the device during this process.</source>
        <translation>大约 30 秒。此过程中请勿拔下设备。</translation>
    </message>
    <message>
        <location filename="dialogs/flashconfirmationdialog.cpp" line="47"/>
        <source>Flash</source>
        <translation>刷写</translation>
    </message>
    <message>
        <location filename="dialogs/flashconfirmationdialog.cpp" line="150"/>
        <source>(in bootloader -- version unknown)</source>
        <translation>(处于引导加载程序中 — 版本未知)</translation>
    </message>
    <message>
        <location filename="dialogs/flashconfirmationdialog.cpp" line="159"/>
        <source>Legacy binary (no metadata)</source>
        <translation>旧版二进制文件（无元数据）</translation>
    </message>
    <message>
        <location filename="dialogs/flashconfirmationdialog.cpp" line="171"/>
        <source>Same generation -- your configuration will be preserved.</source>
        <translation>同一代 — 您的配置将被保留。</translation>
    </message>
    <message>
        <location filename="dialogs/flashconfirmationdialog.cpp" line="175"/>
        <source>Upgrade -- your configuration will be migrated to the new wire format.</source>
        <translation>升级 — 您的配置将迁移到新的传输格式。</translation>
    </message>
    <message>
        <location filename="dialogs/flashconfirmationdialog.cpp" line="179"/>
        <source>Upgrade -- no migrator available for your current firmware. Your configuration will be saved to disk but the device will factory-reset.</source>
        <translation>升级 — 没有适用于您当前固件的迁移器。您的配置将保存到磁盘，但设备将恢复出厂设置。</translation>
    </message>
    <message>
        <location filename="dialogs/flashconfirmationdialog.cpp" line="185"/>
        <source>Downgrade -- the target firmware is older than the current. Your configuration will be saved to disk but cannot be auto-restored; the device will factory-reset.</source>
        <translation>降级 — 目标固件比当前固件旧。您的配置将保存到磁盘，但无法自动恢复；设备将恢复出厂设置。</translation>
    </message>
    <message>
        <location filename="dialogs/flashconfirmationdialog.cpp" line="191"/>
        <source>Recovery flash -- the device is in bootloader mode. No backup is possible; the device will factory-reset.</source>
        <translation>恢复刷写 — 设备处于引导加载程序模式。无法备份；设备将恢复出厂设置。</translation>
    </message>
    <message>
        <location filename="dialogs/flashconfirmationdialog.cpp" line="196"/>
        <source>Incompatible -- the selected firmware does not match this device&apos;s board type. Flash refused.</source>
        <translation>不兼容 — 所选固件与此设备的开发板类型不匹配。已拒绝刷写。</translation>
    </message>
    <message>
        <location filename="dialogs/flashconfirmationdialog.cpp" line="212"/>
        <source>%1. Save a backup of the current configuration to disk.</source>
        <translation>%1。将当前配置的备份保存到磁盘。</translation>
    </message>
    <message>
        <location filename="dialogs/flashconfirmationdialog.cpp" line="215"/>
        <source>%1. Reboot the device into bootloader mode.</source>
        <translation>%1。将设备重启进入引导加载程序模式。</translation>
    </message>
    <message>
        <location filename="dialogs/flashconfirmationdialog.cpp" line="216"/>
        <source>%1. Transfer the new firmware (~%2 bytes).</source>
        <translation>%1。传输新固件（约 %2 字节）。</translation>
    </message>
    <message>
        <location filename="dialogs/flashconfirmationdialog.cpp" line="218"/>
        <source>%1. Wait for the device to restart and re-enumerate.</source>
        <translation>%1。等待设备重启并重新枚举。</translation>
    </message>
    <message>
        <location filename="dialogs/flashconfirmationdialog.cpp" line="221"/>
        <source>%1. Write the (migrated) configuration back to the device.</source>
        <translation>%1。将（已迁移的）配置写回设备。</translation>
    </message>
    <message>
        <location filename="dialogs/flashconfirmationdialog.cpp" line="223"/>
        <source>%1. Leave the device factory-reset -- restore the backup manually from the saved file if needed.</source>
        <translation>%1。设备保持出厂重置状态 — 如有需要，请从已保存的文件手动恢复备份。</translation>
    </message>
    <message>
        <location filename="dialogs/flashconfirmationdialog.cpp" line="227"/>
        <source>Pick a firmware binary that matches the device&apos;s board, then try again.</source>
        <translation>请选择与设备开发板匹配的固件二进制文件，然后重试。</translation>
    </message>
    <message>
        <location filename="dialogs/flashconfirmationdialog.cpp" line="240"/>
        <source>(unknown)</source>
        <translation>(未知)</translation>
    </message>
    <message>
        <location filename="dialogs/flashconfirmationdialog.cpp" line="256"/>
        <source>Unknown</source>
        <translation>未知</translation>
    </message>
</context>
<context>
    <name>FlashProgressDialog</name>
    <message>
        <location filename="dialogs/flashprogressdialog.ui" line="14"/>
        <source>Flashing firmware</source>
        <translation>正在刷写固件</translation>
    </message>
    <message>
        <location filename="dialogs/flashprogressdialog.ui" line="47"/>
        <source>Status:</source>
        <translation>状态：</translation>
    </message>
    <message>
        <location filename="dialogs/flashprogressdialog.cpp" line="86"/>
        <source>Cancel requested. Waiting for the current step to wind down...</source>
        <translation>已请求取消。正在等待当前步骤结束……</translation>
    </message>
    <message>
        <location filename="dialogs/flashprogressdialog.cpp" line="97"/>
        <source>Close</source>
        <translation>关闭</translation>
    </message>
    <message>
        <location filename="dialogs/flashprogressdialog.cpp" line="100"/>
        <source>Cancel</source>
        <translation>取消</translation>
    </message>
    <message>
        <location filename="dialogs/flashprogressdialog.cpp" line="139"/>
        <source>Preparing...</source>
        <translation>正在准备……</translation>
    </message>
    <message>
        <location filename="dialogs/flashprogressdialog.cpp" line="140"/>
        <source>Step 1 of 5: Saving config backup...</source>
        <translation>第 1/5 步：正在保存配置备份……</translation>
    </message>
    <message>
        <location filename="dialogs/flashprogressdialog.cpp" line="141"/>
        <source>Step 2 of 5: Entering bootloader mode...</source>
        <translation>第 2/5 步：正在进入引导加载程序模式……</translation>
    </message>
    <message>
        <location filename="dialogs/flashprogressdialog.cpp" line="142"/>
        <source>Step 3 of 5: Flashing firmware...</source>
        <translation>第 3/5 步：正在刷写固件……</translation>
    </message>
    <message>
        <location filename="dialogs/flashprogressdialog.cpp" line="143"/>
        <source>Step 4 of 5: Waiting for device to restart...</source>
        <translation>第 4/5 步：正在等待设备重启……</translation>
    </message>
    <message>
        <location filename="dialogs/flashprogressdialog.cpp" line="144"/>
        <source>Step 5 of 5: Restoring config...</source>
        <translation>第 5/5 步：正在恢复配置……</translation>
    </message>
    <message>
        <location filename="dialogs/flashprogressdialog.cpp" line="145"/>
        <source>Done.</source>
        <translation>完成。</translation>
    </message>
    <message>
        <location filename="dialogs/flashprogressdialog.cpp" line="146"/>
        <source>Flash failed.</source>
        <translation>刷写失败。</translation>
    </message>
    <message>
        <location filename="dialogs/flashprogressdialog.cpp" line="147"/>
        <source>Device didn&apos;t return — unplug and replug to recover</source>
        <translation>设备未返回 — 请拔下并重新插入以恢复</translation>
    </message>
</context>
<context>
    <name>FlashSession</name>
    <message>
        <location filename="flash/flashsession.cpp" line="61"/>
        <source>No firmware path supplied to FlashSession.</source>
        <translation>未向 FlashSession 提供固件路径。</translation>
    </message>
    <message>
        <location filename="flash/flashsession.cpp" line="71"/>
        <source>Couldn&apos;t open firmware file: %1</source>
        <translation>无法打开固件文件：%1</translation>
    </message>
    <message>
        <location filename="flash/flashsession.cpp" line="77"/>
        <source>Firmware file is empty: %1</source>
        <translation>固件文件为空：%1</translation>
    </message>
    <message>
        <location filename="flash/flashsession.cpp" line="94"/>
        <source>Reading current device configuration...</source>
        <translation>正在读取当前设备配置……</translation>
    </message>
    <message>
        <location filename="flash/flashsession.cpp" line="110"/>
        <source>Cancelled by user.</source>
        <translation>已被用户取消。</translation>
    </message>
    <message>
        <location filename="flash/flashsession.cpp" line="139"/>
        <source>Cancelled: read of current device configuration failed and user declined to proceed without a backup.</source>
        <translation>已取消：读取当前设备配置失败，且用户拒绝在没有备份的情况下继续。</translation>
    </message>
    <message>
        <location filename="flash/flashsession.cpp" line="147"/>
        <location filename="flash/flashsession.cpp" line="194"/>
        <source>Sending firmware...</source>
        <translation>正在发送固件……</translation>
    </message>
    <message>
        <location filename="flash/flashsession.cpp" line="153"/>
        <source>Rebooting device into bootloader mode...</source>
        <translation>正在将设备重启进入引导加载程序模式……</translation>
    </message>
    <message>
        <location filename="flash/flashsession.cpp" line="168"/>
        <source>Waiting for device to reappear as flasher...</source>
        <translation>正在等待设备以刷写器身份重新出现……</translation>
    </message>
    <message>
        <location filename="flash/flashsession.cpp" line="211"/>
        <source>Bootloader didn&apos;t reappear within %1 s. Try unplugging and replugging the device -- the flash will resume automatically once the bootloader comes back. If unplugging doesn&apos;t help, click Cancel and try again.</source>
        <translation>引导加载程序未在 %1 秒内重新出现。请尝试拔下并重新插入设备 — 引导加载程序恢复后刷写会自动继续。如果拔插无效，请点击“取消”并重试。</translation>
    </message>
    <message>
        <location filename="flash/flashsession.cpp" line="223"/>
        <source>Device didn&apos;t return after %1 s. The flash itself likely succeeded -- the device just hasn&apos;t re-enumerated yet. Try unplugging and replugging now. The configurator will resume automatically once the device comes back.</source>
        <translation>设备在 %1 秒后仍未返回。刷写本身很可能已成功 — 只是设备尚未重新枚举。请现在尝试拔下并重新插入。设备恢复后配置器会自动继续。</translation>
    </message>
    <message>
        <location filename="flash/flashsession.cpp" line="229"/>
        <location filename="flash/flashsession.cpp" line="247"/>
        <location filename="flash/flashsession.cpp" line="380"/>
        <source>Your backup is at: %1</source>
        <translation>您的备份位于：%1</translation>
    </message>
    <message>
        <location filename="flash/flashsession.cpp" line="244"/>
        <source>Recovery cancelled by user. The device may need manual recovery via STM32 Cube Programmer + ST-Link.</source>
        <translation>恢复已被用户取消。设备可能需要通过 STM32 Cube Programmer + ST-Link 手动恢复。</translation>
    </message>
    <message>
        <location filename="flash/flashsession.cpp" line="272"/>
        <source>Bootloader reported SIZE error -- the firmware image exceeds the device&apos;s app region.</source>
        <translation>引导加载程序报告 SIZE 错误 — 固件映像超出了设备的应用程序区域。</translation>
    </message>
    <message>
        <location filename="flash/flashsession.cpp" line="276"/>
        <source>Bootloader reported CRC error -- the transferred bytes do not match the expected checksum. Try the flash again; if it keeps failing the firmware file may be corrupted.</source>
        <translation>引导加载程序报告 CRC 错误 — 传输的字节与预期校验和不匹配。请重试刷写；如果仍然失败，固件文件可能已损坏。</translation>
    </message>
    <message>
        <location filename="flash/flashsession.cpp" line="282"/>
        <source>Bootloader reported ERASE error -- the device flash could not be erased. This usually means the device&apos;s flash is write-protected; recovery via STM32 Cube Programmer + ST-Link may be required.</source>
        <translation>引导加载程序报告 ERASE 错误 — 无法擦除设备的 Flash。这通常意味着设备的 Flash 处于写保护状态；可能需要通过 STM32 Cube Programmer + ST-Link 恢复。</translation>
    </message>
    <message>
        <location filename="flash/flashsession.cpp" line="288"/>
        <source>Lost contact with the device mid-flash. The bootloader&apos;s response stopped coming back -- the USB cable may have been disconnected, or the device&apos;s bootloader may have crashed.

Unplug and replug the device, then retry the flash. If the device doesn&apos;t enumerate at all after replugging, recover via STM32 Cube Programmer + ST-Link.</source>
        <translation>刷写过程中与设备失去联系。引导加载程序停止响应 — 可能是 USB 线缆被断开，或设备的引导加载程序崩溃。

请拔下并重新插入设备，然后重试刷写。如果重新插入后设备完全无法枚举，请通过 STM32 Cube Programmer + ST-Link 恢复。</translation>
    </message>
    <message>
        <location filename="flash/flashsession.cpp" line="298"/>
        <source>Bootloader reported unknown error (status=0x%1).</source>
        <translation>引导加载程序报告未知错误（status=0x%1）。</translation>
    </message>
    <message>
        <location filename="flash/flashsession.cpp" line="316"/>
        <source>Flash complete. Waiting for device to restart...</source>
        <translation>刷写完成。正在等待设备重启……</translation>
    </message>
    <message>
        <location filename="flash/flashsession.cpp" line="338"/>
        <source>Device returned with incompatible firmware -- auto-restore skipped.</source>
        <translation>设备返回时固件不兼容 — 已跳过自动恢复。</translation>
    </message>
    <message>
        <location filename="flash/flashsession.cpp" line="339"/>
        <source>Flash complete. Device runs new firmware but the configurator can&apos;t auto-restore (incompatible wire format).</source>
        <translation>刷写完成。设备运行新固件，但配置器无法自动恢复（传输格式不兼容）。</translation>
    </message>
    <message>
        <location filename="flash/flashsession.cpp" line="355"/>
        <source>Device factory-reset. Backup remains available for manual restore.</source>
        <translation>设备已恢复出厂设置。备份仍可用于手动恢复。</translation>
    </message>
    <message>
        <location filename="flash/flashsession.cpp" line="356"/>
        <source>Flash complete; auto-restore skipped per verdict.</source>
        <translation>刷写完成；根据判定已跳过自动恢复。</translation>
    </message>
    <message>
        <location filename="flash/flashsession.cpp" line="360"/>
        <source>Device back online. Writing saved configuration...</source>
        <translation>设备已重新上线。正在写入已保存的配置……</translation>
    </message>
    <message>
        <location filename="flash/flashsession.cpp" line="373"/>
        <location filename="flash/flashsession.cpp" line="374"/>
        <source>Configuration restored. Flash complete.</source>
        <translation>配置已恢复。刷写完成。</translation>
    </message>
    <message>
        <location filename="flash/flashsession.cpp" line="378"/>
        <source>Flash completed but the post-flash config write failed.</source>
        <translation>刷写已完成，但刷写后的配置写入失败。</translation>
    </message>
</context>
<context>
    <name>Flasher</name>
    <message>
        <location filename="widgets/adv-settings/flasher.ui" line="42"/>
        <source>Connected FreeJoy devices. Select one to make it the configurator&apos;s active device (same effect as the device dropdown in the toolbar). Rows tagged RECOVERY are stuck in bootloader mode -- they accept recovery flashes only.</source>
        <translation>已连接的 FreeJoy 设备。选择一个使其成为配置器的活动设备（与工具栏中的设备下拉框效果相同）。标记为 RECOVERY 的行卡在引导加载程序模式中 — 它们仅接受恢复刷写。</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/flasher.ui" line="67"/>
        <source>Source:</source>
        <translation>源：</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/flasher.ui" line="74"/>
        <source>Pick a firmware build to flash. Entries come from auto-fetched GitHub releases and from local files dropped into the recovery folder. Use the Browse button to pick any .bin from disk.</source>
        <translation>选择要刷写的固件构建。条目来自自动获取的 GitHub 发行版和放入 recovery 文件夹的本地文件。使用“浏览”按钮可从磁盘选择任意 .bin。</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/flasher.ui" line="81"/>
        <source>Open a file picker to choose any .bin from disk (e.g. a fresh build at FreeJoyX/armgcc/build/f103/app/FreeJoy.bin).</source>
        <translation>打开文件选择器以从磁盘选择任意 .bin（例如位于 FreeJoyX/armgcc/build/f103/app/FreeJoy.bin 的新构建）。</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/flasher.ui" line="84"/>
        <source>Browse...</source>
        <translation>浏览……</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/flasher.ui" line="91"/>
        <source>Menu: open the firmware folder (your dev builds), open the recovery folder (fallback builds), or refresh from GitHub.</source>
        <translation>菜单：打开固件文件夹（您的开发构建）、打开 recovery 文件夹（后备构建）或从 GitHub 刷新。</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/flasher.ui" line="105"/>
        <source>Selected firmware</source>
        <translation>已选固件</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/flasher.ui" line="117"/>
        <source>File:</source>
        <translation>文件：</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/flasher.ui" line="131"/>
        <source>Board:</source>
        <translation>开发板：</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/flasher.ui" line="145"/>
        <source>Version:</source>
        <translation>版本：</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/flasher.ui" line="159"/>
        <source>Size:</source>
        <translation>大小：</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/flasher.ui" line="211"/>
        <source>Run the full backup -&gt; bootloader -&gt; flash -&gt; restore sequence in one click. Shows a confirmation dialog with the planned steps before any device state changes.</source>
        <translation>一键执行完整的 备份 -&gt; 引导加载程序 -&gt; 刷写 -&gt; 恢复 流程。在更改任何设备状态前会显示包含计划步骤的确认对话框。</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/flasher.ui" line="214"/>
        <location filename="widgets/adv-settings/flasher.cpp" line="485"/>
        <source>Flash</source>
        <translation>刷写</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/flasher.ui" line="233"/>
        <source>Reboots the device into bootloader (DFU) mode so it can accept a new firmware image. Required before clicking Flash Firmware.</source>
        <translation>将设备重启进入引导加载程序（DFU）模式，以便接受新的固件映像。点击“刷写固件”之前必须执行此操作。</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/flasher.ui" line="236"/>
        <source>Enter Flasher Mode</source>
        <translation>进入固件写入模式</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/flasher.ui" line="252"/>
        <source>Flash the selected firmware to the device. The device must already be in flasher mode (click Enter Flasher Mode first). Source is picked from the dropdown above -- &quot;Browse for file...&quot; opens a file picker; named entries are recovery binaries from the recovery/ folder.</source>
        <translation>将所选固件刷写到设备。设备必须已处于刷写器模式（请先点击“进入刷写器模式”）。源从上方的下拉框中选择 —“浏览文件……”会打开文件选择器；命名条目是 recovery/ 文件夹中的恢复二进制文件。</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/flasher.ui" line="255"/>
        <source>Flash Firmware</source>
        <translation>写入固件</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/flasher.ui" line="271"/>
        <source>Abort flasher mode without flashing. Shows instructions for getting the device back to application mode (the bootloader doesn&apos;t have a software-abort command; the user has to power-cycle).</source>
        <translation>在不刷写的情况下中止刷写器模式。显示将设备恢复到应用程序模式的说明（引导加载程序没有软件中止命令；用户需要重新上电）。</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/flasher.ui" line="274"/>
        <source>Abort</source>
        <translation>中止</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/flasher.cpp" line="179"/>
        <source>Connected flasher: </source>
        <translation>已连接的刷写器：</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/flasher.cpp" line="224"/>
        <source> (download)</source>
        <translation>（下载）</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/flasher.cpp" line="249"/>
        <source>[Browsed] %1</source>
        <translation>[浏览] %1</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/flasher.cpp" line="267"/>
        <source>[Build] %1</source>
        <translation>[构建] %1</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/flasher.cpp" line="290"/>
        <source>[Local] %1</source>
        <translation>[本地] %1</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/flasher.cpp" line="320"/>
        <source>Open firmware (build outputs) folder</source>
        <translation>打开固件（构建输出）文件夹</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/flasher.cpp" line="322"/>
        <source>Open recovery (fallback builds) folder</source>
        <translation>打开 recovery（后备构建）文件夹</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/flasher.cpp" line="325"/>
        <source>Refresh from GitHub</source>
        <translation>从 GitHub 刷新</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/flasher.cpp" line="330"/>
        <source>Clear [Browsed] firmware history</source>
        <translation>清除 [浏览] 固件历史记录</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/flasher.cpp" line="377"/>
        <source>Choose firmware binary</source>
        <translation>选择固件二进制文件</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/flasher.cpp" line="379"/>
        <source>Firmware (*.bin);;All files (*)</source>
        <translation>固件 (*.bin);;所有文件 (*)</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/flasher.cpp" line="414"/>
        <source>Pick a firmware source</source>
        <translation>选择固件源</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/flasher.cpp" line="415"/>
        <source>Select a firmware build from the Source dropdown, or click &lt;b&gt;Browse...&lt;/b&gt; to pick a .bin from disk.</source>
        <translation>从“源”下拉框中选择固件构建，或点击 &lt;b&gt;浏览……&lt;/b&gt; 从磁盘选择 .bin。</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/flasher.cpp" line="452"/>
        <source>Download and flash this firmware?</source>
        <translation>下载并刷写此固件？</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/flasher.cpp" line="453"/>
        <source>&lt;p&gt;The selected firmware isn&apos;t downloaded yet.&lt;/p&gt;&lt;p&gt;Source: &lt;b&gt;%1&lt;/b&gt;&lt;br&gt;Tag: &lt;b&gt;%2&lt;/b&gt;&lt;br&gt;Asset: &lt;b&gt;%3&lt;/b&gt;&lt;/p&gt;&lt;p&gt;Continue?&lt;/p&gt;</source>
        <translation>&lt;p&gt;所选固件尚未下载。&lt;/p&gt;&lt;p&gt;源：&lt;b&gt;%1&lt;/b&gt;&lt;br&gt;标签：&lt;b&gt;%2&lt;/b&gt;&lt;br&gt;资源：&lt;b&gt;%3&lt;/b&gt;&lt;/p&gt;&lt;p&gt;是否继续？&lt;/p&gt;</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/flasher.cpp" line="465"/>
        <source>Downloading...</source>
        <translation>正在下载……</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/flasher.cpp" line="470"/>
        <source>Firmware unavailable</source>
        <translation>固件不可用</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/flasher.cpp" line="471"/>
        <source>The selected firmware source couldn&apos;t be resolved to a file. Try refreshing the source list or pick a different entry.</source>
        <translation>无法将所选固件源解析为文件。请尝试刷新源列表或选择其他条目。</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/flasher.cpp" line="489"/>
        <source>Download failed</source>
        <translation>下载失败</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/flasher.cpp" line="490"/>
        <source>Couldn&apos;t download the selected firmware from GitHub. Check your internet connection and try again.</source>
        <translation>无法从 GitHub 下载所选固件。请检查您的网络连接并重试。</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/flasher.cpp" line="506"/>
        <source>Couldn&apos;t open firmware</source>
        <translation>无法打开固件</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/flasher.cpp" line="507"/>
        <source>Couldn&apos;t read the firmware file:
%1

Check that the file exists and the configurator has permission to read it.</source>
        <translation>无法读取固件文件：
%1

请检查文件是否存在以及配置器是否有读取权限。</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/flasher.cpp" line="544"/>
        <source>[RECOVERY] %1</source>
        <translation>[RECOVERY] %1</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/flasher.cpp" line="641"/>
        <source>F103 (from filename)</source>
        <translation>F103（来自文件名）</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/flasher.cpp" line="643"/>
        <source>F411 (from filename)</source>
        <translation>F411（来自文件名）</translation>
    </message>
    <message>
        <location filename="widgets/adv-settings/flasher.cpp" line="645"/>
        <source>Unknown</source>
        <translation>未知</translation>
    </message>
</context>
<context>
    <name>LED</name>
    <message>
        <location filename="widgets/led/led.ui" line="90"/>
        <source>External</source>
        <translation>外部控制</translation>
    </message>
    <message>
        <location filename="widgets/led/led.h" line="54"/>
        <source>Normal</source>
        <translation>正常</translation>
    </message>
    <message>
        <location filename="widgets/led/led.h" line="55"/>
        <source>Inverted</source>
        <translation>相反</translation>
    </message>
    <message>
        <location filename="widgets/led/led.h" line="60"/>
        <source>-</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="widgets/led/led.h" line="61"/>
        <source>Timer 1</source>
        <translation type="unfinished">计时器 1</translation>
    </message>
    <message>
        <location filename="widgets/led/led.h" line="62"/>
        <source>Timer 2</source>
        <translation type="unfinished">计时器 2</translation>
    </message>
    <message>
        <location filename="widgets/led/led.h" line="63"/>
        <source>Timer 3</source>
        <translation type="unfinished">计时器 3</translation>
    </message>
    <message>
        <location filename="widgets/led/led.h" line="64"/>
        <source>Timer 4</source>
        <translation type="unfinished">计时器 4</translation>
    </message>
</context>
<context>
    <name>LedConfig</name>
    <message>
        <location filename="widgets/led/ledconfig.ui" line="38"/>
        <source>PWM LEDs</source>
        <oldsource>PWM</oldsource>
        <translation>PWM LED</translation>
    </message>
    <message>
        <location filename="widgets/led/ledconfig.ui" line="83"/>
        <source>Pin PA8</source>
        <translation></translation>
    </message>
    <message>
        <location filename="widgets/led/ledconfig.ui" line="122"/>
        <location filename="widgets/led/ledconfig.ui" line="198"/>
        <location filename="widgets/led/ledconfig.ui" line="274"/>
        <location filename="widgets/led/ledconfig.ui" line="347"/>
        <source>Connected to axis</source>
        <translation>绑定到轴</translation>
    </message>
    <message>
        <location filename="widgets/led/ledconfig.ui" line="235"/>
        <source>Pin PB0</source>
        <translation></translation>
    </message>
    <message>
        <location filename="widgets/led/ledconfig.ui" line="363"/>
        <source>Mono LEDs</source>
        <translation>单色 LED</translation>
    </message>
    <message>
        <location filename="widgets/led/ledconfig.ui" line="413"/>
        <source>#</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="widgets/led/ledconfig.ui" line="423"/>
        <source>Source</source>
        <translation>主机</translation>
    </message>
    <message>
        <location filename="widgets/led/ledconfig.ui" line="445"/>
        <source>Input #</source>
        <translation>输入 #</translation>
    </message>
    <message>
        <location filename="widgets/led/ledconfig.ui" line="465"/>
        <source>Timer</source>
        <translation>定时器</translation>
    </message>
    <message>
        <location filename="widgets/led/ledconfig.ui" line="583"/>
        <location filename="widgets/led/ledconfig.ui" line="668"/>
        <location filename="widgets/led/ledconfig.ui" line="753"/>
        <location filename="widgets/led/ledconfig.ui" line="838"/>
        <source> ms</source>
        <translation type="unfinished"> 毫秒</translation>
    </message>
    <message>
        <location filename="widgets/led/ledconfig.ui" line="593"/>
        <source>Timer 1</source>
        <translation type="unfinished">计时器 1</translation>
    </message>
    <message>
        <location filename="widgets/led/ledconfig.ui" line="678"/>
        <source>Timer 2</source>
        <translation type="unfinished">计时器 2</translation>
    </message>
    <message>
        <location filename="widgets/led/ledconfig.ui" line="763"/>
        <source>Timer 3</source>
        <translation type="unfinished">计时器 3</translation>
    </message>
    <message>
        <location filename="widgets/led/ledconfig.ui" line="848"/>
        <source>Timer 4</source>
        <translation type="unfinished">计时器 4</translation>
    </message>
    <message>
        <location filename="widgets/led/ledconfig.ui" line="879"/>
        <source>RGB LEDs - pin A10</source>
        <translation>RGB LED — 引脚 A10</translation>
    </message>
    <message>
        <location filename="widgets/led/ledconfig.ui" line="159"/>
        <source>Pin PB1</source>
        <translation></translation>
    </message>
    <message>
        <location filename="widgets/led/ledconfig.ui" line="308"/>
        <source>Pin PB4</source>
        <translation></translation>
    </message>
    <message>
        <location filename="widgets/led/ledconfig.ui" line="455"/>
        <source>Function</source>
        <translation>作用</translation>
    </message>
</context>
<context>
    <name>LedFunction</name>
    <message>
        <location filename="widgets/led_rgb/ledfunction.ui" line="32"/>
        <source>Logical button state</source>
        <translation>逻辑按钮状态</translation>
    </message>
    <message>
        <location filename="widgets/led_rgb/ledfunction.ui" line="50"/>
        <source>Invert state</source>
        <translation>反转状态</translation>
    </message>
    <message>
        <location filename="widgets/led_rgb/ledfunction.ui" line="57"/>
        <source>Logical button #</source>
        <oldsource>Logical button №</oldsource>
        <translation>逻辑按钮 #</translation>
    </message>
</context>
<context>
    <name>LedRGBConfig</name>
    <message>
        <location filename="widgets/led_rgb/ledrgbconfig.ui" line="54"/>
        <source>Effects</source>
        <translation>效果</translation>
    </message>
    <message>
        <location filename="widgets/led_rgb/ledrgbconfig.ui" line="72"/>
        <source>SimHub</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="widgets/led_rgb/ledrgbconfig.ui" line="79"/>
        <source>Static color</source>
        <oldsource>Static Color</oldsource>
        <translation>静态颜色</translation>
    </message>
    <message>
        <location filename="widgets/led_rgb/ledrgbconfig.ui" line="86"/>
        <source>Rainbow</source>
        <translation>彩虹</translation>
    </message>
    <message>
        <location filename="widgets/led_rgb/ledrgbconfig.ui" line="93"/>
        <source>Flow</source>
        <translation>流动</translation>
    </message>
    <message>
        <location filename="widgets/led_rgb/ledrgbconfig.ui" line="102"/>
        <source>LEDs count</source>
        <translation>LED 数量</translation>
    </message>
    <message>
        <location filename="widgets/led_rgb/ledrgbconfig.ui" line="138"/>
        <source>Rainbow brightness</source>
        <oldsource>RB brightness</oldsource>
        <translation>彩虹亮度</translation>
    </message>
    <message>
        <location filename="widgets/led_rgb/ledrgbconfig.ui" line="148"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Rainbow effect brightness&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;彩虹效果亮度&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location filename="widgets/led_rgb/ledrgbconfig.ui" line="174"/>
        <source>Effect interval</source>
        <translation>效果间隔</translation>
    </message>
    <message>
        <location filename="widgets/led_rgb/ledrgbconfig.ui" line="187"/>
        <source> ms</source>
        <translation type="unfinished"> 毫秒</translation>
    </message>
    <message>
        <location filename="widgets/led_rgb/ledrgbconfig.ui" line="208"/>
        <source>Color selector</source>
        <translation>颜色选择器</translation>
    </message>
</context>
<context>
    <name>MainWindow</name>
    <message>
        <location filename="mainwindow.ui" line="427"/>
        <location filename="mainwindow.cpp" line="2408"/>
        <source>Show debug</source>
        <translation>显示调试信息</translation>
    </message>
    <message>
        <location filename="mainwindow.ui" line="56"/>
        <source> Pin Config </source>
        <translation> Pin 配置 </translation>
    </message>
    <message>
        <location filename="mainwindow.ui" line="75"/>
        <source> Button Config </source>
        <translation> 按钮配置 </translation>
    </message>
    <message>
        <location filename="mainwindow.ui" line="94"/>
        <source> Shifts &amp;&amp; Timers </source>
        <translation> Shift &amp;&amp; 定时器 </translation>
    </message>
    <message>
        <location filename="mainwindow.ui" line="113"/>
        <source> Axes Config </source>
        <oldsource> Axis Config </oldsource>
        <translation> 轴配置 </translation>
    </message>
    <message>
        <location filename="mainwindow.ui" line="135"/>
        <source> Axes Curves </source>
        <oldsource> Axis Curves </oldsource>
        <translation> 轴曲线 </translation>
    </message>
    <message>
        <location filename="mainwindow.ui" line="157"/>
        <source> Shift Registers </source>
        <translation> 移位寄存器 </translation>
    </message>
    <message>
        <location filename="mainwindow.ui" line="179"/>
        <source> Encoders </source>
        <translation> 编码器 </translation>
    </message>
    <message>
        <location filename="mainwindow.ui" line="317"/>
        <source>Version</source>
        <translation>版本</translation>
    </message>
    <message>
        <location filename="mainwindow.ui" line="317"/>
        <location filename="mainwindow.ui" line="318"/>
        <source>The device&apos;s firmware version, reported live by the firmware. FreeJoyX devices show their FREEJOYX_VERSION; upstream FreeJoy devices show their nibble-encoded version (e.g. v1.7.8).</source>
        <translation>设备的固件版本，由固件实时报告。FreeJoyX 设备显示其 FREEJOYX_VERSION；上游 FreeJoy 设备显示其半字节编码的版本（例如 v1.7.8）。</translation>
    </message>
    <message>
        <location filename="mainwindow.ui" line="319"/>
        <source>VID:PID</source>
        <translation>VID:PID</translation>
    </message>
    <message>
        <location filename="mainwindow.ui" line="321"/>
        <source>Serial</source>
        <translation>序列号</translation>
    </message>
    <message>
        <location filename="mainwindow.ui" line="323"/>
        <source>Board</source>
        <translation>开发板</translation>
    </message>
    <message>
        <location filename="mainwindow.ui" line="346"/>
        <source>Upgrade firmware</source>
        <translation>升级固件</translation>
    </message>
    <message>
        <location filename="mainwindow.ui" line="347"/>
        <source>Read your current config, flash the latest matching firmware, then write the config back. One click.</source>
        <translation>读取您的当前配置，刷写最新的匹配固件，然后写回配置。一键完成。</translation>
    </message>
    <message>
        <location filename="mainwindow.ui" line="355"/>
        <source>Config</source>
        <translation>配置</translation>
    </message>
    <message>
        <location filename="mainwindow.ui" line="368"/>
        <source>Open the configs folder in your file manager</source>
        <translation>在文件管理器中打开配置文件夹</translation>
    </message>
    <message>
        <location filename="mainwindow.ui" line="390"/>
        <source>Reset All Settings</source>
        <translation>重置所有设置</translation>
    </message>
    <message>
        <location filename="mainwindow.ui" line="391"/>
        <source>Resets every setting (pins, axes, buttons, encoders, sensors, USB identity, gestures, logic, LEDs, shifts &amp; timers) to factory defaults. The change is in-memory only — click Write Config to apply it to the connected device.</source>
        <translation>将每项设置（引脚、轴、按钮、编码器、传感器、USB 标识、手势、逻辑、LED、Shift 和定时器）重置为出厂默认值。该更改仅在内存中 — 点击“写入配置”以将其应用到已连接的设备。</translation>
    </message>
    <message>
        <location filename="mainwindow.ui" line="400"/>
        <source>App</source>
        <translation>应用</translation>
    </message>
    <message>
        <location filename="mainwindow.ui" line="419"/>
        <source>Firmware</source>
        <translation>固件</translation>
    </message>
    <message>
        <location filename="mainwindow.ui" line="201"/>
        <source> LED/PWM </source>
        <oldsource> LED </oldsource>
        <translation> LED/PWM </translation>
    </message>
    <message>
        <location filename="mainwindow.ui" line="223"/>
        <source> Advanced Settings </source>
        <translation> 高级设置 </translation>
    </message>
    <message>
        <location filename="mainwindow.ui" line="245"/>
        <source> Debug </source>
        <translation></translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="447"/>
        <location filename="mainwindow.cpp" line="695"/>
        <location filename="mainwindow.cpp" line="1379"/>
        <source>Connected</source>
        <translation>已连接</translation>
    </message>
    <message>
        <location filename="mainwindow.ui" line="286"/>
        <source>Device</source>
        <translation>设备</translation>
    </message>
    <message>
        <location filename="mainwindow.ui" line="384"/>
        <source>Save config to file</source>
        <translation>将配置保存到文件</translation>
    </message>
    <message>
        <location filename="mainwindow.ui" line="332"/>
        <source>Read config from Device</source>
        <translation>从设备中读取配置</translation>
    </message>
    <message>
        <location filename="mainwindow.ui" line="270"/>
        <source>2</source>
        <translation>2</translation>
    </message>
    <message>
        <location filename="mainwindow.ui" line="378"/>
        <source>Load config from file</source>
        <translation>从文件中加载配置</translation>
    </message>
    <message>
        <location filename="mainwindow.ui" line="339"/>
        <source>Write config to Device</source>
        <translation>将配置写入设备</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="421"/>
        <source>Disconnected</source>
        <translation>已断开连接</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="687"/>
        <source>Incompatible Firmware</source>
        <translation>固件不兼容</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="89"/>
        <source>%1 Configurator %2</source>
        <translation>%1 Configurator %2</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="180"/>
        <source>— select device —</source>
        <translation>— 选择设备 —</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="418"/>
        <source>Restarting...</source>
        <translation>正在重启……</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="600"/>
        <source>Unknown (%1)</source>
        <translation>未知 (%1)</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="638"/>
        <source>LEDs are not yet supported on Black Pill (F411). Coming in a future update.</source>
        <translation>Black Pill (F411) 尚不支持 LED。将在未来更新中加入。</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="682"/>
        <source>Legacy</source>
        <translation>旧版</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="775"/>
        <source>No device detected</source>
        <translation>未检测到设备</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="776"/>
        <source>Connect a FreeJoy device before using the Flash button. If the device is stuck in DFU mode without enumerating, recover via STM32 Cube Programmer + ST-Link.</source>
        <translation>使用刷写按钮前请先连接 FreeJoy 设备。如果设备卡在 DFU 模式且无法枚举，请通过 STM32 Cube Programmer + ST-Link 恢复。</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="967"/>
        <source>WARNING: device reports firmware v0x%1 but the flashed binary&apos;s footer says v0x%2. Re-flash recommended.</source>
        <translation>警告：设备报告固件 v0x%1，但刷写的二进制文件页脚显示 v0x%2。建议重新刷写。</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="982"/>
        <source>Backup saved to %1</source>
        <translation>备份已保存到 %1</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="1187"/>
        <source>Pending changes. The device still runs its previously-flashed config; the live press preview reflects that, not your edits. Click to write.</source>
        <translation>有待写入的更改。设备仍运行其先前刷写的配置；实时按键预览反映的是该配置，而非您的编辑。点击以写入。</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="1258"/>
        <source>&lt;p&gt;This device is running upstream FreeJoy firmware (%1).&lt;/p&gt;&lt;p&gt;The configurator has read its config and translated it into the current shape. Your existing pin assignments, axes, buttons, shift registers, encoders and LED settings are preserved. New-since-then features (logical buttons, gestures, RGB) carry default values.&lt;/p&gt;&lt;p&gt;To finish upgrading the device:&lt;/p&gt;&lt;ol&gt;&lt;li&gt;Review the imported config in the tabs above. &lt;b&gt;Save it to a file&lt;/b&gt; as a backup.&lt;/li&gt;&lt;li&gt;Flash %2 firmware via &lt;i&gt;Advanced Settings &amp;rarr; Firmware flasher&lt;/i&gt;.&lt;/li&gt;&lt;li&gt;After the device reconnects, click &lt;b&gt;Write config to device&lt;/b&gt; to push the migrated config.&lt;/li&gt;&lt;/ol&gt;</source>
        <translation>&lt;p&gt;此设备运行上游 FreeJoy 固件（%1）。&lt;/p&gt;&lt;p&gt;配置器已读取其配置并转换为当前形态。您现有的引脚分配、轴、按钮、移位寄存器、编码器和 LED 设置均已保留。此后新增的功能（逻辑按钮、手势、RGB）采用默认值。&lt;/p&gt;&lt;p&gt;要完成设备升级：&lt;/p&gt;&lt;ol&gt;&lt;li&gt;在上方各选项卡中检查导入的配置。&lt;b&gt;将其保存为文件&lt;/b&gt;作为备份。&lt;/li&gt;&lt;li&gt;通过&lt;i&gt;高级设置 &amp;rarr; 固件刷写器&lt;/i&gt;刷写 %2 固件。&lt;/li&gt;&lt;li&gt;设备重新连接后，点击&lt;b&gt;将配置写入设备&lt;/b&gt;以推送已迁移的配置。&lt;/li&gt;&lt;/ol&gt;</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="1275"/>
        <source>Legacy config imported</source>
        <translation>已导入旧版配置</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="1308"/>
        <source>Backup failed</source>
        <translation>备份失败</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="1309"/>
        <source>&lt;p&gt;Could not read the device&apos;s current config. Proceeding without a backup means a failed flash could leave the device with default settings (you&apos;d lose your current mappings).&lt;/p&gt;&lt;p&gt;Continue with flash anyway?&lt;/p&gt;</source>
        <translation>&lt;p&gt;无法读取设备的当前配置。在没有备份的情况下继续意味着刷写失败可能使设备保持默认设置（您将丢失当前的映射）。&lt;/p&gt;&lt;p&gt;仍要继续刷写吗？&lt;/p&gt;</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="1334"/>
        <source>Backup OK, writing...</source>
        <translation>备份完成，正在写入……</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="1337"/>
        <source>Pre-write backup failed</source>
        <translation>写入前备份失败</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="1338"/>
        <source>&lt;p&gt;Could not read the device&apos;s current config to back it up before writing.&lt;/p&gt;&lt;p&gt;Continue with the write anyway? If the new config has issues, you&apos;ll have no automatic rollback path -- only configs you previously saved manually.&lt;/p&gt;</source>
        <translation>&lt;p&gt;写入前无法读取设备的当前配置以进行备份。&lt;/p&gt;&lt;p&gt;仍要继续写入吗？如果新配置有问题，将没有自动回滚途径 — 只有您先前手动保存的配置。&lt;/p&gt;</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="1381"/>
        <source>Received</source>
        <translation>已接收</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="1391"/>
        <location filename="mainwindow.cpp" line="1433"/>
        <source>Error</source>
        <translation></translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="1422"/>
        <source>Sent</source>
        <translation>已发送</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="1513"/>
        <source>Disabled because the connected device runs an unsupported firmware version. Flash a known-good build via Advanced Settings → Firmware flasher to regain access.</source>
        <translation>已禁用，因为已连接的设备运行不受支持的固件版本。请通过 高级设置 → 固件刷写器 刷写一个已知良好的构建以恢复访问。</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="1681"/>
        <source>Reset all settings to defaults?</source>
        <translation>将所有设置重置为默认值？</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="1682"/>
        <source>&lt;p&gt;This resets every setting in the configurator -- pins, axes, buttons, encoders, sensors, USB identity, gestures, logic, LEDs, shifts &amp;amp; timers -- to factory defaults.&lt;/p&gt;&lt;p&gt;The change is &lt;b&gt;in-memory only&lt;/b&gt;. The connected device keeps its current settings until you click &lt;b&gt;Write Config&lt;/b&gt;.&lt;/p&gt;&lt;p&gt;Continue?&lt;/p&gt;</source>
        <translation>&lt;p&gt;这会将配置器中的每项设置 — 引脚、轴、按钮、编码器、传感器、USB 标识、手势、逻辑、LED、Shift &amp;amp; 定时器 — 重置为出厂默认值。&lt;/p&gt;&lt;p&gt;该更改&lt;b&gt;仅在内存中&lt;/b&gt;。在您点击&lt;b&gt;写入配置&lt;/b&gt;之前，已连接的设备会保持其当前设置。&lt;/p&gt;&lt;p&gt;是否继续？&lt;/p&gt;</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="1724"/>
        <source>Incomplete Logic Configuration</source>
        <translation>逻辑配置不完整</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="1725"/>
        <source>Logical button %1 has Function = Logic but is missing an operator or Source B. Pick an operator (and Source B for binary operators) before saving.</source>
        <translation>逻辑按钮 %1 的功能 = 逻辑，但缺少运算符或源 B。保存前请选择运算符（二元运算符还需选择源 B）。</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="1766"/>
        <source>Backing up...</source>
        <translation>正在备份……</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="1879"/>
        <source>No device connected</source>
        <translation>未连接设备</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="1880"/>
        <source>Connect a device before starting an upgrade.</source>
        <translation>开始升级前请先连接设备。</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="1898"/>
        <source>No firmware available</source>
        <translation>没有可用的固件</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="1899"/>
        <source>Couldn&apos;t find a matching firmware binary in the configurator&apos;s firmware/ folder. Use Advanced Settings -&gt; Firmware flasher to flash manually.</source>
        <translation>在配置器的 firmware/ 文件夹中找不到匹配的固件二进制文件。请使用 高级设置 -&gt; 固件刷写器 手动刷写。</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="1929"/>
        <source>Upgrade firmware?</source>
        <translation>升级固件？</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="1930"/>
        <source>&lt;p&gt;This will:&lt;/p&gt;&lt;ol&gt;&lt;li&gt;Read your current config and save a backup file&lt;/li&gt;&lt;li&gt;Flash &lt;b&gt;%1&lt;/b&gt; to the device&lt;/li&gt;&lt;li&gt;Write your migrated config back after the device reconnects&lt;/li&gt;&lt;/ol&gt;&lt;p&gt;Current firmware: &lt;b&gt;%2&lt;/b&gt;&lt;br&gt;Target firmware: &lt;b&gt;%3&lt;/b&gt;&lt;/p&gt;%4&lt;p&gt;If anything fails mid-flight the device may be left in DFU mode -- recover via STM32 Cube Programmer + ST-Link.&lt;/p&gt;&lt;p&gt;Continue?&lt;/p&gt;</source>
        <translation>&lt;p&gt;此操作将：&lt;/p&gt;&lt;ol&gt;&lt;li&gt;读取您的当前配置并保存备份文件&lt;/li&gt;&lt;li&gt;将 &lt;b&gt;%1&lt;/b&gt; 刷写到设备&lt;/li&gt;&lt;li&gt;在设备重新连接后写回您已迁移的配置&lt;/li&gt;&lt;/ol&gt;&lt;p&gt;当前固件：&lt;b&gt;%2&lt;/b&gt;&lt;br&gt;目标固件：&lt;b&gt;%3&lt;/b&gt;&lt;/p&gt;%4&lt;p&gt;如果中途出现任何失败，设备可能停留在 DFU 模式 — 请通过 STM32 Cube Programmer + ST-Link 恢复。&lt;/p&gt;&lt;p&gt;是否继续？&lt;/p&gt;</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="1975"/>
        <location filename="mainwindow.cpp" line="2127"/>
        <source>(unnamed)</source>
        <translation>(未命名)</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="1986"/>
        <source>VID:PID already in use</source>
        <translation>VID:PID 已被占用</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="1987"/>
        <source>&lt;p&gt;VID &lt;b&gt;%1&lt;/b&gt;:PID &lt;b&gt;%2&lt;/b&gt; is currently used by: &lt;b&gt;%3&lt;/b&gt;.&lt;/p&gt;&lt;p&gt;Writing this config will give two devices the same USB identity. Windows&apos; OEMName cache is keyed by VID+PID -- both devices will share one OEM name -- and games using DirectInput may pick a random one or conflate them.&lt;/p&gt;&lt;p&gt;Continue with the write anyway?&lt;/p&gt;</source>
        <translation>&lt;p&gt;VID &lt;b&gt;%1&lt;/b&gt;:PID &lt;b&gt;%2&lt;/b&gt; 当前被占用：&lt;b&gt;%3&lt;/b&gt;。&lt;/p&gt;&lt;p&gt;写入此配置会使两个设备具有相同的 USB 标识。Windows 的 OEMName 缓存以 VID+PID 为键 — 两个设备将共用一个 OEM 名称 — 使用 DirectInput 的游戏可能会随机选择其一或将它们混淆。&lt;/p&gt;&lt;p&gt;仍要继续写入吗？&lt;/p&gt;</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="2024"/>
        <source>Cannot write to this firmware version</source>
        <translation>无法写入此固件版本</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="2025"/>
        <source>&lt;p&gt;The connected device runs &lt;b&gt;%1&lt;/b&gt;, which this configurator doesn&apos;t have a reverse migrator for.&lt;/p&gt;&lt;p&gt;To write a config, flash a current FreeJoyX firmware first (Advanced Settings -&gt; Firmware flasher).&lt;/p&gt;</source>
        <translation>&lt;p&gt;已连接的设备运行 &lt;b&gt;%1&lt;/b&gt;，此配置器没有适用于它的反向迁移器。&lt;/p&gt;&lt;p&gt;要写入配置，请先刷写当前的 FreeJoyX 固件（高级设置 -&gt; 固件刷写器）。&lt;/p&gt;</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="2038"/>
        <source>Reverse migration failed</source>
        <translation>反向迁移失败</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="2039"/>
        <source>&lt;p&gt;Couldn&apos;t pack the current config into the %1 wire format. The device wasn&apos;t written to.&lt;/p&gt;</source>
        <translation>&lt;p&gt;无法将当前配置打包为 %1 传输格式。未写入设备。&lt;/p&gt;</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="2052"/>
        <source>Write to %1 firmware?</source>
        <translation>写入 %1 固件？</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="2053"/>
        <source>&lt;p&gt;Writing to %1 firmware will lose the following:&lt;/p&gt;%2&lt;p&gt;The configurator will keep its in-memory copy unchanged -- only the device will see the reduced config. Continue?&lt;/p&gt;</source>
        <translation>&lt;p&gt;写入 %1 固件将丢失以下内容：&lt;/p&gt;%2&lt;p&gt;配置器会保持其内存副本不变 — 只有设备会看到精简后的配置。是否继续？&lt;/p&gt;</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="2118"/>
        <source>No FreeJoy devices detected.</source>
        <translation>未检测到 FreeJoy 设备。</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="2133"/>
        <source>&amp;#9658; marks the device currently selected in the dropdown.</source>
        <translation>&amp;#9658; 标记当前在下拉框中选定的设备。</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="2137"/>
        <source>Connected FreeJoy devices</source>
        <translation>已连接的 FreeJoy 设备</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="2233"/>
        <location filename="mainwindow.cpp" line="2237"/>
        <source>Pin %1 currently: &lt;b&gt;%2&lt;/b&gt;</source>
        <translation>引脚 %1 当前：&lt;b&gt;%2&lt;/b&gt;</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="2242"/>
        <source>Reassign pins to Fast Encoder %1?</source>
        <translation>将引脚重新分配给快速编码器 %1？</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="2243"/>
        <source>&lt;p&gt;Enabling Fast Encoder %1 needs both encoder pins free. The following pin%2 currently held other role%2:&lt;/p&gt;&lt;p&gt;%3&lt;/p&gt;&lt;p&gt;Replace with FAST_ENCODER?&lt;/p&gt;</source>
        <translation>&lt;p&gt;启用快速编码器 %1 需要两个编码器引脚都空闲。以下引脚%2 当前担任其他角色%2：&lt;/p&gt;&lt;p&gt;%3&lt;/p&gt;&lt;p&gt;是否替换为 FAST_ENCODER？&lt;/p&gt;</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="2262"/>
        <source>Fast Encoder %1 unavailable</source>
        <translation>快速编码器 %1 不可用</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="2263"/>
        <source>This board doesn&apos;t expose FAST_ENCODER as a legal role on at least one of the required pins. The encoder wasn&apos;t enabled.</source>
        <translation>此开发板在至少一个所需引脚上未将 FAST_ENCODER 作为合法角色提供。编码器未启用。</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="2331"/>
        <source>Open Config</source>
        <translation>打开配置</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="2331"/>
        <location filename="mainwindow.cpp" line="2351"/>
        <source>Config Files (*.cfg)</source>
        <translation>配置文件 (*.cfg)</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="2350"/>
        <source>Save Config</source>
        <translation>保存配置</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="1576"/>
        <location filename="mainwindow.cpp" line="2400"/>
        <source>Hide debug</source>
        <translation>隐藏调试信息</translation>
    </message>
</context>
<context>
    <name>PinComboBox</name>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="131"/>
        <source>Pin A0</source>
        <translation>A0 引脚</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="132"/>
        <source>Pin A1</source>
        <translation>A1 引脚</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="133"/>
        <source>Pin A2</source>
        <translation>A2 引脚</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="134"/>
        <source>Pin A3</source>
        <translation>A3 引脚</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="135"/>
        <source>Pin A4</source>
        <translation>A4 引脚</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="136"/>
        <source>Pin A5</source>
        <translation>A5 引脚</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="137"/>
        <source>Pin A6</source>
        <translation>A6 引脚</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="138"/>
        <source>Pin A7</source>
        <translation>A7 引脚</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="139"/>
        <source>Pin A8</source>
        <translation>A8 引脚</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="140"/>
        <source>Pin A9</source>
        <translation>A9 引脚</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="141"/>
        <source>Pin A10</source>
        <translation>A10 引脚</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="142"/>
        <source>Pin A15</source>
        <translation>A15 引脚</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="143"/>
        <source>Pin B0</source>
        <translation>B0 引脚</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="144"/>
        <source>Pin B1</source>
        <translation>B1 引脚</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="145"/>
        <source>Pin B3</source>
        <translation>B3 引脚</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="146"/>
        <source>Pin B4</source>
        <translation>B4 引脚</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="147"/>
        <source>Pin B5</source>
        <translation>B5 引脚</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="148"/>
        <source>Pin B6</source>
        <translation>B6 引脚</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="149"/>
        <source>Pin B7</source>
        <translation>B7 引脚</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="150"/>
        <source>Pin B8</source>
        <translation>B8 引脚</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="151"/>
        <source>Pin B9</source>
        <translation>B9 引脚</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="152"/>
        <source>Pin B10</source>
        <translation>B10 引脚</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="153"/>
        <source>Pin B11</source>
        <translation>B11 引脚</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="154"/>
        <source>Pin B12</source>
        <translation>B12 引脚</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="155"/>
        <source>Pin B13</source>
        <translation>B13 引脚</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="156"/>
        <source>Pin B14</source>
        <translation>B14 引脚</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="157"/>
        <source>Pin B15</source>
        <translation>B15 引脚</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="158"/>
        <source>Pin C13</source>
        <translation>C13 引脚</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="159"/>
        <source>Pin C14</source>
        <translation>C14 引脚</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="160"/>
        <source>Pin C15</source>
        <translation>C15 引脚</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="165"/>
        <source>Not Used</source>
        <translation>未使用</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="170"/>
        <source>Button to Gnd</source>
        <translation>按钮接 GND</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="175"/>
        <source>Button to Vcc</source>
        <translation>按钮接 Vcc</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="320"/>
        <source>UART TX</source>
        <translation>UART TX</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="180"/>
        <source>Button Row</source>
        <translation>按钮 行</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="185"/>
        <source>Button Column</source>
        <translation>按钮 列</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="190"/>
        <source>ShiftReg LATCH</source>
        <translation>移位寄存器 LATCH引脚</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="195"/>
        <source>ShiftReg DATA</source>
        <translation>移位寄存器 DATA引脚</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="200"/>
        <source>ShiftReg CLK</source>
        <translation>移位寄存器 CLK引脚</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="205"/>
        <source>TLE5011 CS</source>
        <translation>TLE5011 CS引脚</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="210"/>
        <source>TLE5012B CS</source>
        <translation>TLE5012B CS引脚</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="215"/>
        <source>MCP3201 CS</source>
        <translation>MCP3201 CS引脚</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="220"/>
        <source>MCP3202 CS</source>
        <translation>MCP3202 CS引脚</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="225"/>
        <source>MCP3204 CS</source>
        <translation>MCP3204 CS引脚</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="230"/>
        <source>MCP3208 CS</source>
        <translation>MCP3208 CS引脚</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="235"/>
        <source>MLX90393 CS</source>
        <translation>MLX90393 CS引脚</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="240"/>
        <source>MLX90363 CS</source>
        <translation>MLX90363 CS引脚</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="245"/>
        <source>AS5048A CS</source>
        <translation>AS5048A CS引脚</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="250"/>
        <source>LED Single</source>
        <translation>单独LED</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="255"/>
        <source>LED Row</source>
        <translation>LED 行</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="260"/>
        <source>LED Column</source>
        <translation>LED 列</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="265"/>
        <source>LED PWM</source>
        <translation>LED PWM引脚</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="270"/>
        <source>LED WS2812b</source>
        <translation>LED WS2812b</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="275"/>
        <source>LED PL9823</source>
        <translation>LED PL9823</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="280"/>
        <source>Axis Analog</source>
        <translation>模拟轴</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="285"/>
        <source>Fast Encoder</source>
        <translation>高速编码器</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="290"/>
        <source>SPI SCK</source>
        <translation>SPI SCK引脚</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="295"/>
        <source>SPI MOSI</source>
        <translation>SPI MOSI引脚</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="300"/>
        <source>SPI MISO</source>
        <translation>SPI MISO引脚</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="305"/>
        <source>TLE5011 GEN</source>
        <translation>TLE5011 GEN引脚</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="310"/>
        <source>I2C SCL</source>
        <translation>I2C SCL引脚</translation>
    </message>
    <message>
        <location filename="widgets/pins/pincombobox.h" line="315"/>
        <source>I2C SDA</source>
        <translation>I2C SDA引脚</translation>
    </message>
</context>
<context>
    <name>PinConfig</name>
    <message>
        <location filename="widgets/pins/pinconfig.ui" line="56"/>
        <source>Select board</source>
        <translation>开发板选择</translation>
    </message>
</context>
<context>
    <name>PinTypeHelper</name>
    <message>
        <location filename="widgets/pins/pintypehelper.ui" line="32"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt;Hover any row below for wiring details. &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700;&quot;&gt;Be sure&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt; to read the &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700;&quot;&gt;Wiki&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt; for details&lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700;&quot;&gt;!!&lt;/span&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <oldsource>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt;Brief information. &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700;&quot;&gt;Be sure&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt; to read the &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700;&quot;&gt;Wiki&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt; for details&lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700;&quot;&gt;!!&lt;/span&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</oldsource>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt;将鼠标悬停在下方任意行上以查看接线详情。&lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700;&quot;&gt;务必&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt;阅读 &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700;&quot;&gt;Wiki&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt; 了解详情&lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700;&quot;&gt;！！&lt;/span&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location filename="widgets/pins/pintypehelper.ui" line="35"/>
        <source>Pin Info</source>
        <translation>引脚信息</translation>
    </message>
    <message>
        <location filename="widgets/pins/pintypehelper.ui" line="56"/>
        <source>Hover any row for wiring details</source>
        <translation>将鼠标悬停在任意行上以查看接线详情</translation>
    </message>
    <message>
        <location filename="widgets/pins/pintypehelper.ui" line="66"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt;You can connect buttons, toggle switches, encoders, both directly &amp;quot;&lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700; color:#1982f0;&quot;&gt;Button to GND&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt;/&lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700; color:#aaaa00;&quot;&gt;Vcc&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt;&amp;quot;, and combining them into a matrix &amp;quot;&lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700; color:#7882fa;&quot;&gt;Button Row/Column&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt;&amp;quot;. When connected to &amp;quot;&lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700; color:#1982f0;&quot;&gt;Button to GND&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt;&amp;quot; the second contact of the button is connected to &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700; color:#010101;&quot;&gt;GND&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt;, with &amp;quot;&lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700; color:#aaaa00;&quot;&gt;Button Vcc&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt;&amp;quot; to 3.3v.&lt;/span&gt;&lt;/p&gt;&lt;p&gt;&lt;img src=&quot;:/Images/buttonEncToggle.png&quot;/&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;您可以连接按钮、拨动开关、编码器，既可直接连接 &amp;quot;&lt;span style=&quot; font-size:11pt; font-weight:700; color:#1982f0;&quot;&gt;Button to GND&lt;/span&gt;/&lt;span style=&quot; font-size:11pt; font-weight:700; color:#aaaa00;&quot;&gt;Vcc&lt;/span&gt;&amp;quot;，也可将它们组合成矩阵 &amp;quot;&lt;span style=&quot; font-size:11pt; font-weight:700; color:#7882fa;&quot;&gt;Button Row/Column&lt;/span&gt;&amp;quot;。连接到 &amp;quot;&lt;span style=&quot; font-size:11pt; font-weight:700; color:#1982f0;&quot;&gt;Button to GND&lt;/span&gt;&amp;quot; 时，按钮的第二个触点接 &lt;span style=&quot; font-size:11pt; font-weight:700; color:#010101;&quot;&gt;GND&lt;/span&gt;；连接 &amp;quot;&lt;span style=&quot; font-size:11pt; font-weight:700; color:#aaaa00;&quot;&gt;Button Vcc&lt;/span&gt;&amp;quot; 时接 3.3V。&lt;/p&gt;&lt;p&gt;&lt;img src=&quot;:/Images/buttonEncToggle.png&quot;/&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location filename="widgets/pins/pintypehelper.ui" line="192"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Fast Encoder&lt;img src=&quot;:/Images/icons/lucide/info.svg&quot; width=&quot;10&quot; align=&quot;right&quot;/&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;快速编码器&lt;img src=&quot;:/Images/icons/lucide/info.svg&quot; width=&quot;10&quot; align=&quot;right&quot;/&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location filename="widgets/pins/pintypehelper.ui" line="79"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt;Shift registers &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700;&quot;&gt;74HC165&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt; and &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700;&quot;&gt;CD4021&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt; are supported. To connect, use the table below on the selected pins, and then set the number of buttons in the &amp;quot;Registers&amp;quot; tab.&lt;/span&gt;&lt;/p&gt;&lt;p&gt;&lt;span style=&quot; font-size:11pt; font-weight:700; color:#69b437;&quot;&gt;ShiftReg_CLK&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt; and &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700; color:#69b437;&quot;&gt;ShiftReg_LATCH&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt; - can be common to each shift register chain, or common to all shift register chains.&lt;/span&gt;&lt;/p&gt;&lt;p&gt;&lt;span style=&quot; font-size:11pt; font-weight:700; color:#69b437;&quot;&gt;ShiftReg_DATA&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt; - individually for each chain of shift registers.&lt;br/&gt;&lt;/span&gt;&lt;/p&gt;&lt;table border=&quot;0&quot; style=&quot; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; border-collapse:collapse;&quot; cellspacing=&quot;2&quot; cellpadding=&quot;0&quot; bgcolor=&quot;#22272e&quot;&gt;&lt;thead&gt;&lt;tr&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p align=&quot;center&quot;&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt; font-weight:700;&quot;&gt;Name in configurator&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p align=&quot;center&quot;&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt; font-weight:700;&quot;&gt;74HC165 pin&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p align=&quot;center&quot;&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt; font-weight:700;&quot;&gt;CD4021 pin&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p align=&quot;center&quot;&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt; font-weight:700;&quot;&gt;Alternative pin names&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;/tr&gt;&lt;/thead&gt;&lt;tr&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt; color:#69b437;&quot;&gt;ShiftReg_CLK&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;CLK&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;CLOCK&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;SCK&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;/tr&gt;&lt;tr&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt; color:#69b437;&quot;&gt;ShiftReg_DATA&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;Qh&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;Q8&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;DATA/SERIAL_OUT/OUT&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;/tr&gt;&lt;tr&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt; color:#69b437;&quot;&gt;ShiftReg_LATCH&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;SH/LD&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;PAR/SER CONTROL&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;LATCH&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;/tr&gt;&lt;tr&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt; color:#ff7f27;&quot;&gt;3.3V&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;VCC&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;VDD&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;5V/V+&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;/tr&gt;&lt;tr&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt; color:#000000;&quot;&gt;GND&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;GND&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;VSS&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;V-&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;/tr&gt;&lt;/table&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt;支持移位寄存器 &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700;&quot;&gt;74HC165&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt; and &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700;&quot;&gt;CD4021&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt;。要连接，请按下表在所选引脚上接线，然后在 &amp;quot;Registers&amp;quot; 选项卡中设置按钮数量。&lt;/span&gt;&lt;/p&gt;&lt;p&gt;&lt;span style=&quot; font-size:11pt; font-weight:700; color:#69b437;&quot;&gt;ShiftReg_CLK&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt; and &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700; color:#69b437;&quot;&gt;ShiftReg_LATCH&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt; - 可以为每条移位寄存器链共用，也可以为所有链共用。&lt;/span&gt;&lt;/p&gt;&lt;p&gt;&lt;span style=&quot; font-size:11pt; font-weight:700; color:#69b437;&quot;&gt;ShiftReg_DATA&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt; - 每条移位寄存器链单独设置。&lt;br/&gt;&lt;/span&gt;&lt;/p&gt;&lt;table border=&quot;0&quot; style=&quot; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; border-collapse:collapse;&quot; cellspacing=&quot;2&quot; cellpadding=&quot;0&quot; bgcolor=&quot;#22272e&quot;&gt;&lt;thead&gt;&lt;tr&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p align=&quot;center&quot;&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt; font-weight:700;&quot;&gt;配置器中的名称&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p align=&quot;center&quot;&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt; font-weight:700;&quot;&gt;74HC165 引脚&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p align=&quot;center&quot;&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt; font-weight:700;&quot;&gt;CD4021 引脚&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p align=&quot;center&quot;&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt; font-weight:700;&quot;&gt;其他引脚名称&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;/tr&gt;&lt;/thead&gt;&lt;tr&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt; color:#69b437;&quot;&gt;ShiftReg_CLK&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;CLK&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;CLOCK&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;SCK&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;/tr&gt;&lt;tr&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt; color:#69b437;&quot;&gt;ShiftReg_DATA&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;Qh&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;Q8&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;DATA/SERIAL_OUT/OUT&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;/tr&gt;&lt;tr&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt; color:#69b437;&quot;&gt;ShiftReg_LATCH&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;SH/LD&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;PAR/SER CONTROL&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;LATCH&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;/tr&gt;&lt;tr&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt; color:#ff7f27;&quot;&gt;3.3V&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;VCC&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;VDD&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;5V/V+&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;/tr&gt;&lt;tr&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt; color:#000000;&quot;&gt;GND&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;GND&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;VSS&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;V-&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;/tr&gt;&lt;/table&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location filename="widgets/pins/pintypehelper.ui" line="72"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Buttons, encoders, etc &lt;img src=&quot;:/Images/icons/lucide/info.svg&quot; width=&quot;10&quot; align=&quot;right&quot;/&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <oldsource>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Shift registers&lt;img src=&quot;:/Images/info_icon.png&quot; width=&quot;10&quot; align=&quot;right&quot;/&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</oldsource>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;按钮、编码器等 &lt;img src=&quot;:/Images/icons/lucide/info.svg&quot; width=&quot;10&quot; align=&quot;right&quot;/&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location filename="widgets/pins/pintypehelper.ui" line="92"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;&lt;span style=&quot; font-size:11pt; font-weight:700;&quot;&gt;Potentiometer&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt; and &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700;&quot;&gt;Hall sensor&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt; can be connected directly to the &amp;quot;&lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700; color:#00a000;&quot;&gt;Axis Analog&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt;&amp;quot; pin.&lt;/span&gt;&lt;/p&gt;&lt;p&gt;&lt;img src=&quot;:/Images/potentiometer.png&quot; width=&quot;240&quot;/&gt;&lt;img src=&quot;:/Images/hall2.png&quot; width=&quot;240&quot;/&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;&lt;span style=&quot; font-size:11pt; font-weight:700;&quot;&gt;电位器&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt;和&lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700;&quot;&gt;霍尔传感器&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt;可直接连接到 &amp;quot;&lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700; color:#00a000;&quot;&gt;Axis Analog&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt;&amp;quot; 引脚。&lt;/span&gt;&lt;/p&gt;&lt;p&gt;&lt;img src=&quot;:/Images/potentiometer.png&quot; width=&quot;240&quot;/&gt;&lt;img src=&quot;:/Images/hall2.png&quot; width=&quot;240&quot;/&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location filename="widgets/pins/pintypehelper.ui" line="85"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Shift registers&lt;img src=&quot;:/Images/icons/lucide/info.svg&quot; width=&quot;10&quot; align=&quot;right&quot;/&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <oldsource>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Analog Axis&lt;img src=&quot;:/Images/info_icon.png&quot; width=&quot;10&quot; align=&quot;right&quot;/&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</oldsource>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;移位寄存器&lt;img src=&quot;:/Images/icons/lucide/info.svg&quot; width=&quot;10&quot; align=&quot;right&quot;/&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location filename="widgets/pins/pintypehelper.ui" line="105"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt;It is possible to connect &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700;&quot;&gt;Hall sensors &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700; color:#329978;&quot;&gt;TLE5011&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt;, &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700; color:#329978;&quot;&gt;TLE5010&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt;, &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700; color:#329978;&quot;&gt;TLE5012B&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt;, &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700; color:#329978;&quot;&gt;MLX90393&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt;, &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700; color:#329978;&quot;&gt;MLX90363&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt;, &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700; color:#329978;&quot;&gt;AS5048A&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt; and &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700;&quot;&gt;analog-to-digital converters &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700; color:#329978;&quot;&gt;MCP32XX&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt; to &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700; color:#329978;&quot;&gt;SPI&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt;. See the &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700;&quot;&gt;Wiki&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt; for details.&lt;/span&gt;&lt;/p&gt;&lt;p&gt;&lt;img src=&quot;:/Images/MLX90393.png&quot;/&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <oldsource>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt;It is possible to connect &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700;&quot;&gt;Hall sensors&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;/&gt;&lt;span style=&quot; font-size:11pt; font-weight:700; color:#329978;&quot;&gt;TLE5011&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt;, &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700; color:#329978;&quot;&gt;TLE5010&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt;, &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700; color:#329978;&quot;&gt;TLE5012B&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt;, &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700; color:#329978;&quot;&gt;MLX90393&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt;, &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700; color:#329978;&quot;&gt;MLX90363&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt;, &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700; color:#329978;&quot;&gt;AS5048A&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt; and &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700;&quot;&gt;analog-to-digital converters&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;/&gt;&lt;span style=&quot; font-size:11pt; font-weight:700; color:#329978;&quot;&gt;MCP32XX&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt; to &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700; color:#329978;&quot;&gt;SPI&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt;. See the &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700;&quot;&gt;Wiki&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt; for details.&lt;/span&gt;&lt;/p&gt;&lt;p&gt;&lt;img src=&quot;:/Images/MLX90393.png&quot;/&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</oldsource>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;可以将&lt;span style=&quot; font-size:11pt; font-weight:700;&quot;&gt;霍尔传感器&lt;/span&gt; &lt;span style=&quot; font-size:11pt; font-weight:700; color:#329978;&quot;&gt;TLE5011&lt;/span&gt;、&lt;span style=&quot; font-size:11pt; font-weight:700; color:#329978;&quot;&gt;TLE5010&lt;/span&gt;、&lt;span style=&quot; font-size:11pt; font-weight:700; color:#329978;&quot;&gt;TLE5012B&lt;/span&gt;、&lt;span style=&quot; font-size:11pt; font-weight:700; color:#329978;&quot;&gt;MLX90393&lt;/span&gt;、&lt;span style=&quot; font-size:11pt; font-weight:700; color:#329978;&quot;&gt;MLX90363&lt;/span&gt;、&lt;span style=&quot; font-size:11pt; font-weight:700; color:#329978;&quot;&gt;AS5048A&lt;/span&gt; 以及&lt;span style=&quot; font-size:11pt; font-weight:700;&quot;&gt;模数转换器&lt;/span&gt; &lt;span style=&quot; font-size:11pt; font-weight:700; color:#329978;&quot;&gt;MCP32XX&lt;/span&gt; 连接到 &lt;span style=&quot; font-size:11pt; font-weight:700; color:#329978;&quot;&gt;SPI&lt;/span&gt;。详情请见 &lt;span style=&quot; font-size:11pt; font-weight:700;&quot;&gt;Wiki&lt;/span&gt;。&lt;/p&gt;&lt;p&gt;&lt;img src=&quot;:/Images/MLX90393.png&quot;/&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location filename="widgets/pins/pintypehelper.ui" line="98"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Analog Axis&lt;img src=&quot;:/Images/icons/lucide/info.svg&quot; width=&quot;10&quot; align=&quot;right&quot;/&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <oldsource>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;SPI&lt;img src=&quot;:/Images/info_icon.png&quot; width=&quot;10&quot; align=&quot;right&quot;/&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</oldsource>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;模拟轴&lt;img src=&quot;:/Images/icons/lucide/info.svg&quot; width=&quot;10&quot; align=&quot;right&quot;/&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location filename="widgets/pins/pintypehelper.ui" line="118"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt;It is possible to connect to &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700; color:#5a9b8c;&quot;&gt;I2C SDA/SCL &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700;&quot;&gt;AS5600 Hall sensor&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt;, no more than &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700;&quot;&gt;1&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt; pcs., or &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700;&quot;&gt;ADS1115 analog-to-digital converter&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt;, up to &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700;&quot;&gt;4&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt; pcs., but the total number of axes cannot be more than &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700;&quot;&gt;8&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt;.&lt;/span&gt;&lt;/p&gt;&lt;p&gt;&lt;img src=&quot;:/Images/ADS1115.png&quot;/&gt;&lt;img src=&quot;:/Images/AS5600.png&quot;/&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <oldsource>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt;It is possible to connect to &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700; color:#5a9b8c;&quot;&gt;I2C SDA/SCL&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;/&gt;&lt;span style=&quot; font-size:11pt; font-weight:700;&quot;&gt;AS5600 Hall sensor&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt;, no more than &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700;&quot;&gt;1&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt; pcs., or &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700;&quot;&gt;ADS1115 analog-to-digital converter&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt;, up to &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700;&quot;&gt;4&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt; pcs., but the total number of axes cannot be more than &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700;&quot;&gt;8&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt;.&lt;/span&gt;&lt;/p&gt;&lt;p&gt;&lt;img src=&quot;:/Images/ADS1115.png&quot;/&gt;&lt;img src=&quot;:/Images/AS5600.png&quot;/&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</oldsource>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;可以向 &lt;span style=&quot; font-size:11pt; font-weight:700; color:#5a9b8c;&quot;&gt;I2C SDA/SCL&lt;/span&gt; 连接 &lt;span style=&quot; font-size:11pt; font-weight:700;&quot;&gt;AS5600 霍尔传感器&lt;/span&gt;，不超过 &lt;span style=&quot; font-size:11pt; font-weight:700;&quot;&gt;1&lt;/span&gt; 个，或 &lt;span style=&quot; font-size:11pt; font-weight:700;&quot;&gt;ADS1115 模数转换器&lt;/span&gt;，最多 &lt;span style=&quot; font-size:11pt; font-weight:700;&quot;&gt;4&lt;/span&gt; 个，但轴的总数不能超过 &lt;span style=&quot; font-size:11pt; font-weight:700;&quot;&gt;8&lt;/span&gt;。&lt;/p&gt;&lt;p&gt;&lt;img src=&quot;:/Images/ADS1115.png&quot;/&gt;&lt;img src=&quot;:/Images/AS5600.png&quot;/&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location filename="widgets/pins/pintypehelper.ui" line="111"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;SPI&lt;img src=&quot;:/Images/icons/lucide/info.svg&quot; width=&quot;10&quot; align=&quot;right&quot;/&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <oldsource>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;I2C&lt;img src=&quot;:/Images/info_icon.png&quot; width=&quot;10&quot; align=&quot;right&quot;/&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</oldsource>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;SPI&lt;img src=&quot;:/Images/icons/lucide/info.svg&quot; width=&quot;10&quot; align=&quot;right&quot;/&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location filename="widgets/pins/pintypehelper.ui" line="124"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;I2C&lt;img src=&quot;:/Images/icons/lucide/info.svg&quot; width=&quot;10&quot; align=&quot;right&quot;/&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;I2C&lt;img src=&quot;:/Images/icons/lucide/info.svg&quot; width=&quot;10&quot; align=&quot;right&quot;/&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location filename="widgets/pins/pintypehelper.ui" line="134"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt;FJ sends joystick data via &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700; color:#5a9b8c;&quot;&gt;UART TX &lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt;every 10 ms, baud rate 115200. You can connect, for example, the ESP32 C3 to UART, which can be used as a Bluetooth joystick.&lt;/span&gt;&lt;/p&gt;&lt;table border=&quot;0&quot; style=&quot; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; border-collapse:collapse;&quot; cellspacing=&quot;2&quot; cellpadding=&quot;0&quot; &gt;&lt;thead&gt;&lt;tr&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p align=&quot;center&quot;&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt; font-weight:700;&quot;&gt;Name&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p align=&quot;center&quot;&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt; font-weight:700;&quot;&gt;Length bytes&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p align=&quot;center&quot;&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt; font-weight:700;&quot;&gt;Desc&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;/tr&gt;&lt;/thead&gt;&lt;tr&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;Header&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p align=&quot;center&quot;&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;1&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;ASCII &apos;H&apos;&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;/tr&gt;&lt;tr&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;Separator&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p align=&quot;center&quot;&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;1&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;ASCII &apos;-&apos;&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;/tr&gt;&lt;tr&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;Message code&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p align=&quot;center&quot;&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;1&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;message num 0-255&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;/tr&gt;&lt;tr&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;Axis data&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p align=&quot;center&quot;&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;16&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;2 bytes for axis&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;/tr&gt;&lt;tr&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;Buttons data&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p align=&quot;center&quot;&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;16&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;one bit = one button&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;/tr&gt;&lt;tr&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;CRC16&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p align=&quot;center&quot;&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;2&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;crc16 polynomial 0x8005&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;/tr&gt;&lt;/table&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt;FJ 通过 &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700; color:#5a9b8c;&quot;&gt;UART TX &lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt;every 10 ms, baud rate 115200. You can connect, for example, the ESP32 C3 to UART, which can be used as a Bluetooth joystick.&lt;/span&gt;&lt;/p&gt;&lt;table border=&quot;0&quot; style=&quot; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; border-collapse:collapse;&quot; cellspacing=&quot;2&quot; cellpadding=&quot;0&quot; &gt;&lt;thead&gt;&lt;tr&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p align=&quot;center&quot;&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt; font-weight:700;&quot;&gt;名称&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p align=&quot;center&quot;&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt; font-weight:700;&quot;&gt;长度（字节）&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p align=&quot;center&quot;&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt; font-weight:700;&quot;&gt;说明&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;/tr&gt;&lt;/thead&gt;&lt;tr&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;头部&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p align=&quot;center&quot;&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;1&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;ASCII &apos;H&apos;&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;/tr&gt;&lt;tr&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;分隔符&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p align=&quot;center&quot;&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;1&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;ASCII &apos;-&apos;&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;/tr&gt;&lt;tr&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;消息代码&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p align=&quot;center&quot;&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;1&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;消息编号 0-255&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;/tr&gt;&lt;tr&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;轴数据&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p align=&quot;center&quot;&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;16&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;每个轴 2 字节&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;/tr&gt;&lt;tr&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;按钮数据&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p align=&quot;center&quot;&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;16&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;一位 = 一个按钮&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;/tr&gt;&lt;tr&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;CRC16&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p align=&quot;center&quot;&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;2&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;td style=&quot; padding-left:7; padding-right:7; padding-top:3; padding-bottom:3; border-top:1px; border-right:1px; border-bottom:1px; border-left:1px; border-top-color:#555555; border-right-color:#555555; border-bottom-color:#555555; border-left-color:#555555; border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid;&quot;&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Noto Sans&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:11pt;&quot;&gt;crc16 多项式 0x8005&lt;/span&gt;&lt;/p&gt;&lt;/td&gt;&lt;/tr&gt;&lt;/table&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location filename="widgets/pins/pintypehelper.ui" line="147"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;&lt;span style=&quot; font-size:11pt; font-weight:700;&quot;&gt;WS2812b&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt; and &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700;&quot;&gt;PL9823&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt; addressable LEDs are supported. It is possible to customize the color of each LED, use some effects, or control the LEDs via &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700;&quot;&gt;SimHub&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt;. Connect the &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700;&quot;&gt;DIN&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt; pin of the LED to the selected &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700; color:#ff55ff;&quot;&gt;WS2812b/PL9823 LED&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt; pin on the controller board. Settings in the &amp;quot;LED/PWM&amp;quot; tab.&lt;/span&gt;&lt;/p&gt;&lt;p&gt;&lt;img src=&quot;:/Images/rgb.png&quot; width=&quot;500&quot;/&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <oldsource>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;&lt;span style=&quot; font-size:11pt; font-weight:700;&quot;&gt;WS2812b&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt; addressable LEDs are supported. It is possible to customize the color of each LED, use some effects, or control the LEDs via &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700;&quot;&gt;SimHub&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt;. Connect the &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700;&quot;&gt;DIN&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt; pin of the LED to the selected &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700; color:#ff55ff;&quot;&gt;WS2812b LED&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt; pin on the controller board. Settings in the &amp;quot;LED/PWM&amp;quot; tab.&lt;/span&gt;&lt;/p&gt;&lt;p&gt;&lt;img src=&quot;:/Images/rgb.png&quot; width=&quot;500&quot;/&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</oldsource>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;支持 &lt;span style=&quot; font-size:11pt; font-weight:700;&quot;&gt;WS2812b&lt;/span&gt; 和 &lt;span style=&quot; font-size:11pt; font-weight:700;&quot;&gt;PL9823&lt;/span&gt; 可寻址 LED。可以自定义每个 LED 的颜色、使用一些效果，或通过 &lt;span style=&quot; font-size:11pt; font-weight:700;&quot;&gt;SimHub&lt;/span&gt; 控制 LED。将 LED 的 &lt;span style=&quot; font-size:11pt; font-weight:700;&quot;&gt;DIN&lt;/span&gt; 引脚连接到控制板上所选的 &lt;span style=&quot; font-size:11pt; font-weight:700; color:#ff55ff;&quot;&gt;WS2812b/PL9823 LED&lt;/span&gt; 引脚。设置在 &amp;quot;LED/PWM&amp;quot; 选项卡中。&lt;/p&gt;&lt;p&gt;&lt;img src=&quot;:/Images/rgb.png&quot; width=&quot;500&quot;/&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location filename="widgets/pins/pintypehelper.ui" line="140"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;UART&lt;img src=&quot;:/Images/icons/lucide/info.svg&quot; width=&quot;10&quot; align=&quot;right&quot;/&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <oldsource>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;ARGB LEDs&lt;img src=&quot;:/Images/info_icon.png&quot; width=&quot;10&quot; style=&quot;float: right;&quot;/&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</oldsource>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;UART&lt;img src=&quot;:/Images/icons/lucide/info.svg&quot; width=&quot;10&quot; align=&quot;right&quot;/&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location filename="widgets/pins/pintypehelper.ui" line="160"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt;Single-color LEDs with brightness control in the &amp;quot;LED/PWM&amp;quot; tab, no more than 20mA. Connect the &lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700; color:#d37a0d;&quot;&gt;+&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt; of the LED to the &amp;quot;&lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700; color:#c85a46;&quot;&gt;PWM LED&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt;&amp;quot; pin. The value of the current-limiting resistance is recommended to be calculated individually for the LEDs used.&lt;/span&gt;&lt;/p&gt;&lt;p&gt;&lt;img src=&quot;:/Images/pwmLed.png&quot;/&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;单色 LED，可在 &amp;quot;LED/PWM&amp;quot; 选项卡中调节亮度，不超过 20 mA。将 LED 的 &lt;span style=&quot; font-size:11pt; font-weight:700; color:#d37a0d;&quot;&gt;+&lt;/span&gt; 连接到 &amp;quot;&lt;span style=&quot; font-size:11pt; font-weight:700; color:#c85a46;&quot;&gt;PWM LED&lt;/span&gt;&amp;quot; 引脚。建议根据所用 LED 单独计算限流电阻值。&lt;/p&gt;&lt;p&gt;&lt;img src=&quot;:/Images/pwmLed.png&quot;/&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location filename="widgets/pins/pintypehelper.ui" line="153"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;ARGB LEDs&lt;img src=&quot;:/Images/icons/lucide/info.svg&quot; width=&quot;10&quot; style=&quot;float: right;&quot;/&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <oldsource>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;PWM LEDs&lt;img src=&quot;:/Images/info_icon.png&quot; width=&quot;10&quot; align=&quot;right&quot;/&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</oldsource>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;ARGB LED&lt;img src=&quot;:/Images/icons/lucide/info.svg&quot; width=&quot;10&quot; style=&quot;float: right;&quot;/&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location filename="widgets/pins/pintypehelper.ui" line="173"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt;Single-color LEDs, not more than 20mA, linked to the pressed buttons in the &amp;quot;LED/PWM&amp;quot; tab. Both direct &amp;quot;&lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700; color:#c89646;&quot;&gt;LED Single&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt;&amp;quot; and LED matrix &amp;quot;&lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700; color:#c88246;&quot;&gt;LED Row/Column&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt;&amp;quot; connection are supported. The value of the current-limiting resistance is recommended to be calculated individually for the LEDs used.&lt;/span&gt;&lt;/p&gt;&lt;p&gt;&lt;img src=&quot;:/Images/monoLed.png&quot;/&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;单色 LED，不超过 20 mA，在 &amp;quot;LED/PWM&amp;quot; 选项卡中与按下的按钮关联。支持直接连接 &amp;quot;&lt;span style=&quot; font-size:11pt; font-weight:700; color:#c89646;&quot;&gt;LED Single&lt;/span&gt;&amp;quot; 和 LED 矩阵 &amp;quot;&lt;span style=&quot; font-size:11pt; font-weight:700; color:#c88246;&quot;&gt;LED Row/Column&lt;/span&gt;&amp;quot;。建议根据所用 LED 单独计算限流电阻值。&lt;/p&gt;&lt;p&gt;&lt;img src=&quot;:/Images/monoLed.png&quot;/&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location filename="widgets/pins/pintypehelper.ui" line="166"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;PWM LEDs&lt;img src=&quot;:/Images/icons/lucide/info.svg&quot; width=&quot;10&quot; align=&quot;right&quot;/&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <oldsource>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Mono LEDs&lt;img src=&quot;:/Images/info_icon.png&quot; width=&quot;10&quot; align=&quot;right&quot;/&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</oldsource>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;PWM LED&lt;img src=&quot;:/Images/icons/lucide/info.svg&quot; width=&quot;10&quot; align=&quot;right&quot;/&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location filename="widgets/pins/pintypehelper.ui" line="186"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt;It is possible to connect one incremental encoder with a high frequency, it can only be used as an axis source. Pins &amp;quot;&lt;/span&gt;&lt;span style=&quot; font-size:11pt; font-weight:700; color:#379619;&quot;&gt;Fast encoder&lt;/span&gt;&lt;span style=&quot; font-size:11pt;&quot;&gt;&amp;quot;.&lt;/span&gt;&lt;/p&gt;&lt;p&gt;&lt;img src=&quot;:/Images/fastEncoder.png&quot;/&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;可以连接一个高频增量式编码器，它只能用作轴源。引脚 &amp;quot;&lt;span style=&quot; font-size:11pt; font-weight:700; color:#379619;&quot;&gt;Fast encoder&lt;/span&gt;&amp;quot;。&lt;/p&gt;&lt;p&gt;&lt;img src=&quot;:/Images/fastEncoder.png&quot;/&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location filename="widgets/pins/pintypehelper.ui" line="179"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Mono LEDs&lt;img src=&quot;:/Images/icons/lucide/info.svg&quot; width=&quot;10&quot; align=&quot;right&quot;/&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <oldsource>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Fast Encoder&lt;img src=&quot;:/Images/info_icon.png&quot; width=&quot;10&quot; align=&quot;right&quot;/&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</oldsource>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;单色 LED&lt;img src=&quot;:/Images/icons/lucide/info.svg&quot; width=&quot;10&quot; align=&quot;right&quot;/&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
</context>
<context>
    <name>QObject</name>
    <message>
        <location filename="configtofile.cpp" line="295"/>
        <source>I2C is configured on slots 21/22 (B10 SCL + B11 SDA on Blue Pill). B2 on Black Pill isn&apos;t bonded for I2C on the F411 UFQFPN48 package, so the I2C pair will be cleared during conversion.</source>
        <translation>I2C 配置在槽 21/22 上（Blue Pill 上的 B10 SCL + B11 SDA）。Black Pill 上的 B2 在 F411 UFQFPN48 封装中未引出用于 I2C，因此转换时将清除该 I2C 引脚对。</translation>
    </message>
    <message>
        <location filename="configtofile.cpp" line="300"/>
        <source> %1 axis(es) sourced from I2C will also be reset to &quot;unused&quot; since the bus is being torn down.</source>
        <translation> 由于该总线被拆除，%1 个以 I2C 为源的轴也将被重置为“未使用”。</translation>
    </message>
    <message>
        <location filename="configtofile.cpp" line="307"/>
        <source>Slot 22 is in use. After conversion this slot routes to %1 instead of %2 -- verify your physical wiring matches the slot index, not the original pin name.</source>
        <translation>槽 22 正在使用中。转换后此槽将路由到 %1 而非 %2 — 请确认您的物理接线与槽索引匹配，而非原始引脚名称。</translation>
    </message>
    <message>
        <location filename="configtofile.cpp" line="316"/>
        <source>This config was saved for %1, but the connected device is %2.

29 of the 30 pin slots map identically between the two boards. The exception is slot 22 (PB11 on Blue Pill, PB2 on Black Pill).

</source>
        <translation>此配置是为 %1 保存的，但已连接的设备是 %2。

30 个引脚槽中有 29 个在两块开发板之间的映射完全相同。例外是槽 22（Blue Pill 上为 PB11，Black Pill 上为 PB2）。

</translation>
    </message>
    <message>
        <location filename="configtofile.cpp" line="324"/>
        <source>Convert this config for %1?

Choosing No leaves the loaded config unchanged -- the device will refuse the write until you reconnect the matching board or convert.</source>
        <translation>为 %1 转换此配置？

选择“否”将保持已加载的配置不变 — 在您重新连接匹配的开发板或进行转换之前，设备会拒绝写入。</translation>
    </message>
    <message>
        <location filename="configtofile.cpp" line="332"/>
        <source>Cross-board config</source>
        <translation>跨开发板配置</translation>
    </message>
</context>
<context>
    <name>SelectFolder</name>
    <message>
        <location filename="widgets/selectfolder.ui" line="26"/>
        <source>Dialog</source>
        <translation></translation>
    </message>
    <message>
        <location filename="widgets/selectfolder.ui" line="44"/>
        <source>Ok</source>
        <translation>确认</translation>
    </message>
    <message>
        <location filename="widgets/selectfolder.ui" line="53"/>
        <source>Current folder:</source>
        <translation>当前文件夹：</translation>
    </message>
    <message>
        <location filename="widgets/selectfolder.cpp" line="11"/>
        <source>Configs folder path</source>
        <translation>配置文件夹目录</translation>
    </message>
    <message>
        <location filename="widgets/selectfolder.cpp" line="44"/>
        <source>Select configs folder</source>
        <translation>选择配置文件的保存文件夹</translation>
    </message>
</context>
<context>
    <name>ShiftRegisters</name>
    <message>
        <location filename="widgets/shift-reg/shiftregisters.ui" line="50"/>
        <location filename="widgets/shift-reg/shiftregisters.ui" line="330"/>
        <location filename="widgets/shift-reg/shiftregisters.ui" line="350"/>
        <location filename="widgets/shift-reg/shiftregisters.cpp" line="15"/>
        <source>Not defined</source>
        <oldsource>NotDefined</oldsource>
        <translation>未定义</translation>
    </message>
    <message>
        <location filename="widgets/shift-reg/shiftregisters.ui" line="291"/>
        <source>Latch pin</source>
        <translation>Latch引脚</translation>
    </message>
    <message>
        <location filename="widgets/shift-reg/shiftregisters.ui" line="340"/>
        <source>CLK pin</source>
        <translation>CLK引脚</translation>
    </message>
    <message>
        <location filename="widgets/shift-reg/shiftregisters.ui" line="63"/>
        <source>Shift register type</source>
        <translation>移位寄存器类型</translation>
    </message>
    <message>
        <location filename="widgets/shift-reg/shiftregisters.ui" line="317"/>
        <source>Data pin</source>
        <translation>Data引脚</translation>
    </message>
    <message>
        <location filename="widgets/shift-reg/shiftregisters.ui" line="278"/>
        <source>Registers count</source>
        <translation>寄存器计数</translation>
    </message>
    <message>
        <location filename="widgets/shift-reg/shiftregisters.ui" line="304"/>
        <source>Button count</source>
        <translation>按钮计数</translation>
    </message>
    <message>
        <location filename="widgets/shift-reg/shiftregisters.h" line="71"/>
        <source>HC165 Pull Down</source>
        <translation>HC165下拉</translation>
    </message>
    <message>
        <location filename="widgets/shift-reg/shiftregisters.h" line="72"/>
        <source>CD4021 Pull Down</source>
        <translation>CD4021下拉</translation>
    </message>
    <message>
        <location filename="widgets/shift-reg/shiftregisters.h" line="73"/>
        <source>HC165 Pull Up</source>
        <translation>HC165上拉</translation>
    </message>
    <message>
        <location filename="widgets/shift-reg/shiftregisters.h" line="74"/>
        <source>CD4021 Pull Up</source>
        <translation>CD4021上拉</translation>
    </message>
</context>
<context>
    <name>ShiftsTimersConfig</name>
    <message>
        <location filename="widgets/shifts-timers/shiftstimersconfig.ui" line="35"/>
        <source>Shifts</source>
        <translation>Shift</translation>
    </message>
    <message>
        <location filename="widgets/shifts-timers/shiftstimersconfig.ui" line="56"/>
        <source>Shift 1</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="widgets/shifts-timers/shiftstimersconfig.ui" line="71"/>
        <location filename="widgets/shifts-timers/shiftstimersconfig.ui" line="109"/>
        <location filename="widgets/shifts-timers/shiftstimersconfig.ui" line="147"/>
        <location filename="widgets/shifts-timers/shiftstimersconfig.ui" line="185"/>
        <location filename="widgets/shifts-timers/shiftstimersconfig.ui" line="223"/>
        <location filename="widgets/shifts-timers/shiftstimersconfig.ui" line="261"/>
        <location filename="widgets/shifts-timers/shiftstimersconfig.ui" line="299"/>
        <location filename="widgets/shifts-timers/shiftstimersconfig.ui" line="337"/>
        <source>Logical button</source>
        <translation>逻辑按钮</translation>
    </message>
    <message>
        <location filename="widgets/shifts-timers/shiftstimersconfig.ui" line="94"/>
        <source>Shift 2</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="widgets/shifts-timers/shiftstimersconfig.ui" line="132"/>
        <source>Shift 3</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="widgets/shifts-timers/shiftstimersconfig.ui" line="170"/>
        <source>Shift 4</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="widgets/shifts-timers/shiftstimersconfig.ui" line="208"/>
        <source>Shift 5</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="widgets/shifts-timers/shiftstimersconfig.ui" line="246"/>
        <source>Shift 6</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="widgets/shifts-timers/shiftstimersconfig.ui" line="284"/>
        <source>Shift 7</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="widgets/shifts-timers/shiftstimersconfig.ui" line="322"/>
        <source>Shift 8</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="widgets/shifts-timers/shiftstimersconfig.ui" line="363"/>
        <source>Encoder timings</source>
        <translation>编码器时序</translation>
    </message>
    <message>
        <location filename="widgets/shifts-timers/shiftstimersconfig.ui" line="380"/>
        <source>Encoder press timer</source>
        <translation>编码器按压定时器</translation>
    </message>
    <message>
        <location filename="widgets/shifts-timers/shiftstimersconfig.ui" line="387"/>
        <location filename="widgets/shifts-timers/shiftstimersconfig.ui" line="437"/>
        <location filename="widgets/shifts-timers/shiftstimersconfig.ui" line="449"/>
        <location filename="widgets/shifts-timers/shiftstimersconfig.ui" line="461"/>
        <location filename="widgets/shifts-timers/shiftstimersconfig.ui" line="473"/>
        <location filename="widgets/shifts-timers/shiftstimersconfig.ui" line="485"/>
        <location filename="widgets/shifts-timers/shiftstimersconfig.ui" line="513"/>
        <location filename="widgets/shifts-timers/shiftstimersconfig.ui" line="527"/>
        <source> ms</source>
        <translation type="unfinished"> 毫秒</translation>
    </message>
    <message>
        <location filename="widgets/shifts-timers/shiftstimersconfig.ui" line="394"/>
        <source>Encoders polling</source>
        <translation>编码器轮询</translation>
    </message>
    <message>
        <location filename="widgets/shifts-timers/shiftstimersconfig.ui" line="399"/>
        <location filename="widgets/shifts-timers/shiftstimersconfig.ui" line="495"/>
        <source>Polling interval. A low value will not improve the operation of the buttons; on the contrary, increasing it may improve encoder operation.</source>
        <translation>轮询间隔。较低的值不会改善按钮的工作；相反，增大它可能会改善编码器的工作。</translation>
    </message>
    <message>
        <location filename="widgets/shifts-timers/shiftstimersconfig.ui" line="402"/>
        <location filename="widgets/shifts-timers/shiftstimersconfig.ui" line="498"/>
        <source> ns</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="widgets/shifts-timers/shiftstimersconfig.ui" line="415"/>
        <source>Button defaults</source>
        <translation>按钮默认值</translation>
    </message>
    <message>
        <location filename="widgets/shifts-timers/shiftstimersconfig.ui" line="431"/>
        <source>Timer 1</source>
        <translation type="unfinished">计时器 1</translation>
    </message>
    <message>
        <location filename="widgets/shifts-timers/shiftstimersconfig.ui" line="443"/>
        <source>Timer 2</source>
        <translation type="unfinished">计时器 2</translation>
    </message>
    <message>
        <location filename="widgets/shifts-timers/shiftstimersconfig.ui" line="455"/>
        <source>Timer 3</source>
        <translation type="unfinished">计时器 3</translation>
    </message>
    <message>
        <location filename="widgets/shifts-timers/shiftstimersconfig.ui" line="467"/>
        <source>Debounce timer</source>
        <translation>去抖定时器</translation>
    </message>
    <message>
        <location filename="widgets/shifts-timers/shiftstimersconfig.ui" line="479"/>
        <source>Axes-to-buttons debounce</source>
        <translation>轴转按钮去抖</translation>
    </message>
    <message>
        <location filename="widgets/shifts-timers/shiftstimersconfig.ui" line="491"/>
        <source>Buttons polling</source>
        <translation>按钮轮询</translation>
    </message>
    <message>
        <location filename="widgets/shifts-timers/shiftstimersconfig.ui" line="507"/>
        <source>Tap cutoff</source>
        <translation>点击截止时间</translation>
    </message>
    <message>
        <location filename="widgets/shifts-timers/shiftstimersconfig.ui" line="521"/>
        <source>Double tap cutoff</source>
        <translation>双击截止时间</translation>
    </message>
</context>
</TS>
