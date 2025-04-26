正点原子的《STM32MP1嵌入式Linux驱动开发指南v2.1》第二十五章 pinctrl和gpio子系统，很适合小白，可以从头看。我就是跟着他的思路来的：

# pinctrl 子系统

## pinctrl 的设备树配置

arhc/arm/boot/dts/stm32mp151.dtsi 文件包含 pinctrl 相关内容如下：

```dts
/*
 * Break node order to solve dependency probe issue between
 * pinctrl and exti.
 */
 /*
 DT标准规范0.4版本：@50002000 必须与其第一个reg属性相同，否则忽略。
 pinctrl：节点标签，用于其他节点通过 &pinctrl 引用此节点。
 pin-controller@50002000：节点名称，表示这是一个引脚控制器，物理基地址为 0x50002000。
 */
pinctrl: pin-controller@50002000 { 
	/* 1. 地址与大小单元定义
		#address-cells：子节点中 reg 属性的地址字段占 1个u32。
		#size-cells：子节点中 reg 属性的大小字段占 1个u32。
		例如：子节点中 reg = <0x00 0x400> 表示从基地址偏移 0x00 开始，长度为 0x400。
	*/
	#address-cells = <1>;
	#size-cells = <1>;
	/* 2. 兼容性标识：
		绑定到驱动程序的表示ID字符串，平台总线会于匹配 */
	compatible = "st,stm32mp157-pinctrl"; 
	/* 3. 地址映射：
		定义子节点地址到父地址空间的映射：子地址空间起始：0，父地址空间起始：0x50002000，映射长度：0xA400（即 41,984 字节）。
		表示子节点的寄存器地址相对于 0x50002000 进行偏移解析。覆盖PA0~PK7范围引脚。 */
	ranges = <0 0x50002000 0xa400>; 
	/* 4. 中断控制器关联：
	指定中断的父控制器为 exti（外部中断控制器）。引脚产生的中断信号将通过EXTI控制器传递到中断子系统。*/
	interrupt-parent = <&exti>;  
    /* 5. ST专属系统配置
    	STM32特有属性，用于配置EXTI控制器与引脚的关系：
			&exti：指向EXTI控制器节点。
			0x60：EXTI寄存器组中与引脚配置相关的偏移量。
			0xff：掩码，用于过滤或配置特定寄存器位。
    */
	st,syscfg = <&exti 0x60 0xff>;
	/* 6. 硬件锁配置
		使用硬件信号量（HSEM）保护对引脚控制器的并发访问：
			&hsem：指向硬件信号量控制器。
			0：使用HSEM的第0个实例。
			1：使用信号量ID 1，确保原子操作。
    */
	hwlocks = <&hsem 0 1>;
	/* 7. 引脚编号模式 */
	pins-are-numbered; /* 表示引脚按数字编号而非名称引用,简化驱动对引脚的访问，无需解析符号名称。 */

	gpioa: gpio@50002000 {
		gpio-controller;
		#gpio-cells = <2>;
		interrupt-controller;
		#interrupt-cells = <2>;
		reg = <0x0 0x400>;
		clocks = <&rcc GPIOA>;
		st,bank-name = "GPIOA";
		status = "disabled";
	};

	gpiob: gpio@50003000 {
		gpio-controller;
		#gpio-cells = <2>;
		interrupt-controller;
		#interrupt-cells = <2>;
		reg = <0x1000 0x400>;
		clocks = <&rcc GPIOB>;
		st,bank-name = "GPIOB";
		status = "disabled";
	};
	/* 省略其他引脚节点到gpiok */

};

pinctrl_z: pin-controller-z@54004000 {
	#address-cells = <1>;
	#size-cells = <1>;
	compatible = "st,stm32mp157-z-pinctrl";
	ranges = <0 0x54004000 0x400>;
	pins-are-numbered;
	interrupt-parent = <&exti>;
	st,syscfg = <&exti 0x60 0xff>;
	hwlocks = <&hsem 0 1>;

	gpioz: gpio@54004000 {
		gpio-controller;
		#gpio-cells = <2>;
		interrupt-controller;
		#interrupt-cells = <2>;
		reg = <0 0x400>;
		clocks = <&scmi0_clk CK_SCMI0_GPIOZ>;
		st,bank-name = "GPIOZ";
		st,bank-ioport = <11>;
		status = "disabled";
	};
};
```

此节点定义了STM32MP157的引脚控制器：

