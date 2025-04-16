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

#include <curses.h>
#include <stdio.h>

#include <stdlib.h>
#include <sys/signal.h>
#include <sys/wait.h>

#include "fon.h" /* Primitives de la boite a outils */

#include <string.h>
#include <time.h>
#include <unistd.h> /* Pour read() */

/* Définition des codes couleurs ANSI pour l'affichage */
#define ANSI_COLOR_GREEN "\033[32m"
#define ANSI_COLOR_RESET "\033[0m"

/* Définition du numéro de service par défaut */
#define SERVICE_DEFAUT "1111"

/* Prototypes des fonctions auxiliaires */
int *generate_secret_code(int niveau); // Génère un code secret aléatoire
void calculate_result(int niveau, int secret[], int proposition[], int *red,
                      int *white); // Compare la proposition avec le code secret
                                   // pour calculer les pointeurs Red et White
static int
read_line(int sock, char *buf,
          int max_len); // Lit une ligne depuis la socket (pareil dans client.c)

/* Prototype de la fonction qui gère une session client individuelle */
void game_session(int sock_client);

void serveur_appli(char *service); /* programme serveur */

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

  /* Pour éviter les processus zombies, on ignore SIGCHLD */
  signal(SIGCHLD, SIG_IGN);

  /* service est le service (ou numero de port) auquel sera affecte
       ce serveur*/

  serveur_appli(service);
  return 0;
}

void serveur_appli(char *service)
/* Traitement du serveur en mode parallèle */
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

  /* Boucle infinie d'acceptation de connexions entrantes */
  while (1) {
    sock_client = h_accept(sock, &adr_client);
    if (sock_client < 0) {
      perror("Erreur lors de l'acceptation d'une connexion");
      continue;
    }
    printf("Connexion établie avec un client.\n");

    /* Création d'un processus fils pour gérer cette session client */
    pid_t pid = fork();
    if (pid < 0) {
      perror("Erreur lors de fork");
      h_close(sock_client);
      continue;
    }
    if (pid == 0) {
      /* Processus fils */
      h_close(sock); // Le fils n'a pas besoin de la socket d'écoute
      game_session(sock_client);
      /* La fonction game_session se charge de fermer la socket et de terminer
       */
    } else {
      /* Processus père : il ferme la socket client et retourne à l'accept */
      h_close(sock_client);
    }
  }

  /* Note : on n'atteint jamais ce point */
  h_close(sock);
}

/*
   Fonction game_session :
   Gère une session de jeu avec un client. Cette fonction envoie le message de
   bienvenue, lit la commande LEVEL, lance le jeu (plusieurs parties si le
   client souhaite rejouer) et ferme la connexion à la fin.
*/
void game_session(int sock_client) {
  char buffer[256];
  int niveau;

  /* Envoi du message de bienvenue */
  memset(buffer, 0, sizeof(buffer));
  sprintf(buffer, "Bienvenue dans Mastermind !\n");
  h_writes(sock_client, buffer, strlen(buffer));
  printf("Message de bienvenue envoyé au client (PID fils: %d).\n", getpid());

  /* Boucle de session de jeu pour ce client */
  int session = 1;
  while (session) {
    /* Lecture de la commande LEVEL envoyée par le client */
    memset(buffer, 0, sizeof(buffer));
    if (read_line(sock_client, buffer, sizeof(buffer)) <= 0) {
      printf(
          "Erreur lors de la lecture de la commande LEVEL ou déconnexion.\n");
      break;
    }
    if (sscanf(buffer, "LEVEL %d", &niveau) != 1) {
      printf("Commande LEVEL non reconnue.\n");
      break;
    }
    printf("Niveau reçu : %d positions (client PID: %d).\n", niveau, getpid());

    srand(time(NULL)); // Initialisation du générateur de nombres aléatoires
    /* Génération du code secret */
    int *secret = generate_secret_code(niveau);
    printf(ANSI_COLOR_GREEN "Code secret généré pour le client (PID %d) : " ANSI_COLOR_RESET , getpid());
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
        printf("Erreur de lecture ou déconnexion du client (PID: %d).\n",
               getpid());
        gagne = 1;
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
    memset(buffer, 0, sizeof(buffer)); // on vide le buffer
    sprintf(buffer, "VOULEZ_VOUS_REJOUER?\n"); 
    h_writes(sock_client, buffer,
             strlen(buffer)); // on envoie la question au client
    // on lui demande s'il veut rejouer ou pas
    // on lui demande de nous donner une réponse
    memset(buffer, 0, sizeof(buffer));
    if (read_line(sock_client, buffer, sizeof(buffer)) <=
        0) { // on lit la réponse du client
      // on controlle si la lecture s'est bien passée
      // si la lecture n'est pas bien passée on sort de la boucle
      printf("Erreur lors de la lecture du choix de rejouer.\n");
      break;
    }



    if (!(buffer[0] == 'o' || buffer[0] == 'O')) {
      session = 0;
      printf("Le client (PID %d) a choisi de ne pas rejouer.\n", getpid());
    } else {
      printf("Nouvelle partie demandée par le client (PID %d).\n", getpid());
    }
  } /* Fin de la session de jeu pour ce client */

  h_close(sock_client);
  printf("Session terminée pour le client (PID %d).\n", getpid());
  exit(0);
}
/******************************************************************************/

