#include "quakedef.h"
#include "vid_os2.h"
#include "vid_dos.h"

#define INCL_DOS
#define INCL_VIO
#include <os2.h>

static VIOMODEINFO old_mode_a[2];
static VIOMODEINFO tmp_mode_a[2];
static VIOMODEINFO new_mode_a[2];
static VIOPHYSBUF phys_a[2];

static VIOMODEINFO *old_mode;
static VIOMODEINFO *tmp_mode;
static VIOMODEINFO *new_mode;
static VIOPHYSBUF *phys;

static int gr_mode = 0;

void *vid_os2_set_g_mode(void)
{
	int r;
	old_mode = _THUNK_PTR_STRUCT_OK(old_mode_a) ? old_mode_a : old_mode_a + 1;
	tmp_mode = _THUNK_PTR_STRUCT_OK(tmp_mode_a) ? tmp_mode_a : tmp_mode_a + 1;
	new_mode = _THUNK_PTR_STRUCT_OK(new_mode_a) ? new_mode_a : new_mode_a + 1;
	phys = _THUNK_PTR_STRUCT_OK(phys_a) ? phys_a : phys_a + 1;
	old_mode->cb = sizeof(VIOMODEINFO);
	if (!gr_mode) {
		if ((r = VioGetMode(old_mode, 0))) Sys_Error("VioGetMode: %d", r);
	} else {
		VioSetMode(old_mode, 0);
	}
	memcpy(new_mode, old_mode, sizeof(VIOMODEINFO));
	new_mode->color = 8;
	new_mode->fbType = VGMT_OTHER | VGMT_GRAPHICS;
	new_mode->hres = 320;
	new_mode->vres = 200;
	new_mode->col = 40;
	new_mode->row = 25;
	if ((r = VioSetMode(new_mode, 0))) Sys_Error("VioSetMode: %d", r);
	gr_mode = 1;
	phys->pBuf = (PBYTE)0xa0000;
	phys->cb = 0x10000;
	if ((r = VioGetPhysBuf(phys, 0))) Sys_Error("VioGetPhysBuf: %d", r);
	return MAKEP(phys->asel[0], 0);
}

void vid_os2_shutdown(void)
{
	int r;
	if (gr_mode && (r = VioSetMode(old_mode, 0))) fprintf(stderr, "VioSetMode: %d\n", r);
	gr_mode = 0;
}

int vid_os2_can_update(void)
{
	UCHAR st;
	if (VioScrLock(LOCKIO_NOWAIT, &st, 0)) return 0;
	if (st != LOCK_SUCCESS) return 0;
	return 1;
}

void vid_os2_wait_update(void)
{
	UCHAR st;
	int r;
	if ((r = VioScrLock(LOCKIO_WAIT, &st, 0))) Sys_Error("VioScrLock: %d", r);
	if (st != LOCK_SUCCESS) Sys_Error("VioScrLock: status %d", (int)st);
}

void vid_os2_end_update(void)
{
	if (VioScrUnLock(0)) Sys_Error("VioScrUnlock");
}

void VID_LockBuffer(void)
{
}

void VID_UnlockBuffer(void)
{
}

static int ext_setmode(viddef_t *vid, struct vmode_s *pcurrentmode);
static void ext_swapbuffers(viddef_t *vid, struct vmode_s *pcurrentmode, vrect_t *rects);
static void ext_begindirectrect(viddef_t *vid, struct vmode_s *pcurrentmode, int x, int y, byte *pbitmap, int width, int height);
static void ext_enddirectrect(viddef_t *vid, struct vmode_s *pcurrentmode, int x, int y, int width, int height);

HFILE screen_h = 0;

static int set_page(int page)
{
	struct {
		ULONG  length;
		USHORT bank;
		USHORT modetype;
		USHORT bankmode;
	} parameter;
	ULONG datalen = 0, parmlen;
	parmlen = sizeof(parameter);
	parameter.length = sizeof parameter;
	parameter.bank = page;
	parameter.modetype = 2;
	parameter.bankmode = 1;
	if (DosDevIOCtl(screen_h, 0x80, 1, &parameter, parmlen, &parmlen, NULL, 0, &datalen)) return -1;
	return 0;
}

static void *try_mode(int x, int y)
{
	void *p;
	new_mode->cb = 12;
	new_mode->fbType = 0x0b;
	new_mode->color = 8;
	new_mode->col = x / 8;
	new_mode->row = y / 16;
	new_mode->hres = x;
	new_mode->vres = y;
	if (VioSetMode(new_mode, 0)) return NULL;
	if (x * y > 65536 && (set_page(1) || set_page(0))) return NULL;
	phys->pBuf = (PBYTE) 0xa0000;
	phys->cb = 0x10000;
	if (VioGetPhysBuf(phys, 0)) return NULL;
	p = MAKEP(phys->asel[0], 0);
	return p;
}

static char *title = "    **** os/2 extended modes *****    ";

