# 端口配置工具使用说明

## 项目概述

本项目是基于 Qt 5.12 的端口配置工具，用于搜索、连接、管理和配置 WQ/NPort 端口设备。主要功能包括设备搜索、网络配置、端口参数配置、端口工作模式配置、端口管理、设备管理和 Linux Real COM 端口映射管理。

## 系统要求

- 操作系统：Linux
- Qt 版本：Qt 5.12.10 或更高
- 依赖模块：Qt Widgets、Qt Network、Qt GUI
- NPort 驱动目录：`/usr/lib/wq_nport/driver`
- 配置文件：`/usr/lib/wq_nport/driver/wq_nportd.cf`

## 编译运行

```bash
qmake
make
./chuankou
```

如果需要执行端口映射相关操作，系统需要可用的提权工具：

```bash
which pkexec
# 或
which sudo
```

## 端口映射说明

端口映射功能通过驱动自带命令完成：

```bash
/usr/lib/wq_nport/driver/mxaddsvr
/usr/lib/wq_nport/driver/mxdelsvr
```

这些命令需要 root 权限，因为它们会修改 `/usr/lib/wq_nport/driver/wq_nportd.cf`，并创建或删除 `/dev/ttywXX`、`/dev/curXX` 等设备节点。

手工添加 16 口设备示例：

```bash
cd /usr/lib/wq_nport/driver
sudo ./mxaddsvr 192.168.5.123 16
```

`mxaddsvr` 成功后会自己通知 `wq_nportd`。如果守护进程已运行，它会发送：

```bash
kill -USR1 <wq_nportd_pid>
```

因此应用程序执行 `mxaddsvr/mxdelsvr` 成功后，不再额外发送 `SIGUSR1`，避免普通用户进程向 root 守护进程发信号失败，也避免重复弹出 `pkexec` 授权。

## 端口规则

端口计算规则必须与 `mxaddsvr.c` 保持一致。

默认起始数据端口为 `950`，默认起始命令端口为 `966`：

| 端口 | 数据端口 | 命令端口 |
| --- | ---: | ---: |
| 1 | 950 | 966 |
| 2 | 951 | 967 |
| 3 | 952 | 968 |
| 4 | 953 | 969 |
| 5 | 954 | 970 |
| 16 | 965 | 981 |

计算公式：

```text
dataPort = startPort + (portIndex - 1)
cmdPort  = startPort + 16 + (portIndex - 1)
```

不要使用 `cmdPort = dataPort + 1`。这种旧规则会生成错误的映射，例如 `950/951`，设备节点可能存在但端口通信不可用。

## 选择性添加限制

驱动命令 `mxaddsvr` 的参数语义是：

```bash
mxaddsvr <ip> <totalport> [data port] [cmd port]
```

它只能表示“从某个起始端口开始，连续添加 N 个端口”，不能表达任意端口组合。因此应用中的选择性添加只支持连续端口范围。

支持示例：

```text
1, 2, 3
4, 5, 6, 7
```

不支持示例：

```text
1, 3
2, 5, 6
```

## 常见问题

### 添加成功但提示 daemon reload failed

旧逻辑中，程序在 `mxaddsvr` 成功后又额外执行了一次：

```cpp
kill(pid, SIGUSR1);
```

如果 Qt 程序是普通用户运行，而 `wq_nportd` 是 root 运行，就会出现“向进程发送信号失败”。现在应用不再额外 reload，因为 `mxaddsvr/mxdelsvr` 已经负责通知守护进程。

### 设备节点存在但不能通信

优先检查端口配置是否正确：

```bash
cat /usr/lib/wq_nport/driver/wq_nportd.cf
```

正确的 16 口默认映射应为 `950-965` 对应 `966-981`。如果看到 `950/951`、`951/952` 这类配置，说明命令端口规则错误，需要删除后重新添加。

### 守护进程状态检查

```bash
ps -ef | grep wq_nportd | grep -v grep
```

手工通知守护进程重新加载：

```bash
sudo kill -USR1 $(pgrep -x wq_nportd)
```

## 注意事项

1. 端口映射操作涉及系统目录和设备节点，需要 root 权限。
2. 删除或重新添加映射前，建议备份 `wq_nportd.cf`。
3. 固件升级、恢复出厂设置、重启设备等操作执行前应确认网络连接稳定。
4. 如果修改了映射规则，必须重新编译并在 Linux 目标环境验证。
