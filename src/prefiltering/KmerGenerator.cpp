#include "KmerGenerator.h"
#include <emmintrin.h>
#include <mmintrin.h>
#include <smmintrin.h>
#include "Util.h"


KmerGenerator::KmerGenerator(size_t kmerSize, size_t alphabetSize, short threshold ){
    this->threshold = threshold;
    this->kmerSize = kmerSize;

    this->indexer = new Indexer((int) alphabetSize, (int)kmerSize);
//    calcDivideStrategy();
}

KmerGenerator::~KmerGenerator(){
    delete [] this->stepMultiplicator;
    delete [] this->highestScorePerArray;
    delete [] this->possibleRest;
    delete [] this->kmerIndex;
    delete [] this->divideStep;
    delete [] this->matrixLookup;
    for(size_t i = 0 ; i < this->divideStepCount - 1; i++){
        delete outputScoreArray[i];
        delete outputIndexArray[i];

    }
    delete [] outputScoreArray;
    delete [] outputIndexArray;
    delete indexer;
}

void KmerGenerator::setDivideStrategy(ScoreMatrix ** one){
    this->divideStepCount = kmerSize;
    this->matrixLookup = new ScoreMatrix*[divideStepCount];
    this->divideStep   = new unsigned int[divideStepCount];
    for(size_t i = 0; i < kmerSize; i++){
        this->divideStep[i] = 1;
        this->matrixLookup[i] = one[i];
    }
    initDataStructure(divideStepCount);
}

void KmerGenerator::setDivideStrategy(ScoreMatrix * three, ScoreMatrix * two){
    const size_t threeDivideCount = this->kmerSize / 3;

    switch(kmerSize%3){
        case 0:
            this->divideStepCount=threeDivideCount;
            this->matrixLookup= new ScoreMatrix*[divideStepCount];
            this->divideStep = new unsigned int[divideStepCount];
            for(size_t i = 0; i < threeDivideCount; i++){
                this->divideStep[i] = 3;
                this->matrixLookup[i] = three;
            }
            break;
        case 1: 
            this->divideStepCount=threeDivideCount+1;
            this->matrixLookup= new ScoreMatrix*[divideStepCount];
            this->divideStep = new unsigned int[divideStepCount];
            for(size_t i = 0; i < threeDivideCount-1; i++){
                this->divideStep[i] = 3;
                this->matrixLookup[i] = three;
            }
            this->divideStep[threeDivideCount-1]=2;
            this->matrixLookup[threeDivideCount-1] = two;
            
            this->divideStep[threeDivideCount]=2;
            this->matrixLookup[threeDivideCount] = two;
            
            break;
        case 2:
            this->divideStepCount=threeDivideCount+1;
            this->matrixLookup= new ScoreMatrix*[divideStepCount];
            this->divideStep = new unsigned int[divideStepCount];
            for(size_t i = 0; i < threeDivideCount; i++){
                this->divideStep[i] = 3;
                this->matrixLookup[i] = three;
            }
            this->divideStep[threeDivideCount]=2;
            this->matrixLookup[threeDivideCount] = two;
            
            break;
    }

    initDataStructure(divideStepCount);
//    std::reverse(this->matrixLookup, &this->matrixLookup[divideStepCount]);
//    std::reverse(this->divideStep, &this->divideStep[divideStepCount]);

}


void KmerGenerator::initDataStructure(size_t divide_steps){
    this->stepMultiplicator = new unsigned int[divideStepCount];
    this->highestScorePerArray = new short[divideStepCount];
    // init possibleRest
    this->possibleRest = new short[divideStepCount];
    this->possibleRest[divideStepCount-1] = 0;
    this->kmerIndex = new unsigned int[divideStepCount];
    
    outputScoreArray = new short *[divide_steps];
    outputIndexArray = new unsigned int *[divide_steps];

    for(size_t i = 0 ; i < divide_steps - 1; i++){
        outputScoreArray[i] = (short *)        Util::mem_align(16,MAX_KMER_RESULT_SIZE * sizeof(short));
        outputIndexArray[i] = (unsigned int *) Util::mem_align(16,MAX_KMER_RESULT_SIZE * sizeof(unsigned int));
    }
}


