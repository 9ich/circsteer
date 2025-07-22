#include <stdio.h>
#include <math.h>
#include <windows.h>
#include <xinput.h>
#include "vJoy/SDK/inc/public.h"
#include "vJoy/SDK/inc/vjoyinterface.h"
#include "a.h"

#define USE_CURSES  1
#define TIMER_RES   1
#define POLL_MS     1

V              v;
static UI      ui;
static DWORD   pad;
static UINT    vjd;
static LONG    vjmin, vjmax;
static HANDLE  timer;

static const char*
vjdstat2s(UINT s){
	static const char *a[] = {"owned","free","busy","missing","unknown"};
	return a[CLAMP(s, VJD_STAT_OWN, VJD_STAT_UNKN)];
}

int
axis2hid(int a){
	static int h[] = {HID_USAGE_X,HID_USAGE_SL0,HID_USAGE_SL1};
	return h[a];
}

const char*
axis2s(int a){
	static const char *s[] = {"steering","L trigger","R trigger"};
	return s[a];
}

static int
cfgsave(void){
	FILE *f = fopen("circsteer.cfg", "wb");
	if(!f){
		printf("failed to open circsteer.cfg for writing\n");
		return 1;
	}
	fprintf(f, "range=%-4d  # steering range (degrees lock-to-lock)\n",
		(int)rad2deg(v.wlimit));
	fprintf(f, "thresh=%-4d # how far the stick has to be deflected to"
		" start steering (percent)\n", (int)(v.grabthresh*100));
	fflush(f); fclose(f); return 0;
}

static int
cfgload(void){
	static char buf[1000] = {0};
	FILE *f = fopen("circsteer.cfg", "rb");
	if(!f){return 1;}
	fseek(f, 0, SEEK_END);
	size_t sz = MIN(ftell(f), SZ(buf)-1);
	rewind(f); fread(buf, 1, sz, f); fclose(f);

	char *p = buf;
	char k[30]; int x;
	while(p && *p && sscanf(p, "%29[a-z]=%d", k, &x) == 2){
		if(strcmp(k, "range") == 0)      {v.wlimit = deg2rad(x);}
		else if(strcmp(k, "thresh") == 0){v.grabthresh = (double)(x)/100.0;}
		p = strchr(p, '\n') + 1;
	}
	return 0;
}

static void
sleepns(LONGLONG ns){
	ns /= 100;
	if(!timer){
		timer = CreateWaitableTimer(NULL, TRUE, NULL);
		if(!timer){return;}
	}
	LARGE_INTEGER x; x.QuadPart = -ns;
	if(!SetWaitableTimer(timer, &x, 0, NULL, NULL, FALSE)){return;}
	WaitForSingleObject(timer, INFINITE);
}

static void
padinit(void){
	int ok = 0;
	F(i, (DWORD)XUSER_MAX_COUNT){
		XINPUT_STATE s = {0};
		DWORD r = XInputGetState(i, &s);
		if(r == ERROR_SUCCESS){
			printf("xinput device %ld: connected\n", i);
			if(!ok){ok++; pad = i;}
		}else{
			printf("xinput device %ld: not connected\n", i);
		}
	}
	if(!ok){printf("no xinput devices connected\n");}
	else   {printf("using xinput device %ld\n", pad);}
	fflush(stdout);
}

static void
vjoyinit(void){
	printf("vjoy version: %d\n", GetvJoyVersion());
	printf("vjoy enabled: %d\n", vJoyEnabled());
	for(UINT i=1; i<=16; ++i){
		VjdStat s = GetVJDStatus(i);
		printf("vjoy device %2d: %s\n", i, vjdstat2s(s));
		if(vjd == 0 && s == VJD_STAT_FREE){vjd = i;}
	}
	if(vjd == 0){printf("no vjoy devices free\n"); exit(1);}
	else        {printf("acquiring vjoy device %d\n", vjd);}

	if(!AcquireVJD(vjd)){printf("failed to acquire vjoy device;"
		" trying to continue anyway\n");}
	ResetVJD(vjd);
	if(!GetVJDAxisMin(vjd, HID_USAGE_X, &vjmin)){vjmin = 0;}
	if(!GetVJDAxisMax(vjd, HID_USAGE_X, &vjmax)){vjmax = 0x7fff;}
	fflush(stdout);
}

