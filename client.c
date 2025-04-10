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

#include <stdio.h>
// #include <curses.h> 		/* Primitives de gestion d'ecran */
#include <stdlib.h>
#include <sys/signal.h>
#include <sys/wait.h>

#include "fon.h" /* primitives de la boite a outils */

#include <string.h>
#include <unistd.h> /* Pour read() */

/* Définition des codes couleurs ANSI pour l'affichage */
#define ANSI_COLOR_RED "\033[31m"
#define ANSI_COLOR_GREEN "\033[32m"
#define ANSI_COLOR_YELLOW "\033[33m"
#define ANSI_COLOR_RESET "\033[0m"

#define SERVICE_DEFAUT "1111"
#define SERVEUR_DEFAUT "127.0.0.1"

void client_appli(char *serveur, char *service);

//* Prototypes des fonctions auxiliaires */
static int read_line(
    int sock, char *buf,
    int max_len); // Lit une ligne depuis la socket (pareil dans serveur.c)
static int validate_proposal(const char *input,
                             int niveau); // pour controller le format du
                                          // proposal (donnée par utilisateur)

static void print_response_with_color(
    const char *response); // pour afficher la réponse avec des couleurs

/*****************************************************************************/
/*--------------- Programme client -----------------------*/

int main(int argc, char *argv[]) {

  char *serveur = SERVEUR_DEFAUT; /* serveur par défaut */
  char *service =
      SERVICE_DEFAUT; /* numero de service par defaut (no de port) */

  /* Permet de passer un nombre de parametre variable a l'executable */
  switch (argc) {
  case 1: /* arguments par defaut */
    printf("Serveur par défaut: %s\n", serveur);
    printf("Service par défaut: %s\n", service);
    break;
  case 2: /* serveur renseigne  */
    serveur = argv[1];
    printf("Service par défaut: %s\n", service);
    break;
  case 3: /* serveur, service renseignes */
    serveur = argv[1];
    service = argv[2];
    break;
  default:
    printf("Usage: client serveur(nom ou @IP) service (nom ou port) \n");
    exit(1);
  }

  /* serveur est le nom (ou l'adresse IP) auquel le client va acceder */
  /* service le numero de port sur le serveur correspondant au  */
  /* service desire par le client */

  client_appli(serveur, service);

  return 0;
}

/*****************************************************************************/
void client_appli(char *serveur, char *service)
/* procedure correspondant au traitement du client de votre application */