/*
   Fonction auxiliaire : génération du code secret.
   Alloue et retourne un tableau d’entiers de taille 'niveau' contenant le code
   secret. Chaque élément est un nombre aléatoire entre 1 et 8 pour representer
   les couleurs.
*/
int *generate_secret_code(int niveau) {
  int *secret = (int *)malloc(
      niveau * sizeof(int)); // allocation de mémoire pour le code secret
  if (secret == NULL) {
    perror("Erreur d'allocation pour le code secret");
    exit(EXIT_FAILURE);
  }
  for (int i = 0; i < niveau; i++) { // on génère le code secret aléatoirement
    secret[i] = (rand() % 8) + 1;
  }
  return secret;
}

/*
   Fonction auxiliaire : calcul du résultat.
   Compare la proposition du client avec le code secret pour déterminer :
      - red   : nombre de couleurs bien placées,
      - white : nombre de couleurs présentes mais mal placées.
   Utilise deux passes pour marquer les positions correctes puis
   rechercher les correspondances restantes pour bien vérifier le valuer de
   White dans le cas où on a plusieurs couleurs identiques dans le code secret
*/
void calculate_result(int niveau, int secret[], int proposition[], int *red,
                      int *white) {
  *red = 0;
  *white = 0;
  int used_secret[niveau]; // tableau pour marquer les positions déjà utilisées
                           // dans le code secret
  int used_prop[niveau]; // tableau pour marquer les positions déjà utilisées
                         // dans la proposition
  // on initialise les tableaux à 0
  // pour dire qu'aucune position n'est utilisée encore
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
    if (!used_prop[i]) { // si la position n'est pas utilisée dans la
                         // proposition
      for (int j = 0; j < niveau; j++) {
        if (!used_secret[j] &&
            proposition[i] ==
                secret[j]) { // si la couleur est présente dans le code secret
          // et la position n'est pas utilisée dans le code secret
          // on incrémente le nombre de couleurs mal placées
          // et on marque la position comme utilisée
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
    n = read(sock, &c, 1); // Lecture d'un caractère au lieu de toute la ligne
    // on lit un caractère à la fois
    // et on le met dans le buffer
    if (n < 0) {
      perror("Erreur lors de read");
      return -1;
    }
    if (n == 0) {
      break;
    }
    buf[pos++] = c;
    if (c == '\n')
      break;
  }
  buf[pos] = '\0'; // on met le caractère de fin de chaine pour indiquer la fin
  return pos;
}
