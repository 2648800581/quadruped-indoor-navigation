# 机器人 SDK 占位符

此目录应包含专有的机器人 SDK 文件。

## 所需文件

在编译前，请将以下文件放置在此处：

```
mc_sdk_lib/
├── lib/
│   ├── libmc_sdk_aarch64.so  # ARM64 SDK 库
│   └── libmc_sdk_x86_64.so   # x86_64 SDK 库
└── include/
    └── highlevel.h            # SDK 头文件
```

## 说明

由于专有许可限制，SDK 文件未包含在此仓库中。
请从机器人制造商处获取 SDK。
