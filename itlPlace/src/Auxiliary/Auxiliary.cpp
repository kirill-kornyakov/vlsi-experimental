#include "Auxiliary.h"
#include "stdlib.h"
#include "stdio.h"
#include <time.h>
#include <io.h>
#include <shlobj.h>

namespace Aux
{
  const int TIME_STRING_LENGTH = 32;

  //��������� dT=a0 + a1*index_1 + a2 * index_2
  void MNK(const double *index_1,
            const double *index_2,
            const double *values,
            const int n, //length of index_1
            const int m, //length of index_2
            double& a0,
            double& a1,
            double& a2)
  {
    const int dim = 3;//task dimension
  	
	  double *A = new double [n * m * dim];

	  //������� ������� A = [1,index_1,index_2] , (n*m �����)
	  for(int i1 = 0 ; i1 < n ; i1++)
		  for(int j1 = 0 ; j1 < m ; j1++)
		  {
			  A[(j1 + m * i1) * dim + 0] = 1;
			  A[(j1 + m * i1) * dim + 1] = index_1[i1];
			  A[(j1 + m * i1) * dim + 2] = index_2[j1];
		  };

	  double *M = new double [dim * dim];
  	
	  //M=0
	  for(int i3 = 0 ; i3 < dim * dim ; i3++)
		  M[i3] = 0.0;

	  //M = transp(A) * A
	  for (int i = 0 ; i < dim ; i++)
		  for (int j = 0 ; j < dim ; j++)
			  for (int k = 0 ; k < m * n ; k++)
				  M[i * dim + j] += A[k * dim + i] * A[k * dim + j];
  				
	  double *Y = new double [dim];
  	
	  //Y = 0
	  for(int i2 = 0; i2 < dim; i2++)
		  Y[i2] = 0.0;

	  //Y = transp(A) * values
	  for(int h = 0 ; h < dim ; h++)
		  for(int g = 0 ; g < m * n ; g++)
			  Y[h] += A[g * dim + h] * values[g];

	  //detM=||M||
	  double detM = (M[0*3+1] * M[2*3+0] * M[1*3+2] -
				     M[0*3+1] * M[1*3+0] * M[2*3+2] +
				     M[1*3+0] * M[2*3+1] * M[0*3+2] +
				     M[1*3+1] * M[0*3+0] * M[2*3+2] -
				     M[2*3+0] * M[1*3+1] * M[0*3+2] -
				     M[2*3+1] * M[0*3+0] * M[1*3+2]);

	  //calculate T and R
	  a0 = (M[1*3+1] * Y[0] * M[2*3+2]
      - M[1*3+1] * M[0*3+2] * Y[2]
      + M[0*3+2] * M[2*3+1] * Y[1]
      - Y[0] * M[2*3+1] * M[1*3+2]
      + M[0*3+1] * M[1*3+2] * Y[2]
      - M[0*3+1] * Y[1] * M[2*3+2]) / detM;


	  a1 = - (-Y[0] * M[2*3+0] * M[1*3+2]
      +	Y[0] * M[1*3+0] * M[2*3+2]
      +	M[0*3+0] * M[1*3+2] * Y[2]
      -	M[0*3+0] * Y[1] * M[2*3+2]
      -	M[0*3+2] * M[1*3+0] * Y[2]
      +	M[0*3+2] * M[2*3+0] * Y[1]) / detM;
  		
	  a2 = (-M[0*3+1] * M[1*3+0] * Y[2]
      - M[2*3+1] * M[0*3+0] * Y[1]
      + M[1*3+0] * M[2*3+1] * Y[0]
      +	M[0*3+1] * M[2*3+0] * Y[1]
      + M[1*3+1] * M[0*3+0] * Y[2]
      - M[2*3+0] * M[1*3+1] * Y[0]) / detM;

	  delete [] Y;
	  delete [] M;
	  delete [] A;
  }

  void GetCurrentTimeStamp(char* timeString)
  {
    time_t ltime = time(0);
    strftime(timeString, TIME_STRING_LENGTH, "20%y-%m-%d_%H-%M-%S", localtime(&ltime));
  }

  void GetTimeStamp(char* timeString)
  {
    static char timeStamp[256];
    static bool onlyOnce = true;
    if (onlyOnce)
    {
      GetCurrentTimeStamp(timeStamp);
      onlyOnce = false;
    }
    strcpy(timeString, timeStamp);
  }

  string GetUniqueName(string dirName, char* timeStamp, string fileNameBase, string extension)
  {
    string uniqueName = dirName + timeStamp +  "_" + fileNameBase + extension;

    //duplicates checking optimization
    static string previousUniqueFileName;
    static int nFilesExist = 0;
    if (previousUniqueFileName != uniqueName)
    {
      nFilesExist = 0;
      previousUniqueFileName = uniqueName;
    }
    
    //check duplicates and generate unique name
    char buf[10];
    FILE* f;
    int lenFileName = uniqueName.length();
    for (; (f = fopen(uniqueName.c_str(), "r")) != NULL; fclose(f))
    {
      if (nFilesExist < 9)
        sprintf(buf, "0%d", ++nFilesExist);
      else
        sprintf(buf, "%d", ++nFilesExist);
      uniqueName = uniqueName.substr(0, lenFileName - extension.length()) + "_" + buf +  extension;
    }

    return uniqueName;
  }

  string CreateCoolFileName(string dirName, string fileNameBase, string extension)
  {
    //���������� ������ ������������� \\
    //����� ����� ����������� � ����� bin
    //��� �� �������� ���������� � �� �������� ����������
    //���������� �����������

    char timeStamp[TIME_STRING_LENGTH];
    GetTimeStamp(timeStamp);

    if ((extension[0] != '.') && (extension != ""))
      extension = "." + extension;

    char deniedSymbols[] = "./\\:*?\"<>|";
    for (unsigned int i = 0; i < strlen(deniedSymbols); ++i)
      replace(fileNameBase.begin(), fileNameBase.end(), deniedSymbols[i],'_');

    for (unsigned int i = 0; i < dirName.length(); i++)
      if (dirName[i] == '\\')
        CreateDirectory(dirName.substr(0, i).c_str(), NULL);

    return GetUniqueName(dirName, timeStamp, fileNameBase, extension);
  }
}