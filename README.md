# ESP32-Deauther
![GitHub Issues or Pull Requests](https://img.shields.io/github/issues/tesa-klebeband/ESP32-Deauther)
![GitHub License](https://img.shields.io/github/license/tesa-klebeband/ESP32-Deauther)
![GitHub Repo stars](https://img.shields.io/github/stars/tesa-klebeband/ESP32-Deauther?style=flat)
![GitHub forks](https://img.shields.io/github/forks/tesa-klebeband/ESP32-Deauther?style=flat)
![logo](https://github.com/user-attachments/assets/4e2ac65f-1b25-4a97-822a-6a91ca71b5be)

A powerful WiFi deauther based on ESP32 board

ORIGIN: [here](https://github.com/tesa-klebeband/ESP32-Deauther)

Developing Environment: VS Code+Platformio

Board: ESP32-C3 SuperMini

## 对于原项目的修改：
* 关闭了原项目对于LED的控制(换掉了原先定义的LED Pin)，但代码未删除，可以自己改回来；同时，加入AP MAC随机化+对于周围WiFi的被动扫描
* `main.cpp`: 添加了新的LED控制，具体来说，刷入固件后给板子上电，待Web界面准备完毕后LED会blink一下(duration 100ms); 添加了关于Boot按钮的控制，按下Boot按钮LED也会blink~; 同时，现在**板子在开机时会先扫描一遍周围网络(passive)，这样在Web界面里面就不用再按Rescan按钮啦**.限制了最大发射功率到12.5dBm
* `web_interface.cpp`:更加优雅的UI，添加主芯片温度显示，添加DEAUTH ALL和StopAP按钮，控制更方便.添加自定义Reason Code输入框
* `deauth.cpp`:改动最大，*Grok3*重写了Deauth All的代码，实现了真正的踢掉周围所有设备的网络(逻辑来源：[here](https://github.com/zRCrackiiN/DeauthKeychain)). 注：Deauth All时，板子发出的AP将会断开
* `Others`: 小改动，懒得统计啦

> [!NOTE]
> 板子运行时，芯片温度将会很高！(70°C+/158°F+) 请注意不要烫到手

> 所有代码在ESP32-C3 SuperMini上测试通过，应该可以在所有搭载ESP32-C3主控的板子上运行

> [!WARNING]
> 技术本无罪，善恶在人心！在中国等许多国家，非法入侵、干扰他人信号属于违法行为！本项目所有测试在内网进行，仅供学习研究目的，请妥善使用项目，若使用此项目造成损失乃至违法，造成的一切后果与作者无关（若不同意此协议，请勿使用本项目）

ps:尊重原项目作者，本项目也使用GNU v3 License


以下为原项目README.md内容：

A project for the ESP32 that allows you to deauthenticate stations connected to WiFi network
# DISCLAIMER
This tool has been made for educational and testing purposes only. Any misuse or illegal activities conducted with the tool are strictly prohibited. I am **not** responsible for any consequences arising from the use of the tool, which is done at your own risk.
## Building
Clone this repo using:

`git clone https://github.com/tesa-klebeband/ESP32-Deauther`

1) Install vscode and PlatformIO
2) Install the PlatformIO extension in vscode
3) Open the cloned folder in vscode and press the upload button

## Installing directly from your Browser
Open my [ESP32-Webflasher](https://tesa-klebeband.github.io/ESP32-Webflasher) in Chrome or Edge, select `ESP32-Deauther` and follow the listed instructions

## Using ESP32-Deauther
The ESP32 hosts a WiFi network with the name of `ESP32-Deauther` and a password of `esp32wroom32`. Connect to this network and type the IP of your ESP32 (typically **192.168.4.1**) into a webbrowser of a choice. You will see the following options:
* Rescan networks: Rescan and detect all WiFi networks in your area. After a successful scan, the networks are listed in the above table.
* Launch Deauth-Attack: Deauthenticates all clients connected to a network. Enter the network number from the table at the top and a reason code from the table at the bottom of the page. After that click the button and your ESP32's LED will flash when it deauthenticates a station.
* Deauth all networks: Launches a Deauth-Attack on all networks and stations with a specific reason. In order to stop this, you have to reset your ESP32 (no other way to code this since the ESP32 rapidly changes through all network channels and has to disable its AP)
* Stop Deauth-Attack: Stops an ongoing deauthentication attack
## License
All files within this repo are released under the GNU GPL V3 License as per the LICENSE file stored in the root of this repo.
