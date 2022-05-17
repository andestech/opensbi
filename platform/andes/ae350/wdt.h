// See LICENSE for license details.

#ifndef _AE350_WDT_H
#define _AE350_WDT_H

#define ATCWDT200_WP_NUM 0x5aa5
#define WREN_OFF	0x18
#define CTRL_OFF	0x10
#define RST_TIME_OFF	8
#define RST_TIME_MSK	(0x3 << RST_TIME_OFF)
#define RST_CLK_128		(0   << RST_TIME_OFF)
#define RST_CLK_256		(1   << RST_TIME_OFF)
#define RST_CLK_512		(2   << RST_TIME_OFF)
#define RST_CLK_1024	(3   << RST_TIME_OFF)
#define INT_TIME_OFF	4
#define INT_TIME_MSK	(0xf << INT_TIME_OFF)
#define INT_CLK_64		(0   << INT_TIME_OFF)
#define INT_CLK_256		(1   << INT_TIME_OFF)
#define INT_CLK_1024	(2   << INT_TIME_OFF)
#define INT_CLK_2048	(3   << INT_TIME_OFF)
#define INT_CLK_4096	(4   << INT_TIME_OFF)
#define INT_CLK_8192	(5   << INT_TIME_OFF)
#define INT_CLK_16384	(6   << INT_TIME_OFF)
#define INT_CLK_32768	(7   << INT_TIME_OFF)
#define RST_EN			(1 << 3)
#define INT_EN			(1 << 2)
#define CLK_PCLK		(1 << 1)
#define WDT_EN			(1 << 0)

struct wdt_data {
	unsigned long addr;
};

#endif /* _AE350_WDT_H */
