/******************************************************************************/
/*         Application: Mastermind - Serveur                                */
/******************************************************************************/
/*                                                                        */
/*            Programme  SERVEUR                                          */
/*                                                                        */
/******************************************************************************/
/*                                                                        */
/*    Auteurs : UNAY Arhan et GEMICIOGLU Utku                             */
/*    Date :  10.04.2025                                                  */
/*                                                                        */
/******************************************************************************/

#include "fon.h"            /* Primitives de la boite à outils */
#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>         /* Pour read() */

/* Définition du numéro de service par défaut */
#define SERVICE_DEFAUT "1111"

/* Prototypes des fonctions auxiliaires */
int *generate_secret_code(int niveau);
void calculate_result(int niveau, int secret[], int proposition[], int *red, int *white);
static int read_line(int sock, char *buf, int max_len);

void serveur_appli(char *service); /* Programme serveur */

/*****************************************************************************/
/*---------------- Programme serveur ------------------------------*/

int main(int argc, char *argv[]) {

  char *service = SERVICE_DEFAUT; /* numéro de service par défaut */
  switch (argc) {
    case 1:
      printf("Service par défaut = %s\n", service);
      break;
    case 2:
      service = argv[1];
      break;
    default:
      printf("Usage: serveur service (nom ou port)\n");
      exit(1);
  }
  serveur_appli(service);
  return 0;
}

void serveur_appli(char *service)
/* Traitement du serveur de l'application Mastermind */
{
  int sock, sock_client;
  struct sockaddr_in *adr_serv;
  struct sockaddr_in adr_client;
  char buffer[256];

  /* Création de la socket TCP */
  sock = h_socket(AF_INET, SOCK_STREAM);
  if (sock < 0) {
    perror("Erreur lors de h_socket");
    exit(EXIT_FAILURE);
  }

  /* Préparation de l'adresse d'écoute – écouter sur toutes les interfaces */
  adr_socket(service, NULL, SOCK_STREAM, &adr_serv);

  /* Association (bind) et mise en écoute */
  h_bind(sock, adr_serv);
  h_listen(sock, 5);
  printf("Serveur en écoute sur le port %s...\n", service);

  /* Acceptation d'une connexion client */
  sock_client = h_accept(sock, &adr_client);
  printf("Connexion établie avec un client.\n");

  /* Envoi d'un message de bienvenue au client */
  memset(buffer, 0, sizeof(buffer));
  sprintf(buffer, "Bienvenue dans Mastermind !\n");
  h_writes(sock_client, buffer, strlen(buffer));
  printf("Message de bienvenue envoyé au client.\n");

  /* Boucle de session de jeu sur une même connexion */
  int session = 1;
  while (session) {
    /* Lecture de la commande LEVEL envoyée par le client */
    memset(buffer, 0, sizeof(buffer));
    if (read_line(sock_client, buffer, sizeof(buffer)) <= 0) {
      printf("Erreur lors de la lecture de la commande LEVEL.\n");
      break;
    }

    int niveau;
    if (sscanf(buffer, "LEVEL %d", &niveau) != 1) {
      printf("Commande LEVEL non reconnue.\n");
      break;
    }
    printf("Niveau reçu : %d positions.\n", niveau);

    /* Génération du code secret */
    int *secret = generate_secret_code(niveau);
    printf("Code secret généré : ");
    for (int i = 0; i < niveau; i++) {
      printf("%d ", secret[i]);
    }
    printf("\n");

    /* Boucle de traitement d'une partie */
    int gagne = 0;
    while (!gagne) {
      memset(buffer, 0, sizeof(buffer));
      int nb_octets = read_line(sock_client, buffer, sizeof(buffer));
      if (nb_octets <= 0) {
        printf("Erreur de lecture ou déconnexion du client.\n");
        gagne = 1; /* Quitter la partie */
        break;
      }
      
      /* Décodage de la proposition */
      int *proposition = (int *)malloc(niveau * sizeof(int));
      if (proposition == NULL) {
        perror("Erreur d'allocation mémoire pour la proposition");
        break;
      }
      int j = 0;
      char *token = strtok(buffer, " ");
      while (token != NULL && j < niveau) {
        proposition[j++] = atoi(token);
        token = strtok(NULL, " ");
      }
      if (j != niveau) {
        printf("Proposition incomplète reçue.\n");
        free(proposition);
        continue;
      }
      
      /* Calcul du résultat */
      int red, white;
      calculate_result(niveau, secret, proposition, &red, &white);
      free(proposition);
      
      memset(buffer, 0, sizeof(buffer));
      if (red == niveau) {
        sprintf(buffer, "GAGNE! Red: %d, White: %d\n", red, white);
        gagne = 1;
      } else {
        sprintf(buffer, "Red: %d, White: %d\n", red, white);
      }
      h_writes(sock_client, buffer, strlen(buffer));
    } /* Fin d'une partie */

    free(secret);

    /* Invitation à rejouer */
    memset(buffer, 0, sizeof(buffer));
    sprintf(buffer, "Voulez-vous rejouer ? (o/n) :\n");
    h_writes(sock_client, buffer, strlen(buffer));
    memset(buffer, 0, sizeof(buffer));
    if (read_line(sock_client, buffer, sizeof(buffer)) <= 0) {
      printf("Erreur lors de la lecture du choix de rejouer.\n");
      break;
    }
    if (!(buffer[0] == 'o' || buffer[0] == 'O')) {
      session = 0;
      printf("Le client a choisi de ne pas rejouer.\n");
    } else {
      printf("Nouvelle partie demandée par le client.\n");
    }
  } /* Fin de la session de jeux */

  h_close(sock_client);
  h_close(sock);
  printf("Connexion terminée.\n");
}

