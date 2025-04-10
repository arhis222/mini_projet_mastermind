/* Programme d'exemple de gestion fenetres et curseur */
/* gcc ThisFile.c  -L /usr/ccs/lib -L /usr/ucblib -lcurses -ltermcap  */
/* #include <ncurses.h> */\

#include <curses.h>
#include <unistd.h>

main()
{
	WINDOW *W1, *L1, *W2, *L2, *C;
  	pid_t PID;
 	int SZ ; /* taille d'un cadre */
 	char CAR ;
	
 	initscr(); /* Initialisation (nettoyage) de l'ecran */
	/* printw("%i LINES %i COLS \n ", LINES, COLS);*/
	refresh();
	
	SZ = (LINES-2)/3;
	if(SZ<6 || COLS < 12)
	{  endwin(); printf("ERREUR : Fenetre trop petite \n"); exit(1) ;}
	/* creation de trois cadres superposes separes par une ligne */
 	W1= newwin( SZ, COLS, 0, 0); 
	L1= newwin( 1, COLS, SZ, 0);
	W2= newwin( SZ, COLS, SZ+1, 0);
 	L2= newwin( 1, COLS, (2*SZ)+1, 0);
	
 	C= newwin( LINES-(2*SZ)-2, COLS,2+(2*SZ), 0);
	/* pas de curseur */
 	leaveok(W2,TRUE);leaveok(C,TRUE);
 	leaveok(L1,TRUE); leaveok(L2,TRUE);
	leaveok(W1,TRUE);
	
	/* scroll automatique sur W1, W2, C */
	scrollok(W1,TRUE);scrollok(W2,TRUE);scrollok(C,TRUE);
	/* tracer les lignes */
 	whline(L1, '=', COLS);whline(L2, '=', COLS);
	wrefresh(L1);wrefresh(L2);
	/* ecriture dans le cadre de controle */
 	wprintw(C,"  vous pouvez taper\n le . termine  ....\n");
 	wrefresh(C);
	
	/* recuperation des caracteres tels que frappes */
 	cbreak();
	/* pas d'echo automatique */
	noecho();
	
	while((CAR=getch())!='.')
	{
		waddch(W1,CAR);
		wrefresh(W1);
		waddch(W2,CAR);
		wrefresh(W2);
	}
	wprintw(C,"on termine \n ....\n");
 	wrefresh(C);
	endwin();
	
}