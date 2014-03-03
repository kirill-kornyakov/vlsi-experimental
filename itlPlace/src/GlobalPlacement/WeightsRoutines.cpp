#include "ObjectivesConstraints.h"
#include "WeightsRoutines.h"
#include "Clustering.h"
#include "Timing.h"

#include "GPBuffering.h"

void InitWeights(double* x, AppCtx* context)
{
    context->weights.lseW = 1.0;
    context->weights.sodW = 1.0;
    context->weights.lrW  = 1.0;
    context->weights.sprW = 1.0;

    //TODO: just try
    //GPBuffering bufferer(*(context->hd));
    //bufferer.DoBuffering(*context, 0, 0);
    TotalCostAndGradients(context, x);

    double lseInitialRatio = context->hd->cfg.ValueOf(".Weights.lseInitialRatio", 1.0);
    double sprInitialRatio = context->hd->cfg.ValueOf(".Weights.sprInitialRatio", 1.0);
    double lrInitialRatio = context->hd->cfg.ValueOf(".Weights.lrInitialRatio", 1.0);

    double bigValue = 1.0e+6;

    if (context->useLogSumExp)
        context->weights.lseW = bigValue * lseInitialRatio / context->criteriaValues.lse;
    else
        context->weights.lseW = 0.0;

    if (context->useSpreading)
        context->weights.sprW = context->sprData.sprWInitial = bigValue * sprInitialRatio / context->criteriaValues.spr;
    else
        context->weights.sprW = context->sprData.sprWInitial = 0.0;

    if (context->useLR)
        context->weights.lrW = bigValue * lrInitialRatio / context->criteriaValues.lr;
    else
        context->weights.lrW = 0.0;

    ReportWeights(*context);
}

void ApplyWeights(AppCtx* context)
{
    context->criteriaValues.lse *= context->weights.lseW;
    context->criteriaValues.sod *= context->weights.sodW;
    context->criteriaValues.lr  *= context->weights.lrW;
    context->criteriaValues.spr *= context->weights.sprW;

    for (int i = 0; i < context->nVariables; i++)
    {
        context->criteriaValues.gLSE[i] *= context->weights.lseW;
        context->criteriaValues.gSOD[i] *= context->weights.sodW;
        context->criteriaValues.gLR[i]  *= context->weights.lrW;
        context->criteriaValues.gQS[i]  *= context->weights.sprW;
    }
}

double GetMultiplierAccordingDesiredRatio(double ratio, double desiredRatio, double multiplier)
{
    if (ratio > desiredRatio)
        return multiplier;
    else
        return 1.0/multiplier;
}
double GetMultiplierAccordingDesiredRatio(HDesign& hd, double deltaRatio, int updateFunction)
{
    double multiplier = 1.0;

    double N = hd.cfg.ValueOf(".Weights.N", 1.0);
    double R = hd.cfg.ValueOf(".Weights.R", 1.1);
    double M = hd.cfg.ValueOf(".Weights.M", 1.0);
    double P = hd.cfg.ValueOf(".Weights.P", 1.1);
    
    //Spreading Update Functions
    if (updateFunction == 1) {
        int s = deltaRatio > 0 ? 1 : (deltaRatio < 0) ? -1 : 0;
        double f = fabs(deltaRatio);
        multiplier = 1 + s*pow(f, N);
    }
    else if (updateFunction == 2) {
        multiplier = pow(1 + atan(deltaRatio), N);
    }
    else if (updateFunction == 3) {
        multiplier = pow(R, deltaRatio);
    }
    else if (updateFunction == 4) {
        multiplier = pow(1 + deltaRatio, N);
    }
    //LSE Update Functions
    else if (updateFunction == 5) {
        int s = deltaRatio > 0 ? 1 : (deltaRatio < 0) ? -1 : 0;
        double f = fabs(deltaRatio);
        multiplier = 1 + s*pow(f, M);
    }
    else if (updateFunction == 6) {
        multiplier = pow(1 + atan(deltaRatio), M);
    }
    else if (updateFunction == 7) {
        multiplier = pow(P, deltaRatio);
    }
    else if (updateFunction == 8) {
        multiplier = pow(1 + deltaRatio, M);
    }

    return multiplier;
}

