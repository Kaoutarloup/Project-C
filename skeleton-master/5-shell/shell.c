
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>


void* ealloc(void* prev, size_t size)//fais l'allocation
{
	void* ptr = realloc(prev, size);
	assert(ptr != NULL);
	return ptr;
}



char* triage(char* entree)//fais du triage
{
	if (NULL == entree) return NULL;
	char *p = entree;
    int l = strlen(entree);
    while(isspace(p[l - 1])) p[--l] = 0;
    while(*p && (isspace(*p) || '\0' == *p)) ++p, --l;
    memmove(entree, p, l + 1);//copie l+1 characteres de p dans entree
    return entree;
}


char* lisLigne()
{
	static const char EXIT[] = "exit";

	char* entree = NULL;
	unsigned long taille = 0;
	write(STDOUT_FILENO, "$ ", 2);
	getline(&entree, &taille, stdin);
	if ('\0' == *entree) return strdup(EXIT);
	entree = triage(entree);
	if ('\0' == *entree) {//\0 signifie null
		free(entree);
		return NULL;
	}
	return entree;
}


char* lisPartie(char* entree)
{
	static char first, *precedant;

	if (entree == NULL) entree = precedant;
	else first = entree[0];
	if (!first) return NULL;

	char* partie;
	entree[0] = first;

	if ('$' != first) {//si premier charactere est pas le $ 
		partie = entree;
		while(entree[0] && '$' != entree[0]) entree++;//avance dans la liste
	} else if('{' != entree[1]) {
		partie = entree++;//avance que de 1
		while(
			entree[0] && (isalnum(entree[0]) || entree[0] == '_' || entree[0] == '?')//continue d'avancer
		) entree++;
	} else {
		partie = ++entree;
		while(entree[0] && '}' != entree[0]) entree++;//avance jusqu'au crochet fermant
		if('}' == entree[0]) *(entree++) = '\0';//met un null 
		*partie = '$';
	}

	precedant = entree;//decalage de 1
	first = entree[0];
	entree[0] = '\0';

	return partie;
}

char* etendre(char* entree)
{
	size_t size = 0;
	char* buffer = NULL;
	while(entree[0] && isspace(entree[0])) entree++;//avance a l'espace

	char* partie = lisPartie(entree);
	while(partie != NULL) {
		if('$' == partie[0]) partie = getenv(partie + 1);//retourne null si la variable existe pas
		if(partie != NULL) {
			size_t len = strlen(partie);
			buffer = ealloc(buffer, (size + len + 1) * sizeof(char));//augmente la memoire reservée
			strcpy(buffer + size, partie);
			size += len;
		}
		partie = lisPartie(NULL);
	}

	return buffer;
}

char* lecture()//lis en appelant les autres fonctions
{
	char *entree = NULL, *etendu;
	do { entree = lisLigne(); }
	while(NULL == entree);
	etendu = etendre(entree);
	free(entree);
	return etendu;
}


int assignementCheck(char* entree)
{
	if (!(isalnum(entree[0]) || entree[0] == '_' || entree[0] == '?')) return 0;//retourne faux si commence par chiffre,lettre, _ ou ?
	while(entree[0] != '\0' && (isalnum(entree[0]) || entree[0] == '_' || entree[0] == '?')) entree++;
	return entree[0] == '=';//retourne vrai si il y a = apres les chiffres,lettre, _ ou ?
}

void assignementProgres(int status)
{
	char valeur[4];
	sprintf(valeur, "%d", status);
	setenv("?", valeur, 1);//ajoute une variable d'environment ou met -1 si erreur
}


void assignement(char* entree)
{
	char* place = strsep(&entree, "=");//cherche le = dans entree
	assignementProgres(setenv(place, entree, 1));//setenv retourne 0 si assez de place, -1 si erreur
}

char* creePartie(char* entree)//si null, retourne precedant
{
	static char *precedant;

	if(NULL == entree) entree = precedant;

	while(entree && entree[0] && isspace(entree[0])) entree++;//avance jusqu'a l'espace
	if(NULL == entree || '\0' == entree[0]) return NULL;

	char* partie;
	if ('"' == entree[0]) {
		partie = ++entree;
		while(entree[0] && '"' != entree[0]) entree++;//avance dans l'entree
	} else {
		partie = entree;
		while(entree[0] && !isspace(entree[0])) entree++;
	}

	if('\0' == entree[0]) precedant = NULL;
	else precedant = entree + 1;

	entree[0] = '\0';//\0 signifie null
	return partie;
}


typedef struct exec {
	pid_t id;
	struct exec* prev;
	int pipe[2];
	char* ecrire;
	char* lire;
	char* effacer;
	int argc;
	char** argv;
} exect;

exect* exectNouv(exect* precedant)
{
	exect* processus = ealloc(NULL, sizeof(exect));//va donner plus de memoire

	processus->prev = NULL;//remet les valeurs en place
	processus->argc = 0;
	processus->argv = ealloc(NULL, sizeof(char*));
	processus->ecrire = processus->lire = processus->effacer = NULL;

	if (NULL != precedant) {
		processus->prev = precedant;
		pipe(precedant->pipe);
	} else {
		processus->pipe[0] = processus->pipe[1] = 0;
	}

	return processus;
}

