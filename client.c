/******************************************************************************/
/*         Application: Mastermind - Client                                 */
/******************************************************************************/
/*                                                                        */
/*            Programme  CLIENT                                           */
/*                                                                        */
/******************************************************************************/
/*                                                                        */
/*    Auteurs : UNAY Arhan et GEMICIOGLU Utku                             */
/*    Date :  10.04.2025                                                  */
/*                                                                        */
/******************************************************************************/

#include <curses.h>         /* Primitives de gestion d'ecran */
#include <stdio.h>
#include <stdlib.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>         /* Pour read() */

#include "fon.h"            /* Primitives de la boite à outils */

/* Définition des codes couleurs ANSI pour l'affichage */
#define ANSI_COLOR_RED     "\033[31m"
#define ANSI_COLOR_GREEN   "\033[32m"
#define ANSI_COLOR_YELLOW  "\033[33m"
#define ANSI_COLOR_RESET   "\033[0m"

#define SERVICE_DEFAUT "1111"
#define SERVEUR_DEFAUT "127.0.0.1"

/* Prototypes */
void client_appli(char *serveur, char *service);
static int read_line(int sock, char *buf, int max_len);
static int validate_proposal(const char *input, int niveau);

  
/* Fonction auxiliaire pour lire une ligne depuis la socket, caractère par caractère */
static int read_line(int sock, char *buf, int max_len) {
    int pos = 0;
    char c;
    int n;
    while (pos < max_len - 1) {
        n = read(sock, &c, 1); // Lecture d'un caractère
        if(n < 0) {
            perror("Erreur lors de read");
            return -1;
        }
        if(n == 0) { // Connexion fermée par le pair
            break;
        }
        buf[pos++] = c;
        if(c == '\n')
            break;
    }
    buf[pos] = '\0';
    return pos;
}

/* Fonction auxiliaire pour valider le format de la proposition.
   Pour un niveau > 1, on exige que les chiffres soient séparés par au moins un espace.
   On vérifie ensuite que le nombre de tokens correspond exactement au nombre attendu (niveau).
   Retourne 1 si format valide, 0 sinon.
*/
static int validate_proposal(const char *input, int niveau) {
    if(niveau > 1 && strchr(input, ' ') == NULL)
        return 0;
    char temp[256];
    strncpy(temp, input, sizeof(temp));
    temp[sizeof(temp) - 1] = '\0';
    int count = 0;
    char *token = strtok(temp, " \n\r\t");
    while(token != NULL) {
        count++;
        token = strtok(NULL, " \n\r\t");
    }
    return (count == niveau);
}

/*****************************************************************************/
/*--------------- Programme client -----------------------*/

int main(int argc, char *argv[]) {

  char *serveur = SERVEUR_DEFAUT; /* serveur par défaut */
  char *service = SERVICE_DEFAUT; /* numéro de service par défaut (port) */

  /* Analyse des paramètres */
  switch (argc) {
    case 1:
      printf("Serveur par défaut: %s\n", serveur);
      printf("Service par défaut: %s\n", service);
      break;
    case 2:
      serveur = argv[1];
      printf("Service par défaut: %s\n", service);
      break;
    case 3:
      serveur = argv[1];
      service = argv[2];
      break;
    default:
      printf("Usage: client serveur(nom ou @IP) service (nom ou port) \n");
      exit(1);
  }

  /* Lancer le traitement principal du client */
  client_appli(serveur, service);
  return 0;
}

/* Fonction auxiliaire pour afficher la réponse avec "Red:" en rouge */
static void print_response_with_color(const char *response) {
  const char *redTag = "Red:";
  char *pos = strstr((char *)response, redTag);
  if (pos != NULL) {
      int front_len = pos - response;
      /* Affiche la partie avant "Red:" */
      printf("Réponse du serveur : ");
      if(front_len > 0) {
          char front[256];
          strncpy(front, response, front_len);
          front[front_len] = '\0';
          printf("%s", front);
      }
      /* Affiche "Red:" en rouge */
      printf(ANSI_COLOR_RED "%s" ANSI_COLOR_RESET, redTag);
      /* Affiche le reste de la chaîne */
      printf("%s", pos + strlen(redTag));
  } else {
      printf("Réponse du serveur : %s", response);
  }
}

