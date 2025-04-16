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

void serveur_appli(char *service); /* programme serveur */

/* Prototypes des fonctions auxiliaires */
int *generate_secret_code(int niveau); // Génère un code secret aléatoire
void calculate_result(int niveau, int secret[], int proposition[], int *red,
                      int *white); // Compare la proposition avec le code secret
                                   // pour calculer les pointeurs Red et White
static int
read_line(int sock, char *buf,
          int max_len); // Lit une ligne depuis la socket (pareil dans client.c)

/*****************************************************************************/
/*---------------- Programme serveur ------------------------------*/

int main(int argc, char *argv[]) {

  char *service = SERVICE_DEFAUT; /* numéro de service par défaut */

  /* Permet de passer un nombre de parametre variable a l'executable */
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

  /* service est le service (ou numero de port) auquel sera affecte
        ce serveur*/

  serveur_appli(service);

  return 0;
}

/******************************************************************************/
void serveur_appli(char *service)
/* Procedure correspondant au traitemnt du serveur de votre application */

{
  int sock, sock_client;         // Socket d'écoute et de connexion
  struct sockaddr_in *adr_serv;  // Adresse du serveur
  struct sockaddr_in adr_client; // Adresse du client
  char buffer[256];              // Buffer pour les messages

  sock = h_socket(AF_INET, SOCK_STREAM); // création de la socket TCP en
                                         // utilisant IP protocol family.
  // Comme décrit dans la page 4 du SOCKET.pdf, le mode de la socket est
  // SOCK_STREAM pour TCP.
  if (sock < 0) { // si la socket n'est pas créée on affiche une erreur
    perror("Erreur lors de h_socket");
    exit(EXIT_FAILURE); // sortie du programme
  }

  /* Préparation de l'adresse d'écoute – écouter sur toutes les interfaces */
  adr_socket(service, NULL, SOCK_STREAM, &adr_serv);

  /* Association (bind) et mise en écoute */
  h_bind(sock, adr_serv); // lie la socket à l'adresse server (avec no port)
  h_listen(sock, 5);      // met la socket en écoute
  printf("Serveur en écoute sur le port %s...\n", service);

  /* Acceptation d'une connexion client */
  sock_client = h_accept(sock, &adr_client);
  printf("Connexion établie avec un client.\n");

  /* Envoi d'un message de bienvenue au client */
  // c'est comme un test de connection
  memset(buffer, 0, sizeof(buffer));
  sprintf(buffer, "Bienvenue dans Mastermind !\n");
  h_writes(sock_client, buffer, strlen(buffer));
  printf("Message de bienvenue envoyé au client.\n");

  /* Boucle de session de jeu sur une même connexion */
  int session = 1; // pour controller le deroulement de la session
  // on initialise la session à 1 pour que le client puisse jouer
  // on va lui demander de jouer jusqu'à ce qu'il ne veuille plus
  // ou qu'il y ait une erreur
  while (session) {
    /* Lecture de la commande LEVEL envoyée par le client */
    memset(buffer, 0, sizeof(buffer)); // on vide le buffer
    if (read_line(sock_client, buffer, sizeof(buffer)) <=
        0) { // read and prend la valeur de niveau donnée par le client dans un
             // buffer
      printf("Erreur lors de la lecture de la commande LEVEL.\n");
      break;
    }

    int niveau;
    if (sscanf(buffer, "LEVEL %d", &niveau) !=
        1) { // extraction du niveau depuis un buffer
      printf("Commande LEVEL non reconnue.\n");
      break;
    }
    printf("Niveau reçu : %d positions.\n", niveau);

    /* Génération du code secret */
    srand(time(NULL)); // Initialisation du générateur de nombres aléatoires
    int *secret = generate_secret_code(
        niveau); // on génère le code secret avec une methode auxiliare
    printf(ANSI_COLOR_GREEN  "Code secret généré : "  ANSI_COLOR_RESET);
    for (int i = 0; i < niveau; i++) {
      printf("%d ", secret[i]); // on affiche le code secret
    }
    printf("\n");

    /* Boucle de traitement d'une partie */
    int gagne = 0; // une variable pour controller si le client a gagné ou pas
    // on lui demande de nous donner une proposition jusqu'à ce qu'il gagne ou
    // quitte
    while (!gagne) {
      memset(buffer, 0, sizeof(buffer)); // on vide le buffer
      int nb_octets =
          read_line(sock_client, buffer,
                    sizeof(buffer)); // on lit la proposition du client avec une
                                     // methode auxiliaire
      // on controlle si la lecture s'est bien passée
      if (nb_octets <= 0) {
        printf("Erreur de lecture ou déconnexion du client.\n");
        gagne = 1; /* Quitter la partie */
        break;
      }

      /* Décodage de la proposition */
      int *proposition = (int *)malloc(
          niveau * sizeof(int)); // allocation de mémoire pour la proposition
      if (proposition ==
          NULL) { // on controlle si l'allocation s'est bien passée
        perror("Erreur d'allocation mémoire pour la proposition");
        break;
      }
      int j = 0;
      char *token = strtok(
          buffer, " "); // on découpe la proposition en fonction des espaces
      // et puis on les convertit en entiers un par un (dans un tableau de
      // propostion qui était déjà alloué)
      while (token != NULL && j < niveau) {
        proposition[j++] =
            atoi(token); // conversion de la chaine de caractère en entier
        token = strtok(NULL, " "); // on continue à découper la chaine
      }
      if (j != niveau) { // on controlle si la proposition est complète
        printf("Proposition incomplète reçue.\n");
        free(proposition); // on libère la mémoire allouée
        continue;
      }

      /* Calcul du résultat */
      int red, white; // on initialise le nombre de couleurs bien placées et mal
                      // placées
      calculate_result(
          niveau, secret, proposition, &red,
          &white); // on utlise une methode auxiliare pour assigner les valeurs
      free(proposition); // on libère la mémoire allouée

      memset(buffer, 0, sizeof(buffer)); // on vide le buffer
      if (red == niveau) { // si on a gagné ca veut dire on trouve le code
                           // secret (rouge=niveau)
        sprintf(buffer, "GAGNE! Red: %d, White: %d\n", red, white);
        gagne = 1; // sortir de la boucle
      } else {
        sprintf(buffer, "Red: %d, White: %d\n", red, white);
      }
      h_writes(sock_client, buffer,
               strlen(buffer)); // on envoie le résultat au client
               
    } /* Fin d'une partie */

    free(secret); // on libère la mémoire allouée pour le code secret

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



    if (!(buffer[0] == 'o' ||
          buffer[0] ==
              'O')) { // si le client ne veut pas rejouer on ferme la session
      session = 0;
      printf("Le client a choisi de ne pas rejouer.\n");
    } else { // sinon on continue à jouer
      printf("Nouvelle partie demandée par le client.\n");
    }
  }

  h_close(sock_client); // on ferme la socket client
  h_close(sock);        // on ferme la socket serveur
  printf("Connexion terminée.\n");
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
