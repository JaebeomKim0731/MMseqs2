
#include <iostream>
#include <string>
#include <sys/time.h>

#include "Prefiltering.h"
#include "Util.h"
#include "Parameters.h"

#include "MMseqsMPI.h"
#ifdef OPENMP
#include <omp.h>
#endif
int prefilter(int argc, const char **argv)
{
    MMseqsMPI::init(argc, argv);

    std::string usage("\nCalculates k-mer similarity scores between all sequences in the query database and all sequences in the target database.\n");
    usage.append("Written by Martin Steinegger (martin.steinegger@mpibpc.mpg.de) & Maria Hauser (mhauser@genzentrum.lmu.de)\n");
    usage.append("USAGE: prefilter <queryDB> <targetDB> <outDB> [opts]\n");

    Parameters par;
    par.parseParameters(argc, argv, usage, par.prefilter, 3);

    struct timeval start, end;
    gettimeofday(&start, NULL);


#ifdef OPENMP
    omp_set_num_threads(par.threads);
#endif

    Debug(Debug::WARNING) << "Initialising data structures...\n";
#ifdef HAVE_MPI
    std::pair<std::string, std::string> filenamePair = Util::createTmpFileNames(par.db3, par.db3Index, MMseqsMPI::rank);
    std::cout << filenamePair.first.c_str() << "\t" <<  filenamePair.second.c_str() << std::endl;
    Prefiltering* pref = new Prefiltering(par.db1,par.db1Index,
                                          par.db2,par.db2Index,
                                          filenamePair.first.c_str(), filenamePair.second.c_str(), par);
#else
    Prefiltering* pref = new Prefiltering(par.db1,par.db1Index,
            par.db2,par.db2Index,
            par.db3,par.db3Index,par);
#endif
    gettimeofday(&end, NULL);
    int sec = end.tv_sec - start.tv_sec;
    Debug(Debug::WARNING) << "Time for init: " << (sec / 3600) << " h " << (sec % 3600 / 60) << " m " << (sec % 60) << "s\n\n\n";
    gettimeofday(&start, NULL);
#ifdef HAVE_MPI
    if(par.split > MMseqsMPI::numProc){
        // if split size is great than nodes than we have to
        // distribute all splits equally over all nodes
        int * splitCntPerProc = new int[MMseqsMPI::numProc];
        memset(splitCntPerProc, 0, sizeof(int) * MMseqsMPI::numProc);
        for(int i = 0; i < par.split; i++){
            splitCntPerProc[i % MMseqsMPI::numProc] += 1;
        }
        int fromSplit = 0;
        for(int i = 0; i < MMseqsMPI::rank; i++){
            fromSplit += splitCntPerProc[i];
        }
        pref->run(fromSplit, splitCntPerProc[MMseqsMPI::rank]);
        delete [] splitCntPerProc;
    }if(par.split == 0 ){
        // if database fits into the memory of a single node split by query.
        // each node should just compute 1 query split
        pref->setSplit(MMseqsMPI::numProc);
        pref->run(MMseqsMPI::rank, 1);
    }else{
        // if more nodes exists than splits are needed set split to the amount of nodes
        // each node needs to compute 1 target split
        pref->setSplit(MMseqsMPI::numProc);
        pref->run(MMseqsMPI::rank, 1);
    }
#else
    pref->run(0, (par.split == 0) ?  1 : par.split );
#endif
    delete pref;

#ifdef HAVE_MPI
    MPI_Barrier(MPI_COMM_WORLD);
    if(MMseqsMPI::rank == 0){
        std::vector<std::pair<std::string, std::string> > splitFiles;
        for(int procs = 0; procs < MMseqsMPI::numProc; procs++){
            splitFiles.push_back(Util::createTmpFileNames(par.db3, par.db3Index, procs));
        }
        // merge output ffindex databases
        Prefiltering::mergeFiles(splitFiles, pref->getSplitMode(), par.db3, par.db3Index);
    }
#endif
    gettimeofday(&end, NULL);
    sec = end.tv_sec - start.tv_sec;
    Debug(Debug::WARNING) << "\nOverall time for prefiltering run: " << (sec / 3600) << " h " << (sec % 3600 / 60) << " m " << (sec % 60) << "s\n";

#ifdef HAVE_MPI
    MPI_Finalize();
#endif
    return EXIT_SUCCESS;
}