vmode_t ext_modes[] = {
	/*{NULL, "320x200", NULL, 320, 200, 0, 0, 0, 0, NULL, ext_setmode, ext_swapbuffers, VGA_SetPalette, ext_begindirectrect, ext_enddirectrect},*/
	{NULL, "320x240", NULL, 320, 240, 0, 0, 0, 0, NULL, ext_setmode, ext_swapbuffers, VGA_SetPalette, ext_begindirectrect, ext_enddirectrect},
	{NULL, "400x300", NULL, 400, 300, 0, 0, 0, 0, NULL, ext_setmode, ext_swapbuffers, VGA_SetPalette, ext_begindirectrect, ext_enddirectrect},
	{NULL, "512x384", NULL, 512, 384, 0, 0, 0, 0, NULL, ext_setmode, ext_swapbuffers, VGA_SetPalette, ext_begindirectrect, ext_enddirectrect},
	{NULL, "640x400", NULL, 640, 400, 0, 0, 0, 0, NULL, ext_setmode, ext_swapbuffers, VGA_SetPalette, ext_begindirectrect, ext_enddirectrect},
	{NULL, "640x480", NULL, 640, 480, 0, 0, 0, 0, NULL, ext_setmode, ext_swapbuffers, VGA_SetPalette, ext_begindirectrect, ext_enddirectrect},
	{NULL, "800x600", NULL, 800, 600, 0, 0, 0, 0, NULL, ext_setmode, ext_swapbuffers, VGA_SetPalette, ext_begindirectrect, ext_enddirectrect},
	{NULL, "1024x768", NULL, 1024, 768, 0, 0, 0, 0, NULL, ext_setmode, ext_swapbuffers, VGA_SetPalette, ext_begindirectrect, ext_enddirectrect},
	{NULL, "1152x864", NULL, 1152, 864, 0, 0, 0, 0, NULL, ext_setmode, ext_swapbuffers, VGA_SetPalette, ext_begindirectrect, ext_enddirectrect},
	{NULL, "1280x1024", NULL, 1280, 1024, 0, 0, 0, 0, NULL, ext_setmode, ext_swapbuffers, VGA_SetPalette, ext_begindirectrect, ext_enddirectrect},
	/*{NULL, "1600x1200", NULL, 1600, 1200, 0, 0, 0, 0, NULL, ext_setmode, ext_swapbuffers, VGA_SetPalette, ext_begindirectrect, ext_enddirectrect}, quake engine hangs at this resolution... */
};

void VID_InitExtra(void)
{
	int r;
	int found = 0;
	vmode_t *m;
	ULONG act;
	if (DosOpen("SCREEN$", &screen_h, &act, 0, 0, OPEN_ACTION_OPEN_IF_EXISTS, OPEN_FLAGS_NOINHERIT | OPEN_SHARE_DENYNONE | OPEN_ACCESS_READWRITE, NULL)) return;
	Con_Printf("Probing os/2 extended modes ...\n");
	old_mode = _THUNK_PTR_STRUCT_OK(old_mode_a) ? old_mode_a : old_mode_a + 1;
	tmp_mode = _THUNK_PTR_STRUCT_OK(tmp_mode_a) ? tmp_mode_a : tmp_mode_a + 1;
	new_mode = _THUNK_PTR_STRUCT_OK(new_mode_a) ? new_mode_a : new_mode_a + 1;
	phys = _THUNK_PTR_STRUCT_OK(phys_a) ? phys_a : phys_a + 1;
	tmp_mode->cb = sizeof(VIOMODEINFO);
	r = VioGetMode(tmp_mode, 0);
	m = (void *)((char *)ext_modes + sizeof ext_modes);
	do {
		m--;
		if (try_mode(m->width, m->height)) {
			m->aspect = ((float)m->height / (float)m->width) * (320.0 / 240.0);
			m->rowbytes = m->width;
			m->numpages = 1;
			m->pextradata = NULL;
			
			found++;
			m->pnext = pvidmodes;
			pvidmodes = m;
			numvidmodes++;
		}
	} while (m > ext_modes);
	if (found) {
		pvidmodes->header = title;
		Con_Printf("... %d modes found\n", found);
	}
	if (!r) VioSetMode(tmp_mode, 0);
}

static int ext_setmode(viddef_t *lvid, struct vmode_s *pcurrentmode)
{
	void *addr;
	lvid->numpages = 1;
// clean up any old vid buffer lying around, alloc new if needed
	if (!VGA_FreeAndAllocVidbuffer (lvid, lvid->numpages == 1))
		return -1;	// memory alloc failed

// clear the screen and wait for the next frame. VGA_pcurmode, which
// VGA_ClearVideoMem relies on, is guaranteed to be set because mode 0 is
// always the first mode set in a session
	if (VGA_pcurmode)
		VGA_ClearVideoMem (VGA_pcurmode->planar);

// set the mode
	if (!(addr = try_mode(pcurrentmode->width, pcurrentmode->height))) return -1;
	VGA_pagebase = addr;
	VGA_width = 320;
	VGA_height = 200;
	VGA_rowbytes = 320;

	lvid->colormap = host_colormap;
	lvid->rowbytes = lvid->width;

	lvid->direct = VGA_pagebase;
	lvid->conrowbytes = lvid->rowbytes;
	lvid->conwidth = lvid->width;
	lvid->conheight = lvid->height;

	lvid->maxwarpwidth = WARP_WIDTH;
	lvid->maxwarpheight = WARP_HEIGHT;

	VGA_pcurmode = pcurrentmode;

	D_InitCaches (vid_surfcache, vid_surfcachesize);

	return 1;
}

static void ext_swapbuffers(viddef_t *lvid, struct vmode_s *pcurrentmode, vrect_t *rects)
{
	int page = 0;
	char *src = lvid->buffer;
	char *end = (char *)lvid->buffer + lvid->width * lvid->height;
	do {
		int len;
		set_page(page++);
		len = end - src;
		if (len > 0x10000) len = 0x10000;
		memcpy(VGA_pagebase, src, len);
		src += 0x10000;
	} while (src < end);
}

static void ext_begindirectrect(viddef_t *vid, struct vmode_s *pcurrentmode, int x, int y, byte *pbitmap, int width, int height)
{
}

static void ext_enddirectrect(viddef_t *vid, struct vmode_s *pcurrentmode, int x, int y, int width, int height)
{
}