{
  int sock;                     // Socket de connexion
  struct sockaddr_in *adr_serv; // Adresse du serveur
  char buffer[256];             // Buffer pour les messages

  /* Création de la socket TCP */
  sock = h_socket(AF_INET, SOCK_STREAM); // création de la socket TCP en
                                         // utilisant IP protocol family.
  // Comme décrit dans la page 4 du SOCKET.pdf, le mode de la socket est
  // SOCK_STREAM pour TCP.
  if (sock < 0) { // si la socket n'est pas créée on affiche une erreur
    fprintf(stderr,
            ANSI_COLOR_RED "Erreur lors de h_socket\n" ANSI_COLOR_RESET);
    exit(EXIT_FAILURE); // sortie du programme
  }

  /* Renseignement de l'adresse du serveur */
  adr_socket(service, serveur, SOCK_STREAM,
             &adr_serv); // adr_serv contient maintenant les adresses du serveur

  /* Connexion au serveur */
  h_connect(sock, adr_serv);

  /* Réception du message de bienvenue du serveur */
  memset(buffer, 0, sizeof(buffer)); // on vide le buffer
  // on lit le message de bienvenue du serveur
  // on utilise une fonction auxiliaire pour lire une ligne depuis la socket
  if (read_line(sock, buffer, sizeof(buffer)) <= 0) {
    fprintf(stderr, ANSI_COLOR_RED "Erreur lors de la réception du message de "
                                   "bienvenue.\n" ANSI_COLOR_RESET);
    h_close(sock);
    exit(EXIT_FAILURE);
  }
  printf(ANSI_COLOR_GREEN
         "Message de bienvenue reçu du serveur: %s" ANSI_COLOR_RESET,
         buffer);
  printf("Connecté au serveur %s sur le port %s.\n", serveur, service);

  /* Boucle de jeu multiple sur la même connexion */
  int session_en_cours = 1; // pour controller le deroulement de la session
  // on initialise la session à 1 pour que le client puisse jouer
  // on va lui demander de jouer jusqu'à ce qu'il ne veuille plus
  // ou qu'il y ait une erreur
  while (session_en_cours) {
    /* Saisie du niveau de jeu */
    printf(ANSI_COLOR_YELLOW
           "Entrez le nombre de positions (niveau) : " ANSI_COLOR_RESET);
    memset(buffer, 0, sizeof(buffer));
    if (fgets(buffer, sizeof(buffer), stdin) == NULL) { // lecture de la saisie
      fprintf(stderr, ANSI_COLOR_RED
              "Erreur lors de la saisie du niveau\n" ANSI_COLOR_RESET);
      break;
    }
    int niveau = atoi(buffer); // conversion de la saisie en entier
    if (niveau <= 0) {
      fprintf(stderr,
              ANSI_COLOR_RED "Niveau incorrect. Fin.\n" ANSI_COLOR_RESET);
      break;
    }

    /* Envoi de la commande LEVEL au serveur */
    memset(buffer, 0, sizeof(buffer));     // on vide le buffer
    sprintf(buffer, "LEVEL %d\n", niveau); // on met le niveau dans le buffer
    h_writes(
        sock, buffer,
        strlen(
            buffer)); // on envoie le niveau au serveur comme "LEVEL <niveau>"
    printf("Niveau %d envoyé au serveur.\n", niveau);

    /* Boucle de partie */
    int gagne = 0; // une variable pour controller si le client a gagné ou pas
    // on lui demande de nous donner une proposition jusqu'à ce qu'il gagne ou
    // quitte
    while (!gagne) {
      int valid = 0; // pour controller la validité de la saisie on repose la
                     // question tant que la saisie n'est pas valide
      do {
        printf(ANSI_COLOR_YELLOW
               "Entrez votre proposition (ex: '1 2 3 ...' pour %d positions) "
               "ou tapez 'q' pour quitter cette partie: " ANSI_COLOR_RESET,
               niveau);
        memset(buffer, 0, sizeof(buffer)); // on vide le buffer

        if (fgets(buffer, sizeof(buffer), stdin) ==
            NULL) { // on lit la proposition du client avec une methode
                    // auxiliare
          fprintf(
              stderr, ANSI_COLOR_RED
              "Erreur lors de la saisie de la proposition.\n" ANSI_COLOR_RESET);
          valid = 0;
          break;
        }
        if (buffer[0] == 'q' ||
            buffer[0] == 'Q') { // si l'utilisateur veut quitter
          printf(
              ANSI_COLOR_YELLOW
              "Vous avez choisi de quitter cette partie.\n" ANSI_COLOR_RESET);
          gagne = 1; // Pour sortir de la boucle de partie
          valid = 1; // pour sortir de la boucle de validation
          break;
        }
        // si non
        valid = validate_proposal(buffer,
                                  niveau); // on controlle si le saisie est
                                           // valide ou pas (control de format)
        if (!valid) {
          printf(ANSI_COLOR_RED
                 "Format incorrect. Veuillez saisir %d chiffre(s) séparé(s) "
                 "par des espaces.\n" ANSI_COLOR_RESET,
                 niveau);
        }
      } while (!valid);

      if (gagne) // Si l'utilisateur a quitté la partie
        break;

      /* Envoi de la proposition au serveur */
      h_writes(sock, buffer, strlen(buffer));

      /* Lecture de la réponse du serveur */
      memset(buffer, 0, sizeof(buffer)); // on vide le buffer
      if (read_line(sock, buffer, sizeof(buffer)) <=
          0) { // on lit la réponse du serveur aprés l'envoi du proposition
               // on controlle si la lecture s'est bien passée
        fprintf(stderr, ANSI_COLOR_RED
                "Erreur ou déconnexion du serveur.\n" ANSI_COLOR_RESET);
        session_en_cours = 0;
        break;
      }

      if (strstr(buffer, "GAGNE!") != NULL) { // si le client a gagné
        // on utilise une fonction auxiliaire pour afficher la réponse avec des
        // couleurs
        printf(ANSI_COLOR_GREEN);
        print_response_with_color(buffer);
        printf(ANSI_COLOR_RESET);
        printf("\nVous avez gagné !\n");
        gagne = 1;
      } else {
        print_response_with_color(buffer);
      }
    }

    /* Demande de rejouer après la fin de la partie */
    // printf(ANSI_COLOR_YELLOW "Voulez-vous rejouer ? (o/n) : "
    // ANSI_COLOR_RESET);
    memset(buffer, 0, sizeof(buffer)); // on vide le buffer
                                       /*
                                           if (fgets(buffer, sizeof(buffer), stdin) ==
                                           NULL) { // on lit la réponse du client
                                         fprintf(stderr, ANSI_COLOR_RED
                                                 "Erreur lors de la saisie pour rejouer.\n" ANSI_COLOR_RESET);
                                         break;
                                       }
                                           */
    if (read_line(sock, buffer, sizeof(buffer)) <= 0) { // on lit la réponse du
													  // serveur
      fprintf(stderr, "Erreur lors de la lecture de la question de rejouer.\n");
      break;
    }

    // On vérifie si le serveur a demandé de rejouer
    // On utilise la fonction strstr pour vérifier si la question de rejouer est
    // présente dans la réponse du serveur
    if (strstr(buffer, "VOULEZ_VOUS_REJOUER?") != NULL) {
      printf("Voulez-vous rejouer ? (o/n) : ");
      memset(buffer, 0, sizeof(buffer));
      if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        fprintf(stderr, "Erreur lors de la saisie pour rejouer.\n");
        break;
      }

      h_writes(sock, buffer, strlen(buffer)); // on envoie la réponse au
											  // serveur

      if (buffer[0] != 'o' && buffer[0] != 'O') {
        printf("Fin de la session.\n");
        session_en_cours = 0;
      }
    }
  }

  h_close(sock); // on ferme la socket
}