double ChooseSpreadingMultiplier(HDesign& hd, int iteration, double currentHPWL, double currentLHPWL)
{
    double sprRatio = currentLHPWL / currentHPWL;
    double sprDesiredRatio = hd.cfg.ValueOf(".Weights.sprDesiredRatio", 1.1);
    double mult = 0.0;

    bool useSprUpdateFunction = hd.cfg.ValueOf(".Weights.useSprUpdateFunction", false);	
    if (useSprUpdateFunction) //&& iteration > 10
    {
        int sprUpdateFunction = hd.cfg.ValueOf(".Weights.sprUpdateFunction", 2);
        double deltaRatio = sprRatio - sprDesiredRatio;
        mult = GetMultiplierAccordingDesiredRatio(hd, deltaRatio, sprUpdateFunction);
    }
    else
    {
        double sprUpdateMultiplier = hd.cfg.ValueOf(".Weights.sprUpdateMultiplier", 2.0);
        mult = GetMultiplierAccordingDesiredRatio(sprRatio, sprDesiredRatio, sprUpdateMultiplier);
    }
    ALERT("  spreading multiplier = %.2f", mult);

    return mult;
}

double ChooseLSEMultiplier(HDesign& hd, double currentLHPWL, double initialLHPWL)
{
    double lseRatio = currentLHPWL / initialLHPWL;
    double lseDesiredRatio = hd.cfg.ValueOf(".Weights.lseDesiredRatio", 1.0);
    double lseUpdateMultiplier = hd.cfg.ValueOf(".Weights.lseUpdateMultiplier", 1.0);

    bool useLseUpdateFunction = hd.cfg.ValueOf(".Weights.useLseUpdateFunction", false);

    if (useLseUpdateFunction)
    {
        int lseUpdateFunction = hd.cfg.ValueOf(".Weights.lseUpdateFunction", 7);
        double deltaRatio = lseRatio - lseDesiredRatio;
        return GetMultiplierAccordingDesiredRatio(hd, deltaRatio, lseUpdateFunction);
    }

    return GetMultiplierAccordingDesiredRatio(lseRatio, lseDesiredRatio, lseUpdateMultiplier);
}

void UpdateWeights(HDesign& hd, AppCtx& context, PlacementQualityAnalyzer* QA, int iteration)
{
    string objective = hd.cfg.ValueOf("params.objective");

    double initialLHPWL = QA->GetInitialMetricValue(PlacementQualityAnalyzer::MetricHPWLleg);
    double currentLHPWL = QA->GetCurrentMetricValue(PlacementQualityAnalyzer::MetricHPWLleg);
    double currentHPWL = QA->GetCurrentMetricValue(PlacementQualityAnalyzer::MetricHPWL);

    context.weights.sprW *= ChooseSpreadingMultiplier(hd, iteration, currentHPWL, currentLHPWL);

    if (objective == "LR")
    {
        context.LRdata.UpdateMultipliers(hd);        
        context.weights.lseW *= ChooseLSEMultiplier(hd, currentLHPWL, initialLHPWL);
    }

    if (hd.cfg.ValueOf("LSE.Clustering.useNetWeights", false))
    {
        ComputeAndExportWeights(hd); //do weighting
        //WriteWeightsToClusteredNets(hd, ci);
        //int netIdx = 0;

        for (int i = 0; i < static_cast<int>(context.ci->netList.size()); i++)
        {
            AssignWeightForClusteredNet(hd, *(context.ci), i);
        }
    }
}

void ReportWeights(AppCtx& context) 
{
    ALERT("Cost terms weights:");
    ALERT("  lseW = " + Aux::SciFormat, context.weights.lseW);
    ALERT("  lrW  = " + Aux::SciFormat, context.weights.lrW);
    ALERT("  sprW = " + Aux::SciFormat, context.weights.sprW);
}
