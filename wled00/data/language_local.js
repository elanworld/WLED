function translateText() {
  var translations = {
    // index.html
    "Colors": "颜色",
    "Effects": "效果",
    "Segments": "分段",
    "Presets": "预设",
    "Power": "电源",
    "Timer": "定时器",
    "Sync": "同步",
    "Peek": "预览",
    "Info": "信息",
    "Nodes": "节点",
    "Config": "配置",
    "PC Mode": "电脑模式",
    "Brightness": "亮度",
    "Effect speed": "效果速度",
    "Effect intensity": "效果强度",
    "Custom 1": "自定义 1",
    "Custom 2": "自定义 2",
    "Custom 3": "自定义 3",
    "Loading...": "加载中...",
    "Reset segments": "重置分段",
    "Transition:": "过渡：",
    "s": "秒",
    "Presets": "预设",
    "Search": "搜索",
    "Loading...": "加载中...",
    "Refresh": "刷新",
    "Instance List": "实例列表",
    "Update WLED": "更新 WLED",
    "Reboot WLED": "重启 WLED",
    "Made with ❤️ by Aircoookie and the WLED community": "由 Aircoookie 和 WLED 社区制作 ❤️",
    "WLED instances": "WLED 实例",
    "Loading...": "加载中...",
    "To use built-in effects, use an override button below.": "要使用内置效果，请在下方使用覆盖按钮。",
    "You can return to realtime mode by pressing the star in the top left corner.": "您可以通过点击左上角的星星返回实时模式。",
    "For best performance, it is recommended to turn off the streaming source when not in use.": "为了获得最佳性能，建议在不使用时关闭流媒体源。",
    "Color palette not used": "颜色调色板未使用",
    // settings.html
    "Back": "返回",
    "WiFi Setup": "WiFi 设置",
    "LED Preferences": "LED 首选项",
    "2D Configuration": "2D 配置",
    "User Interface": "用户界面",
    "DMX Output": "DMX 输出",
    "Sync Interfaces": "同步接口",
    "Time & Macros": "时间和宏",
    "Usermods": "用户自定义",
    "Security & Updates": "安全性和更新",
    // settings_wifi.html
    "Back": "返回",
    "Save & Connect": "保存并连接",
    "WiFi setup": "WiFi设置",
    "Connect to existing network": "连接到现有网络",
    "Scan": "扫描",
    "Network name (SSID, empty to not connect):": "网络名称（SSID，留空表示不连接）：",
    "Network password:": "网络密码：",
    "Static IP (leave at 0.0.0.0 for DHCP):": "静态IP（DHCP请保持为0.0.0.0）：",
    "Static gateway:": "静态网关：",
    "Static subnet mask:": "静态子网掩码：",
    "mDNS address (leave empty for no mDNS):": "mDNS地址（留空表示不使用mDNS）：",
    "Client IP:": "客户端IP：",
    "Configure Access Point": "配置接入点",
    "AP SSID (leave empty for no AP):": "AP的SSID（留空表示不使用AP）：",
    "Hide AP name:": "隐藏AP名称：",
    "AP password (leave empty for open):": "AP密码（留空表示开放）：",
    "Access Point WiFi channel:": "接入点WiFi频道：",
    "AP opens:": "AP开启：",
    "No connection after boot": "启动后不连接",
    "Disconnected": "断开连接",
    "Always": "始终开启",
    "Never (not recommended)": "从不开启（不推荐）",
    "Experimental": "实验性功能",
    "Disable WiFi sleep:": "禁用WiFi休眠：",
    "Can help with connectivity issues.": "可帮助解决连接问题。",
    "Do not enable if WiFi is working correctly, increases power consumption.": "如果WiFi正常工作，请勿启用，会增加功耗。",
    "Ethernet Type": "以太网类型",
    "None": "无",
    "ESP32-POE": "ESP32-POE",
    "ESP32Deux": "ESP32Deux",
    "KIT-VE": "KIT-VE",
    "QuinLED-Dig-Octa": "QuinLED-Dig-Octa",
    "QuinLED-ESP32": "QuinLED-ESP32",
    "TwilightLord-ESP32": "TwilightLord-ESP32",
    "WESP32": "WESP32",
    "WT32-ETH01": "WT32-ETH01",
    // settings_sync.html
    "Colors": "颜色",
    "Effects": "效果",
    "Sync setup": "同步设置",
    "WLED Broadcast": "WLED广播",
    "UDP Port:": "UDP端口：",
    "2nd Port:": "第二端口：",
    "Sync groups": "同步组",
    "Send:": "发送：",
    "Receive:": "接收：",
    "Receive: Brightness,": "接收：亮度，",
    "Color,": "颜色，",
    "and Effects": "和效果",
    "Segment options,": "段选项，",
    "bounds": "界限",
    "Send notifications on direct change:": "在直接更改时发送通知：",
    "Send notifications on button press or IR:": "在按下按钮或使用红外线时发送通知：",
    "Send Alexa notifications:": "发送Alexa通知：",
    "Send Philips Hue change notifications:": "发送Philips Hue更改通知：",
    "Send Macro notifications:": "发送宏通知：",
    "UDP packet retransmissions:": "UDP数据包重传：",
    "Reboot required to apply changes.": "需要重新启动才能应用更改。",
    "Instance List": "实例列表",
    "Enable instance list:": "启用实例列表：",
    "Make this instance discoverable:": "使此实例可发现：",
    "Realtime": "实时",
    "Receive UDP realtime:": "接收UDP实时数据：",
    "Use main segment only:": "仅使用主段：",
    "Network DMX input": "网络DMX输入",
    "Type:": "类型：",
    "E1.31 (sACN)": "E1.31（sACN）",
    "Art-Net": "Art-Net",
    "Custom port": "自定义端口",
    "Port:": "端口：",
    "Multicast:": "组播：",
    "Start universe:": "起始宇宙：",
    "Reboot required. Check out": "需要重新启动。查看",
    "LedFx": "LedFx",
    "Skip out-of-sequence packets:": "跳过不按顺序的数据包：",
    "DMX start address:": "DMX起始地址：",
    "DMX segment spacing:": "DMX段间距：",
    "E1.31 port priority:": "E1.31端口优先级：",
    "DMX mode:": "DMX模式：",
    "Disabled": "已禁用",
    "Single RGB": "单个RGB",
    "Single DRGB": "单个DRGB",
    "Effect": "效果",
    "Effect + White": "效果 + 白色",
    "Effect Segment": "效果段",
    "Effect Segment + White": "效果段 + 白色",
    "Multi RGB": "多个RGB",
    "Dimmer + Multi RGB": "调光器 + 多个RGB",
    "Multi RGBW": "多个RGBW",
    "Preset": "预设",
    "Colors": "颜色",
    "Effects": "效果",
    "E1.31 info": "E1.31信息",
    "Timeout:": "超时时间：",
    "ms": "毫秒",
    "Force max brightness:": "强制最大亮度：",
    "Disable realtime gamma correction:": "禁用实时伽马校正：",
    "Realtime LED offset:": "实时LED偏移：",
    "Alexa Voice Assistant": "Alexa语音助手",
    "This firmware build does not include Alexa support.": "此固件版本不包含Alexa支持。",
    "Emulate Alexa device:": "模拟Alexa设备：",
    "Alexa invocation name:": "Alexa调用名称：",
    "Also emulate devices to call the first": "同时模拟设备调用第一个",
    "presets": "预设",
    "MQTT and Hue sync all connect to external hosts!": "MQTT和Hue同步均连接到外部主机！",
    "This may impact the responsiveness of WLED.": "这可能会影响WLED的响应性。",
    "For best results, only use one of these services at a time.": "为了获得最佳效果，请一次只使用其中一项服务。",
    "(alternatively, connect a second ESP to them and use the UDP sync)": "(或者，将第二个ESP连接到它们，并使用UDP同步)",
    "MQTT": "MQTT",
    "This firmware build does not include MQTT support.": "此固件版本不包含MQTT支持。",
    "Enable MQTT:": "启用MQTT：",
    "Broker:": "代理：",
    "Port:": "端口：",
    "The MQTT credentials are sent over an unsecured connection.": "MQTT凭据通过不安全的连接发送。",
    "Never use the MQTT password for another service!": "永远不要将MQTT密码用于其他服务！",
    "Username:": "用户名：",
    "Password:": "密码：",
    "Client ID:": "客户端ID：",
    "Device Topic:": "设备主题：",
    "Group Topic:": "群组主题：",
    "Publish on button press:": "按钮按下后发布：",
    "Reboot required to apply changes.": "需要重新启动设备以应用更改。",
    "MQTT info": "MQTT信息",
    "Philips Hue": "飞利浦Hue",
    "This firmware build does not include Philips Hue support.": "此固件版本不包含飞利浦Hue支持。",
    "You can find the bridge IP and the light number in the 'About' section of the hue app.": "您可以在Hue应用的“关于”部分找到桥的IP和灯的编号。",
    "Poll Hue light": "轮询Hue灯",
    "every": "每隔",
    "ms": "毫秒",
    "Then, receive": "然后，接收",
    "On/Off": "开/关",
    "Brightness": "亮度",
    "Color": "颜色",
    "Hue Bridge IP:": "Hue桥的IP：",
    "Press the pushlink button on the bridge, after that save this page!": "在桥上按下推送按钮，然后保存此页面！",
    "(when first connecting)": "(初次连接时)",
    "Hue status:": "Hue状态：",
    "Disabled in this build": "此版本中禁用",
    // settings_leds.h
    "LED & Hardware setup": "LED和硬件设置",
    "5V 6A supply connected to LEDs": "5V 6A电源连接到LED：",
    "(for most effects, ~2A is enough)": "（对于大多数效果，~2A就足够了）",
    "Keep at <1A if powering LEDs directly from the ESP 5V pin!": "如果直接从ESP 5V引脚供电LED，请保持在<1A！",
    "If you are using an external power supply, enter its rating.": "如果使用外部电源，请输入其额定值。",
    "(Current estimated usage:": "（当前估计使用情况：",
    "Skip first LEDs:": "跳过第一个LED：",
    "Total LEDs:": "总LED数：",
    "Recommended power supply for brightest white:": "建议用于最亮白光的电源：",
    "Enable automatic brightness limiter:": "启用自动亮度限制器：",
    "Maximum Current:": "最大电流：",
    "Automatically limits brightness to stay close to the limit.": "自动限制亮度，保持接近限制。",
    "LED voltage (Max. current for a single LED):": "LED电压（单个LED的最大电流）：",
    "Keep at default if you are unsure about your type of LEDs.": "如果您不确定LED类型，请保持默认设置。",
    "Hardware setup": "硬件设置",
    "LED outputs:": "LED输出：",
    "LED Memory Usage:": "LED内存使用情况：",
    "You might run into stability or lag issues.": "您可能遇到稳定性或延迟问题。",
    "Use less than": "使用少于",
    "for the best experience!": "以获得最佳体验！",
    "Make a segment for each output:": "为每个输出创建一个段：",
    "Custom bus start indices:": "自定义总线起始索引：",
    "Use global LED buffer:": "使用全局LED缓冲区：",
    "Color Order Override:": "颜色顺序覆盖：",
    "Disable internal pull-up/down:": "禁用内部上拉/下拉电阻：",
    "Touch threshold:": "触摸阈值：",
    "IR GPIO:": "红外GPIO：",
    "Remote disabled": "遥控器已禁用",
    "24-key RGB": "24键RGB",
    "24-key with CT": "24键带CT",
    "40-key blue": "40键蓝色",
    "44-key RGB": "44键RGB",
    "21-key RGB": "21键RGB",
    "6-key black": "6键黑色",
    "9-key red": "9键红色",
    "JSON remote": "JSON遥控器",
    "Apply IR change to main segment only:": "仅将红外更改应用于主段：",
    "JSON file:": "JSON文件：",
    "Upload": "上传",
    "IR info": "红外信息",
    "Relay GPIO:": "继电器GPIO：",
    "Invert": "反转",
    "Turn LEDs on after power up/reset:": "开机/复位后打开LED：",
    "Default brightness:": "默认亮度：",
    "Apply preset": "应用预设",
    "at boot (0 uses defaults)": "在启动时（0使用默认值）",
    "Use Gamma correction for color:": "使用Gamma校正颜色：",
    "Use Gamma correction for brightness:": "使用Gamma校正亮度：",
    "Use Gamma value:": "使用Gamma值：",
    "Brightness factor:": "亮度因子：",
    "Transitions": "过渡效果",
    "Crossfade:": "交叉淡入淡出：",
    "Transition Time:": "过渡时间：",
    "Enable Palette transitions:": "启用调色板过渡：",
    "Random Cycle": "随机循环",
    "Palette Time:": "调色板时间：",
    "Timed light": "定时照明",
    "Default Duration:": "默认持续时间：",
    "Default Target brightness:": "默认目标亮度：",
    "Mode:": "模式：",
    "White management": "白光管理",
    "White Balance correction:": "白平衡校正：",
    "Global override for Auto-calculate white:": "自动计算白光的全局覆盖：",
    "Calculate CCT from RGB:": "从RGB计算CCT：",
    "CCT additive blending:": "CCT加法混合：",
    "Advanced": "高级",
    "Palette blending:": "调色板混合：",
    "Target refresh rate:": "目标刷新率：",
    "Config template:": "配置模板：",
    "Apply": "应用",
    "Save": "保存",
    "Back": "返回",
    // settings_sec.htm
    "?": "？",
    "Security & Update setup": "安全性和更新设置",
    "Settings PIN:": "设置PIN：",
    "Lock wireless (OTA) software update:": "锁定无线（OTA）软件更新：",
    "Passphrase:": "密码短语：",
    "To enable OTA, for security reasons you need to also enter the correct password!": "为了启用OTA，出于安全原因，您还需要输入正确的密码！",
    "The password should be changed when OTA is enabled.": "在启用OTA时应更改密码。",
    "Disable OTA when not in use, otherwise an attacker can reflash device software!": "不使用时禁用OTA，否则攻击者可以重新刷写设备软件！",
    "Settings on this page are only changable if OTA lock is disabled!": "只有在禁用OTA锁定时，才能更改此页面上的设置！",
    "Deny access to WiFi settings if locked:": "如果被锁定，则禁止访问WiFi设置：",
    "Factory reset:": "恢复出厂设置：",
    "All settings and presets will be erased.": "所有设置和预设将被擦除。",
    "Software Update": "软件更新",
    "Manual OTA Update": "手动OTA更新",
    "Enable ArduinoOTA:": "启用ArduinoOTA：",
    "Backup & Restore": "备份和恢复",
    "Backup presets": "备份预设",
    "Restore presets": "恢复预设",
    "Backup configuration": "备份配置",
    "Restore configuration": "恢复配置",
    "Incorrect configuration may require a factory reset or re-flashing of your ESP.": "错误的配置可能需要恢复出厂设置或重新刷写ESP。",
    "For security reasons, passwords are not backed up.": "出于安全原因，密码不会被备份。",
    "About": "关于",
    "WLED": "WLED",
    "version": "版本",
    "Contributors, dependencies and special thanks": "贡献者、依赖项和特别鸣谢",
    "A huge thank you to everyone who helped me create WLED!": "非常感谢所有帮助我创建WLED的人！",
    "(c) 2016-2023 Christian Schwinne": "(c) 2016-2023 Christian Schwinne",
    "Licensed under the": "根据授权",
    "Server message:": "服务器消息：",
    "Response error!": "响应错误！",
    "&#9888; Unencrypted transmission. Be prudent when selecting PIN, do NOT use your banking, door, SIM, etc. pin!": "&#9888; 未加密传输。选择 PIN 时请谨慎，不要使用您的银行、门禁、SIM 卡等 PIN！",
    "&#9888; Unencrypted transmission. An attacker on the same network can intercept form data!": "&#9888; 未加密传输。同一网络上的攻击者可以拦截表单数据！",
    "&#9888; Restoring presets/configuration will OVERWRITE your current presets/configuration.": "&#9888; 恢复预设/配置将覆盖当前的预设/配置。",
    // simple.htm
    "Loading WLED UI...": "正在加载 WLED 用户界面...",
    "Sorry, WLED UI needs JavaScript!": "抱歉，WLED 用户界面需要 JavaScript！",
    "Power": "电源",
    "Info": "信息",
    "Nodes": "节点",
    "Config": "配置",
    "Expand": "展开",
    "Global brightness": "全局亮度",
    "Brightness": "亮度",
    "Quick Load": "快速加载",
    "Solid color": "纯色",
    "Red": "红色",
    "Orange": "橙色",
    "Yellow": "黄色",
    "Warm White": "暖白",
    "White": "白色",
    "Black": "黑色",
    "Pink": "粉色",
    "Blue": "蓝色",
    "Cyan": "青色",
    "Green": "绿色",
    "Random": "随机",
    "Value": "值",
    "Temperature": "色温",
    "RGB channels": "RGB 通道",
    "White channel": "白色通道",
    "White balance": "白平衡",
    "Color slots": "颜色插槽",
    "Color slots": "颜色插槽",
    "Presets": "预设",
    "Search": "搜索",
    "Effect": "效果",
    "Effect speed": "效果速度",
    "Effect intensity": "效果强度",
    "Solid": "纯色",
    "Default": "默认",
    "Search": "搜索",
    "Refresh": "刷新",
    "Close Info": "关闭信息",
    "Instance List": "实例列表",
    "Reboot WLED": "重启 WLED",
    "Made with ❤️ by Aircoookie and the WLED community": "由 Aircoookie 和 WLED 社区制作 ❤️",
    // welcome.htm
    "WLED instances": "WLED 实例",
    "Loading...": "加载中...",
    "Close list": "关闭列表",
    "Welcome to WLED!": "欢迎来到 WLED！",
    "Thank you for installing my application!": "感谢您安装我的应用程序！",
    "Next steps:": "下一步:",
    "Connect the module to your local WiFi here!": "在这里连接模块到您的本地 WiFi！",
    "WiFi settings": "WiFi 设置",
    "Just trying this out in AP mode?": "只是在 AP 模式下尝试吗？",
    "To the controls!": "前往控制界面！",
    // update.htm
    "WLED Software Update": "WLED 软件更新",
    "Installed version:": "已安装版本：",
    "Download the latest binary:": "下载最新二进制文件：",
    "Update!": "更新！",
    "Back": "返回",
    "Updating...": "正在更新...",
    "Please do not close or refresh the page :)": "请不要关闭或刷新页面 :)",
    // setting_dmx.htm
    "Imma firin ma lazer (if it has DMX support)": "如果支持 DMX，则发射激光",
    "Change to something less-meme-related": "更改为与迷因无关的内容",
    "Proxy Universe": "代理 Universe",
    "from E1.31 to DMX (0=disabled)": "从 E1.31 到 DMX（0=禁用）",
    "This will disable the LED data output to DMX configurable below": "这将禁用下面可配置的 LED 数据输出到 DMX",
    "Number of fixtures is taken from LED config page": "灯具数量从 LED 配置页面获取",
    "Channels per fixture (15 max):": "每个灯具的通道数（最多15个）：",
    "Start channel:": "起始通道：",
    "Spacing between start channels:": "起始通道之间的间隔：",
    "info": "信息",
    "WARNING: Channel gap is lower than channels per fixture.": "警告：通道间隔小于每个灯具的通道数。",
    "This will cause overlap.": "这将导致重叠。",
    "DMX Map": "DMX 映射",
    "DMX fixtures start LED:": "DMX 灯具起始 LED：",
    "Channel functions": "通道功能",
    // setting_time.htm
    "Time setup": "时间设置",
    "Get time from NTP server:": "从 NTP 服务器获取时间：",
    "Use 24h format:": "使用24小时制：",
    "Time zone:": "时区：",
    "UTC offset:": "UTC 偏移：",
    "Current local time is": "当前本地时间是",
    "Latitude:": "纬度：",
    "Longitude:": "经度：",
    "Get location": "获取位置",
    "(opens new tab, only works in browser)": "(打开新标签页，仅在浏览器中有效)",
    "Clock": "时钟",
    "Analog Clock overlay:": "模拟时钟叠加：",
    "First LED:": "第一个 LED：",
    "Last LED:": "最后一个 LED：",
    "12h LED:": "12小时制 LED：",
    "Show 5min marks:": "显示5分钟标记：",
    "Seconds (as trail):": "秒钟（作为轨迹）：",
    "Countdown Mode:": "倒计时模式：",
    "Countdown Goal:": "倒计时目标：",
    "Date:": "日期：",
    "Time:": "时间：",
    "Macro presets": "宏预设",
    "Macros have moved!": "宏已移动！",
    "Presets now also can be used as macros to save both JSON and HTTP API commands.": "预设现在也可以用作宏，以保存 JSON 和 HTTP API 命令。",
    "Just enter the preset ID below!": "只需在下面输入预设 ID！",
    "Use 0 for the default action instead of a preset": "使用 0 作为默认操作，而不是预设",
    "Alexa On/Off Preset:": "Alexa 开/关预设：",
    "Countdown-Over Preset:": "倒计时结束预设：",
    "Timed-Light-Over Presets:": "定时灯效结束预设：",
    "Button actions": "按钮动作",
    "push switch": "按下开关",
    "short on->off": "短按开->关",
    "long off->on": "长按关->开",
    "double N/A": "双击 N/A",
    "Analog Button setup": "模拟按钮设置",
    "Time-controlled presets": "时间控制的预设",
    // setting_ui.htm
    "Web Setup": "Web 设置",
    "Server description:": "服务器描述：",
    "Sync button toggles both send and receive:": "同步按钮同时切换发送和接收：",
    "This firmware build does not include simplified UI support.": "此固件版本不包含简化的用户界面支持。",
    "Enable simplified UI:": "启用简化的用户界面：",
    "The following UI customization settings are unique both to the WLED device and this browser.": "以下用户界面定制设置同时适用于 WLED 设备和此浏览器。",
    "You will need to set them again if using a different browser, device or WLED IP address.": "如果使用不同的浏览器、设备或 WLED IP 地址，您需要重新设置它们。",
    "Refresh the main UI to apply changes.": "刷新主界面以应用更改。",
    "Loading settings...": "正在加载设置...",
    "UI Appearance": "用户界面外观",
    "I hate dark mode:": "我不喜欢暗黑模式：",
    "Why would you? ": "为什么呢？",
    "BG image URL": "背景图像 URL",
    "Random BG image": "随机背景图像",
    "Custom CSS:": "自定义 CSS：",
    "Holidays:": "节日：",
    "Clear local storage": "清除本地存储",
    "Show button labels": "显示按钮标签",
    "Color selection methods": "颜色选择方法",
    "Color Wheel": "颜色选择器",
    "RGB sliders": "RGB滑块",
    "Quick color selectors": "快速颜色选择器",
    "HEX color input": "HEX颜色输入",
    "Show bottom tab bar in PC mode": "在PC模式下显示底部选项卡栏",
    "Show preset IDs": "显示预设 ID",
    "Set segment length instead of stop LED": "设置段长度而不是停止 LED",
    "Hide segment power & brightness": "隐藏段功率和亮度",
    "Always expand first segment": "始终展开第一个段",
    "Enable custom CSS": "启用自定义 CSS",
    "Enable custom Holidays list": "启用自定义假期列表",
    "Background opacity": "背景不透明度",
    "Button opacity": "按钮不透明度",
    "BG image URL": "背景图片 URL",
    "Random BG image": "随机背景图片",
    "BG HEX color": "背景 HEX 颜色",
    // setting_pin.htm
    "Please enter settings PIN code": "请输入设置的PIN码",
    "Submit": "提交",
    // setting_dmx.htm
    "Save": "保存",
    "Configuration saved!": "配置已保存！",
    "Could not load configuration.": "无法加载配置。",
    "Usermod Setup": "用户模块设置",
    "Global I²C GPIOs (HW)": "全局 I²C GPIO（硬件）",
    "(only changable on ESP32, change requires reboot!)": "（仅适用于 ESP32，更改后需要重新启动！）",
    "SDA:": "SDA：",
    "SCL:": "SCL：",
    "Global SPI GPIOs (HW)": "全局 SPI GPIO（硬件）",
    "MOSI:": "MOSI：",
    "MISO:": "MISO：",
    "SCLK:": "SCLK：",
    "Loading settings...": "正在加载设置...",
    "Usermods configuration not found.": "找不到用户模块配置。",
    // setting_2D.htm
    "2D setup": "2D设置",
    "Strip or panel:": "带或面板：",
    "1D Strip": "1D带",
    "2D Matrix": "2D矩阵",
    "Matrix Generator": "矩阵生成器",
    ">": ">",
    "Panel dimensions (WxH):": "面板尺寸（宽x高）：",
    "Horizontal panels:": "水平面板：",
    "Vertical panels:": "垂直面板：",
    "1st panel:": "第一个面板：",
    "Top": "顶部",
    "Bottom": "底部",
    "Left": "左侧",
    "Right": "右侧",
    "Orientation:": "方向：",
    "Horizontal": "水平",
    "Vertical": "垂直",
    "Serpentine:": "蛇形：",
    "Pressing Populate will create LED panel layout with pre-arranged matrix.": "按下“Populate”将使用预先排列好的矩阵创建LED面板布局。",
    "Values above will not affect final layout.": "上述值不会影响最终布局。",
    "WARNING: You may need to update each panel parameters after they are generated.": "警告：生成面板后，您可能需要更新每个面板的参数。",
    "Populate": "填充",
    "Panel set-up": "面板设置",
    "Number of panels:": "面板数量：",
    "A matrix is made of 1 or more physical LED panels.": "一个矩阵由一个或多个物理LED面板组成。",
    "Each panel can be of different size and/or have different LED orientation and/or starting point and/or layout.": "每个面板的尺寸、LED方向、起始点和布局可以不同。",
    "LED panel layout": "LED面板布局",
    "Gap file:": "间隙文件：",
    "Upload": "上传",
    "Note: Gap file is a .json file containing an array with number of elements equal to the matrix size.": "注意：间隙文件是一个包含与矩阵尺寸相等数量元素的.json文件。",
    "A value of -1 means that pixel at that position is missing, a value of 0 means never paint that pixel, and 1 means regular pixel.": "值为-1表示该位置的像素丢失，值为0表示不要绘制该像素，值为1表示常规像素。",
    // usermod.htm
    "No usermod custom web page set.": "没有设置用户模块自定义网页。",
    // msg.htm
    "WLED Message": "WLED消息",
    "Sample Message.": "示例消息。",
    "Sample Detail.": "示例详情。",
    "WLED Live Preview": "WLED实时预览",
    // javescript.js
    "Start LED": "开始LED",
    "Stop LED": "停止LED",
    "Offset": "偏移",
    "Grouping": "分组",
    "Spacing": "间距",
    "Reverse direction": "反向",
    "Mirror effect": "镜像效果",
    "Add segment": "添加段",
    "Playlist": "播放列表",
    "Quick load label:": "快速加载标签：",
    "Overwrite with state": "用状态覆盖",
    "API commands": "API命令",
    "Save to ID": "保存到ID",
    "(leave empty for no Quick load button)": "（留空则不显示快速加载按钮）",
    // usermod_*.htm
    "language": "语言",
    "UseLocalTranslation": "使用翻译",
    "LanguageJsUrl": "语言翻译脚本",
    "Bluetooth": "蓝牙",
    "BleOpen": "打开蓝牙接口",

  };

  effectList = {
    "Solid": "纯色",
    "Blink": "闪烁",
    "Blink Rainbow": "彩虹闪烁",
    "Strobe": "频闪",
    "Strobe Rainbow": "彩虹频闪",
    "Wipe": "擦拭",
    "Sweep": "扫过",
    "Wipe Random": "随机擦拭",
    "Sweep Random": "随机扫过",
    "Random Colors": "随机颜色",
    "Dynamic": "动态",
    "Dynamic Smooth": "平滑动态",
    "Breathe": "呼吸",
    "Fade": "渐变",
    "Scan": "扫描",
    "Scan Dual": "双色扫描",
    "Colorloop": "彩虹循环",
    "Rainbow": "彩虹",
    "Theater": "剧院",
    "Theater Rainbow": "彩虹剧院",
    "Running Dual": "双色流动",
    "Running": "流动",
    "Saw": "锯齿",
    "Twinkle": "闪烁",
    "Dissolve": "溶解",
    "Dissolve Rnd": "随机溶解",
    "Sparkle": "闪烁",
    "Sparkle Dark": "暗闪烁",
    "Sparkle+": "闪烁+",
    "Strobe Mega": "超大频闪",
    "Android": "安卓",
    "Chase": "追逐",
    "Chase Random": "随机追逐",
    "Chase Rainbow": "彩虹追逐",
    "Rainbow Runner": "彩虹跑步",
    "Colorful": "多彩",
    "Traffic Light": "交通灯",
    "Chase Flash": "追逐闪烁",
    "Chase Flash Rnd": "随机追逐闪烁",
    "Chase 2": "追逐2",
    "Stream": "流动",
    "Scanner": "扫描",
    "Scanner Dual": "双色扫描",
    "Lighthouse": "灯塔",
    "Fireworks": "烟花",
    "Rain": "雨",
    "Fire Flicker": "火焰闪烁",
    "Gradient": "渐变",
    "Loading": "加载",
    "Police": "警灯",
    "Two Dots": "两点",
    "Fairy": "仙女",
    "Fairytwinkle": "仙女闪烁",
    "Chase 3": "追逐3",
    "ICU": "ICU",
    "Tri Wipe": "三色擦拭",
    "Tri Fade": "三色渐变",
    "Multi Comet": "多彩彗星",
    "Stream 2": "流动2",
    "Oscillate": "振荡",
    "Lightning": "闪电",
    "Pride 2015": "骄傲2015",
    "Juggle": "抛接",
    "Palette": "调色板",
    "Fire 2012": "火焰2012",
    "Colorwaves": "彩色波纹",
    "Bpm": "节拍",
    "Fill Noise": "填充噪声",
    "Noise 1": "噪声1",
    "Noise 2": "噪声2",
    "Noise 3": "噪声3",
    "Noise 4": "噪声4",
    "Colortwinkles": "彩色闪烁",
    "Lake": "湖泊",
    "Meteor": "流星",
    "Meteor Smooth": "平滑流星",
    "Railway": "铁路",
    "Ripple": "涟漪",
    "Ripple Rainbow": "彩虹涟漪",
    "Twinklefox": "闪烁狐狸",
    "Twinklecat": "闪烁猫",
    "Halloween Eyes": "万圣节眼睛",
    "Solid Pattern": "实心图案",
    "Solid Pattern Tri": "实心图案三角",
    "Spots": "斑点",
    "Spots Fade": "渐变斑点",
    "Bouncing Balls": "弹跳球",
    "Sinelon": "单色流动",
    "Sinelon Dual": "双色流动",
    "Sinelon Rainbow": "彩虹流动",
    "Glitter": "闪粉",
    "Solid Glitter": "实心闪粉",
    "Popcorn": "爆米花",
    "Candle": "蜡烛",
    "Candle Multi": "多彩蜡烛",
    "Fireworks Starburst": "烟花爆炸",
    "Fireworks 1D": "烟花1D",
    "Drip": "滴水",
    "Tetrix": "俄罗斯方块",
    "Plasma": "等离子",
    "Percent": "百分比",
    "Heartbeat": "心跳",
    "Pacifica": "太平洋",
    "Sunrise": "日出",
    "Phased": "相位",
    "Phased Noise": "相位噪声",
    "Twinkleup": "向上闪烁",
    "Noise Pal": "噪声调色板",
    "Sine": "正弦",
    "Flow": "流动",
    "Chunchun": "春春",
    "Dancing Shadows": "舞动阴影",
    "Washing Machine": "洗衣机",
    "Blends": "混合",
    "TV Simulator": "电视模拟器",
    "Aurora": "极光",
    "Perlin Move": "Perlin移动",
    "Wavesins": "波浪",
    "Flow Stripe": "流动条纹",
    "Black Hole": "黑洞",
    "Colored Bursts": "彩色爆发",
    "DNA": "DNA",
    "DNA Spiral": "螺旋DNA",
    "Drift": "漂移",
    "Firenoise": "火焰噪声",
    "Frizzles": "卷曲",
    "Game Of Life": "生命游戏",
    "Hiphotic": "催眠",
    "Julia": "茱莉亚",
    "Lissajous": "利萨如",
    "Matrix": "矩阵",
    "Metaballs": "元球",
    "Noise2D": "二维噪声",
    "Plasma Ball": "等离子球",
    "Polar Lights": "极光",
    "Pulser": "脉冲",
    "Sindots": "单色点",
    "Squared Swirl": "方形涡旋",
    "Sun Radiation": "太阳辐射",
    "Tartan": "苏格兰方格",
    "Spaceships": "太空飞船",
    "Crazy Bees": "疯狂蜜蜂",
    "Ghost Rider": "鬼骑士",
    "Blobs": "斑点",
    "Scrolling Text": "滚动文字",
    "Drift Rose": "漂移玫瑰",
    "Ripple Peak": "涟漪峰值",
    "Swirl": "涡旋",
    "Waverly": "波动",
    "Gravcenter": "重力中心",
    "Gravcentric": "重力中心",
    "Gravimeter": "重力计",
    "Juggles": "抛接",
    "Matripix": "矩阵",
    "Midnoise": "中噪声",
    "Noisefire": "噪声火焰",
    "Noisemeter": "噪声计",
    "Pixelwave": "像素波",
    "Plasmoid": "等离子体",
    "Puddlepeak": "水坑",
    "Puddles": "水坑",
    "Pixels": "像素",
    "Blurz": "模糊",
    "DJ Light": "DJ灯光",
    "Freqmap": "频率映射",
    "Freqmatrix": "频率矩阵",
    "Freqpixels": "频率像素",
    "Freqwave": "频率波",
    "Gravfreq": "重力频率",
    "Noisemove": "噪声移动",
    "Rocktaves": "摇滚波",
    "Waterfall": "瀑布",
    "GEQ": "GEQ",
    "Funky Plank": "时髦木板",
    "Akemi": "暁",
    "Distortion Waves": "扭曲波",
    "Soap": "肥皂",
  }


  var elements = document.querySelectorAll('*')
  // 替换子节点文本
  elements.forEach(function (element) {
    if (element) {
      for (var i = 0; i < element.childNodes.length; i++) {
        var node = element.childNodes[i];
        // 判断节点类型是否为文本节点
        let text = node.textContent.trim()
        if (node.nodeType === Node.TEXT_NODE && translations.hasOwnProperty(text) && node.childNodes.length <= 1) {
          // 替换文本内容
          node.textContent = translations[text]
        }
      }
    }
  })


  // 替换效果名称
  var elements = document.querySelectorAll("#fxlist > div > label > div > span")
  elements.forEach(function (element) {
    var match = element.textContent.match(/^[a-zA-Z0-9\s]+(?=\P{L})/u);
    var text = match ? match[0] : "";
    if (effectList.hasOwnProperty(text.trim()) && element.childNodes.length <= 1) {
      element.textContent = effectList[text.trim()] + element.textContent.replace(text, "");
    }
  });
  console.log("language hook sucess!")

}

console.log("language hook start!")
var count = 0;
var intervalId = setInterval(() => {
  translateText()
  count++;
  if (count === 10) {
    clearInterval(intervalId);
  }
}, 500);