/*****************************************************************************/

/* Fonction auxiliaire pour lire une ligne depuis la socket, caractère par
 * caractère */
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
    if (n == 0) { // Connexion fermée par le pair
      break;
    }
    buf[pos++] = c;
    if (c == '\n')
      break;
  }
  buf[pos] = '\0';
  return pos;
}

/* Fonction auxiliaire pour valider le format de la proposition.
   Pour un niveau > 1, on exige que les chiffres soient séparés par au moins un
   espace. On vérifie ensuite que le nombre de tokens correspond exactement au
   nombre attendu (niveau). Retourne 1 si format valide, 0 sinon.
*/
static int validate_proposal(const char *input, int niveau) {
  if (niveau > 1 &&
      strchr(input, ' ') ==
          NULL) // on cherche le premier espace
                //  si le niveau est supérieur à 1 et qu'il n'y a pas d'espace
                //  on ne peut pas avoir de proposition valide
    return 0;
  char temp[256];
  strncpy(temp, input, sizeof(temp)); // on copie la chaîne d'entrée dans temp
  // pour ne pas modifier l'entrée d'origine
  temp[sizeof(temp) - 1] = '\0';
  int count = 0;
  char *token = strtok(temp, " \n\r\t"); // on decoupe la chaîne en tokens
  // on utilise strtok pour séparer les tokens en utilisant les espaces et les
  // caractères de nouvelle ligne comme séparateurs
  while (token != NULL) {
    count++;
    token = strtok(NULL, " \n\r\t");
  }
  return (count ==
          niveau); // on vérifie si le nombre de tokens est égal au niveau
}

/* Fonction auxiliaire pour afficher la réponse avec "Red:" en rouge */
static void print_response_with_color(const char *response) {
  const char *redTag = "Red:";
  char *pos = strstr((char *)response, redTag);
  if (pos != NULL) {
    int front_len = pos - response;
    /* Affiche la partie avant "Red:" */
    printf("Réponse du serveur : ");
    if (front_len > 0) {
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
