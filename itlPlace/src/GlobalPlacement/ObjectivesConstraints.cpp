#include "ObjectivesConstraints.h"
#include "WeightsRoutines.h"
#include "LogSumExp.h"
#include "SumOfDelays.h"
#include "LagrangianRelaxation.h"
#include "Spreading.h"

#include "time.h"
//FIXME:#include "omp.h"

extern timetype expTime;

void SetGradientToZero(double* gradient, int nComponents)
{
    for (int i = 0; i < nComponents; i++)
    {
        gradient[i] = 0.0;
    }
}

void NullCriteriaValues(AppCtx* context) 
{
    context->criteriaValues.objective = 0.0;
    context->criteriaValues.lse = 0.0;
    context->criteriaValues.lr = 0.0;
    context->criteriaValues.sod = 0.0;
    context->criteriaValues.spr = 0.0;
    context->criteriaValues.gLSENorm = 0.0;
    context->criteriaValues.gLRNorm = 0.0;
    context->criteriaValues.gSprNorm = 0.0;

    SetGradientToZero(context->criteriaValues.gLSE, context->nVariables);
    SetGradientToZero(context->criteriaValues.gSOD, context->nVariables);
    SetGradientToZero(context->criteriaValues.gLR,  context->nVariables);
    SetGradientToZero(context->criteriaValues.gQS,  context->nVariables);
}

void PrintDerivatives(int nClusterVariables, int nBufferVariables, double* gradient,
                      bool clusterX, bool clusterY, bool ki)
{
    const std::string format = "%.10f ";

    if (clusterX)
    {
        ALERT("Cluster X derivatives:");
        for (int i = 0; i < nClusterVariables/2; i++)
        {
            WRITE(format.c_str(), gradient[2*i]);
            if ((i+1) % 10 == 0)
            {
                WRITELINE("");
            }
        }
        WRITELINE("");
    }

    if (clusterY)
    {
        ALERT("Cluster Y derivatives:");
        for (int i = 0; i < nClusterVariables/2; i++)
        {
            WRITE(format.c_str(), gradient[2*i+1]);
            if ((i+1) % 10 == 0)
            {
                WRITELINE("");
            }
        }
        WRITELINE("");
    }

    if (ki)
    {
        ALERT("ki derivatives:");
        for (int i = nClusterVariables; i < nClusterVariables + nBufferVariables; i++)
        {
            WRITE(format.c_str(), gradient[i]);
            if ((i - nClusterVariables + 1) % 10 == 0)
            {
                WRITELINE("");
            }
        }
        WRITELINE("");
    }
}

void CalcGradientsNorms( AppCtx* context )
{
    double gLSE = 0;
    double gLR = 0;
    double gSpreading = 0;

    for (int i = 0; i < context->nVariables; i++)
    {
        gLSE += context->criteriaValues.gLSE[i] * context->criteriaValues.gLSE[i];
        gLR += context->criteriaValues.gLR[i] * context->criteriaValues.gLR[i];
        gSpreading += context->criteriaValues.gQS[i] * context->criteriaValues.gQS[i];
    }

    context->criteriaValues.gLSENorm = sqrt(gLSE);
    context->criteriaValues.gLRNorm = sqrt(gLR);
    context->criteriaValues.gSprNorm = sqrt(gSpreading);

    //ALERT("LSE gradient value: %.3e", context->criteriaValues.gHPWL);
    //ALERT("LR  gradient value: %.3e", context->criteriaValues.gLR);
    //ALERT("Spr gradient value: %.3e", context->criteriaValues.gSpr);
}

void ReportDebugInfo(AppCtx* context, PetscScalar * solution, PetscScalar * gradient)
{
    //PrintCoordinates(nClusterVariables, nBufferVariables, solution);
    //PrintDerivatives(nClusterVariables, nBufferVariables, gradient, true, true, false);

    //ALERT("Corr gLSE&gLR %f\t", Aux::correlation(context->gLSE, context->gLR, context->nVariables));
    //ALERT("Corr gLSE&gQS %f\t", Aux::correlation(context->gLSE, context->gQS, context->nVariables));
    //ALERT("Corr gLR&gQS %f\t", Aux::correlation(context->gLR, context->gQS, context->nVariables));

    static bool plotSolverState = context->hd->cfg.ValueOf(".Plotting.plotSolverState", false);
    if (plotSolverState)
    {
        context->hd->Plotter->VisualizeState(context->ci->mCurrentNumberOfClusters, 
            context->sprData.binGrid.nBinRows, context->sprData.binGrid.nBinCols, 
            context->ci->netList.size(), 
            (double*)solution, 
            context->criteriaValues.gLSE, context->criteriaValues.gSOD, context->criteriaValues.gLR, context->criteriaValues.gQS, 
            gradient);

        if (context->useSumOfDelays)
        {
            context->hd->Plotter->ClearHistogram();
            context->hd->Plotter->PlotKi(context->ci->mCurrentNumberOfClusters, context->ci->netList.size(), 
                (double*)solution, Color_Violet);
            context->hd->Plotter->RefreshHistogram();
        }
    }
}

void CostTermsAndTheirGradients(AppCtx* context, double* solution)
{
    #pragma omp parallel sections
    {
        #pragma omp section
        {
            if (context->useLogSumExp)
            {
                LSE_AddObjectiveAndGradient(context, solution);
            }
            if (context->useSumOfDelays)
            {
                SOD_AddObjectiveAndGradient(context, solution);
            }
            if (context->useLR)
            {
                LR_AddObjectiveAndGradient(context, solution);
            }
        }

        #pragma omp section
        {
            if (context->useSpreading)
            {
                QS_AddObjectiveAndGradient(context, solution);
            }
        }
    }
}

void TotalCostAndGradients(AppCtx* context, double* solution) 
{
    NullCriteriaValues(context);

    timetype start;
    timetype finish;

    start = GET_TIME_METHOD();
    PrecalcExponents(context, solution);
    finish = GET_TIME_METHOD();
    expTime += finish - start;

    CostTermsAndTheirGradients(context, solution);

    ApplyWeights(context);

    context->criteriaValues.objective = context->criteriaValues.lse + 
        context->criteriaValues.lr + 
        context->criteriaValues.sod + 
        context->criteriaValues.spr;

    CalcGradientsNorms(context);
}

int AnalyticalObjectiveAndGradient(TAO_APPLICATION taoapp, Vec X, double* f, Vec G, void* ctx)
{
    AppCtx* context = (AppCtx*)ctx;

    PetscScalar *solution;
    iCHKERRQ VecGetArray(X, &solution);

    TotalCostAndGradients(context, solution);

    *f = context->criteriaValues.objective;

    PetscScalar *gradient;
    iCHKERRQ VecGetArray(G, &gradient);
    SetGradientToZero((double*)gradient, context->nVariables);
    for (int i = 0; i < context->nVariables; i++)
    {
        gradient[i] += context->criteriaValues.gLSE[i];
        gradient[i] += context->criteriaValues.gSOD[i];
        gradient[i] += context->criteriaValues.gLR[i];
        gradient[i] += context->criteriaValues.gQS[i];
    }

    iCHKERRQ VecRestoreArray(X, &solution);
    iCHKERRQ VecRestoreArray(G, &gradient);

    ReportDebugInfo(context, solution, gradient);

    return OK;
}
