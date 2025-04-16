Jeu de Couleurs MASTERMIND - Client / Serveur
==================================

Ce projet implémente un jeu basé sur des communications entre un serveur et un ou plusieurs clients,
avec deux versions de serveur : itératif et parallèle.

Instructions de Jeu
-------------------

- Important : Suivez les instructions ci-dessous pour jouer correctement.
- Le jeu utilise des chiffres (1 à 8) à la place des couleurs. Chaque chiffre représente une couleur différente.

Saisie pendant le jeu :
- Pour indiquer le niveau :
  Tapez directement le chiffre du niveau, puis appuyez sur ENTER.

- Pour une proposition :
  Tapez les chiffres un par un avec un espace entre eux, comme ceci :
  1 2 3 4
  puis appuyez sur ENTER.

Compilation et Lancement
------------------------

1. Compilation :

    gmake clean all

2. Exécution :

Serveur Itératif :

    Dans deux terminaux séparés :

        ./serveur       # dans le premier terminal
        ./client        # dans le deuxième terminal

Serveur Parallèle :

    Dans autant de terminaux que vous souhaitez :

        ./serveur_parallele     # dans un terminal
        ./client                # dans un autre terminal
        ./client                # dans un autre terminal
        ...

    Le serveur parallèle accepte plusieurs connexions clients simultanées.

Exécution à Distance
--------------------

Il est aussi possible de jouer à distance :

    ./serveur [no_port]
    ./client [nom_ou_ip] [no_port]

- Exemple serveur :

      ./serveur 1234

- Exemple client :

      ./client 192.168.1.10 1234

Remarques Importantes
---------------------

- Ordre de lancement :
  Il faut lancer le serveur (serveur ou serveur_parallele) AVANT d'exécuter un client.

- Si vous lancez ./client avant le serveur, le client va se bloquer,
  ce qui est prévu par le fonctionnement du programme (puisqu’il n’arrive pas à se connecter).

- Il faut bien suivre les instructions de jeu donné en terminal pour que le programme fonctionne correctement.

Fichiers Principaux
-------------------

- serveur.c              --> serveur itératif
- serveur_parallele.c    --> serveur parallèle
- client.c               --> client

Amusez-vous bien !
