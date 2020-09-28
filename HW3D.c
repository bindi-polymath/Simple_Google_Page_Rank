#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "mpi.h"


int main(int argc, char *argv[]) {

	// Part D: MPI Sparse 
	int i, j;
	int K = 1000;
	// damping factor
	double q = 0.15;
	int NumPg = 160000; // make problem 100 times larger
	double b = 1.0 / NumPg;
	double s = 0.5;
	double startTime, endTime;
	int numprocs, local_m, myid;

	/* Initialize MPI and get number of processes and my number or rank*/
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
	MPI_Comm_rank(MPI_COMM_WORLD, &myid);

	/* Processor zero sets the number of rows per processor*/
	if (myid == 0)
	{
		// Number of rows of the matrix per processor
		local_m = NumPg / numprocs;
		startTime = MPI_Wtime();
	}
	/* Broadcast number of rows for each processor to all processes */
	MPI_Bcast(&local_m, 1, MPI_INT, 0, MPI_COMM_WORLD);

	// Create S Matrix
	int nnz = 2 * NumPg - 1; // Number of non-zero entries in Matrix S
	double *local_S = (double *)malloc(sizeof(double)*2*local_m);
	int *local_iS = (int *)malloc(sizeof(int)*(local_m+1));
	int *local_jS = (int *)malloc(sizeof(int)*2*local_m);  

	for (i = 0; i < (2 * local_m); i++)
	{
		local_S[i] = 0.5;
	}

	if (myid == 0)
	{
		local_S[2] = 1;
	}

	//// Now populate array local_iS
	for (i = 0; i <= local_m; i++)
	{
		local_iS[i] = 2 * (i + (myid*local_m));
	}

	if (myid == numprocs - 1)
	{
		local_iS[local_m] = nnz;
	}

		// Now populate array local_jS
		int counter = 0;
		int local_row = 0;
		int global_row;
		for (i = 0; i < local_m; i++)
		{
			global_row = local_row + (myid * local_m);
			local_jS[counter] = global_row - 1; 
			counter += 1;
			local_jS[counter] = global_row + 1;
			local_row += 1;
			counter += 1;
		}
		if (myid == 0)
		{
			local_jS[0] = 1;
			local_jS[1] = NumPg - 1;
	    }
														 
	// Create X vector
	double *local_X = (double *)malloc(sizeof(double)*local_m);
	for (i = 0; i < local_m; i++)
	{
		local_X[i] = 1.0 / NumPg;
	}

	// Sx = y
	double *local_y = (double *)malloc(sizeof(double)*local_m);
	double *global_X = (double *)malloc(sizeof(double)*NumPg);
	int m,k;

	// Start Timer
	if (myid == 0)
	{
		startTime = MPI_Wtime();
	}

	for (m = 0; m < K; m++)
	{
		MPI_Allgather(local_X, local_m, MPI_DOUBLE, global_X, local_m, MPI_DOUBLE, MPI_COMM_WORLD);
		
		for (i = 0; i < local_m; i++)
		{
			local_y[i] = 0.0;
			for (k = local_iS[i]; k < local_iS[i + 1]; k++)
			{
				local_y[i] += local_S[k-local_iS[0]] * global_X[local_jS[k-local_iS[0]]];
			}
		}

		for (i = 0; i < local_m; i++)
		{
			// implementing damping without extra storage
			local_y[i] = (1 - q)*local_y[i] + q / NumPg;
		}

		for (i = 0; i < local_m; i++)
		{
			local_X[i] = local_y[i];
		}
	}

	double *global_y = (double *)malloc(sizeof(double)*NumPg);
	MPI_Gather(local_y, local_m, MPI_DOUBLE, global_y, local_m, MPI_DOUBLE, 0, MPI_COMM_WORLD);

	// End Timer
	if (myid == 0)
	{
		endTime = MPI_Wtime();
		printf("\nRunTime: %.15f\n", endTime - startTime);
	}

	if (myid == 0)
	{
		// Find maximum value in Page Rank Vector
		double maximum;
		int location;
		int c;

		maximum = global_y[0];
		for (c = 1; c < NumPg; c++)
		{
			if (global_y[c] > maximum)
			{
				maximum = global_y[c];
				location = c + 1;
			}
		}

		printf("\nMaximum Value in Page Rank Vector is %.15f\n", maximum);

		// Find minimum value in Page Rank Vector
		double minimum;
		int location2;
		int d;

		minimum = global_y[0];

		for (d = 1; d < NumPg; d++)
		{
			if (global_y[d] < minimum)
			{
				minimum = global_y[d];
				location2 = d + 1;
			}
		}
		printf("\nMinimum Value in Page Rank Vector is %.15f\n", minimum);
	}

	free(global_X);
	free(global_y);
	free(local_S);
	free(local_X);
	free(local_y);
	free(local_iS);
	free(local_jS);
	
	MPI_Finalize();

	return 0;
}