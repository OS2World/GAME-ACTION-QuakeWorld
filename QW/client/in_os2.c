// in_null.c -- for systems without a mouse

#include "quakedef.h"

#define INCL_DOS
#define INCL_MOU
#include <os2.h>

#include <sys/builtin.h>
#include <sys/fmutex.h>

int mouse_works = 0;
HMOU mh;

MOUEVENTINFO ev_a[2];
MOUEVENTINFO *ev;

//SCALEFACT scale_a[2];
//SCALEFACT *scale;

void Force_CenterView_f (void)
{
	cl.viewangles[PITCH] = 0;
}

static int buttons = -1;
static int old_buttons = 0;

static int xpos, ypos;

_fmutex sem;

void mouse_thread(void *p)
{
	HMOU h = mh;
	USHORT rd = MOU_WAIT;
	DosSetPriority(PRTYS_THREAD, PRTYC_TIMECRITICAL, 31, 0);
	while (1) {
	if (MouReadEventQue(ev, &rd, h)) return;
	if (!mouse_works) goto ret;
	_fmutex_request(&sem, _FMR_IGNINT);
	xpos += ev->col;
	ypos += ev->row;
	buttons =
	  ((ev->fs & (MOUSE_MOTION_WITH_BN1_DOWN|MOUSE_BN1_DOWN)) ? 1 : 0) +
	  ((ev->fs & (MOUSE_MOTION_WITH_BN2_DOWN|MOUSE_BN2_DOWN)) ? 2 : 0) +
	  ((ev->fs & (MOUSE_MOTION_WITH_BN3_DOWN|MOUSE_BN3_DOWN)) ? 4 : 0);
	_fmutex_release(&sem);
	}
	ret:
	MouClose(h);
}

void IN_Init (void)
{
	USHORT m_mask = MOUSE_MOTION_WITH_BN1_DOWN | MOUSE_BN1_DOWN |
			MOUSE_MOTION_WITH_BN2_DOWN | MOUSE_BN2_DOWN |
			MOUSE_MOTION_WITH_BN3_DOWN | MOUSE_BN3_DOWN |
			MOUSE_MOTION;
	USHORT m_flags = MOUSE_DISABLED | MOUSE_MICKEYS;
	if (mouse_works) return;
	Cmd_AddCommand ("force_centerview", Force_CenterView_f);
	if (MouOpen(NULL, &mh)) return;
	ev = _THUNK_PTR_STRUCT_OK(ev_a) ? ev_a : ev_a + 1;
	//scale = _THUNK_PTR_STRUCT_OK(scale_a) ? scale_a : scale_a + 1;
	mouse_works = 1;
	//scale->rowScale = 1;
	//scale->colScale = 1;
	//if (MouSetScaleFact(scale, mh)) Sys_Error("MouSetScaleFact");
	if (MouSetDevStatus(&m_flags, mh)) Sys_Error("MouSetDevStatus");
	MouSetEventMask(&m_mask, mh);
	{
		//THRESHOLD t;
		USHORT status = -1;
		MouGetNumButtons(&status, mh);
		//Con_Printf("os/2 mouse: %d buttons\n", (int)status);
		/*
		t.Length = sizeof t;
		MouGetThreshold(&t, mh);
		Con_Printf("t: %d %d %d %d %d\n", t.Length, t.Level1, t.Lev1Mult, t.Level2, t.lev2Mult);
		*/
	}
	_fmutex_create(&sem, 0);
	xpos = ypos = 0;
	if (_beginthread(mouse_thread, NULL, 65536, NULL) == -1) {
		MouClose(mh);
		mouse_works = 0;
	}
}

void IN_Shutdown (void)
{
	//if (mouse_works) MouClose(mh);
	mouse_works = 0;
}

void IN_Commands (void)
{
	if (!mouse_works) return;
	_fmutex_request(&sem, _FMR_IGNINT);
	if (buttons == -1) goto ret;
	if ((buttons & 1) && !(old_buttons & 1)) Key_Event(K_MOUSE1, true);
	if (!(buttons & 1) && (old_buttons & 1)) Key_Event(K_MOUSE1, false);
	if ((buttons & 2) && !(old_buttons & 2)) Key_Event(K_MOUSE2, true);
	if (!(buttons & 2) && (old_buttons & 2)) Key_Event(K_MOUSE2, false);
	if ((buttons & 4) && !(old_buttons & 4)) Key_Event(K_MOUSE3, true);
	if (!(buttons & 4) && (old_buttons & 4)) Key_Event(K_MOUSE3, false);
	old_buttons = buttons;
	ret:
	_fmutex_release(&sem);
}

void IN_Move (usercmd_t *cmd)
{
	float mouse_x, mouse_y;

	if (in_mlook.state & 1)
		V_StopPitchDrift ();

	if (!mouse_works) return;

	_fmutex_request(&sem, _FMR_IGNINT);
	mouse_x = xpos;
	mouse_y = ypos;
	xpos = 0;
	ypos = 0;
	_fmutex_release(&sem);
	//if (mouse_x || mouse_y) Con_Printf("%f %f %08x %08lx\n", mouse_x, mouse_y, (int)ev->fs, ev->time);

	/* and this is copied from vid_svgalib.c */
	mouse_x *= sensitivity.value * 1.95;
	mouse_y *= sensitivity.value * 1.95;

// add mouse X/Y movement to cmd
	if ( (in_strafe.state & 1) || (lookstrafe.value && (in_mlook.state & 1) ))
		cmd->sidemove += m_side.value * mouse_x;
	else
		cl.viewangles[YAW] -= m_yaw.value * mouse_x;
	
	if ( (in_mlook.state & 1) && !(in_strafe.state & 1))
	{
		cl.viewangles[PITCH] += m_pitch.value * mouse_y;
		if (cl.viewangles[PITCH] > 80)
			cl.viewangles[PITCH] = 80;
		if (cl.viewangles[PITCH] < -70)
			cl.viewangles[PITCH] = -70;
	}
	else
	{
		if ((in_strafe.state & 1) && noclip_anglehack)
			cmd->upmove -= m_forward.value * mouse_y;
		else
			cmd->forwardmove -= m_forward.value * mouse_y;
	}
}

/*
===========
IN_ModeChanged
===========
*/
void IN_ModeChanged (void)
{
}

