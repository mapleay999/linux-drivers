`devm_gpiod_get_index` 和 `devm_gpiod_get` 是 Linux 内核 GPIO 子系统中用于获取 GPIO 描述符的 API，它们的核心区别在于 **GPIO 属性的索引处理方式**，以下是详细对比：

---

### **功能对比**
| 函数                   | 适用场景                         | 设备树属性名要求                 | 索引处理                      |
| ---------------------- | -------------------------------- | -------------------------------- | ----------------------------- |
| `devm_gpiod_get`       | 单个 GPIO 引脚（无需索引）       | 属性名必须唯一（如 `led-gpio`）  | 隐式使用索引 **0**            |
| `devm_gpiod_get_index` | 多个 GPIO 引脚（需通过索引区分） | 属性名支持复数形式（如 `gpios`） | 显式指定索引（如 0, 1, 2...） |

---

### **代码原型对比**
```c
// 获取单个 GPIO（隐式索引 0）
struct gpio_desc *devm_gpiod_get(
    struct device *dev,
    const char *con_id,   // 设备树属性名（如 "led" 对应 `led-gpios`）
    enum gpiod_flags flags
);

// 通过索引获取 GPIO
struct gpio_desc *devm_gpiod_get_index(
    struct device *dev,
    const char *con_id,   // 设备树属性名（如 "key" 对应 `key-gpios`）
    unsigned int idx,     // 索引号（从 0 开始）
    enum gpiod_flags flags
);
```

---

### **设备树属性示例**
#### 1. **单引脚场景**（适合 `devm_gpiod_get`）
```dts
led {
    compatible = "my-led-driver";
    led-gpio = <&gpioh 10 GPIO_ACTIVE_HIGH>; // 属性名用单数形式
};
```
驱动代码：
```c
struct gpio_desc *led = devm_gpiod_get(dev, "led", GPIOD_OUT_HIGH);
```

#### 2. **多引脚场景**（适合 `devm_gpiod_get_index`）
```dts
keys {
    compatible = "my-key-driver";
    key-gpios = <&gpiog 3 GPIO_ACTIVE_LOW>,  // 索引 0
                <&gpioh 7 GPIO_ACTIVE_LOW>; // 索引 1
};
```
驱动代码：
```c
// 获取第一个按键（索引 0）
struct gpio_desc *key0 = devm_gpiod_get_index(dev, "key", 0, GPIOD_IN);

// 获取第二个按键（索引 1）
struct gpio_desc *key1 = devm_gpiod_get_index(dev, "key", 1, GPIOD_IN);
```

---

### **底层实现关系**
`devm_gpiod_get` 本质是 `devm_gpiod_get_index` 的简化版：
```c
// 内核源码实现
static inline struct gpio_desc *devm_gpiod_get(
    struct device *dev,
    const char *con_id,
    enum gpiod_flags flags
) {
    return devm_gpiod_get_index(dev, con_id, 0, flags); // 索引固定为 0
}
```

---

### **关键选择依据**
| 条件                         | 推荐函数               | 原因                                            |
| ---------------------------- | ---------------------- | ----------------------------------------------- |
| 设备树属性名以 `-gpios` 结尾 | `devm_gpiod_get_index` | 兼容多 GPIO 配置（如 `key-gpios` 包含多个引脚） |
| 设备树属性名以 `-gpio` 结尾  | `devm_gpiod_get`       | 单 GPIO 配置（如 `led-gpio`）                   |
| 需要明确区分多个 GPIO        | `devm_gpiod_get_index` | 通过索引精准控制（如按键阵列、多路 LED 控制）   |

---

### **调试技巧**
1. **检查设备树绑定状态**：
   ```bash
   # 查看 GPIO 映射状态
   cat /sys/kernel/debug/gpio
   ```

2. **验证 GPIO 属性解析**：
   ```bash
   # 查看设备树节点属性
   dtc -I fs /sys/firmware/devicetree/base | grep "key-gpios"
   ```

3. **内核日志监控**：
   ```bash
   dmesg | grep gpiod_
   ```

---

通过合理选择这两个 API，可以更安全、高效地管理 GPIO 资源，同时保持代码与设备树的强一致性。