- 物理基地址为 `0x50002000`，地址空间长度 `0xA400`。
- 通过EXTI控制器处理中断，使用硬件信号量防止并发冲突。
- 绑定到ST专属驱动，支持按数字管理引脚配置。

典型应用场景：在设备树子节点中定义具体引脚功能（如UART、I2C复用），例如：

```
&pinctrl {
    uart4_pins: uart4-0 {
        pins {
            pinmux = <STM32_PINMUX('G', 11, AF6)>; // PG11复用为UART4_TX
            bias-disable;
            drive-push-pull;
        };
    };
};
```

**pin-controlle 节点看起来，根本没有 PIN 相关的配置，别急！去打开 stm32mp15-pinctrl.dtsi 文件，你们能找到如下内容：**

```dts
/* 这个文件里全是外设模块的引脚配置信息，ADC/IIC/SPI/SAI/sdmmc/…… 全部的片上外设配置 */

#include <dt-bindings/pinctrl/stm32-pinfunc.h>
&pinctrl {
	m_can1_pins_a: m-can1-0 {
		pins1 {
			pinmux = <STM32_PINMUX('H', 13, AF9)>; /* 设定 CAN1_TX 的引脚编号及复用模式 */
			slew-rate = <1>;
			drive-push-pull;
			bias-disable;
		};
		pins2 {
			pinmux = <STM32_PINMUX('I', 9, AF9)>; /* CAN1_RX */
			bias-disable;
		};
	};

	pwm1_pins_a: pwm1-0 {
		pins {
			pinmux = <STM32_PINMUX('E', 9, AF1)>, /* TIM1_CH1 */
				 <STM32_PINMUX('E', 11, AF1)>, /* TIM1_CH2 */
				 <STM32_PINMUX('E', 14, AF1)>; /* TIM1_CH4 */
			bias-pull-down;
			drive-push-pull;
			slew-rate = <0>;
		};
	};
	
	spi4_pins_a: spi4-0 {
		pins1 {
			pinmux = <STM32_PINMUX('E', 12, AF5)>, /* SPI4_SCK */
				 <STM32_PINMUX('E', 14, AF5)>; /* SPI4_MOSI */
			bias-disable;    /* 禁止使用內部偏置电压 */
			drive-push-pull; /* 推挽输出  */
			slew-rate = <1>; /* 设置引脚速度：枚举 <数值>：0/1/2/3,3最高 */
		};

		pins2 {
			pinmux = <STM32_PINMUX('E', 13, AF5)>; /* SPI4_MISO */
			bias-disable; 
		};
	};
};
```

绑定文档 Documentation/devicetree/bindings/pinctrl/st,stm32-pinctrl.yaml 描述了如何在设备树中设置 STM32 的 PIN 信息。

**`stm32mp15-pinctrl.dtsi`** 文件里描述的是所有外设模块的引脚配置信息，是对**`stm32mp151.dtsi`**文件内容的逻辑追加。这些配置信息解析如下：

1. **pinmux 属性 必选**

   此属性设定外设模块所需的全部 IO 引脚编号和复用模式。**必须配置的属性**。

   **STM32_PINMUX** 这宏来配置引脚坐标和引脚的复用功能，此宏定义在include/dt-bindings/pinctrl/stm32-pinfunc.h 文件里面，内容如下：

   ```
    #define PIN_NO(port, line)   (((port) - 'A') * 0x10 + (line))
    #define STM32_PINMUX(port, line, mode) (((PIN_NO(port, line)) << 8)  | (mode)) 
   ```

   例如：pinmux = <STM32_PINMUX('H', 13, AF9)>; 

   **port**：表示用那一组 GPIO(例：H 表示为 GPIO 第 H 组，也就是 GPIOH)。 
   **line**：表示这组 GPIO 的第几个引脚(例：13 表示为 GPIOH_13，也就是 PH13)。 
   **mode**：表示当前引脚要做那种复用功能(例：AF9 表示为用第 9 个复用功能)，这个需要查阅 STM32MP1 数据手册来确定使用哪个复用功能。打开《STM32MP157A&D 数据手册》，一个IO 最大有 16 种复用方法：AF0~AF15，打开第 4 章“Pinouts, pin description and alternate functions”。stm32-pinfunc.h 文件里面定义了一个 PIN  的所有配置项 AF0~AF9。

   tips：此文件中将所有外设的引脚模式都描述出来了，肯定会有引脚复用，但只在运行时保证引脚不被同时占用即可。