static void
txn(int s){
	v.state = s;
}

void
bindstart(void){
	txn(S_BINDWAIT);
	F(i, LEN(v.bindfrac)){v.bindfrac[i] = 0;}
}

void
bindcancel(void){
	txn(S_MAIN);
}

static void
die(void){
	ui.kill();
	cfgsave();
	for(UINT i=1; i<=16; ++i)
		if(GetVJDStatus(i) == VJD_STAT_OWN){RelinquishVJD(i);}
	if(timer){CloseHandle(timer);}
	timeEndPeriod(TIMER_RES);
}

static void
uiinit(void){
	if(USE_CURSES){cursesinit(&ui);}
	else          {coninit(&ui);}
}

int
main(int argc, char **argv){
	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	timeBeginPeriod(TIMER_RES);
	atexit(die);
	uiinit(); padinit(); vjoyinit();
	v.wlimit = deg2rad(540.0);
	v.grabthresh = 0.5;
	if(cfgload() != 0){cfgsave();};
	V v0 = v;

	for(;;){
		sleepns(POLL_MS*1000000);

		XINPUT_STATE s = {0};
		DWORD r = XInputGetState(pad, &s);
		if(r != ERROR_SUCCESS){v.errstate |= ES_NOPAD;}
		else                  {v.errstate &= ~ES_NOPAD;}

		VjdStat vs = GetVJDStatus(vjd);
		if(vs != VJD_STAT_OWN){v.errstate |= ES_NOJOY;}
		else                  {v.errstate &= ~ES_NOJOY;}

		switch(v.state){
		case S_MAIN:{
			double old[2];
			VEC{old[k] = v.winput[k];}
			v.winput[0] = (double)(s.Gamepad.sThumbLX)/32767.0;
			v.winput[1] = (double)(s.Gamepad.sThumbLY)/32767.0;
			double mag = sqrt(SQ(v.winput[0]) + SQ(v.winput[1]));
			if(mag > 0.0){VEC{v.winput[k] /= mag;}}  // normalize
			v.grabbed = mag >= v.grabthresh;
			if(!v.grabbed){
				v.wheel = 0.0;
			}else{
				// first steering event is relative to 12 o'clock
				if(!v0.grabbed){old[0] = 0, old[1] = 1;}

				// signed angle diff
				v.wheel += atan2(old[0]*v.winput[1] - old[1]*v.winput[0],
					old[0]*v.winput[0] + old[1]*v.winput[1]);

				// ignore repeated turns beyond lock
				double lim = (v.wlimit + TAU)*0.5;
				while(v.wheel < -lim){v.wheel += PI;}
				while(v.wheel > lim){v.wheel -= PI;}
			}

			// feed to vjoy
			double lim = v.wlimit*0.5, w = CLAMP(v.wheel, -lim, lim);
			v.out[A_STEER] = -w/(v.wlimit) + 0.5;
			v.out[A_TRIGL] = (double)(s.Gamepad.bLeftTrigger)/255.0;
			v.out[A_TRIGR] = (double)(s.Gamepad.bRightTrigger)/255.0;
			F(a, LEN(v.out)){
				LONG x = (LONG)LERP(vjmin, vjmax, v.out[a]);
				SetAxis(x, vjd, axis2hid(a));
			}
			break;
		}
		case S_BINDWAIT:
			v.bindfrac[A_STEER] += 0.02*abs(s.Gamepad.sThumbLX/32767.0);
			v.bindfrac[A_TRIGL] += 0.02*abs(s.Gamepad.bLeftTrigger/255.0);
			v.bindfrac[A_TRIGR] += 0.02*abs(s.Gamepad.bRightTrigger/255.0);
			F(i, LEN(v.bindfrac)){
				v.bindfrac[i] = SATURATE(v.bindfrac[i]);
				if(v.bindfrac[i] >= 1.0){
					v.bindaxis = i;
					txn(S_BIND);
					break;
				}
			}
			break;
		case S_BIND:
			v.bindosc = fmod(v.bindosc + 1/1000.0, 1.0);
			SetAxis((LONG)(LERP(vjmin, vjmax, v.bindosc)), vjd,
				axis2hid(v.bindaxis));
			break;
		default: break;
		}
		v0 = v;
		if(!ui.update()){break;}
	}
	return 0;
}
