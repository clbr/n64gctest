#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdint.h>
#include <libdragon.h>

typedef uint64_t u64;
typedef unsigned int u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef int64_t s64;
typedef signed int s32;
typedef int16_t s16;
typedef int8_t s8;

/* hardware definitions */
// Pad buttons
#define A_BUTTON(a)		((a) & 0x8000)
#define B_BUTTON(a)		((a) & 0x4000)
#define Z_BUTTON(a)		((a) & 0x2000)
#define START_BUTTON(a) ((a) & 0x1000)

struct controller_data gKeys;

volatile u32 frames;

/* input - do getButtons() first, then getAnalogX() and/or getAnalogY() */
unsigned short getButtons()
{
	// Read current controller status
	controller_scan();
	gKeys = get_keys_pressed();
	return (unsigned short)(gKeys.c[0].data >> 16);
}

display_context_t lockVideo(int wait)
{
	display_context_t dc;

	if (wait)
		while (!(dc = display_lock()));
	else
		dc = display_lock();
	return dc;
}

void unlockVideo(display_context_t dc)
{
	if (dc)
		display_show(dc);
}

/* text functions */
void printText(display_context_t dc, char *msg, int x, int y)
{
	if (dc)
		graphics_draw_text(dc, x*8, y*8, msg);
}

/* vblank callback */
void vblCallback(void)
{
	frames++;
}

void delay(u32 cnt)
{
	u32 then = frames + cnt;
	while (then > frames);
}

/* initialize console hardware */
void init_n64(void)
{
	/* enable interrupts (on the CPU) */
	init_interrupts();

	/* Initialize peripherals */
	display_init(RESOLUTION_640x240, DEPTH_32_BPP, 2, GAMMA_NONE, ANTIALIAS_OFF);

	register_VI_handler(vblCallback);

	controller_init();
}

enum {
	N64_CONTROLLER,
	GC_CONTROLLER,
	UNKNOWN_CONTROLLER,
	NUM_CTRL_TYPES
};

/* main code entry point */
int main(void)
{
	display_context_t disp;
	struct controller_data ctrl, prevctrl;
	struct controller_origin_data orig;
	char buf[64];
	u32 i;
	u8 type[4];
	u8 err[4];
	u8 rumble[4] = { 0 };
	u16 id[4];

	static const char * const ctrlnames[NUM_CTRL_TYPES] = {
		"N64 controller",
		"GC controller",
		"unknown"
	};

	init_n64();
	get_accessories_present(&ctrl);

	for (i = 0; i < 4; i++) {
		err[i] = ctrl.c[i].err;
		id[i] = ctrl.c[i].data >> 16;

		type[i] = UNKNOWN_CONTROLLER;
		switch (id[i]) {
			case 0x0500:
				type[i] = N64_CONTROLLER;
			break;
			case 0x0900:
				type[i] = GC_CONTROLLER;
			break;
		}
	}

	while (1)
	{
		u32 color;

		disp = lockVideo(1);
		color = graphics_make_color(0xb6, 0xff, 0xb6, 0xFF);
		graphics_fill_screen(disp, color);

		color = graphics_make_color(0x00, 0x00, 0x00, 0xFF);
		graphics_set_color(color, 0);

		printText(disp, "Nintendo 64 GC controller test v0.1, (C) TMI", 4, 2);
		printText(disp, "Press start to toggle rumble, X to check origins (only GC)", 4, 3);

		for (i = 0; i < 4; i++) {
			if (err[i]) {
				sprintf(buf, "Port %u: disconnected", i + 1);
				printText(disp, buf, 4, 5 + i * 6);
				continue;
			}

			sprintf(buf, "Port %u: id %04x, %s", i + 1, id[i],
				ctrlnames[type[i]]);
			printText(disp, buf, 4, 5 + i * 6);

			controller_read(&ctrl);
			controller_read_gc(&ctrl, rumble);

			if (type[i] == N64_CONTROLLER) {
				sprintf(buf, "%4s %5s %2s %4s %5s %1s %1s %1s",
					ctrl.c[i].left ? "left" : "",
					ctrl.c[i].right ? "right" : "",
					ctrl.c[i].up ? "up" : "",
					ctrl.c[i].down ? "down" : "",
					ctrl.c[i].start ? "start" : "",
					ctrl.c[i].L ? "L" : "",
					ctrl.c[i].R ? "R" : "",
					ctrl.c[i].Z ? "Z" : "");
				printText(disp, buf, 6, 5 + i * 6 + 2);

				sprintf(buf, "% 4d % 4d %1s %1s %4s %6s %6s %7s",
					ctrl.c[i].x,
					ctrl.c[i].y,
					ctrl.c[i].A ? "A" : "",
					ctrl.c[i].B ? "B" : "",
					ctrl.c[i].C_up ? "C-up" : "",
					ctrl.c[i].C_down ? "C-down" : "",
					ctrl.c[i].C_left ? "C-left" : "",
					ctrl.c[i].C_right ? "C-right" : "");
				printText(disp, buf, 6, 5 + i * 6 + 3);
			} else if (type[i] == GC_CONTROLLER) {
				sprintf(buf, "%4s %5s %2s %4s %5s %1s %1s %1s",
					ctrl.gc[i].left ? "left" : "",
					ctrl.gc[i].right ? "right" : "",
					ctrl.gc[i].up ? "up" : "",
					ctrl.gc[i].down ? "down" : "",
					ctrl.gc[i].start ? "start" : "",
					ctrl.gc[i].l ? "L" : "",
					ctrl.gc[i].r ? "R" : "",
					ctrl.gc[i].z ? "Z" : "");
				printText(disp, buf, 6, 5 + i * 6 + 2);

				sprintf(buf, "%3u %3u C %3u %3u LR %3u %3u %1s %1s %1s %1s",
					ctrl.gc[i].stick_x,
					ctrl.gc[i].stick_y,
					ctrl.gc[i].cstick_x,
					ctrl.gc[i].cstick_y,
					ctrl.gc[i].analog_l,
					ctrl.gc[i].analog_r,
					ctrl.gc[i].a ? "A" : "",
					ctrl.gc[i].b ? "B" : "",
					ctrl.gc[i].x ? "X" : "",
					ctrl.gc[i].y ? "Y" : "");
				printText(disp, buf, 6, 5 + i * 6 + 3);

				if (ctrl.gc[i].origin_unchecked)
					sprintf(buf, "origin unchecked");
				else
					sprintf(buf, "origin vals: %3u %3u C %3u %3u LR %3u %3u dead %3u %3u",
						orig.gc[i].data.stick_x,
						orig.gc[i].data.stick_y,
						orig.gc[i].data.cstick_x,
						orig.gc[i].data.cstick_y,
						orig.gc[i].data.analog_l,
						orig.gc[i].data.analog_r,
						orig.gc[i].deadzone0,
						orig.gc[i].deadzone1);
				printText(disp, buf, 6, 5 + i * 6 + 4);

				if (ctrl.gc[i].start && !prevctrl.gc[i].start)
					rumble[i] ^= 1;
				if (ctrl.gc[i].x && !prevctrl.gc[i].x)
					controller_read_gc_origin(&orig);
			}
		}

		unlockVideo(disp);

		memcpy(&prevctrl, &ctrl, sizeof(struct controller_data));

		delay(1);
	}

	return 0;
}
