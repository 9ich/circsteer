#include "a.h"

void conkill(void){}

int conupdate(void){return 1;}

void
coninit(UI *ui){
	ui->kill = conkill;
	ui->update = conupdate;
}