2. **电气属性 可选**

   此类属性在pinctrl里不是必须的，可以配置，也可以不配置。stm32-pinctrl.yaml 文件里面也描述了如何设置STM32 的电气属性：

   | 电气特性属性     | 类型     | 作用                                        |
   | ---------------- | -------- | ------------------------------------------- |
   | bias-disable     | bootlean | 禁止使用內部偏置电压                        |
   | bias-pull-down   | bootlean | 内部下拉                                    |
   | bias-pull-up     | bootlean | 内部上拉                                    |
   | drive-push-pull  | bootlean | 推挽输出                                    |
   | drive-open-drain | bootlean | 开漏输出                                    |
   | output-low       | bootlean | 输出低电平                                  |
   | output-high      | bootlean | 输出高电平                                  |
   | slew-rate        | enum     | 引脚的速度，可设置：0~3，  0 最慢，3 最高。 |



## stm32的 pin 驱动程序

所有的东西都已经准备好了，包括寄存器地址和寄存器值，Linux 内核相应的驱动文件就会根据这些值来做相应的初始化。接下来就找一下哪个驱动文件来做这一件事情，pinctrl 节点中 compatible 属性的值为“st,stm32mp157-pinctrl”，在 Linux 内核中全局搜索“pinctrl”字符串就会找到对应的驱动文件。在文件 drivers/pinctrl/stm32/pinctrl-stm32mp157.c 中有如下内容： 

```c

static struct stm32_pinctrl_match_data stm32mp157_match_data = {
	.pins = stm32mp157_pins,
	.npins = ARRAY_SIZE(stm32mp157_pins),
};

static struct stm32_pinctrl_match_data stm32mp157_z_match_data = {
	.pins = stm32mp157_z_pins,
	.npins = ARRAY_SIZE(stm32mp157_z_pins),
	.pin_base_shift = STM32MP157_Z_BASE_SHIFT,
};

static const struct of_device_id stm32mp157_pctrl_match[] = {
	{
		.compatible = "st,stm32mp157-pinctrl",
		.data = &stm32mp157_match_data,
	},
	{
		.compatible = "st,stm32mp157-z-pinctrl",
		.data = &stm32mp157_z_match_data,
	},
	{ }
};

static const struct dev_pm_ops stm32_pinctrl_dev_pm_ops = {
	 SET_LATE_SYSTEM_SLEEP_PM_OPS(NULL, stm32_pinctrl_resume)
};

static struct platform_driver stm32mp157_pinctrl_driver = {
	.probe = stm32_pctl_probe,
	.driver = {
		.name = "stm32mp157-pinctrl",
		.of_match_table = stm32mp157_pctrl_match,
		.pm = &stm32_pinctrl_dev_pm_ops,
	},
};

static int __init stm32mp157_pinctrl_init(void)
{
	return platform_driver_register(&stm32mp157_pinctrl_driver);
}
arch_initcall(stm32mp157_pinctrl_init);
```

上面代码，主要向内核中注册了一个平台驱动：stm32mp157_pinctrl_driver。匹配成功后，会调用probe函数：stm32_pctl_probe函数，它被定义在 drivers/pinctrl/stm32/pinctrl-stm32.c 文件中，这里给他复制过来：

