#ifndef __MYIRQ_H__
#define __MYIRQ_H__

struct gpio_key{
	int gpio;
	struct gpio_desc *gpiod;
	int flag;
	int irq;
} ;

#endif