ScoreMatrix KmerGenerator::generateKmerList(const int * int_seq){
    int dividerBefore=0;
    // pre compute phase
    // find first threshold
    for(size_t i = 0; i < this->divideStepCount; i++){
        const int divider=divideStep[i];
        
        const unsigned int index = this->indexer->int2index(int_seq,dividerBefore,dividerBefore+divider);
        this->kmerIndex[i] = index;

        stepMultiplicator[i] = this->indexer->powers[dividerBefore];

        const ScoreMatrix * scoreMatrix = this->matrixLookup[i];
        // get highest element in array for index
        const short score = scoreMatrix->score[index*scoreMatrix->rowSize];
        this->highestScorePerArray[i] = score; //highest score
        dividerBefore+=divider;
        
    }
    for(size_t i = this->divideStepCount -1; i >= 1 ; i--){
        this->possibleRest[i-1] = this->highestScorePerArray[i] + possibleRest[i];
    }
    
    // create kmer list
    short cutoff1 = this->threshold - this->possibleRest[0];
    const size_t index  = this->kmerIndex[0];
    const ScoreMatrix * inputScoreMatrix = this->matrixLookup[0];
    size_t sizeInputMatrix = inputScoreMatrix->elementSize;
    const short        * inputScoreArray = &inputScoreMatrix->score[index*inputScoreMatrix->rowSize];
    const unsigned int * inputIndexArray = &inputScoreMatrix->index[index*inputScoreMatrix->rowSize];
    size_t i;
    for(i = 0; i < this->divideStepCount-1; i++){
        const size_t index = this->kmerIndex[i+1];
        const ScoreMatrix * nextScoreMatrix = this->matrixLookup[i+1];
        const short        * nextScoreArray = &nextScoreMatrix->score[index*nextScoreMatrix->rowSize];
        const unsigned int * nextIndexArray = &nextScoreMatrix->index[index*nextScoreMatrix->rowSize];

        const int lastElm=calculateArrayProduct(inputScoreArray,
                                                inputIndexArray,
                                                sizeInputMatrix,
                                                nextScoreArray,
                                                nextIndexArray,
                                                nextScoreMatrix->elementSize,
                                                outputScoreArray[i],
                                                outputIndexArray[i],
                                                cutoff1,
                                                possibleRest[i+1],
                                                stepMultiplicator[i+1]);
        if(lastElm==-1){
            return ScoreMatrix(NULL,NULL,0,0);
        }
        inputScoreArray = this->outputScoreArray[i];
        inputIndexArray = this->outputIndexArray[i];
        cutoff1 = -1000; //all must be inspected
        sizeInputMatrix = lastElm;
    }

    return ScoreMatrix(outputScoreArray[i-1], outputIndexArray[i-1], sizeInputMatrix, MAX_KMER_RESULT_SIZE);
}




int KmerGenerator::calculateArrayProduct(const short        * __restrict scoreArray1,
                                         const unsigned int * __restrict indexArray1,
                                         const size_t array1Size,
                                         const short        * __restrict scoreArray2,
                                         const unsigned int * __restrict indexArray2,
                                         const size_t array2Size,
                                         short              * __restrict outputScoreArray,
                                         unsigned int       * __restrict outputIndexArray,
                                         const short cutoff1,
                                         const short possibleRest,
                                         const unsigned int pow){
    size_t counter=0;
    const __m128i * scoreArray2_simd = (const __m128i *) scoreArray2;
    const __m128i * indexArray2_simd = (const __m128i *) indexArray2;
    const __m128i pow_simd     = _mm_set1_epi32(pow);

    for(size_t i = 0 ; i < array1Size; i++){
        const short score_i = scoreArray1[i];
        if(score_i < cutoff1 )
            break;
        const unsigned int kmer_i = indexArray1[i];
        const short cutoff2 = this->threshold - score_i - possibleRest;
        const __m128i cutoff2_simd = _mm_set1_epi16(cutoff2);
        const __m128i score_i_simd = _mm_set1_epi16(score_i);
        const __m128i kmer_i_simd  = _mm_set1_epi32(kmer_i);
        const size_t SIMD_SIZE = 8;
        const size_t array2SizeSIMD = (array2Size / SIMD_SIZE)+1;
        for(size_t j = 0; j < array2SizeSIMD; j++){
            if(counter + SIMD_SIZE >= MAX_KMER_RESULT_SIZE )
                return counter;
            const __m128i score_j_simd   = _mm_load_si128(scoreArray2_simd + j);
            
            const __m128i kmer_j_1_simd  = _mm_load_si128(indexArray2_simd + (j*2));
            const __m128i kmer_j_2_simd  = _mm_load_si128(indexArray2_simd + (j*2+1));
            // score_j < cutoff2 -> fffff, score_j > cutoff2 -> 0000
            const __m128i cmp = _mm_cmplt_epi16 (score_j_simd, cutoff2_simd);
            const unsigned int score_j_lt_cutoff = _mm_movemask_epi8(cmp);
            
            
            __m128i * scoreOutput_simd = (__m128i *) (outputScoreArray + counter);
            __m128i * indexOutput_simd = (__m128i *) (outputIndexArray + counter);
            _mm_storeu_si128(scoreOutput_simd,     _mm_add_epi16(score_i_simd,score_j_simd));
            const __m128i kmer_j_1 = _mm_mullo_epi32(kmer_j_1_simd, pow_simd);
            const __m128i kmer_j_2 = _mm_mullo_epi32(kmer_j_2_simd, pow_simd);
            _mm_storeu_si128(indexOutput_simd,     _mm_add_epi32(kmer_i_simd, kmer_j_1));
            _mm_storeu_si128(indexOutput_simd + 1, _mm_add_epi32(kmer_i_simd, kmer_j_2));
            counter += std::min(SIMD_SIZE,  array2Size - (j*SIMD_SIZE)); //protect from running to far
             // if(score_j < cutoff2)
            if (score_j_lt_cutoff > 0){
                for(size_t vec_index = 0; vec_index < SIMD_SIZE; vec_index++){
                    if(CHECK_BIT(score_j_lt_cutoff,vec_index*2)){ // all with 1 is not a result
                        counter--;
                    }
                }
                break;
            }
        }
    }
    return counter;
}

