#include <curses.h>
#include "a.h"

static V          v0;
static WINDOW     *win;
static const int  colw[] = {13, 19, 59};
static int        colx[LEN(colw)];

void
meter(int y, int x, int w, double frac){
	double t = SATURATE(frac);
	mvhline(y, x+1, '-', w-2);
	mvaddch(y, x, '[');
	mvaddch(y, x+w-1, ']');
	for(int i=1; i<4; ++i){mvaddch(y, (int)LERP(x+1, x+w-2, i/4.0), ' ');}
	attron(COLOR_PAIR(2));
	mvaddch(y, (int)LERP(x+1, x+w-2, t), 'X');
	attroff(COLOR_PAIR(2));
}

void
fillmeter(int y, int x, int w, double frac){
	double t = SATURATE(frac);
	mvaddch(y, x, '[');
	mvaddch(y, x+w-1, ']');
	attron(COLOR_PAIR(2));
	mvhline(y, x+1, '#', (int)(t*(w-2)));
	attroff(COLOR_PAIR(2));
}

void curseskill(void){endwin();}

int
cursesupdate(void){
	if(is_termresized()){resize_term(0, 0); clear();}

	erase();
	touchwin(win);

	switch(v.state){
	case S_MAIN:{
		attron(COLOR_PAIR(2));
		mvprintw(getmaxy(win)-1, 0, "keys: (b)ind (q)uit");
		attroff(COLOR_PAIR(2));

		switch(getch()){
		case 'q':  return 0;
		case 'b':  bindstart(); clear(); break;
		default:   break;
		}

		F(i, LEN(v.out)){
			mvprintw(i, colx[0], axis2s(i));
			mvprintw(i, colx[1], "%.6f", v.out[i]);
			meter(i, colx[2], colw[2], v.out[i]);
		}
		break;
	}
	case S_BINDWAIT:{
		attron(COLOR_PAIR(2));
		mvprintw(getmaxy(win)-1, 0, "keys: (b)ind (c)ancel (q)uit");
		attroff(COLOR_PAIR(2));

		switch(getch()){
		case 'q':  return 0;
		case 'b':  bindstart(); clear(); break;
		case 'c':  bindcancel(); break;
		default:   break;
		}

		mvprintw(0, 0, "move a control to bind...");
		F(i, LEN(v.bindfrac)){
			if(v.bindfrac[i] <= 0){continue;}
			mvprintw(1+1*i, colx[0], "%s", axis2s(i));
			fillmeter(1+1*i, colx[2], colw[2], v.bindfrac[i]);
		}
		break;
	}
	case S_BIND:{
		attron(COLOR_PAIR(2));
		mvprintw(getmaxy(win)-1, 0, "keys: (b)ind (c)ancel (q)uit");
		attroff(COLOR_PAIR(2));

		switch(getch()){
		case 'q':  return 0;
		case 'b':  bindstart(); clear(); break;
		case 'c':  bindcancel(); break;
		default:   break;
		}

		meter(0, 0, 72, v.bindosc);
		mvprintw(1, 0, "sweeping axis <%s>. "
			"don't touch anything on your controller.\n\n"
			"alt+tab to the game -> go to input options -> let it detect "
			"this axis.\n\n"
			"press (c)ancel when done, or (b)ind to bind another axis.",
			axis2s(v.bindaxis));
		break;
	}
	default:  break;
	}

	attron(COLOR_PAIR(3));
	if(v.errstate & ES_NOJOY){mvprintw(getmaxy(win)-3, 0, "not connected to vjoy device");}
	if(v.errstate & ES_NOPAD){mvprintw(getmaxy(win)-2, 0, "no controller connected");}
	attroff(COLOR_PAIR(3));

	move(getmaxy(win)-1, getmaxx(win)-1);
	refresh();
	v0 = v;
	return 1;
}

void
cursesinit(UI *ui){
	ui->kill = curseskill;
	ui->update = cursesupdate;

	F(i, LEN(colw)-1){colx[i+1] = colx[i] + colw[i];}

	win = initscr();
	nodelay(win, TRUE);
	start_color();
	init_pair(2, COLOR_WHITE, COLOR_BLUE);
	init_pair(3, COLOR_RED,   COLOR_BLACK);
}
