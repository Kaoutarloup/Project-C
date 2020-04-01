#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

char* nomDeBase(char* path)
{
	size_t i = strlen(path) - 1;
	while(path[i] != '/') i--;//trouve le /, puis sors de la boucle
	return path + i + 1;
}


void renduFichier(char* nomFichier)
{
	struct stat file_stat;

	char* base = nomDeBase(nomFichier);

	if (-1 == lstat(nomFichier, &file_stat)) {//-1 = erreur
		printf("%s: non valide\n", base);
		return;
	}

	switch(file_stat.st_mode & S_IFMT) {
		case S_IFDIR:
			printf("%s/\n", base);//dossier avec un /
			break;

		case S_IFLNK:
			printf("%s@\n", base);//lien avec un @
			break;

		case S_IFREG:
			if (file_stat.st_mode & S_IEXEC) printf("%s*\n", base);//executables avec *, sinon rien
			else printf("%s\n", base);
			break;

		default:
			printf("%s: non valide\n", base);//aucun des cas ci dessus
			break;
	}
}

void renduDossier(char* nomDossier)
{
	DIR* dossier = opendir(nomDossier);
	if (dossier == NULL) {
		renduFichier(nomDossier);//si pas un dossier, passe a l'etape suivant
		return;
	}

	struct dirent* entree;
	while((entree = readdir(dossier)) != NULL) {//tans que sa reste un dossier
		if (entree->d_name[0] == '.') continue;

		char chemin[1024];
		strcpy(chemin, nomDossier);
		strcat(chemin, "/");
		strcat(chemin, entree->d_name);//fais du joli texte
		
		renduFichier(chemin);
	}

	closedir(dossier);
}

int main(int argc, char** argv)
{
	if(argc == 1) {
		renduDossier(".");//que 1 argument
		return 0;
	}

	if(argc == 2) {
		renduDossier(realpath(argv[1], NULL));//plusieurs arguments
		return 0;
	}

	perror("non valide, voir manuel");
	return 1;
}