/*
   Fonction auxiliaire : génération du code secret.
   Alloue et retourne un tableau d’entiers de taille 'niveau' contenant le code secret.
   Chaque élément est un nombre aléatoire entre 1 et 8.
*/
int *generate_secret_code(int niveau) {
  int *secret = (int *)malloc(niveau * sizeof(int));
  if (secret == NULL) {
    perror("Erreur d'allocation pour le code secret");
    exit(EXIT_FAILURE);
  }
  for (int i = 0; i < niveau; i++) {
    secret[i] = (rand() % 8) + 1;
  }
  return secret;
}

/*
   Fonction auxiliaire : calcul du résultat.
   Compare la proposition du client avec le code secret pour déterminer :
      - red   : nombre de couleurs bien placées,
      - white : nombre de couleurs présentes mais mal placées.
   Cette version utilise deux passes pour marquer les positions correctes puis
   rechercher les correspondances restantes.
*/
void calculate_result(int niveau, int secret[], int proposition[], int *red, int *white) {
  *red = 0;
  *white = 0;
  int used_secret[niveau];
  int used_prop[niveau];
  for (int i = 0; i < niveau; i++) {
    used_secret[i] = 0;
    used_prop[i] = 0;
  }
  /* Première passe : compter les "red" */
  for (int i = 0; i < niveau; i++) {
    if (secret[i] == proposition[i]) {
      (*red)++;
      used_secret[i] = 1;
      used_prop[i] = 1;
    }
  }
  /* Deuxième passe : compter les "white" */
  for (int i = 0; i < niveau; i++) {
    if (!used_prop[i]) {
      for (int j = 0; j < niveau; j++) {
        if (!used_secret[j] && proposition[i] == secret[j]) {
          (*white)++;
          used_secret[j] = 1;
          break;
        }
      }
    }
  }
}

/* Fonction auxiliaire pour lire une ligne depuis la socket */
static int read_line(int sock, char *buf, int max_len) {
  int pos = 0;
  char c;
  int n;
  while (pos < max_len - 1) {
    n = read(sock, &c, 1);
    if(n < 0) {
      perror("Erreur lors de read");
      return -1;
    }
    if(n == 0) {
      break;
    }
    buf[pos++] = c;
    if(c == '\n')
      break;
  }
  buf[pos] = '\0';
  return pos;
}
