# NPort 配置管理模块

该模块负责读取、展示和调用驱动命令维护 NPort Real COM 映射配置。

## 配置文件

当前驱动配置文件路径：

```text
/usr/lib/wq_nport/driver/wq_nportd.cf
```

模块通过驱动命令修改配置，不直接手写新增映射：

```text
/usr/lib/wq_nport/driver/mxaddsvr
/usr/lib/wq_nport/driver/mxdelsvr
```

## 权限模型

`mxaddsvr` 和 `mxdelsvr` 需要 root 权限，因为它们会修改驱动目录下的配置文件并维护 `/dev` 设备节点。Qt 程序应通过 `pkexec` 或 `sudo` 执行这些命令。

`mxaddsvr/mxdelsvr` 内部会负责通知 `wq_nportd` 重新加载配置。模块在命令执行成功后只重新读取配置文件，不再额外调用 `signalDaemonReload()`。

## 端口计算

端口规则与 `mxaddsvr.c` 保持一致：

```text
dataPort = startPort + (portIndex - 1)
cmdPort  = startPort + 16 + (portIndex - 1)
```

默认 `startPort=950` 时：

```text
端口1: 950 / 966
端口2: 951 / 967
端口3: 952 / 968
...
端口16: 965 / 981
```

不要使用 `cmdPort = dataPort + 1`。

## 选择性添加

`mxaddsvr` 只支持“起始端口 + 连续数量”，不支持任意端口组合。因此选择性添加只允许连续端口。

允许：

```text
1,2,3
4,5,6,7
```

不允许：

```text
1,3
2,5,6
```

## 目录结构

```text
src/config/
  controllers/  业务控制和驱动命令调用
  models/       配置文件数据模型
  utils/        输入校验、端口计算等工具
  views/        配置管理界面
```
