#ifndef KMERGENERATOR_H 
#define KMERGENERATOR_H 
#include <string>
#include <vector>
#include "Indexer.h"
#include "ScoreMatrix.h"
#include "Debug.h"


class KmerGenerator 
{
    public: 
        KmerGenerator(size_t kmerSize,size_t alphabetSize, short threshold);
        ~KmerGenerator();
        /*calculates the kmer list */
        ScoreMatrix generateKmerList(const int * intSeq);

        /* kmer splitting stragety (3,2)
         fill up the divide step and calls init_result_list */
        void setDivideStrategy(ScoreMatrix * three, ScoreMatrix * two );

        /* kmer splitting stragety (1)
         fill up the divide step and calls init_result_list */
        void setDivideStrategy(ScoreMatrix ** one);

    private:
    
        /*creates the product between two arrays and write it to the output array */
        int calculateArrayProduct(const short        * __restrict scoreArray1,
                                  const unsigned int * __restrict indexArray1,
                                  const size_t array1Size,
                                  const short        * __restrict scoreArray2,
                                  const unsigned int * __restrict indexArray2,
                                  const size_t array2Size,
                                  short              * __restrict outputScoreArray,
                                  unsigned int       * __restrict outputIndexArray,
                                  const short cutoff1,
                                  const short possibleRest,
                                  const unsigned int pow);
    
    
        /* maximum return values */
        const static size_t MAX_KMER_RESULT_SIZE = 8192;
        /* min score  */
        short threshold;
        /* size of kmer  */
        size_t kmerSize;
        /* partition steps of the kmer size in (2,3)  */
        size_t divideStepCount;
        /* divider of the steps (2,3) */
        unsigned int * divideStep;
        unsigned int * kmerIndex;
        unsigned int * stepMultiplicator;
        short * highestScorePerArray;
        short * possibleRest;
        Indexer * indexer;
        ScoreMatrix  ** matrixLookup;
        short        ** outputScoreArray;
        unsigned int ** outputIndexArray;


        /* init the output vectors for the kmer calculation*/
        void initDataStructure(size_t divideSteps);
    
};
#endif

