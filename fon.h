/********************************************************************************/
/*				FON.H						*/
/********************************************************************************/
/*			Auteurs	: 						*/
/*				  Pascal Sicard					*/
/*				  Marc Garnier					*/
/*				  Elizabeth Maillet				*/
/*				  Christian Rabedaoro				*/
/*										*/
/*			date 	: 14 Septembre 1994				*/
/********************************************************************************/
/*	Prototypage des fonctions sur les sockets ( pour fon.c ).		*/
/********************************************************************************/

/*=================== INCLUSIONS STANDARDS de PROTOTYPES =============*/
#include <stdio.h>
#include<unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

/*------------------ CAS des prises TCP/IP ----------*/
#include <netinet/in.h>
#include <netdb.h>

/*------------------ definitions --------------------*/
#define	CLIENT		1
#define	SERVEUR		0
#define TOUT_CLIENT	0

/* definition des sens de fermeture pour shutdown :*/
#define FIN_RECEPTION 0
#define FIN_EMISSION 1
#define FIN_ECHANGES 2



/*			    +===================+			  	*/
/*==========================|	H_SOCKET	|===============================*/
/*			    +===================+				*/
int h_socket ( int domaine, int mode );

/*			    +===================+				*/
/*==========================|	H_BIND		|===============================*/
/*			    +===================+				*/
void h_bind ( int num_soc, struct sockaddr_in *p_adr_socket );

/*			    +===================+				*/
/*==========================| 	H_CONNECT	|===============================*/
/*			    +===================+				*/
void h_connect( int num_soc, struct sockaddr_in *p_adr_serv );

/*			    +===================+				*/
/*==========================|  H_LISTEN  	|===============================*/
/*			    +===================+				*/
void h_listen ( int num_soc, int nb_req_att );

/*			    +===================+				*/
/*==========================| H_ACCEPT 		|===============================*/
/*			    +===================+				*/
int h_accept( int num_soc, struct sockaddr_in *p_adr_client );

/*			    +===================+				*/
/*==========================| 	H_READS		|===============================*/
/*			    +===================+				*/
int h_reads ( int num_soc, char *tampon, int nb_octets );

/*			    +===================+				*/
/*==========================|	H_WRITES	|===============================*/
/*			    +===================+				*/
int h_writes ( int num_soc, char *tampon, int nb_octets );

/*			    +===================+				*/
/*==========================|  	H_SENDTO	|===============================*/
/*			    +===================+				*/
int h_sendto( int num_soc, char *tampon, int nb_octets,
				struct sockaddr_in *p_adr_distant );

/*			    +===================+				*/
/*==========================|  H_RECVFROM	|===============================*/
/*			    +===================+				*/
int h_recvfrom( int num_soc, char *tampon, int nb_octets,			
				struct sockaddr_in *p_adr_distant );


/*			    +===================+				*/
/*==========================| 	H_SHUTDOWN	|===============================*/
/*			    +===================+				*/
void h_shutdown ( int socket, int sens ) ;

/*			    +===================+				*/
/*==========================|	H_CLOSE		|===============================*/
/*			    +===================+				*/
void h_close ( int socket ); 

/*			    +===================+				*/
/*==========================|	ADR_SOCKET	|===============================*/
/*			    +===================+				*/
void adr_socket( char *service, char *serveur, int typesock,
		 struct sockaddr_in **p_adr_serv);


/*================ SORTIE_ERR =================================================*/


/*============================================================================*/
