#include <stdio.h>
#include <string.h>
  
#define limiteTextes 100
#define limiteTexte 128
#define true 1
#define false 0
#define Boolean int


static Boolean separationTextes(char *source, char *destination)
{  
  int i,l;
  
  l = strlen(source);//longueur de texte
  for (i = 0; i < l; i++)//cherche dans ce texte
  {
    if (source[i] == '\\')
    {
      if (source[i+1] == 'n')// \\=\, n=n , \n = nouvelle ligne
      {
        source[i] = 0; //remplace le \ par 0
        strcpy(destination,&source[i+2]);//copies source[i+2] dans destination qui est source+1, donc enleve aussi le \n
        return true;
      }
    }
  }
  return false;
}


int main (int argc, char **argv)
{
  int i,k,nombreTextes;
  char listeTextes[limiteTextes][limiteTexte];
  int partieTriee[limiteTextes];  
  int taille;
  Boolean scanFini, inclu;
  Boolean montant = true;
  Boolean separe;
  

  for (i = 1; i < argc; i++)
  {
    if (strcmp(argv[i], "-r") == 0) //si -r, va reverser l'ordre
    {
      montant = false;   
    }
  }

  nombreTextes = 0;
  scanFini = false;
  do
  {
    if (NULL == fgets(listeTextes[nombreTextes], sizeof(listeTextes[nombreTextes]), stdin))//si a fini de lire, ou rencontre une erreur
    {
      scanFini = true;//met fin au scan
    } 
    else
    {
      i = strlen(listeTextes[nombreTextes]);
      if (i > 0) 
      {
        listeTextes[nombreTextes][i-1] = 0;
      }
    }
    
    do
    {
      separe = separationTextes(listeTextes[nombreTextes],listeTextes[nombreTextes+1]);//vrai si nouvelle ligne, et va enlever le /n 
      if (listeTextes[nombreTextes][0] != 0)
      {
 
        inclu = false;
        for (i = 0; (i < nombreTextes) && (!inclu); i++)
        {
          taille = strcoll(listeTextes[nombreTextes], listeTextes[partieTriee[i]]); 
          if ((montant && (taille < 0))        //compare les deux textes, strcoll donnant un negatif, positif ou 0
              || ((!montant) && (taille > 0))) //prend en compte si r a été utilisé ou non
          { 
          
            for (k = nombreTextes; k >= i; k--)
            {
              partieTriee[k] = partieTriee[k-1];//descend dans cette liste
            }
            partieTriee[i] = nombreTextes; //change le nombre contenu dans partietriee i
            nombreTextes++; 
            inclu = true;
          }
        }
        if (!inclu)
        { 
          partieTriee[nombreTextes] = nombreTextes;
          nombreTextes++;
        }
      }
    } while (separe);
  } while ((!scanFini) && (nombreTextes < limiteTextes));//scan jusqu'a la fin, ou a la limite
  for (i = 0; i < nombreTextes; i++)
  {
    puts(listeTextes[partieTriee[i]]);
  }     

  return 0;
}

