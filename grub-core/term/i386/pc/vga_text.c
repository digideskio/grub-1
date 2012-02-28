/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2007, 2008, 2010  Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <grub/dl.h>
#include <grub/i386/vga_common.h>
#include <grub/cpu/io.h>
#include <grub/types.h>
#include <grub/vga.h>

#if defined (GRUB_MACHINE_COREBOOT) || defined (GRUB_MACHINE_QEMU) || defined (GRUB_MACHINE_MIPS_QEMU_MIPS) || defined (GRUB_MACHINE_MULTIBOOT)
#include <grub/machine/console.h>
#endif

GRUB_MOD_LICENSE ("GPLv3+");

#define COLS	80
#define ROWS	25

static int grub_curr_x, grub_curr_y;

#ifdef __mips__
#define VGA_TEXT_SCREEN		((grub_uint16_t *) 0xb00b8000)
#else
#define VGA_TEXT_SCREEN		((grub_uint16_t *) 0xb8000)
#endif

static void
screen_write_char (int x, int y, short c)
{
  VGA_TEXT_SCREEN[y * COLS + x] = c;
}

static short
screen_read_char (int x, int y)
{
  return VGA_TEXT_SCREEN[y * COLS + x];
}

static void
update_cursor (void)
{
  unsigned int pos = grub_curr_y * COLS + grub_curr_x;
  grub_vga_cr_write (pos >> 8, GRUB_VGA_CR_CURSOR_ADDR_HIGH);
  grub_vga_cr_write (pos & 0xFF, GRUB_VGA_CR_CURSOR_ADDR_LOW);
}

static void
inc_y (void)
{
  grub_curr_x = 0;
  if (grub_curr_y < ROWS - 1)
    grub_curr_y++;
  else
    {
      int x, y;
      for (y = 0; y < ROWS - 1; y++)
        for (x = 0; x < COLS; x++)
          screen_write_char (x, y, screen_read_char (x, y + 1));
      for (x = 0; x < COLS; x++)
	screen_write_char (x, ROWS - 1, ' ' | (grub_console_cur_color << 8));
    }
}

static void
inc_x (void)
{
  if (grub_curr_x >= COLS - 1)
    inc_y ();
  else
    grub_curr_x++;
}

static void
grub_vga_text_putchar (struct grub_term_output *term __attribute__ ((unused)),
		       const struct grub_unicode_glyph *c)
{
  switch (c->base)
    {
      case '\b':
	if (grub_curr_x != 0)
	  screen_write_char (grub_curr_x--, grub_curr_y, ' ');
	break;
      case '\n':
	inc_y ();
	break;
      case '\r':
	grub_curr_x = 0;
	break;
      default:
	screen_write_char (grub_curr_x, grub_curr_y,
			   c->base | (grub_console_cur_color << 8));
	inc_x ();
    }

  update_cursor ();
}

static grub_uint16_t
grub_vga_text_getxy (struct grub_term_output *term __attribute__ ((unused)))
{
  return (grub_curr_x << 8) | grub_curr_y;
}

static void
grub_vga_text_gotoxy (struct grub_term_output *term __attribute__ ((unused)),
		      grub_uint8_t x, grub_uint8_t y)
{
  grub_curr_x = x;
  grub_curr_y = y;
  update_cursor ();
}

static void
grub_vga_text_cls (struct grub_term_output *term)
{
  int i;
  for (i = 0; i < ROWS * COLS; i++)
    VGA_TEXT_SCREEN[i] = ' ' | (grub_console_cur_color << 8);
  grub_vga_text_gotoxy (term, 0, 0);
}

static void
grub_vga_text_setcursor (struct grub_term_output *term __attribute__ ((unused)),
			 int on)
{
  grub_uint8_t old;
  old = grub_vga_cr_read (GRUB_VGA_CR_CURSOR_START);
  if (on)
    grub_vga_cr_write (old & ~GRUB_VGA_CR_CURSOR_START_DISABLE,
		       GRUB_VGA_CR_CURSOR_START);
  else
    grub_vga_cr_write (old | GRUB_VGA_CR_CURSOR_START_DISABLE,
		       GRUB_VGA_CR_CURSOR_START);
}

static grub_err_t
grub_vga_text_init_fini (struct grub_term_output *term)
{
  grub_vga_text_cls (term);
  return 0;
}

static struct grub_term_output grub_vga_text_term =
  {
    .name = "vga_text",
    .init = grub_vga_text_init_fini,
    .fini = grub_vga_text_init_fini,
    .putchar = grub_vga_text_putchar,
    .getwh = grub_console_getwh,
    .getxy = grub_vga_text_getxy,
    .gotoxy = grub_vga_text_gotoxy,
    .cls = grub_vga_text_cls,
    .setcolorstate = grub_console_setcolorstate,
    .setcursor = grub_vga_text_setcursor,
    .flags = GRUB_TERM_CODE_TYPE_CP437,
    .normal_color = GRUB_TERM_DEFAULT_NORMAL_COLOR,
    .highlight_color = GRUB_TERM_DEFAULT_HIGHLIGHT_COLOR,
  };

#if defined (GRUB_MACHINE_COREBOOT) || defined (GRUB_MACHINE_QEMU) || defined (GRUB_MACHINE_MIPS_QEMU_MIPS) || defined (GRUB_MACHINE_MULTIBOOT)
void grub_vga_text_init (void)
#else
GRUB_MOD_INIT(vga_text)
#endif
{
  grub_term_register_output ("vga_text", &grub_vga_text_term);
}

#if defined (GRUB_MACHINE_COREBOOT) || defined (GRUB_MACHINE_QEMU) || defined (GRUB_MACHINE_MIPS_QEMU_MIPS) || defined (GRUB_MACHINE_MULTIBOOT)
void grub_vga_text_fini (void)
#else
GRUB_MOD_FINI(vga_text)
#endif
{
  grub_term_unregister_output (&grub_vga_text_term);
}
