#ifndef __LRSIZING__
#define __LRSIZING__
#include "HDesign.h"
#include <hdpgrid.h>
#include <vector>

class LRSizer{
private:
		int size;
		std::vector<double>* LambdaIn;
		std::vector<HCell>* cells;

public:
		LRSizer(HDesign& design);
		~LRSizer();

		void InitLambda(HDesign& design, double defaultLambda);
		void doLRSizing(){;}
		void InitPointLambda(HDesign& design, HTimingPoint tp, double defaultLambda);
		int CountLeftArcs(HDesign& design, HNet net);
		std::vector<HCell>* InitCells(HDesign& design);
		std::vector<double> SOLVE_LRS_mu(HDesign& design, std::vector<HCell>& cells);
		void FillVMu(HDesign& design, std::vector<double>& vMu);
		void initVX( std::vector<double> vX, std::vector<HCell>& cells );
		void CalculateVC( HDesign& design, std::vector<double>& vC, std::vector<HCell>& cells, std::vector<double> vX );
		void CalculateVR( HDesign& design, std::vector<double> vR, std::vector<HCell>& cells, std::vector<double> vX, std::vector<double> vMu );
		void CalculateVX( std::vector<HCell>& cells, std::vector<double>& vX, std::vector<double> vMu, std::vector<double> vC, std::vector<double> vR );
		bool CheckStopConditionForLRS_Mu( std::vector<double> prevVX, std::vector<double> nextVX, double accuracy);
		//SOLVE_LRS_mu()
};


#endif