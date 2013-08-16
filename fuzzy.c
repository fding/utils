/* fuzzy.c - fuzzy searching of file names.
 * Ranks all files/paths using levenshtein distance
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
#include <unistd.h>

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
    int v0[PATH_MAX+1];
    int v1[PATH_MAX+1];
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
            // deletion from  s2 better than from s1
            v1[j+1]=v1[j]+1<v0[j+1]+1 ? v1[j]+1 : v0[j+1]+1;
            if (v0[j]+cost<v1[j+1])
                v1[j+1]=v0[j]+cost;
        }
        for (j=0;j<=s2len;j++)
        {
            v0[j]=v1[j];
        }
    }
    return v1[s2len];
}

static void list_dir (const char * dir_name, const char * name, int show_all)
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
        if (d_name[0]=='.' && !show_all)
            continue;
        /* Print the name of the file and directory. */
        /*
        printf ("%s/%s\n", dir_name, d_name);
        */
        char full_name[PATH_MAX], non_period_name[PATH_MAX];
        strcpy(full_name,dir_name);
        strcat(full_name,"/");
        strcat(full_name,d_name);
        
        int i;
        for (i=0; i<strlen(d_name); i++)
        {
            if (d_name[i]=='.' && i>0) break;
            non_period_name[i]=d_name[i];
        }
        non_period_name[i]=0;

        int score=levenshtein(name,d_name);
        int score1=levenshtein(name,non_period_name);
        if (score1<score) score=score1;
        if (strstr(d_name,name))
        {
            int score2=(strlen(non_period_name)-strlen(name))/2;
            if (score2<score) score=score2;
        }
        
        if (score<max_distance)
        {
            scores[max_distance_index]=score;
            strcpy(files[max_distance_index],full_name);
            max_distance=score;
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

        if (entry->d_type == DT_DIR) {

            /* Check that the directory is not "d" or d's parent. */
            
            if (strcmp (d_name, "..") != 0 &&
                strcmp (d_name, ".") != 0) {
                int path_length;
                char path[PATH_MAX];
 
                path_length = snprintf (path, PATH_MAX,
                                        "%s/%s", dir_name, d_name);
                /* Recursively call "list_dir" with the new path. */
                if (path_length >= PATH_MAX) {
                    fprintf (stderr, "Path length has got too long.\n");
                    exit (EXIT_FAILURE);
                }
                else if (access(path, X_OK)==0)
                    list_dir (path, name, show_all);
                else
                    fprintf(stderr, "Unable to access %s\n", path);
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

int get_results(char* name, int show_all)
{
    int i;
    for (i=0; i<NUM_RESULTS;i++)
        scores[i]=10000;
    max_distance=10000;
    list_dir (".", name, show_all);

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
    {
        if (scores[i]>100 || scores[i]>6*(scores[0]+1)) break;
        int threshold=strlen(name);
        int j=0;
        int file_length=strlen(files[i]);
        for (j=file_length-1; j>=0 && files[i][j]!='/'; j--);
        if (file_length-j-1>threshold)
            threshold=file_length-j-1;
        if (scores[i]>=4*threshold/5) break;
        printf("%s\n",files[i]);
    }
    
    return 0;
}

void print_usage()
{
    printf("Usage: ctrlp [-a] phrase\n");
}

int main (int argc, char* argv[])
{
    char* name;
    int show_all=0;
    int i;
    name=NULL;
    if (argc==1)
    {
        print_usage();
        return 1;
    }
    for (i=1; i<argc; i++)
    {
        if (argv[i][0]=='-')
        {
            if (strcmp(argv[i],"-a") || strcmp(argv[i],"--all"))
                show_all=1;
        }
        else
        {
            if (name)
            {
                print_usage();
                return 1;
            }
            name=argv[i];
        }
    }
    get_results(name, show_all);
    return 0;
}
