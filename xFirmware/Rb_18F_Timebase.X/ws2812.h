#ifndef WS2812_H
#define	WS2812_H

// the WS2812 wants bits in the order of:
// R[7..0] G[7..0] B[7..0]
// i.e. R7 is the first bit shifted out

#define WS2812_RED    (0x000000fc)
#define WS2812_ORANGE (0x0000c8e4)
#define WS2812_YELLOW (0x0000e8e8)
#define WS2812_GREEN  (0x0000f800)
#define WS2812_BLUE   (0x00f80000)
#define WS2812_VIOLET (0x00f800f0)
#define WS2812_TEAL   (0x00f8f8e0)
#define WS2812_WHITE  (0x00f8f8f8)
#define WS2812_50PCT  (0x00f0f0f0)
#define WS2812_25PCT  (0x00e0e0e0)
#define WS2812_OFF    (0)
 
// transmit a single WS2812
void	ws2812_send(unsigned long led);
 
#endif	/* WS2812_H */ 