/* Lauren Sherman */

#define _GNU_SOURCE
#include <stdio.h> 
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sched.h>

struct ExperInfoStruct
{
	int gridSize;
	char letter;
	unsigned int seed;
	int numSteps;
	int minString;
};

typedef struct ExperInfoStruct ExperInfo;

char ***grid;
pthread_mutex_t **mutex; 

void *randomWalk(void *arg);
int covered(int dimen, int minString); /* checks if min length there for every space */
void display(int dimen);
void allocGrid(int gridSize);
void allocMutex(int gridSize);

/* Start of main */
int main(int argc, char* argv[])
{
	int gridSize;
	int numSailors;
	int minVisits;
	int i;
	int j;
	ExperInfo *experInfo; /* make an array later */
	pthread_t *thd;
	int nThreads;
	int totalSteps = 0;

	/*if there are not enough input arguments*/
	if(argc < 4) 
	{
		printf("%s: usage %s inputFile outFile \n", argv[0], argv[0]);
		return 2; 
	}

	/* Set the input values to the floor size, number of sailors, and min visits */
	gridSize = atoi(argv[1]); 
	numSailors = atoi(argv[2]); 
	minVisits = atoi(argv[3]); 
	nThreads = numSailors;

	/* Error if grid size is not even */
	if(gridSize < 2 || gridSize%2 != 0)
	{
		printf("Error: Grid size not an even number\n");
		exit(2);
	}
	/* Error if more than 52 sailors */
	if(numSailors > 52)
	{
		printf("Error: Too many sailors\n");
		exit(3);
	}

	/* Allocate grid */
	allocGrid(gridSize);

	thd = (pthread_t *) malloc(sizeof(pthread_t)*nThreads);
	experInfo = (ExperInfo *) malloc(sizeof(ExperInfo)*nThreads);

	printf("Floor is %d by %d, there are %d sailors, and min visits is %d\n", gridSize, gridSize, numSailors, minVisits);
	fflush(stdout);

	/* Allocate mutex */
	allocMutex(gridSize);

	/* For loop to make a thread for each sailor - has own struct and function call */
	for(i = 0; i < nThreads; i++)
	{
		experInfo[i].gridSize = gridSize;
		experInfo[i].letter = (char) ((int)'A' + i);
		experInfo[i].minString = minVisits;
		experInfo[i].seed = (i + 1)*10; 
		experInfo[i].numSteps = 0;

		pthread_create(&(thd[i]), NULL, randomWalk, &(experInfo[i]));
	}

	for(i = 0; i < nThreads; i++)
	{
		pthread_join(thd[i], NULL);
	}
	
	display(gridSize); 

	for(i=0; i < nThreads; i++)
	{
		printf("Sailor %c takes %d steps \n", experInfo[i].letter, experInfo[i].numSteps);
		fflush(stdout);
		totalSteps += experInfo[i].numSteps;
	}
	printf("Total number of steps taken: %d\n", totalSteps);
	fflush(stdout);

	/* Free memory */
	for(i = 0; i < gridSize; i++)
	{
		for(j = 0; j < gridSize; j++)
		{
			free(grid[i][j]);
			grid[i][j] = NULL;
		}
		free(grid[i]);
	}
	free(grid);

	for(i = 0; i < (gridSize/2); i++)
	{
		free(mutex[i]);
	}
	free(mutex);
	free(thd);
	free(experInfo);

	return 0;
}

/* Functoin to allocate grid */
void allocGrid(int gridSize)
{
	int i;
	int j;
	grid = (char***) malloc(gridSize*sizeof(char**));
	for(i = 0; i < gridSize; i++)
	{
		grid[i] = (char **) malloc(gridSize*sizeof(char*));
		for(j = 0; j < gridSize; j++)
		{
			grid[i][j] = NULL;
		}
	}
}

/* Function to allocate mutex */
void allocMutex(int gridSize)
{
	int i;
	int j;
	mutex = (pthread_mutex_t**) malloc((gridSize/2)*sizeof(pthread_mutex_t*));
	for(i = 0; i < (gridSize/2); i++)
	{
		mutex[i] = (pthread_mutex_t*) malloc((gridSize/2)*sizeof(pthread_mutex_t));
		for(j = 0; j < (gridSize/2); j++)
		{
			pthread_mutex_init(&mutex[i][j], NULL);
		}
	} 
}

void *randomWalk(void *arg)
{
	ExperInfo info = *((ExperInfo *) arg);
	int i;
	int r;
	int c;
	int done;
	char *temp; 
	int gridSize = info.gridSize;
	int rMove = 0; /* Distance sailor will move in the row */
	int cMove = 0; /* Distance sailor will move in the column */
	info.numSteps = 0;

	r = rand_r(&info.seed)%gridSize;
	c = rand_r(&info.seed)%gridSize;

	/* While the sailor is not out of bounds */
	while(!covered(gridSize, info.minString))
	{
		done = 0;
		while(!done)
		{
			rMove = (rand_r(&info.seed)%3) - 1;
			cMove = (rand_r(&info.seed)%3) - 1;
			/* If rMove and cMove are both 0, go again */
			done = ((rMove!=0)||(cMove!=0));
		}
		
		r = (r + rMove)%gridSize;
		c = (c + cMove)%gridSize;
		if(r < 0)
		{
			r = gridSize - 1;
		}
		if(c < 0)
		{
			c = gridSize - 1;
		}

		pthread_mutex_lock(&mutex[r/2][c/2]);
		if(grid[r][c] == NULL)
		{
			grid[r][c] = (char *) malloc(sizeof(char)*2);
			grid[r][c][0] = info.letter;
			grid[r][c][1] = '\0';
		}
		else
		{
			temp = (char *) malloc(sizeof(char)*(strlen(grid[r][c])+2));
			for(i = 0; i < strlen(grid[r][c]); i++)
			{
				temp[i] = grid[r][c][i];
			}
			temp[strlen(grid[r][c])] = info.letter;
			temp[strlen(grid[r][c])+1] = '\0';

			if (grid[r][c] != NULL)
			{
				free(grid[r][c]);
			}

			grid[r][c] = temp; 
		}
		
		info.numSteps++;
		pthread_mutex_unlock(&mutex[r/2][c/2]);
		usleep(1);
		sched_yield();

	}
	
	*((ExperInfo *) arg) = info;
	return NULL;
}

/* Function to see if each grid has been visited the min num of times */
int covered(int dimen, int minVisits)
{
	int i;
	int j;

	for(i = 0; i < dimen; i++)
	{
		for(j = 0; j < dimen; j++)
		{
			pthread_mutex_lock(&mutex[i/2][j/2]);
			if (grid[i][j] == NULL || strlen(grid[i][j]) < minVisits)
			{
				pthread_mutex_unlock(&mutex[i/2][j/2]);
				return 0;
			}
			pthread_mutex_unlock(&mutex[i/2][j/2]);
			
		}
	}
	return 1;
}

/* Function to display the grid */
void display(int dimen)
{
	int i;
	int j;
	for(i = 0; i < dimen; i++)
	{
		for(j = 0; j < dimen; j++)
		{
			pthread_mutex_lock(&mutex[i/2][j/2]);
			printf("[%s] ", grid[i][j]);
			pthread_mutex_unlock(&mutex[i/2][j/2]);
		}
		printf("\n");
	}
}