void libere(exect* processus)
{
	while (NULL != processus) {//va liberer de la memoire
		exect* prev = processus->prev;
		free(processus->argv);
		free(processus);
		processus = prev;
	}
}

void ajoute(exect* processus, char* argument)//ajoute un argument, de la memoire avec cette nouvelle donee, puis enleve le processus ajouté
{
	processus->argv[processus->argc++] = argument;
	processus->argv = ealloc(processus->argv, (processus->argc + 1) * sizeof(char*));
	processus->argv[processus->argc] = NULL;
}

void paralelle(exect* processus)//pour les pipes et forks
{
	while(NULL != processus)
	{
		processus->id = fork();
		if (processus->id < 0) {
			perror("fork(): erreur\n");
			exit(EXIT_FAILURE);
		}

		if (0 != processus->id) {
			if (0 != processus->pipe[0]) {
				close(processus->pipe[0]);
				close(processus->pipe[1]);
			}
			processus = processus->prev;
			continue;
		}


		if (NULL != processus->ecrire) {//contiendra adress fichier
			close(STDIN_FILENO);
			open(processus->ecrire, O_RDONLY);//va a l'adresse, ouvre en read
		}


		if (NULL != processus->lire) {//contiendra adresse fichier
			close(STDOUT_FILENO);
			open(processus->lire, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);//va a l'adress, ouvre en write only, si pas de fichier le cree, donne aussi des droits de lecture, ecriture et execution (s_irwxu)
		}


		if (NULL != processus->effacer) {//idem 
			close(STDERR_FILENO);
			open(processus->effacer, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);//idem aussi
		}


		if (0 != processus->pipe[0]) {
			close(STDOUT_FILENO);
			close(processus->pipe[STDIN_FILENO]);
			dup2(processus->pipe[STDOUT_FILENO], STDOUT_FILENO);
			close(processus->pipe[STDOUT_FILENO]);
		}


		if (NULL != processus->prev) {
			close(STDIN_FILENO);
			close(processus->prev->pipe[STDOUT_FILENO]);
			dup2(processus->prev->pipe[STDIN_FILENO], STDIN_FILENO);
			close(processus->prev->pipe[STDIN_FILENO]);
		}

		execvpe(processus->argv[0], processus->argv, environ);
		perror("commande inconnue\n");
		exit(EXIT_FAILURE);
	}
}

void attendre(exect* processus)//va attendre que des processus soit nulls
{
	int status;
	while (NULL != processus) {
		waitpid(processus->id, &status, 0);
		processus = processus->prev;
	}
	assignementProgres(WEXITSTATUS(status));
}

void execute(char* entree)
{
	int enPipe = 0, enFond = 0, enEchec = 0;
	exect *processus = exectNouv(NULL);

	char* partie = creePartie(entree);//refait un joli string pour travailler
	while (NULL != partie) {

		if (0 == strcmp(partie, "&")) {//le & pour tache secondaire
			enFond = 1;
		} else if (0 == strcmp(partie, "|")) {//le | pour pipeline
			enPipe = 1;
			processus = exectNouv(processus);
		} else if (0 == strcmp(partie, "<")) {//le < pour lire
			partie = creePartie(NULL);
			if (NULL == partie) {
				enEchec = 1;
				perror("manque nom de fichier\n");
				break;
			}
			processus->ecrire = partie;
		} else if (0 == strcmp(partie, ">")) {//le > pour ecrire
			partie = creePartie(NULL);
			if (NULL == partie) {
				enEchec = 1;
				perror("manque nom de fichier\n");
				break;
			}
			processus->lire = partie;
		} else if (0 == strcmp(partie, "2>")) {//pour effacer
			partie = creePartie(NULL);
			if (NULL == partie) {
				enEchec = 1;
				perror("manque nom de fichier\n");
				break;
			}
			processus->effacer = partie;
		} else {
			ajoute(processus, partie);
		}

		partie = creePartie(NULL);
	}

	if (0 == processus->argc) {//pas d'argument suivant
		enEchec = 1;
		perror("manque commande suivante\n");
	}

	if (enPipe && enFond) {//peux pas avoir les deux
		enEchec = 1;
		perror("pas de tache de fond en pipeline\n");
	}

	if (!enEchec) {//si pas echoué a cause d'autres erreurs
		paralelle(processus);
		if (!enFond) attendre(processus);
	}

	libere(processus);
}



int main(int argc, char** argv)
{
	char* entree = NULL;
	while(1)
	{
		entree = lecture();

		if (0 == strcmp(entree, "exit")) {//si on entre "exit", sort de la boucle, retourne exit success
			free(entree);//libere cet espace
			break;
		}

		if (assignementCheck(entree)) {//si assignement
			assignement(entree);//fais l'assignement
			free(entree);//libere
			continue;
		}

		execute(entree);//pour tous le reste
		free(entree);//libère
	}
	return EXIT_SUCCESS;
}