void client_appli(char *serveur, char *service)
/* Traitement du client de l'application Mastermind */
{
int sock;
struct sockaddr_in *adr_serv;
char buffer[256];

/* Création de la socket TCP */
sock = h_socket(AF_INET, SOCK_STREAM);
if (sock < 0) {
  fprintf(stderr, ANSI_COLOR_RED "Erreur lors de h_socket\n" ANSI_COLOR_RESET);
  exit(EXIT_FAILURE);
}

/* Renseignement de l'adresse du serveur */
adr_socket(service, serveur, SOCK_STREAM, &adr_serv);

/* Connexion au serveur */
h_connect(sock, adr_serv);

/* Réception du message de bienvenue du serveur */
memset(buffer, 0, sizeof(buffer));
if (read_line(sock, buffer, sizeof(buffer)) <= 0) {
  fprintf(stderr, ANSI_COLOR_RED "Erreur lors de la réception du message de bienvenue.\n" ANSI_COLOR_RESET);
  h_close(sock);
  exit(EXIT_FAILURE);
}
printf(ANSI_COLOR_GREEN "Message de bienvenue reçu du serveur: %s" ANSI_COLOR_RESET, buffer);
printf("Connecté au serveur %s sur le port %s.\n", serveur, service);

/* Boucle de jeu multiple sur la même connexion */
int session_en_cours = 1;
while (session_en_cours) {
  /* Saisie du niveau de jeu */
  printf(ANSI_COLOR_YELLOW "Entrez le nombre de positions (niveau) : " ANSI_COLOR_RESET);
  memset(buffer, 0, sizeof(buffer));
  if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
    fprintf(stderr, ANSI_COLOR_RED "Erreur lors de la saisie du niveau\n" ANSI_COLOR_RESET);
    break;
  }
  int niveau = atoi(buffer);
  if (niveau <= 0) {
    fprintf(stderr, ANSI_COLOR_RED "Niveau incorrect. Fin.\n" ANSI_COLOR_RESET);
    break;
  }

  /* Envoi de la commande LEVEL au serveur */
  memset(buffer, 0, sizeof(buffer));
  sprintf(buffer, "LEVEL %d\n", niveau);
  h_writes(sock, buffer, strlen(buffer));
  printf("Niveau %d envoyé au serveur.\n", niveau);

  /* Boucle de partie */
  int gagne = 0;
  while (!gagne) {
    int valid = 0;
    do {
      printf(ANSI_COLOR_YELLOW "Entrez votre proposition (ex: '1 2 3 ...' pour %d positions) ou tapez 'q' pour quitter cette partie: " ANSI_COLOR_RESET, niveau);
      memset(buffer, 0, sizeof(buffer));
      if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        fprintf(stderr, ANSI_COLOR_RED "Erreur lors de la saisie de la proposition.\n" ANSI_COLOR_RESET);
        valid = 0;
        break;
      }
      if (buffer[0] == 'q' || buffer[0] == 'Q') {
        printf(ANSI_COLOR_YELLOW "Vous avez choisi de quitter cette partie.\n" ANSI_COLOR_RESET);
        gagne = 1;  // Pour sortir de la boucle de partie
        valid = 1;
        break;
      }
      valid = validate_proposal(buffer, niveau);
      if (!valid) {
        printf(ANSI_COLOR_RED "Format incorrect. Veuillez saisir %d chiffre(s) séparé(s) par des espaces.\n" ANSI_COLOR_RESET, niveau);
      }
    } while (!valid);

    if (gagne)  // Si l'utilisateur a quitté la partie
      break;

    /* Envoi de la proposition au serveur */
    h_writes(sock, buffer, strlen(buffer));

    /* Lecture de la réponse du serveur */
    memset(buffer, 0, sizeof(buffer));
    if (read_line(sock, buffer, sizeof(buffer)) <= 0) {
      fprintf(stderr, ANSI_COLOR_RED "Erreur ou déconnexion du serveur.\n" ANSI_COLOR_RESET);
      session_en_cours = 0;
      break;
    }
    
    /* Affichage de la réponse avec "Red:" en rouge */
    if (strstr(buffer, "GAGNE!") != NULL) {
      printf(ANSI_COLOR_GREEN);
      print_response_with_color(buffer);
      printf(ANSI_COLOR_RESET);
      printf("\nVous avez gagné !\n");
      gagne = 1;
    } else {
      print_response_with_color(buffer);
    }
  } /* Fin de la partie */

  /* Demande de rejouer après la fin de la partie */
  printf(ANSI_COLOR_YELLOW "Voulez-vous rejouer ? (o/n) : " ANSI_COLOR_RESET);
  memset(buffer, 0, sizeof(buffer));
  if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
    fprintf(stderr, ANSI_COLOR_RED "Erreur lors de la saisie pour rejouer.\n" ANSI_COLOR_RESET);
    break;
  }
  h_writes(sock, buffer, strlen(buffer));
  if (buffer[0] != 'o' && buffer[0] != 'O') {
    printf(ANSI_COLOR_YELLOW "Fin de la session.\n" ANSI_COLOR_RESET);
    session_en_cours = 0;
  }
}
h_close(sock);
}