```c
int stm32_pctl_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct device_node *child;
	const struct of_device_id *match;
	struct device *dev = &pdev->dev;
	struct stm32_pinctrl *pctl;
	struct pinctrl_pin_desc *pins;
	int i, ret, hwlock_id, banks = 0;

	if (!np)
		return -EINVAL;

	match = of_match_device(dev->driver->of_match_table, dev);
	if (!match || !match->data)
		return -EINVAL;

	if (!of_find_property(np, "pins-are-numbered", NULL)) {
		dev_err(dev, "only support pins-are-numbered format\n");
		return -EINVAL;
	}

	pctl = devm_kzalloc(dev, sizeof(*pctl), GFP_KERNEL);
	if (!pctl)
		return -ENOMEM;

	platform_set_drvdata(pdev, pctl);

	/* check for IRQ controller (may require deferred probe) */
	pctl->domain = stm32_pctrl_get_irq_domain(np);
	if (IS_ERR(pctl->domain))
		return PTR_ERR(pctl->domain);

	/* hwspinlock is optional */
	hwlock_id = of_hwspin_lock_get_id(pdev->dev.of_node, 0);
	if (hwlock_id < 0) {
		if (hwlock_id == -EPROBE_DEFER)
			return hwlock_id;
	} else {
		pctl->hwlock = hwspin_lock_request_specific(hwlock_id);
	}

	spin_lock_init(&pctl->irqmux_lock);

	pctl->dev = dev;
	pctl->match_data = match->data;

	/*  get package information */
	stm32_pctl_get_package(np, pctl);

	pctl->pins = devm_kcalloc(pctl->dev, pctl->match_data->npins,
				  sizeof(*pctl->pins), GFP_KERNEL);
	if (!pctl->pins)
		return -ENOMEM;

	ret = stm32_pctrl_create_pins_tab(pctl, pctl->pins);
	if (ret)
		return ret;

	ret = stm32_pctrl_build_state(pdev);
	if (ret) {
		dev_err(dev, "build state failed: %d\n", ret);
		return -EINVAL;
	}

	if (pctl->domain) {
		ret = stm32_pctrl_dt_setup_irq(pdev, pctl);
		if (ret)
			return ret;
	}

	pins = devm_kcalloc(&pdev->dev, pctl->npins, sizeof(*pins),
			    GFP_KERNEL);
	if (!pins)
		return -ENOMEM;

	for (i = 0; i < pctl->npins; i++)
		pins[i] = pctl->pins[i].pin;

	pctl->pctl_desc.name = dev_name(&pdev->dev);
	pctl->pctl_desc.owner = THIS_MODULE;
	pctl->pctl_desc.pins = pins;
	pctl->pctl_desc.npins = pctl->npins;
	pctl->pctl_desc.link_consumers = true;
	pctl->pctl_desc.confops = &stm32_pconf_ops;  //构建操作函数，注入pinctrl子系统
	pctl->pctl_desc.pctlops = &stm32_pctrl_ops;  //构建操作函数，注入pinctrl子系统
	pctl->pctl_desc.pmxops = &stm32_pmx_ops;     //构建操作函数，注入pinctrl子系统
	pctl->dev = &pdev->dev;
	pctl->pin_base_shift = pctl->match_data->pin_base_shift;

	pctl->pctl_dev = devm_pinctrl_register(&pdev->dev, &pctl->pctl_desc,
					       pctl);

	if (IS_ERR(pctl->pctl_dev)) {
		dev_err(&pdev->dev, "Failed pinctrl registration\n");
		return PTR_ERR(pctl->pctl_dev);
	}

	for_each_available_child_of_node(np, child)
		if (of_property_read_bool(child, "gpio-controller"))
			banks++;

	if (!banks) {
		dev_err(dev, "at least one GPIO bank is required\n");
		return -EINVAL;
	}
	pctl->banks = devm_kcalloc(dev, banks, sizeof(*pctl->banks),
			GFP_KERNEL);
	if (!pctl->banks)
		return -ENOMEM;

	i = 0;
	for_each_available_child_of_node(np, child) {
		struct stm32_gpio_bank *bank = &pctl->banks[i];

		if (of_property_read_bool(child, "gpio-controller")) {
			bank->rstc = of_reset_control_get_exclusive(child,
								    NULL);
			if (PTR_ERR(bank->rstc) == -EPROBE_DEFER)
				return -EPROBE_DEFER;

			bank->clk = of_clk_get_by_name(child, NULL);
			if (IS_ERR(bank->clk)) {
				if (PTR_ERR(bank->clk) != -EPROBE_DEFER)
					dev_err(dev,
						"failed to get clk (%ld)\n",
						PTR_ERR(bank->clk));
				return PTR_ERR(bank->clk);
			}
			i++;
		}
	}

	for_each_available_child_of_node(np, child) {
		if (of_property_read_bool(child, "gpio-controller")) {
			ret = stm32_gpiolib_register_bank(pctl, child);
			if (ret) {
				of_node_put(child);
				return ret;
			}

			pctl->nbanks++;
		}
	}

	dev_info(dev, "Pinctrl STM32 initialized\n");

	return 0;
}
```

这个平台设备模型，就是一个主动探测匹配机制，若成功则调用probe函数。之前跟字符设备驱动关联的时候，在probe函数里注册字符设备驱动等相关操作；现在是跟pinctrl子系统关联，类似，这里是准备好初始化pinctrl所需的各种实现，然后注册到pinctrl子系统。

