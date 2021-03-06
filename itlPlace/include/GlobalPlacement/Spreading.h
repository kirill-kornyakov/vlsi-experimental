#ifndef __SPREADING__
#define __SPREADING__

#include "ObjectivesConstraints.h"

void QS_AddObjectiveAndGradient(AppCtx* context, PetscScalar* solution);

double SpreadingPenalty(AppCtx* user, PetscScalar* x);

int CalcMaxAffectedArea(double potentialSize, double binSize);
void ConstructBinGrid(HDesign& hd, AppCtx& context, int aDesiredNumberOfClustersAtEveryBin);

#endif