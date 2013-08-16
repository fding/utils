/* TODO:
1. Sort results
2. Set result files to environment variables
3. Optimize levenshtein by removing mallocs
4. More precise directory walking (walk only if directory is file)
5. Include option for ignoring hidden files
6. Code error checking
*/
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
/* "readdir" etc. are defined here. */
#include <dirent.h>
/* limits.h defines "PATH_MAX". */
#include <linux/limits.h>

/* List the files in "dir_name". */
#define NUM_RESULTS 10

char  files[NUM_RESULTS][PATH_MAX];
int   scores[NUM_RESULTS]; 
int   max_distance;
int   max_distance_index;

int levenshtein(const char * s1, const char * s2)
{
    int s1len=strlen(s1);
    int s2len=strlen(s2);
    if (strcmp(s1,s2)==0) return 0;
    if (s1len==0) return s2len;
    if (s2len==0) return s1len;
    if (s2len-s1len>max_distance || s1len-s2len>max_distance) return max_distance+1;
    int *v0=(int *)malloc(sizeof(int)*(s2len+1));
    int *v1=(int *)malloc(sizeof(int)*(s2len+1));
    int i;
    for (i=0; i<=s2len; i++)
        v0[i]=i;
    for (i=0; i<s1len; i++)
    {
        v1[0]=i+1;
        int j;
        for (j=0; j<s2len; j++)
        {
            int cost=(s1[i]==s2[j]) ? 0 : 1;
            v1[j+1]=v1[j]<v0[j+1] ? v1[j]+1 : v0[j+1]+1;
            if (v0[j]+cost<v1[j+1])
                v1[j+1]=v0[j]+cost;
        }
        for (j=0;j<=s2len;j++)
        {
            v0[j]=v1[j];
        }
    }
    int ret_val=v1[s2len];
    free(v0);
    free(v1);
    return ret_val;
}


static void
list_dir (const char * dir_name, const char * name)
{
    DIR * d;

    /* Open the directory specified by "dir_name". */

    d = opendir (dir_name);

    /* Check it was opened. */
    if (! d) {
        fprintf (stderr, "Cannot open directory '%s': %s\n",
                 dir_name, strerror (errno));
        exit (EXIT_FAILURE);
    }
    while (1) {
        struct dirent * entry;
        const char * d_name;

        /* "Readdir" gets subsequent entries from "d". */
        entry = readdir (d);
        if (! entry) {
            /* There are no more entries in this directory, so break
               out of the while loop. */
            break;
        }
        d_name = entry->d_name;
        /* Print the name of the file and directory. */
        //printf ("%s/%s\n", dir_name, d_name);
        char full_name[PATH_MAX];
        strcpy(full_name,dir_name);
        strcat(full_name,"/");
        strcat(full_name,d_name);

        int score=levenshtein(name,full_name);
        int score1=levenshtein(name,d_name);
        if (score1<score) score=score1;
        if (score<max_distance)
        {
            scores[max_distance_index]=score;
            strcpy(files[max_distance_index],full_name);
            max_distance=score;
            int i;
            for (i=0; i<NUM_RESULTS; i++)
            {
                if (scores[i]>max_distance)
                {
                    max_distance_index=i;
                    max_distance=scores[i];
                }
            }
        }

        /* See if "entry" is a subdirectory of "d". */

        if (entry->d_type & DT_DIR) {

            /* Check that the directory is not "d" or d's parent. */
            
            if (strcmp (d_name, "..") != 0 &&
                strcmp (d_name, ".") != 0) {
                int path_length;
                char path[PATH_MAX];
 
                path_length = snprintf (path, PATH_MAX,
                                        "%s/%s", dir_name, d_name);
                /*
                int score=levenshtein(name,path);
                if (score<max_distance)
                {
                    scores[max_distance_index]=score;
                    strcpy(files[max_distance_index],path);
                    max_distance=score;
                    for (int i=0; i<NUM_RESULTS; i++)
                    {
                        if (scores[i]>max_distance)
                        {
                            max_distance_index=i;
                            max_distance=scores[i];
                        }
                    }
                }
                */
                if (path_length >= PATH_MAX) {
                    fprintf (stderr, "Path length has got too long.\n");
                    exit (EXIT_FAILURE);
                }
                /* Recursively call "list_dir" with the new path. */
                list_dir (path, name);
            }
        }
    }
    /* After going through all the entries, close the directory. */
    if (closedir (d)) {
        fprintf (stderr, "Could not close '%s': %s\n",
                 dir_name, strerror (errno));
        exit (EXIT_FAILURE);
    }
}

int main (int argc, char* argv[])
{
    int i;
    for (i=0; i<NUM_RESULTS;i++)
        scores[i]=10000;
    max_distance=10000;
    list_dir (".", argv[1]);

    // Insertion sort, there's only 10 stuff to sort anyways
    for (i=0; i<NUM_RESULTS; i++)
    {
        int j,k,tmp;
        char tmpfile[PATH_MAX];
        for (j=0; scores[j]<=scores[i] && j<i; j++);
        if (j==i) continue;
        tmp=scores[i];
        strcpy(tmpfile,files[i]);
        for (k=i; k>j; k--)
        {
            scores[k]=scores[k-1];
            strcpy(files[k], files[k-1]);
        }
        scores[j]=tmp;
        strcpy(files[j],tmpfile);
    }

    for (i=0; i<NUM_RESULTS;i++)
        printf("%s (score=%d) \n",files[i],scores[i]);
    return 0;
}