stm32_pctl_probe 函数是STM32引脚控制器的核心初始化函数，负责整合设备树中的配置，注册到内核的pinctrl和GPIO子系统，管理硬件资源，确保引脚功能正确配置。



## pinctrl子系统内核代码分析

[todo]



# GPIO 子系统

## 设备树中的 gpio 信息

首先肯定是 GPIO 控制器的节点信息，以 PI0 这个引脚所在的 GPIOI 为例，打开 stm32mp151.dtsi，在里面找到如下所示内容：

```dts
  pinctrl: pin-controller@50002000 { 
      #address-cells = <1>; 
      #size-cells = <1>; 
      compatible = "st,stm32mp157-pinctrl"; 
 
      gpioi: gpio@5000a000 {  
          gpio-controller;  /* 必选属性，表示此节点是 GPIO controller */
          #gpio-cells = <2>; 
          interrupt-controller; 
          #interrupt-cells = <2>; 
          reg = <0x8000 0x400>; /* 0X50002000+0X800=0X5000A000 */
          clocks = <&rcc GPIOI>; 
          st,bank-name = "GPIOI"; 
          status = "disabled"; 
      }; 
  }; 
```

gpioi: gpio@5000a000 就是 GPIOI 的控制器信息，属于 pincrtl 的子节点。且对于 STM32MP1而言，pinctrl 和 gpio 这两个子系统的驱动文件是一样的，都为 pinctrl-stm32mp157.c，所以在注册 pinctrl 驱 动 顺 便 会 把 gpio 驱 动 程 序 一 起 注 册 。 绑 定 文 档Documentation/devicetree/bindings/gpio/gpio.txt 详细描述了 gpio 控制器节点各个属性信息。

## stm32的 GPIO 驱动代码

也是 pinctrl-stm32mp157.c 文件中的 stm32_pctl_probe 函数。

```c
int stm32_pctl_probe(struct platform_device *pdev)
{	
	//……
	for_each_available_child_of_node(np, child) {
		if (of_property_read_bool(child, "gpio-controller")) { /* 判断设备树节点，是否有 gpio-controller , 此节点就是 GPIO 控制器节点 */
			ret = stm32_gpiolib_register_bank(pctl, child);  /* 注册到 GPIO 子系统： gpiolib  */
			if (ret) {
				of_node_put(child);
				return ret;
			}

			pctl->nbanks++;
		}
	}
	//……
}
```

## gpio 子系统内核代码分析

[todo]



## gpio 子系统 API 函数 

对于驱动开发人员，设置好设备树以后就可以使用 gpio 子系统提供的 API 函数来操作指定的 GPIO，gpio 子系统向驱动开发人员屏蔽了具体的读写寄存器过程。这就是驱动分层与分离的好处，大家各司其职，做好自己的本职工作即可。gpio 子系统提供的常用的 API 函数有下面几个： 

**gpio_request 函数**

用于申请一个 GPIO 管脚。用前必须先申请，函数原型如下： 
int gpio_request(unsigned gpio,    const char *label) 

- gpio：要申请的 gpio 标号，使用 of_get_named_gpio 函数从设备树获取指定 GPIO 属性信息，此函数会返回这个 GPIO 的标号。 
- label：给 gpio 设置个名字。 
- 返回值：0，申请成功；其他值，申请失败。 

**gpio_free 函数**

void gpio_free(unsigned gpio) 

释放不用的 GPIO 资源。

- gpio：要释放的 gpio 标号。

**gpio_direction_input 函数**

int gpio_direction_input(unsigned gpio) 

设置某个 GPIO 为输入。 

- gpio：要设置为输入的 GPIO 标号。 
- 返回值：0，设置成功；负值，设置失败。

**gpio_direction_output 函数** 
此函数用于设置某个 GPIO 为输出，并且设置默认输出值，函数原型如下： 
int gpio_direction_output(unsigned gpio, int value) 
  函数参数和返回值含义如下： 
  gpio：要设置为输出的 GPIO 标号。 
  value：GPIO 默认输出值。 
  返回值：0，设置成功；负值，设置失败。 

**gpio_get_value 函数** 
  gpio：要获取的 GPIO 标号。 
  返回值：非负值，得到的 GPIO 值；负值，获取失败。 

**gpio_set_value 函数** 
此函数用于设置某个 GPIO 的值
gpio：要设置的 GPIO 标号。 
value：要设置的值。 
返回值：无 





