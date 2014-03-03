#include "AnalyticalPlacementReporting.h"
#include "ObjectivesConstraints.h"
#include "WeightsRoutines.h"

using namespace AnalyticalGlobalPlacement;

extern timetype expTime;
extern timetype lseTime;
extern timetype lseGradTime;
extern timetype calcPotentialsTime;
extern timetype quadraticSpreading;
extern timetype quadraticSpreadingGradTime;

void AnalyticalGlobalPlacement::ReportTimes()
{
    ALERT("Performance counters:");
    ALERT("  EXP Calc time = %f", GETSECONDSFROMTIME(expTime));
    ALERT("  lseTime = %f", GETSECONDSFROMTIME(lseTime));
    ALERT("  lseGradTime = %f", GETSECONDSFROMTIME(lseGradTime));
    ALERT("  calcPotentialsTime = %f", GETSECONDSFROMTIME(calcPotentialsTime));
    ALERT("  quadraticSpreading = %f", GETSECONDSFROMTIME(quadraticSpreading));
    ALERT("  quadraticSpreadingGradTime = %f", GETSECONDSFROMTIME(quadraticSpreadingGradTime));
    expTime = lseTime = lseGradTime = calcPotentialsTime = quadraticSpreading = quadraticSpreadingGradTime;
}

void PrintReason(TaoTerminateReason reason)
{
    char message[256];

    switch(reason)
    {
    case 2:
        strcpy(message, "(residual of optimality conditions <= absolute tolerance)");
        break;
    case 3:
        strcpy(message, "(rtol)"); //residual of optimality conditions / initial residual of optimality conditions <= relative tolerance
        break;
    case 4:
        strcpy(message, "(current trust region size <= trtol)");  
        break;
    case 5:
        strcpy(message, "(function value <= fmin)");  
        break;
    case -2:
        strcpy(message, "(its > maxits)");
        break;
    case -4:
        strcpy(message, "(numerical problems)");
        break;
    case -5:
        strcpy(message, "(number of function evaluations > maximum number of function evaluations)");
        break;
    case -6:
        strcpy(message, "(line search failure)");
        break;
    default:
        strcpy(message, "(unrecognized error)");  
        break;
    }
    ALERT("TAO termination reason = %d %s", reason, message);
}

int AnalyticalGlobalPlacement::ReportTerminationReason(TAO_SOLVER tao, int& innerTAOIterations)
{
    //Get termination information
    double f;
    double gnorm;
    double cnorm;
    double xdiff;
    TaoTerminateReason reason;  // termination reason
    iCHKERRQ TaoGetSolutionStatus(tao, &innerTAOIterations, &f, &gnorm, &cnorm, &xdiff, &reason);
    PrintReason(reason);

    return OK;
}

void AnalyticalGlobalPlacement::ReportPreIterationInfo(HDesign& hd, AppCtx& context, int metaIteration, int iteration)
{
    WRITELINE("");
    ALERT(Color_LimeGreen, "TAO iteration %d.%d", metaIteration, iteration++);

    ReportWeights(context);

    hd.Plotter->ShowGlobalPlacement(hd.cfg.ValueOf(".Plotting.plotWires", false), 
        context.sprData.binGrid.nBinRows, context.sprData.binGrid.nBinCols);
}

void AnalyticalGlobalPlacement::ReportPostIterationInfo(HDesign& hd, AppCtx& context, int metaIteration, int iteration)
{
    //ALERT("Sum of Ki = " + format, CalculateSumOfK(hd, ci))
    //PrintClusterCoordinates(&context);

    //std::string defName = Aux::CreateCoolFileName("GP_Iters/", hd.Circuit.Name() + "_" + Aux::IntToString(iteration - 1), "def");
    //ExportDEF(hd, defName);

    //hd.Plotter->ShowGlobalPlacement(hd.cfg.ValueOf(".Plotting.plotWires", false), 
    //    context.spreadingData.binGrid.nBinRows, context.spreadingData.binGrid.nBinCols, HPlotter::WAIT_3_SECONDS);

    if (hd.cfg.ValueOf(".Plotting.saveTAOmilestones", false))
    {
        hd.Plotter->ShowGlobalPlacement(hd.cfg.ValueOf(".Plotting.plotWires", false),
            context.sprData.binGrid.nBinRows, context.sprData.binGrid.nBinCols);
        hd.Plotter->SaveMilestoneImage(Aux::Format("TAO%d.%d", metaIteration, iteration));
    }
}

void AnalyticalGlobalPlacement::ReportBinGridInfo(AppCtx& context)
{
    ALERT("Bin grid: %d x %d", 
        context.sprData.binGrid.nBinRows, 
        context.sprData.binGrid.nBinCols);
    ALERT("Bin width: %f\tBin height: %f", 
        context.sprData.binGrid.binWidth, 
        context.sprData.binGrid.binHeight);
    ALERT("Potential radius X: %f\tPotential radius Y: %f", 
        context.sprData.potentialRadiusX, 
        context.sprData.potentialRadiusY);
}

void AnalyticalGlobalPlacement::ReportIterationInfo(ClusteringInformation& ci, AppCtx& user)
{
    WRITELINE("");
    ALERT("RELAXATION ITERATION STARTED");
    ALERT("Number of clusters: %d", ci.mCurrentNumberOfClusters);
    ALERT("Number of nets: %d", ci.netList.size());
}

void AnalyticalGlobalPlacement::ReportClusterCoordinates(AppCtx* context)
{
    int clusterIdx = -1;
    while (GetNextActiveClusterIdx(context->ci, clusterIdx))
    {
        ALERT("Cluster %d coordinates: [%.3f, %.3f]", clusterIdx,
            context->ci->clusters[clusterIdx].xCoord, context->ci->clusters[clusterIdx].yCoord);
    }
    WRITELINE("");
}