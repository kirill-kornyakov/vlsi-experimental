#ifndef __LOGSUMEXP__
#define __LOGSUMEXP__

#include "ObjectivesConstraints.h"

double CalcNetLSE(AppCtx* context, PetscScalar* solution, int netIdx);
double CalcNetLSEGradient(AppCtx* context, int netIdx, int idxInSolutionVector);
void GetClusterCoordinates(int clusterIdx, PetscScalar* coordinates, 
                           AppCtx* context, double& x, double& y);

void LSE_AddObjectiveAndGradient(AppCtx* context, PetscScalar* solution);

void PrecalcExponents(AppCtx* context, PetscScalar* solution);

#endif