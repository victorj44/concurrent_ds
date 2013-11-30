// Copyright 2006, Sun Microsystems, Inc.  All Rights Reserved.
// Test harness for concurrent Key-Value maps.
// Supports the following key-value JUC "collection"-style maps:
// -- red-black trees
// -- skiplists
// -- hash tables, etc.
//
// Remarks:
// -- Provides lightweight content integrity checks.
//    The current implementation only covers the keys, but we should someday
//    enhance it track values as well.  Still, it's sufficiently low-cost
//    that we can leave it permanently enabled.
//    The checks are necessary but not sufficient to demonstrate correctness.
//    (The integrity checks admit false negatives).  
//    The lightweight content integrity check was derived from source kits
//    used for the "RLS" transact 2008 data.  
//    See: transact08 : LockElisionRLSmap.java, LockElisionRLSht.java and ListMicro.java. 
// -- See also: 
//    =  NextGen-kvtest.c     
//    =  RA-Harness.c
//    =  tleht.java and predessor LockElisionRLSht.java
//       See "primary worker benchmark loop" comments.  
// -- general thoughts on benchmarking methodology:
//    =  Each worker executes from the following:
//       -- think time : local computation
//       -- private but txl KV accesses : thread-local data structure instance
//       -- shared txl KV map
//     =  Consider building per-thread schedules at thread initialization time
//        -- Structure
//            Next  (CLL)
//            KVMapInstance
//            FreqSet, FreqPut, FreqRemove
//
// TODO: 
// +    rename RB-Harness.c ==> KVMap-Harness.c.  
// +    Allow post-operation think-time component with randomized duration.
//      See also: RH-Model. 
//      At the end of the main worker loop
//      if (_PostMask != 0) {
//         rv = NextRandom(rv);
//         for (int dly = rv & _PostMask; --dly >= 0 ; rv = NextRandom(rv)) ; 
//      }
// +    Add think-time parallel component : advance thread-local PRNG
// +    Reduce use of div and mod: see tleht.java and RA-Harness.c
//      Reduce use of high-latency div and mod.  
//      These can also impact scalability on certain CMT platforms where
//      the DIVIDE unit might be shared between strands.
// +    Many apps demonstrate inter-transaction key locality.
//      Consider using a non-uniform PRNG with "memory" to generate a thread's next key.
//      That is, we'd attempt to approximate inter-operation spatial locality with a 
//      non-uniform PRNG to generate keys.  We're assuming "key locality" reflects as 
//      in spatial locality, which is reasonable.
// +    Consider having the primordial thread completely pre-allocate the full complement of 
//      nodes that the test will ever require (or at least some large N).
//      Presumably the nodes will be near each other spatially, and the TLB span of the
//      nodes (the covering) will be smaller than if we use libumem.so, which reduces
//      contention but can end up populating the tree with nodes from very distant TLBs.
//


#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <malloc.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#if defined(__sun)
#include <sys/lwp.h>
#include <sys/processor.h>
#include <sys/procset.h>
#endif
#include <pthread.h>
#include <unistd.h>
#include <poll.h>
#include <alloca.h>
#include <ctype.h>
#include <sys/utsname.h>

#ifdef PAPI
#include <papi.h>
#endif

#include "TL.h"
#include "RB-Tree.h"

#ifdef PAPI
#define G_EVENT_COUNT 2
int g_events[] = { PAPI_L1_DCM, PAPI_L2_DCM };
long long g_values[MAX_THREADS][G_EVENT_COUNT] = {0,};
#endif

static int non_numa[]= {0,1,2,3,4,5,6,7,8,9,20,21,22,23,24,25,26,27,28,29,10,11,12,13,14,15,16,17,18,19,30,31,32,33,34,35,36,37,38,39};

//enum { MAX_THREADS = 512 } ; 
enum { FIXEDSCALE  = 0x1000 } ; 
typedef void * (*Lambda) (void *) ; 
#define DOUBLE(v) ((double)(v))

static int Verbose = 0 ; 
static volatile int threads_alive = 0;
static volatile int nDead = 0 ; 
static pthread_mutex_t StartGate [1];
typedef enum { _RENDEZVOUS=0, _RUNNING=1, _HALTING=2, _HALTED=3 } RunStates ; 
static volatile int RunState  = 0 ; 
static volatile int can_start = 0;          // AKA: Start
static volatile int stop_now  = 0 ;         // AKA: Halt
static volatile int DogWarn   = 0 ; 
static int ReadKeyLength = 0 ; 
static KVMap * ht;

static long arg_total_ops = 0;
static int arg_private_work_num = 0;
static int arg_shadow_thread_max_num = 0;
static int arg_inserts = 0;
static int arg_deletes = 0;
static int arg_updates = 0 ; 
static int arg_thinks  = 0 ; 
static int arg_non_abortable = 0;
static int arg_non_abortable_readers = 0;
static int sk_inserts  = 0 ; 
static int sk_deletes  = 0 ; 
static int sk_updates  = 0 ; 
static int sk_thinks   = 0 ; 
static int arg_initial_size = -1 ;      // defaults to half arg_range
static int arg_range = 1000000;
static int FillMode  = 0 ; 
static int Determinism = 0 ; 
static int JumpFreq    = 0 ; 
static int uniq ;                       // Starting population
static int PreSum = 0 ; 
static int arg_opgroup = 1;
static int InterTxnDelay = 0 ;          // ideally parallel "think" time
static int _nThreads = 8;
static int _Duration = 10 ;
static char * ExecutableName = NULL ; 
static char * Comment = "" ; 

typedef struct {
  int pid;
  Thread * Self ; 
  pthread_t pthread_self ; 
  volatile int State ; 

  volatile int kSum ; 
  volatile int vSum ; 

  volatile int nUpdates ; 
  volatile int nDeletes ; 
  volatile int nInserts ; 
  volatile int nLookups ; 
  volatile int nMisses ; 
  volatile int nNonAbortable;
  volatile int nNonAbortableReaders;
  volatile long TXAborts ;        // failed
  volatile long TXCompleted ;     // successful
  volatile long total_ops;
  hrtime_t IntervalStart ; 
  hrtime_t IntervalEnd ; 
  double pad [64] ; 
} ThreadBlock ;

static ThreadBlock  thread_data[MAX_THREADS];

static int BindSpan    = 0 ; 
static char * BindMapFile = NULL ; 
static int nCpu     = 0 ; 
static int nConfig  = 0 ; 
static int * CpuMap = NULL ; 

// Linux-x86 port ...
#if defined(__sun)

static int CpuBind () { 
  static volatile int ThreadSeq = 0 ;
  static int CpuBase = 0 ; 
  static pthread_mutex_t IncLock [1] ; 
  int CpuID = -1 ;
  if (nCpu > 1 && BindSpan != 0) {
    if (BindSpan > nCpu || BindSpan <= 0) {
      BindSpan = nCpu ;
    }
    // Atomic fetch-and-add ....
    // EQUIV: ix = Adjust(&ThreadSeq, 1) ;  
    pthread_mutex_lock (IncLock) ; 
    int ix = ThreadSeq ++ ; 
    pthread_mutex_unlock (IncLock) ; 
    CpuID = CpuMap [CpuBase + (ix % BindSpan) ] ;
    int rslt = processor_bind (P_LWPID, P_MYID, CpuID, NULL) ;
    if (rslt != 0) {
      printf ("processor_bind (%d) failed\n", CpuID) ;
    } else
    if (Verbose) { 
      printf ("LWP %d bound to %d\n", _lwp_self(), CpuID) ;
    }
  }
  return CpuID ;
}


// Allow simplistic encoding B@<specification>
// Compact description of mapping between logical LWP ordinals and logical CPUIDs.  
// Logical CPUIDs in turn map to (Die,Core,Exu,Strand) physical resources.  
// Ultimately, we can exert control over LWP to physical resource mapping.
// 
// See also: 
// -- BindMaps/MakeMap-Batoka.c [EXCERPT]
// -- BindMaps/MakeBindMap.py
// -- Solaris: lwp_create() calls lgrp_move_thread (lgrp_choose()) to place a newly created LWP
// -- IP-TAXONOMY.txt
// -- corestat : README and perl script
//
// Remarks:
// -- Topology:
//      Chips/Dies/Sockets
//      Cores
//      uCores or pipelines or EXU
//      TG or "thread groups".  
//      CMT/HT strands within a pipeline
// -- Allow <ThreadID>:<CPUID> or <ThreadID>@<CPUID> pairs so we can
//    specifiy the mapping out-of-order
// -- Observation: on Niagara the mapping is a permutation (AKA sequence)
//    because threadIDs and CPUs are dense.  
// -- BindMap/b7 and b8 seem best 
// 
// Examples
// +  Batoka: 4 chips; 8 cores/chip; 2 pipelines/core; 4 strands/pipeline
//    Apparent Batoka logical CPUID to physical mapping on snv_78:
//      cpuid = (chipid:2, coreid:3, exeuid:1, strandid:2)
//    See also Solaris source kit -- lwp_create() calls lgrp_move_thread (lgrp_choose()) 
//    to place a newly created LWP
//    Batoka and N2 specifics
//      Pipelines share ifetch and instruction picker.
//      cores share one FPU
//      EXU AKA Pipeline AKA "TG" or thread-group
// +  N1 (T1) : 8 cores/chip; 4 strands/core
// +  N2 (T2) : 8 cores/chip; 2 pipelines/core; 4 strands/pipeline
//
// TODO : idealized usage
// --   B@D[0,64,128,196]S[0,1,2,3]X[0,8]C[0,8,16,24,32,40,48,56]
//      D=Die,S=Strand,X=P=Pipeline,C=Core
//      Should be equivalent to MakeMap-Batoka.c "B10"
//      Convert to array form during initialization
//      Internal representation:
//        struct { Node * Next; Node * Prev, char * Name; int nElements; int * RangeList; } 
//        RangeList is -1 terminated
//        Current "cursor" is (Node *, RangeIndex) pair
//        Simply sum the RangeList[RangeIndex] values to generate the CPUID
// --   B:D4@64,S2@1,P4@2,C8@8     EQUIV b7
//      B:D4@64,S2@1,C8@8,P4@2     EQUIV b8 

static int * CpuBuildMap (char * BindMapFile) {
  // TODO-FIXME: timing window here .... don't use sysconf() ...
  nCpu    = sysconf (_SC_NPROCESSORS_ONLN) ;
  nConfig = sysconf (_SC_NPROCESSORS_CONF) ;
  int * CpuMap  = (int *) malloc (sizeof(CpuMap[0]) * (nConfig+1)) ;
  if (BindMapFile == NULL) { 
    memset (CpuMap, 0, sizeof (CpuMap[0]) * nConfig) ;
    int j, found ; 
    for (j = found = 0 ; found < nCpu ; j++ ) {
      int rslt = p_online (j, P_STATUS) ;
      if (rslt == P_ONLINE) {
        CpuMap[found++] = j ;
      }
    }
  } else { 
    FILE * map = fopen (BindMapFile, "r") ; 
    if (map == NULL) { 
      printf ("Couldn't open Bind map file: %s\n", BindMapFile) ; 
      exit (1) ; 
    }
    int putIndex = 0 ; 
    for (;;) {
      if (putIndex > nConfig) {
        printf ("Warning: more entries in %s than are needed - ignored\n", BindMapFile) ; 
        break ; 
      }
      // Read the next CPUID # from the file
      char buf [128];
      memset (buf, 0, sizeof(buf)) ; 
      if (fgets (buf, sizeof(buf)-1, map) == NULL) break ;
      if (buf[0] == '#') continue ;     // skip comment line
      // TODO: allow thread#:cpuid pairs in any order
      // TODO: add an inner loop -- allow multiple numbers per line
      // TODO: consider sscanf()
      // TODO: skip blank lines
      int id = strtol (buf, NULL, 0) ; 
      // Validate CPUID
      if (id < 0 || p_online(id, P_STATUS) != P_ONLINE) {
        printf ("Invalid CPUID %d in Bind Map file: %s\n", id, BindMapFile) ; 
        exit (1) ;
      }
      // Check for duplicates
      int d ; 
      for (d = 0 ; d < putIndex && CpuMap[d] != id; d++) ; 
      if (d != putIndex) { 
        printf ("Warning: duplicate entries in Bind map file: %s %d\n", BindMapFile, id) ; 
      }
      // Insert into vector
      if (Verbose) printf ("%d ", id) ; 
      CpuMap[putIndex++] = id ; 
    }
    fclose (map) ; 
    if (putIndex < nConfig) {
      printf ("Needed %d from %s but only got %d\n", nConfig, BindMapFile, putIndex) ; 
      exit (1) ; 
    }
  }
  return CpuMap ;
}

#else

static int CpuBind () { return 0 ; } 
static int * CpuBuildMap (char * BindMapFile) { return NULL; } 
static int getcpuid () { return 0; }

#endif


// TODO: replace low-quality Linear Congruential LCRNG 
// with a Marsaglia-XOR or Park-Miller.  
static int SimpleLCRng (int * seed) { 
  *seed = ((*seed) * 1103515245 + 12345)  ; 
  return ((*seed) >> 16) & 0x7FFF ; 
}

// Simplistic low-quality Marsaglia shift-xor PRNG.
// Bijective except for the final masking operation.
// Cycle length for non-zero values is 4G-1.
// 0 is absorbing and should be avoided -- fixed point.
// Currently we seed/reseed with gethrtime() assuming that the various threadis
// will typically be seeded with distinct and different values.  
// Beware that if gethrtime() is "coarse" and the threads happen
// to seed at nearly the same time we could end up with common
// seeds and entrainment problems.    

static int MarsagliaNext (int v) {
  if (v == 0) v = 1 ;       // gethrtime()|1
  v ^= v << 6;
  v ^= ((unsigned)v) >> 21;
  v ^= v << 7 ; 
  return v ;
}

static int MarsagliaXOR (int * seed) {
  int x = *seed;
  if (x == 0) x = 1 ;       // gethrtime()|1
  x ^= x << 6;
  x ^= ((unsigned)x) >> 21;
  *seed = x ^= x << 7;
  return x & 0x7FFFFFFF;
}

static int MarsagliaG (int * seed) {
  static int gseed [128]  ; 
  if (seed == NULL) seed = gseed+64 ; 
  int x = *seed;
  if (x == 0) x = 1 ;       // gethrtime()|1
  x ^= x << 6;
  x ^= ((unsigned)x) >> 21;
  *seed = x ^= x << 7;
  return x & 0x7FFFFFFF;
}

static int _MarsagliaXOR (int x) {
  if (x == 0) x = gethrtime()|1 ;  
  x ^= x << 6;
  x ^= ((unsigned)x) >> 21;
  x ^= x << 7;
  return x ;    // CONSIDER:  return x & 0x7FFFFFFF;
}

static int NextRandom (int * x) { 
   *x = _MarsagliaXOR (*x) ; 
   return (*x) & 0x7FFFFFFF ; 
}

static int ParkMillerRNG(int *seed0) {
  const int a =      16807;
  const int m = 2147483647;
  const int q =     127773;  /* m div a */
  const int r =       2836;  /* m mod a */
  int seed = *seed0;
  int hi   = seed / q;
  int lo   = seed % q;
  int test = a * lo - r * hi;
  if (test > 0)
    seed = test;
  else
    seed = test + m;
  *seed0 = seed;
  return seed;
}

#define TLRand(sa) MarsagliaXOR(sa)

void PrivateWork(Thread *Self, KVMap * const _ht, int *p_seed, int rng)
{
	int i, j;
	int key;
	for (i = 0; i < arg_private_work_num; i++)
	{
	    key = TLRand(p_seed) % rng ; 
	    //kv_get(Self, _ht, key);
		for (j = 0; j < 10; j++)
		{
			membarstoreload();
		}
	}
	
}
void * Worker (void *arg) { 
  CpuBind () ; 
  ThreadBlock  * const pdata = (ThreadBlock  *)arg;
  Thread * const Self = TxNewThread (arg_shadow_thread_max_num) ; 
  pdata->Self = Self ; 
  int seed = (((int) &seed) + gethrtime()) | 1 ;
#if 0
  const volatile int tos = (int) &tos; 
  int seed = (tos ^ gethrtime()) | 1 ; 
#endif

#if !(__sparc)

  size_t cpusetsize = __CPU_SETSIZE;
  __cpu_mask mask;
  int numa_offset;

  numa_offset = non_numa[Self->UniqID];
  
  int err;
  mask = __CPUMASK(numa_offset);
  if(err = sched_setaffinity(0, 8, &mask)) 
  {
    printf("HILLEL: err = %d, cpusetsize = %d, mask %llx", err, cpusetsize, mask); 
	exit(88);
  }
  
  //Self->GroupID = Self->UniqID/20;
  printf("NUMA - ID = %d, Group = %d\n", Self->UniqID, Self->UniqID/20);
#endif
  
  if (Determinism) { 
    seed = pdata->pid ; 
    if (seed == 0) seed = 0xD1CE ; 
  }
  
  Self->seed = seed;
  Self->ups_part = arg_updates;
  Self->dels_part = arg_deletes;
  Self->mutations_part = arg_updates + arg_deletes;
  Self->non_abortable_part = arg_non_abortable;
  Self->non_abortable_part_readers = arg_non_abortable_readers;
  Self->private_work_num = arg_private_work_num;
  Self->range = arg_range;

  // Hoist allocation above and outside measurement interval
  const int _ReadKeyLength = ReadKeyLength ; 
  int * const KeyList = (int *) memalign (128, sizeof(*KeyList) * (ReadKeyLength+32)) ; 

  pthread_mutex_lock(StartGate);
  if (pdata->pid == 0) {
    // Delegate responsibility for initializing the map to the 1st worker.  
    // Consider moving this step/duty/responsibility to the primordial thread, 
    // before launching workers.
    const hrtime_t Base = gethrtime() ; 
    printf ("Initializing ...") ; 
    if (arg_initial_size < 0) arg_initial_size = arg_range/2 ; 
    for (int i = 0; i < arg_initial_size; i++) {
      const int key = FillMode ? i : (TLRand(&seed) % arg_range) ;
      int res = 0;
	  
	  Self->is_reader_non_abortable = 1;
	  res = kv_contains (Self, ht, key);
	  Self->is_reader_non_abortable = 0;
	  if (!res)
	  {
		Self->is_writer_non_abortable = 1;
		kv_put(Self, ht, key, key);
		Self->is_writer_non_abortable = 0;
		++uniq ; 
		PreSum += key ;
	  }
	  
    }
    printf ("Initialized %d unique of %d in %d msecs\n", 
      uniq, arg_initial_size, (gethrtime()-Base)/1000000LL) ; 
  }
  ++threads_alive;
  pthread_mutex_unlock(StartGate );

#ifdef PAPI
    if (PAPI_OK != PAPI_start_counters(g_events, G_EVENT_COUNT))
  {
    printf("Problem starting counters 1.");
  }
#endif

  int TallyMisses  = 0 ; 
  int TallyUpdates = 0; 
  int TallyInserts = 0 ; 
  int TallyDeletes = 0 ; 
  int TallyLookups = 0 ; 
  int TallyNonAbortable = 0;
  int TallyNonAbortableReaders = 0;
  int keysum = 0 ;      // key integrity check

  // Hoist globals into local immutables 
  const int _grp  = arg_opgroup ; 
  const int _ups  = arg_updates ;
  const int _ins  = arg_inserts ; 
  const int _dels = arg_deletes ; 
  const int _rng  = arg_range ; 
  const int _dly  = InterTxnDelay ; 
  KVMap * const _ht = ht ; 

  // TODO-FIXME: use a proper sense-reversing consensus barrier.  
  // Consider: poll (NULL, 0, 1) ; 
  // Stay ONPROC to avoid migration
  while (!can_start) ; 
  if (Verbose) printf ("[%d] ", getcpuid()) ; 

  // TODO:
  // -- Add think-time parallel component: advance PRNG
  // -- avoid div and mod (%)
  //    See RA-Harness.c and tleht.java
  
  Self->n_na_writes_lvl_1 = 0;
  
  pdata->total_ops++;
  while (!stop_now) {
    int op = TLRand (&seed) % 100 ; 
	int na_op = TLRand (&seed) % 100 ;
	
	pdata->total_ops--;
	if (pdata->total_ops <= 0)
	{
		break;
	}
#if 0
     // Better: cuts down loop overhead
     int opgroup = 1 ; 
     if (_grp != 1) opgroup = TLRand (&seed) % _grp ; 
#else
     // Original: adds largely useless RNG and MOD to loop overhead.
     int opgroup = TLRand (&seed) % _grp ; 
     if (_grp == 1) opgroup = 1 ; 
#endif
     if (op < _ins) {
       // Case: Insert
	   for (int i = 0; i < opgroup; i++) { 
         //const int is_na = TLRand(&seed) % 100 ; 
		 const int key = TLRand(&seed) % _rng ; 
	     		 
		 //Self->is_writer_non_abortable = 0;
		 //if (is_na < arg_non_abortable)
		 //{
		    /*int i;
			for (i = 0; i < Self->shadow_max_num; i++)
			{
				Self->inner_op_result[i] = 0;
			}*/
			//Self->keysum = 0;
			//Self->is_writer_non_abortable = 1;
			//TallyNonAbortable++;
		 //}
		 
		 Self->keysum = 0;
		 if (kv_insert(Self, _ht, key, key )) keysum += key ; 
         TallyInserts ++ ; 
		 
		 //if (Self->is_writer_non_abortable)
		 //{
			keysum += Self->keysum;
		 //}
		  
		 Self->is_writer_non_abortable = 0;
		 PrivateWork(Self, _ht, &seed, _rng);
	   }
		 
		 
     } else if (op >= _ins && op < (_ins+_ups)) { 
       // Case: Put AKA Update
	   for (int i = 0; i < opgroup; i++) { 
         //const int is_na = TLRand(&seed) % 100 ; 
		 const int key = TLRand(&seed) % _rng ; 
         
		 Self->is_writer_non_abortable = 0;
		 if (na_op < arg_non_abortable)
		 {
			int i;
			for (i = 0; i < Self->shadow_max_num; i++)
			{
				Self->inner_op_result[i] = 0;
			}
			Self->keysum = 0;
			Self->is_writer_non_abortable = 1;
			TallyNonAbortable++;
		 }
		 
		 Self->keysum = 0;
		 Self->inner_ops = 0;
#ifdef BENCH_COUNTER
		 kv_put_counter(Self);
#else
		 if (kv_put (Self, _ht, key, TLRand(&seed))) keysum += key ; 
         
#endif	 
		 TallyUpdates ++ ; 
		 
		 if (Self->is_writer_non_abortable)
		 {
			keysum += Self->keysum;
			pdata->total_ops -= Self->inner_ops;
		 }
		 
		 while (Self->combined_thread_id != -1)
		 {

			if (Self->op_type == 0)
			{
				kv_put (Self, _ht, Self->key, Self->val);

			} else
			{
				kv_delete(Self, _ht, Self->key);
			}
		 }
		 
		 Self->is_writer_non_abortable = 0;
		 
		 PrivateWork(Self, _ht, &seed, _rng);
	   }
	 } else if (op >= (100 - _dels)) {
       // Case: Delete
	   for (int i = 0; i < opgroup; i++) {
         //const int is_na = TLRand(&seed) % 100 ; 
		 const int key = TLRand(&seed) % _rng ; 
	     
		 Self->is_writer_non_abortable = 0;
		 if (na_op < arg_non_abortable)
		 {
			int i;
			for (i = 0; i < Self->shadow_max_num; i++)
			{
				Self->inner_op_result[i] = 0;
			}
			Self->keysum = 0;
			Self->is_writer_non_abortable = 1;
			TallyNonAbortable++;
		 }
		 
		 Self->keysum = 0;
		 Self->inner_ops = 0;
#ifdef BENCH_COUNTER
		 kv_put_counter(Self);
#else
		 if (kv_delete(Self, _ht, key)) keysum -= key ; 
#endif
		 TallyDeletes ++ ; 
		 
		 if (Self->is_writer_non_abortable)
		 {
			keysum += Self->keysum;
			pdata->total_ops -= Self->inner_ops;
		 }
		 
		 while (Self->combined_thread_id != -1)
		 {
			
			if (Self->op_type == 0)
			{
				kv_put (Self, _ht, Self->key, Self->val);

			} else
			{
				kv_delete(Self, _ht, Self->key);
			}
		 }
		 Self->is_writer_non_abortable = 0;
		 
		 PrivateWork(Self, _ht, &seed, _rng);
		 
	   }
     } else {
       // Case: lookup
       // Consider: the key distribution shouldn't be random.
       // Use some non-uniform PRNG that emulates locality.
       // Idea: key = (key * key)/ _rng
       Self->is_writer_non_abortable = 0;
	   if (na_op < arg_non_abortable_readers)
	   {
	     Self->is_reader_non_abortable = 1;
		 TallyNonAbortableReaders++;
	   }
	   
	   if (_ReadKeyLength != 0 ) {
         const int key = TLRand(&seed) % _rng ; 
         const int ct = kv_ReadKeys (Self, _ht, key, KeyList, _ReadKeyLength) ; 
         if (ct == 0) TallyMisses ++ ; 
         TallyLookups ++ ; 
       } else { 
	     for (int i = 0; i < opgroup; i++) { 
           const int key = TLRand(&seed) % _rng ; 
	       Self->keysum = 0;
		   Self->inner_ops = 0;
#ifdef BENCH_COUNTER
		   const int hit = kv_get_counter(Self);
#else		   
		   const int hit = kv_get(Self, _ht, key);
#endif
           TallyLookups ++ ; 
	   
		   keysum += Self->keysum;
		   pdata->total_ops -= Self->inner_ops;
		   
		   //PrivateWork(Self, _ht, &seed, _rng);
           if (!hit) TallyMisses ++ ;       // ISSUE: aborts tally as misses
	     }
       }
	   
	   Self->is_reader_non_abortable = 0;
	}

    // Inter-transaction delay time : ideally parallel
    for (int z = _dly ; --z >= 0 ; ) { 
      TLRand (&seed) ; 
    }
  }

  pdata->nMisses  = TallyMisses ; 
  pdata->nUpdates = TallyUpdates ; 
  pdata->nDeletes = TallyDeletes ; 
  pdata->nLookups = TallyLookups ; 
  pdata->nInserts = TallyInserts ; 
  pdata->nNonAbortable = Self->n_na_writes_lvl_1;
  pdata->nNonAbortableReaders = TallyNonAbortableReaders;
  pdata->kSum     = keysum ; 

  pthread_mutex_lock (StartGate) ;
  ++nDead ;
  -- threads_alive ;
  // XXXX ASSERT (threads_alive >= 0) ; 
  pthread_mutex_unlock (StartGate) ;

#ifdef PAPI
  if (PAPI_OK != PAPI_read_counters(g_values[Self->UniqID], G_EVENT_COUNT))
  {
    printf("Problem reading counters 2.");
  }
#endif
  return NULL;
}

// Variation on Worker() that provides thread-specific inter-transaction
// key-locality via a random walk.
// Key-locality translates into temporal and spatial data locality.
// Temporal data locality, in turn, can be leveraged by the FERAL mechanisms.
// Note too that temporal data locality also suggests temporal meta-data locality.
// Claim: we posit key locality in "real" applications.
//
// The current implementation imposes a rather tight bound on the maximum step.
//
// A slightly better approach would be to build a Step distribution table at 
// startup time.  Most entries in the table would be small, but a very few would
// have large step values.  That is, the distribution of values in the table would
// be non-uniform.  This results in modality or phase behavior.  
//
//    [Variant #1] 
//    rv = Next(rv)
//    Step = StepTable [rv & 0x7FF]
//    HomeKey = (HomeKey + Step) % _rng
//
// In Yet another variation we could have more explicit control over the
// frequency of region "shifts".
//
//    [Variant #2] -- probably best !!!!
//    rv = Next (rv)
//    if ((rv & 0x1FFF) == 0) {
//      // Jump to a new region
//      rv = Next(rv)
//      HomeKey = (rv & 0x7FFFFFFF) % _rng
//    } else {
//      // Use recent history table or bounded random walk
//      int Step = ((rv & 0x1F) - 0xF) ; 
//      if (Step == 0x10) Step = 0 ;        // Fold to avoid asymmetry
//      HomeKey = (HomeKey + Step) % _rng ; 
//      if (HomeKey < 0) HomeKey += _rng ;   // circular key-space
//    }
//
// Yet another variation -- bounds "run" length
//
//    [Variant #3] 
//    rv = Next (rv)
//    if ((rv & 0x1FFF) > run) {
//      // Jump to a new region
//      rv = Next(rv)
//      HomeKey = (rv & 0x7FFFFFFF) % _rng
//      run = 0 ;       // [OR] some non-zero value
//    } else {
//      // Use recent history table or bounded random walk
//      int Step = ((rv & 0x1F) - 0xF) ; 
//      if (Step == 0x10) Step = 0 ;        // Fold to avoid asymmetry
//      HomeKey = (HomeKey + Step) % _rng ; 
//      if (HomeKey < 0) HomeKey += _rng ;   // circular key-space
//      run ++ ; 
//    }
//
// Yet another variant -- uses history table
//    [Variant #4]
//    Initially :
//      for i = 0 ; i < DIM(PickTable) ; i++
//        PickTable[i] = MAXPICK * (NextDouble() RaiseTo (1/Alpha)) 
//        --[OR]--
//        for j = 0 ; NextDouble() < .8 && j < MAXPICK; j++
//        PickTable[i] = j 
//    
//     NextAddress
//       PickIndex = PickTable[NextRandom() & PickTableMask]
//       if PickIndex < N
//         k = History [(Cursor-PickIndex) & HistoryMask]
//       else
//         k = new random address
//       History[(++Cursor) & HistoryMask] = k
//       return k 
//      
//
//
// Remarks:
// *  Keep a short history table to emulate reuse (reuse distance)
// *  See also: Locality.* and discussions on non-uniform distributions. 
// *  See also:
//    On the fractal dimension of computer programs and its application
//    to the prediction of the cache miss ratio.  
//    Thiebaut, IEEE TOCS July 1989.
//    Fractal random walks; levy flight;
//
//    Synthetic traces for trace-driven simulation of cache memories
//    Theibaut; Wolf; Stone
//    IEEE TOCS April 1992
//
//    Synthetic trace generation for the internet
//    Shi et al.
//    University of Alberta
//
//    A hybrid Markov model for accurate memory address generation
//    Hassan et al.
//    ICCS07
//    Very good set of modern references
//
//    Wikipedia: inverse transform sampling
//
// *  Generalized trace generation with reference history stack
//
//    NextAddress() : 
//      if NextRandom() < CutOff          // NexRandom() uniform in [0,1)
//        LastAdress = A = NextRandom() * AddressSpaceSize
//        return A
//      if NextRandom() < RepeatRate : 
//        return LastAddress
//      ix = GenerateIndex()
//      if ix >= StackTop
//        LastAdress = A = NextRandom() * AddressSpaceSize
//      // TODO: have MoveToFront(ix) return value at Stack[ix]
//      LastAddress = A = ElementAt (ix)
//      MoveToFront(ix)
//      return A
//
//    GenerateIndex () 
//      // Generate non-uniform distribution from uniform PRNG
//      // via: LookupTable; formula; inverse transform sampling
//      R = NextRandom()
//      R = R * R
//      return R * StackMax
//
//      R = LookupTable [NextRandom() * DIM(LookupTable)] 
//      return R * StackMax
//
//      R = LookupTable [NextRandom() * DIM(LookupTable)] 
//      ASSERT StackTop > 0
//      return R * StackTop
//
//    The stack data structure exposes the following:
//    ElementAt(ix), MoveToFront(ix), PushAtFront(), StackMax, StackTop
//    Possible implementation variations : 
//    -- Simple array like java.util.Vector
//    -- Simple linked-list
//    -- Red-black tree map : IX->Address
//    -- Array and Red-black-tree map: IX->IX or IX->Displacement
//


// TODO:  Implement Variant-#2, using a JumpFrq variable to control rate


void * WorkerRandomWalk (void *arg) { 
  CpuBind () ; 
  ThreadBlock  * const pdata = (ThreadBlock  *)arg;
  Thread * const Self = TxNewThread (arg_shadow_thread_max_num) ; 
  pdata->Self = Self ; 
  const volatile int tos = (int) &tos; 
  int rv = (tos ^ gethrtime()) | 1 ; 
#if 0
  int seed = (tos ^ gethrtime()) | 1 ; 
#endif
  if (Determinism) { 
    rv = pdata->pid ; 
    if (rv == 0) rv = 0xD1CE ; 
  }

  // Hoist allocation above and outside measurement interval
  const int _ReadKeyLength = ReadKeyLength ; 
  int * const KeyList = (int *) memalign (128, sizeof(*KeyList) * (ReadKeyLength+32)) ; 

  pthread_mutex_lock(StartGate);
  if (pdata->pid == 0) {
    // Delegate responsibility for initializing the map to the 1st worker.  
    // Consider moving this step/duty/responsibility to the primordial thread, 
    // before launching workers.
    const hrtime_t Base = gethrtime() ; 
    printf ("Initializing ...") ; 
    if (arg_initial_size < 0) arg_initial_size = arg_range/2 ; 
    for (int i = 0; i < arg_initial_size; i++) {
      rv = MarsagliaNext (rv); 
      const int key = FillMode ? i : ((rv & 0x7FFFFFFF) % arg_range) ;  
      if (!kv_contains (Self, ht, key)) {  ; 
        kv_put(Self, ht, key, key);
        ++uniq ; 
        PreSum += key ; 
      }
    }
    printf ("Initialized %d unique of %d in %d msecs\n", 
      uniq, arg_initial_size, (gethrtime()-Base)/1000000LL) ; 
  }
  ++threads_alive;
  pthread_mutex_unlock(StartGate );

  int TallyMisses  = 0 ; 
  int TallyUpdates = 0; 
  int TallyInserts = 0 ; 
  int TallyDeletes = 0 ; 
  int TallyLookups = 0 ; 
  int keysum = 0 ;      // key integrity check

  // Hoist globals into local immutables 
  const int _ups  = arg_updates ;
  const int _ins  = arg_inserts ; 
  const int _dels = arg_deletes ; 
  const int _thk  = arg_thinks ; 
  const int _rng  = arg_range ; 
  const int _dly  = InterTxnDelay ; 
  KVMap * const _ht = ht ; 

  // TODO-FIXME: use a proper sense-reversing consensus barrier.  
  // Consider: poll (NULL, 0, 1) ; 
  // Stay ONPROC to avoid migration
  while (!can_start) ; 
  if (Verbose) printf ("[%d] ", getcpuid()) ; 

  // TODO:
  // -- Add think-time parallel component: advance PRNG
  // -- avoid div and mod (%)
  //    See RA-Harness.c and tleht.java

  rv = MarsagliaNext (rv); 
  int HomeKey = (rv & 0x7FFFFFFF) % _rng ; 
  
  while (!stop_now) {
    // randomly select an operation ....
    rv = MarsagliaNext (rv); 
    const int op = (rv & 0x7FFFFFFF) % 100 ; 

    // Randomly select a key ...
    // we use a random walk with a small step "dx".
    // Keywords: random walk; brownian; trajectory
    // They key-space is circular.  
    //
    // BEWARE: it's tempting to write something like:
    // +  Step = (-(rv & 0x10))|(rv & 0xF) ;   
    // +  Step = (rv & 0x1F) - 0xF 
    // However, because of 2's complement representation that 
    // approach results in an asymmetric distribution [-16,15]
    // which in turn yields a walk with slowly decreasing Homekey values.
    // That is, the resultant walk tends toward a decreasing trajectory.
    // Note that a range that's symmetric about 0 will have odd cardinality.
    // Instead, we should use something like the following:
    // +  Step = (rv & 0xF) * ((((rv >> 4) & 1) * 2)-1) ; 
    //    This doubles the frequence of 0, but is at least symmetric about 0.
    // +  msk  = (rv >> 4) & 1 ;               // yields either 0 or 1
    //    Step = ((rv & 0xF) ^ -msk) + msk ;   // Conditionally negate mag
    //    This doubles the frequence of 0, but is at least symmetric about 0.
    //    Based on the 2's complement equality: -x == ~x + 1 
    // +  Step = rv % StepWidth
    //    This doubles the frequence of 0, but is at least symmetric about 0.
    // +  Step = ((rv & 0x1F) - 0xF) ; 
    //    if (Step == 0x10) Step = 0 ; 
    //    This doubles the frequence of 0, but is at least symmetric about 0.
     
    rv = MarsagliaNext (rv); 
    int Step = ((rv & 0x1F) - 0xF) ; 
    if (Step == 0x10) Step = 0 ;        // Fold to avoid asymmetry
    HomeKey = (HomeKey + Step) % _rng ; 
    if (HomeKey < 0) HomeKey += _rng ;   // circular key-space

    if (op < _ins) {
      // Case: Insert
	  if (kv_insert(Self, _ht, HomeKey, HomeKey )) keysum += HomeKey ; 
      TallyInserts ++ ; 
    } else if (op >= _ins && op < (_ins+_ups)) { 
      // Case: Put AKA Update
      if (kv_put (Self, _ht, HomeKey, rv)) keysum += HomeKey ; 
      TallyUpdates ++ ; 
    } else if (op >= (100 - _dels)) {
      // Case: Delete
      if (kv_delete(Self, _ht, HomeKey)) keysum -= HomeKey ; 
      TallyDeletes ++ ; 
    } else {
      // Case: lookup
      // Consider: the key distribution shouldn't be random.
      // Use some non-uniform PRNG that emulates locality.
      // Idea: key = (key * key)/ _rng
	  const int hit = kv_get(Self, _ht, HomeKey);
      TallyLookups ++ ; 
      if (!hit) TallyMisses ++ ;       // ISSUE: aborts tally as misses
	}

    // Inter-transaction delay time : ideally parallel
    // Variations:
    // +  Fixed delay
    // +  Random delay within some interval
    // +  Bernoulli trials with guaranteed eventual progress
    //    for (int v = 0 ; (Next() & msk) >= v ; v++) ; 
    for (int z = _dly ; --z >= 0 ; ) { 
      rv = MarsagliaNext (rv); 
    }
  }

  pdata->nMisses  = TallyMisses ; 
  pdata->nUpdates = TallyUpdates ; 
  pdata->nDeletes = TallyDeletes ; 
  pdata->nLookups = TallyLookups ; 
  pdata->nInserts = TallyInserts ; 
  pdata->kSum     = keysum ; 

  pthread_mutex_lock (StartGate) ;
  ++nDead ;
  -- threads_alive ;
  // XXXX ASSERT (threads_alive >= 0) ; 
  pthread_mutex_unlock (StartGate) ;

  return NULL;
}

void * WorkerNEW (void *arg) { 
  CpuBind () ; 
  ThreadBlock  * const pdata = (ThreadBlock  *)arg;
  Thread * const Self = TxNewThread (arg_shadow_thread_max_num) ; 
  pdata->Self = Self ; 
  const volatile int tos = (int) &tos; 
  int rv = (tos ^ gethrtime()) | 1 ; 
  if (Determinism) { 
    rv = pdata->pid ; 
    if (rv == 0) rv = 0xD1CE ; 
  }

  pthread_mutex_lock(StartGate);
  if (pdata->pid == 0) {
    // Delegate responsibility for initializing the map to the 1st worker.  
    // Consider moving this step/duty/responsibility to the primordial thread, 
    // before launching workers.
    const hrtime_t Base = gethrtime() ; 
    printf ("Initializing ...") ; 
    if (arg_initial_size < 0) arg_initial_size = arg_range/2 ; 
    for (int i = 0; i < arg_initial_size; i++) {
      rv = MarsagliaNext (rv); 
      const int key = FillMode ? i : (rv & (arg_range - 1)) ;  
      if (!kv_contains (Self, ht, key)) {  ; 
        kv_put(Self, ht, key, key);
        ++uniq ; 
        PreSum += key ; 
      }
    }
    printf ("Initialized %d unique of %d in %d msecs\n", 
      uniq, arg_initial_size, (gethrtime()-Base)/1000000LL) ; 
  }
  ++threads_alive;
  pthread_mutex_unlock(StartGate);

  int TallyMisses  = 0 ; 
  int TallyUpdates = 0; 
  int TallyInserts = 0 ; 
  int TallyDeletes = 0 ; 
  int TallyLookups = 0 ; 
  int keysum = 0 ;      // key integrity check

  // Hoist globals into local immutables 
  const int _ups  = sk_updates ;
  const int _ins  = sk_inserts ; 
  const int _dels = sk_deletes ; 
  const int _thks = sk_thinks ; 
  const int _rng  = arg_range - 1 ; 
  const int _dly  = InterTxnDelay ; 
  const int _ReadKeyLength = ReadKeyLength ; 
  int * const KeyList = (int *) memalign (128, sizeof(*KeyList) * (ReadKeyLength+32)) ; 
  KVMap * const _ht = ht ; 


  // TODO-FIXME: use a proper sense-reversing consensus barrier.  
  // Consider: poll (NULL, 0, 1) ; 
  // Stay ONPROC to avoid migration
  while (!can_start) ; 
  pdata->IntervalStart = gethrtime() ; 
  if (Verbose) printf ("[%d] ", getcpuid()) ; 
    
  while (!stop_now) {
    rv = MarsagliaNext (rv) ; 
    int op = rv & (FIXEDSCALE-1) ; 
    if (op < _ins) {
      // Case: Insert
      rv = MarsagliaNext (rv) ; 
      const int key = rv &  _rng ; 
	  if (kv_insert(Self, _ht, key, key )) keysum += key ; 
      TallyInserts ++ ; 
    } else 
    if (op < _ups) { 
      // Case: Put AKA Update AKA PutIfAbsent
      rv = MarsagliaNext (rv) ; 
      const int key = rv & _rng ;  
      if (kv_put (Self, _ht, key, rv = MarsagliaNext(rv))) keysum += key ; 
      TallyUpdates ++ ; 
	} else  
    if (op < _dels) {
      // Case: Delete
      rv = MarsagliaNext (rv) ; 
      const int key = rv & _rng ; 
	  if (kv_delete(Self, _ht, key)) keysum -= key ; 
      TallyDeletes ++ ; 
    } 
    if (op < _thks ) { 
      // Case: Think
      rv = MarsagliaNext (rv) ; 
    } else {
      // Case: lookup
      // Consider: the key distribution shouldn't be random.
      // Use some non-uniform PRNG that emulates locality.
      // Idea: key = (key * key)/ _rng
      rv = MarsagliaNext (rv) ; 
      const int key = rv &  _rng ; 
      if (_ReadKeyLength != 0) { 
        const int ct = kv_ReadKeys (Self, _ht, key, KeyList, _ReadKeyLength) ; 
        if (ct == 0) TallyMisses ++ ; 
        TallyLookups ++ ; 
      } else { 
        int hit = kv_get(Self, _ht, key);
        TallyLookups ++ ; 
        if (!hit) TallyMisses ++ ;      // ISSUE: aborts tally as misses?
      }
	}

    // Inter-transaction delay time : ideally parallel
    for (int z = _dly ; --z >= 0 ; ) { 
      rv = MarsagliaNext (rv) ; 
    }
  }

  pdata->nMisses  = TallyMisses ; 
  pdata->nUpdates = TallyUpdates ; 
  pdata->nDeletes = TallyDeletes ; 
  pdata->nLookups = TallyLookups ; 
  pdata->nInserts = TallyInserts ; 
  pdata->kSum     = keysum ; 
  pdata->IntervalEnd = gethrtime() ; 

  pthread_mutex_lock (StartGate) ;
  ++nDead ;
  -- threads_alive ;
  // XXXX ASSERT (threads_alive >= 0) ; 
  pthread_mutex_unlock (StartGate) ;

  return NULL;
}

volatile char * Observable [256]  ; 
char * volatile PreventElision = NULL ; 
static void * (*WorkFunc)(void * arg) = NULL ; 

static void * WorkerJacket (void * arg) {              // Jacket or wrapper
  // alloca() can inhibit certain optimizations - make sure 
  // the alloc() call is not in the method containing the key 
  // mutator loop.   If Worker2() ends up being inlined we
  // could simply call thru a volatile function pointer.  
  ThreadBlock * tb = (ThreadBlock  *)arg;
  char * x = alloca ((gethrtime() * 7297) & 0xFFFF) ;     // color stack
  if (PreventElision == x) PreventElision = x + 1 ; 
  return WorkFunc (arg) ; 
}

static pthread_t LaunchThread (void * (*func)(void *), void * arg) { 
  pthread_attr_t hx [1];
  pthread_attr_init (hx);
  if (1) {
    pthread_attr_setschedpolicy(hx, SCHED_RR);
    pthread_attr_setscope(hx, PTHREAD_SCOPE_SYSTEM) ;
    // Beware: DETACHED -> not join()able
    pthread_attr_setdetachstate (hx, PTHREAD_CREATE_DETACHED) ;
  }
  pthread_t newid [1] ;
  newid[0] = 0 ; 
  int rc = pthread_create(newid, hx, func, arg);
  if (rc != 0) {
   printf("error creating thread, error: %s\n", strerror(rc));
   return 0 ;
  }
  return newid[0] ; 
}

typedef struct {
  void * (*func)() ; 
  void * arg ; 
  int Color ; 
  intptr_t ReturnValue ;        // Future-like
  volatile int Status ; 
} Trampoline ; 


static void * Thunk (void * tmp) { 
  char * x = alloca ((gethrtime() * 7297) & 0xFFFF) ;     // color stack
  if (PreventElision == x) PreventElision = x + 1 ; 
  void * (*func)() = ((Trampoline *)tmp)->func ; 
  void * arg       = ((Trampoline *)tmp)->arg ; 
  free (tmp) ; 
  return (*func)(arg) ; 
}

static pthread_t LaunchThreadC (void * (*func)(void *), void * arg) { 
  pthread_attr_t hx [1];
  pthread_attr_init (hx);
  if (1) {
    pthread_attr_setschedpolicy(hx, SCHED_RR);
    pthread_attr_setscope(hx, PTHREAD_SCOPE_SYSTEM) ;
    // Beware: DETACHED -> not join()able
    pthread_attr_setdetachstate (hx, PTHREAD_CREATE_DETACHED) ;
  }
  Trampoline * const tmp = (Trampoline *) malloc (sizeof(Trampoline)) ; 
  tmp->func = func ; 
  tmp->arg  = arg ; 
  pthread_t newid [1] ;
  newid[0] = 0 ; 
  const int rc = pthread_create(newid, hx, Thunk, tmp);
  if (rc != 0) {
    free (tmp) ; 
    printf("error creating thread, error: %s\n", strerror(rc));
    return 0 ;
  }
  return newid[0] ; 
}

// Consider: implement watchdog facility in shell scripts or as separate helper program.

static void * WatchDog (void * arg) {
  printf ("WatchDog running; ") ;
  poll (NULL, 0, _Duration * 2 * 1000) ;
  DogWarn = 1 ; 
  printf ("\nWATCHDOG WARNING: Alive=%d Start=%d Halt=%d\n", threads_alive, can_start, stop_now) ; 
  poll (NULL, 0, _Duration * 4 * 1000) ;
  printf ("\nWATCHDOG WARNING: Alive=%d Start=%d Halt=%d\n", threads_alive, can_start, stop_now) ; 
  poll (NULL, 0, 3 * 1000) ;
  printf ("\nERROR! -- WATCHDOG TIMEOUT!\n") ;
  _exit (1) ; 
  return NULL ; 
}

static int ParseInt (char * str) {
  if (*str == '=') ++str ; 
  char * p = strdup (str) ;
  char * const m = p ;
  int mulf = 1 ;           // Units multiplier
  while (isxdigit(*p) || *p == 'x' || *p == 'X') ++p ;
  if (*p == 'k' || *p == 'K') {
    mulf = 1024 ;
    *p = 0 ;
  } else
  if (*p == 'm' || *p == 'M') {
    mulf = 1024*1024 ;
    *p = 0 ;
  }
  const int v = strtol (m, NULL, 0) * mulf ;
  free (m) ;
  return v ;
}

static int AggregateUserTime () { 
  struct rusage ru ; 
  getrusage (RUSAGE_SELF, &ru) ;        // process-wide, accumulate user-time to date.
  // TODO: return (secs*1000L) + (usecs/1000L) ; 
  return  ((ru.ru_utime.tv_sec * 1000000LL) + ru.ru_utime.tv_usec)/1000LL ;     
}

// returns relative-time since program start
static int Since () {                   
  static hrtime_t Epoch = 0 ;       
  hrtime_t Now = gethrtime() ; 
  if (Epoch == 0) Epoch = Now ;         // Beware: initialization not MT-safe
  return (Now-Epoch)/1000000LL ; 
}

static int SelfTest (Thread * Self, KVMap * ht) { 
  enum { RANGE = 5000} ; 
  static int map [RANGE] ;    // Tracks the ht - key:value map
  int i ; 
  int seed = gethrtime() ;  
  printf ("(1) Single-threaded self-test\n") ; 
  for (i = 0 ; i < 2000000 ; i++ ) { 
    int op = TLRand(&seed) % 100 ; 
    if (op < 20) { 
      int key = TLRand (&seed) % RANGE ; 
      kv_delete (Self, ht, key) ; 
      map [key] = 0 ; 
      if (kv_get (Self, ht, key) != 0) { 
        printf ("%d: delete read-back %d\n", key, kv_get (Self, ht, key)) ; 
      }
    } else 
    if (op >= 20 && op < 50) { 
      int key = TLRand (&seed) % RANGE ; 
      int val = TLRand (&seed) ; 
      kv_put (Self, ht, key, val) ; 
      map [key] = val ; 
    } else { 
      int key = TLRand (&seed) % RANGE ; 
      if (kv_get(Self, ht, key) != map[key]) { 
        printf ("%d: map=%d ht=%d\n", key, map[key], kv_get(Self, ht, key)) ; 
      }
    }
  }
  printf ("(2) read-back\n") ; 
  for (i = 0 ; i < RANGE ; i++) { 
    if (kv_get (Self, ht, i) != map[i]) { 
      printf ("%d: map=%d ht=%d\n", i, map[i], kv_get(Self, ht, i)) ; 
    }
  }
  printf ("(3) integrity check\n") ; 
  kv_verify (ht, 1) ; 
  printf ("(4) completed\n") ; 
  return 0 ; 
}


int main(int argc, char *argv[]) { 
  int i, k;
  setbuf (stdout, NULL) ; 
  Since() ; 
  const int ncpus = sysconf(_SC_NPROCESSORS_ONLN) ; 
  int MaxThreads = MAX_THREADS ; 
  ExecutableName = argv[0] ; 
  for (int ix = 0 ; ix < argc; ix++) {
    printf ("%s ", argv[ix]) ; 
  }
  printf ("\n") ; 
  WorkFunc = Worker ; 
  TxOnce () ; 
  kv_init();

  --argc ; 
  ++argv ; 
  while (argc > 0) {
     char * p = *(argv++)  ; 
     if (*p == '-') ++p ; 
     --argc ; 
     switch (*(p++)) { 
     case 'B':
       BindSpan = -1 ;
       BindMapFile = NULL ; 
       if (*p == ':') { 
         BindMapFile = p+1 ; 
         printf ("Binding: %s\n", BindMapFile) ; 
       } else { 
         printf ("Binding 1:1\n") ; 
       }
       CpuMap = CpuBuildMap(BindMapFile) ; 
       break ; 
     case 'F':
       if (*p == 0 && argc > 0) { 
          p = *(argv++) ; --argc ; 
       }
       FillMode = ParseInt (p) ; 
       printf ("FillMode=%d\n", FillMode) ; 
       break ; 
     case 'L':
       //if (*p == 0 && argc > 0) { 
       //   p = *(argv++) ; --argc ; 
       //}
       //ReadKeyLength = ParseInt (p) ; 
	   arg_private_work_num = ParseInt (p) ; 
       break ; 
     case 'D':
       if (*p == 0 && argc > 0) { 
          p = *(argv++) ; --argc ; 
       }
	   arg_total_ops = ParseInt (p) ; 
       //_Duration = ParseInt (p) ; 
       break ; 
     case 's':
       if (*p == 0 && argc > 0) { 
          p = *(argv++) ; --argc ; 
       }
       arg_initial_size = ParseInt (p) ; 
       break ; 
     case 'r':
       if (*p == 0 && argc > 0) { 
          p = *(argv++) ; --argc ; 
       }
       arg_range = ParseInt (p) ; 
       break ; 
     case 'c':
       if (*p == 0 && argc > 0) { 
          p = *(argv++) ; --argc ; 
       }
       arg_thinks = ParseInt (p) ; 
       if (arg_thinks < 0) { printf ("Thinks < 0\n") ; exit (1) ; } 
       break ; 
     case 'n':
	 case 'P':
	   if (*p == 0 && argc > 0) { 
          p = *(argv++) ; --argc ; 
       }
	   arg_non_abortable = ParseInt(p);
	   break;
     case 'Q':
	   if (*p == 0 && argc > 0) { 
          p = *(argv++) ; --argc ; 
       }
	   arg_shadow_thread_max_num = ParseInt(p);
	   break;
	 case 'O':
	   if (*p == 0 && argc > 0) { 
          p = *(argv++) ; --argc ; 
       }
	   arg_non_abortable_readers = ParseInt(p);
	   break;
	 case 'T':
     case 't':
       if (*p == '=') { if (ncpus < MaxThreads) MaxThreads = ncpus ; ++p; } 
       if (*p == 0 && argc > 0) { 
          p = *(argv++) ; --argc ; 
       }
       _nThreads = ParseInt (p) ; 
       if (_nThreads < 0 || _nThreads > MaxThreads)  { 
         printf ("nThreads=%d\n", _nThreads) ; 
         exit (1) ; 
       }
       break ;    
     case 'z':
     case 'Z':
       if (*p == 0 && argc > 0) { 
          p = *(argv++) ; --argc ; 
       }
       InterTxnDelay = ParseInt (p) ; 
       break ; 
     case 'u':
       if (*p == 0 && argc > 0) { 
          p = *(argv++) ; --argc ; 
       }
       arg_updates = ParseInt (p) ; 
       if (arg_updates < 0) {
         printf ("updates < 0\n") ; 
         exit (1) ; 
       }
       break ;    
     case 'i':
       if (*p == 0 && argc > 0) { 
          p = *(argv++) ; --argc ; 
       }
       arg_inserts = ParseInt (p) ; 
       if (arg_inserts < 0) { 
         printf ("inserts < 0\n") ; 
         exit (1) ; 
       }
       break ; 
     case 'd':
       if (*p == 0 && argc > 0) { 
          p = *(argv++) ; --argc ; 
       }
       arg_deletes = ParseInt (p) ; 
       if (arg_deletes < 0) { printf ("deletes < 0\n") ; exit (1) ; } 
       break ; 
     case 'g':
       if (*p == 0 && argc > 0) { 
          p = *(argv++) ; --argc ; 
       }
       arg_opgroup = ParseInt (p) ; 
       break ; 
     case 'C':
       Comment = p ;     // consider strdup+strcat
       break ; 
     case 'V':
       ++Verbose ; 
       break ; 
     case 'R':            // reproducibility
       Determinism ++ ; 
       printf ("Determinism=%d\n", Determinism) ; 
       break ; 
     case 'W':
       WorkFunc = WorkerRandomWalk ; 
       printf ("+ Random Walk\n") ; 
       break ; 
     case 'N':
       WorkFunc = WorkerNEW ; 
       break ; 
     default:
       printf ("UNKNOWN SWITCH: %s\n", p-1) ; 
     }
  }

#ifdef PAPI
  if (PAPI_VER_CURRENT != PAPI_library_init(PAPI_VER_CURRENT))
  {
	printf("PAPI_library_init error.\n");
  }
  printf("PAPI_library_init success.\n");
  
  if (PAPI_OK != PAPI_query_event(PAPI_L1_DCM))
  {
    printf("Cannot count PAPI_L1_DCM.");
  }
  printf("PAPI_query_event: PAPI_L1_DCM OK.\n");
  if (PAPI_OK != PAPI_query_event(PAPI_L2_DCM))
  {
    printf("Cannot count PAPI_L2_DCM.");
  }
  printf("PAPI_query_event: PAPI_L2_DCM OK.\n");

#endif

  if (arg_deletes > arg_updates) {
    printf ("WARNING: deletes >> updates -- tree will converge to empty\n") ; 
  }
  if ((arg_deletes + arg_inserts + arg_updates + arg_thinks) > 100) { 
    printf ("Error: Deletes+inserts+updates+thinks >> 100\n") ; 
    exit (1) ; 
  }

  if (WorkFunc == WorkerNEW) { 
    if ((arg_range & (arg_range-1)) != 0) {
      printf ("Error: Range must be a power-of-two %d\n", arg_range) ; 
      exit (1) ; 
    }
    // Beware: there's some quantization error in the following.
    // XXX ASSERT ((FIXEDSCALE & (FIXEDSCALE-1)) == 0) ; 
    
    sk_inserts = (arg_inserts * FIXEDSCALE) / 1000 ; 
    sk_updates = sk_inserts + ((arg_updates * FIXEDSCALE) / 100) ; 
    sk_deletes = sk_updates + ((arg_deletes * FIXEDSCALE) / 100) ; 
    sk_thinks  = sk_deletes + ((arg_thinks  * FIXEDSCALE) / 100) ; 
    // Order sensitive -- sk_lookups is implicitly FIXEDSCALE
    // ASSERT 0x0 <= sk_inserts <= sk_updates <= sk_deletes <= sk_thinks <= FIXEDSCALE
    printf ("I=%X U=%X D=%X THINK=%X Lookup=%X\n", sk_inserts, sk_updates, sk_deletes, sk_thinks, FIXEDSCALE) ; 
    printf ("I=%d U=%d D=%d THINK=%d Lookup=%d\n", sk_inserts, sk_updates, sk_deletes, sk_thinks, FIXEDSCALE) ; 
  }

  ht = kv_create(-1, NULL);

  // Run a quick smoke-test
  Thread * const Self = TxNewThread (arg_shadow_thread_max_num) ; 
  if (getenv ("SELFTEST") != NULL) { 
      SelfTest (Self, ht) ; 
  }

  TxResetThreadsNum();
  
  if (Verbose & 2) { 
     printf ("  ") ; 
     printf ("V%d ",             kv_verify (ht, 0)) ; 
     printf ("INSERT(55)=%d "  , kv_insert (Self, ht, 55, 55)) ; 
     printf ("INSERT(55)=%d "  , kv_insert (Self, ht, 55, 55)) ; 
     printf ("GET(55)=%d "     , kv_get    (Self, ht, 55)) ;
     printf ("CONTAINS(55)=%d ", kv_contains (Self, ht, 55)) ; 
     printf ("DELETE(55)=%d "  , kv_delete (Self, ht, 55)) ; 
     printf ("DELETE(55)=%d "  , kv_delete (Self, ht, 55)) ; 
     printf ("GET(55)=%d "     , kv_get    (Self, ht, 55)) ;
     printf ("CONTAINS(55)=%d ", kv_contains (Self, ht, 55)) ; 
     printf ("V%d\n",            kv_verify (ht, 0)) ; 
     printf ("  ") ; 
     printf ("Insert ") ; 
     int i ; 
     for (i = 0 ; i < 20 ; i++) kv_insert (Self, ht, i ^ 0x5A, i ^ 0x5A) ; 
     int fail = 0 ;
     for (i = 0 ; i < 20 ; i++) {
       kv_delete (Self, ht, i ^ 0x5A) ; 
       if (fail == 0 && !kv_verify (ht, 0)) {
         ++fail ;
         printf ("fail >> %d (%d)\n", i, (i ^ 0x5A)) ; 
       }
     }
     if (kv_verify (ht, 0) <= 0) printf ("VERIFY FAILURE!\n") ; 

     printf (" " ) ; 
     printf ("GET(1000)=%d "   , kv_get(Self, ht, 1000)) ; 
     printf ("SET(1000,1)=%d " , kv_put(Self, ht, 1000, 1)) ; 
     printf ("GET(1000)=%d "   , kv_get(Self, ht, 1000)) ; 
     printf ("SET(1000,2)=%d " , kv_put(Self, ht, 1000, 2)) ; 
     printf ("GET(1000)=%d "   , kv_get(Self, ht, 1000)) ; 
     printf ("V%d\n", kv_verify (ht, 0)) ; 
     printf ("\n") ; 
  }

  pthread_mutex_init(StartGate , NULL);
  //LaunchThread (WatchDog, NULL) ; 

  printf ("Launching[@+%d]...", Since()) ; 
  for (i = 0; i < _nThreads ; i++) {
    thread_data[i].pid = i;
	thread_data[i].total_ops = arg_total_ops / _nThreads;
    thread_data[i].pthread_self = LaunchThread (WorkerJacket, (void *)(thread_data + i));
  }
  poll (NULL, 0, 100) ; 

  printf ("Launched[@+%d]...", Since()) ; 

  // Wait for all the threads to be running. 
  int dw = 0 ; 
  while (threads_alive < _nThreads) { 
    poll (NULL,0,10); 
    if (DogWarn && dw == 0) { 
      dw = 1 ; 
      printf ("Waiting %d %d %d\n", threads_alive, nDead, _nThreads) ; 
    }
  }

  // Quiesce
  // Allow scheduler to disperse LWPs over CPUs and for LWPs to gain affinity
  // The system should converge on a stable state
  poll (NULL, 0, 100) ; 

#define MARKER() { printf ("@ %d Delta=%lld Usage=%d\n", __LINE__, (gethrtime()-start_time)/1000000LL, AggregateUserTime()) ; } 
  
  printf ("Starting [@+%d]...", Since()) ; 
  hrtime_t start_time = gethrtime();
  can_start = 1;
  //poll (NULL, 0, _Duration*1000) ; 
  //stop_now  = 1;
  while (threads_alive != 0) { poll (NULL, 0, 20); }
  hrtime_t end_time = gethrtime();

  printf ("shutdown [@+%d]...", Since()) ; 

  if (0) { 
    // Beware: our threads are detached and NOT joinable
    // Wait for all the threads to shut down.
    for (i = 0; i < _nThreads ; i++) {
       pthread_join(thread_data[i].pthread_self, NULL);
    }
  } else { 
    while (threads_alive != 0) { poll (NULL, 0, 20); } 
  }

  // TODO: report min, miax, stddev, median, average, spread for (I,D,L,U)

  printf("results:\n");
  int all_inserts = 0 ; 
  int all_misses  = 0 ; 
  int all_updates = 0 ; 
  int all_deletes = 0 ; 
  int all_lookups = 0 ; 
  int all_aborts  = 0 ; 
  int AggSum      = 0 ; 
  int64_t MaxCompleted = 0 ; 
  int64_t MinCompleted = -1 ; 
  int nOps = 0 ; 
  int64_t lds, sts ;
  lds = sts = 0 ; 
  for (i = 0; i < _nThreads; i++) {
     int64_t Completed = 
        thread_data[i].nInserts + 
        thread_data[i].nDeletes +
        thread_data[i].nUpdates + 
        thread_data[i].nLookups ; 
     if (Completed > MaxCompleted) MaxCompleted = Completed ; 
     if (Completed < MinCompleted || MinCompleted < 0) MinCompleted = Completed ; 
     if (Verbose) { 
       printf("(%d, %d, %d, %d) ", 
     	   thread_data[i].nInserts, 
     	   thread_data[i].nDeletes, 
     	   thread_data[i].nUpdates,
         thread_data[i].nLookups) ; 
     }
     lds         += thread_data[i].Self->TxLD ; 
     sts         += thread_data[i].Self->TxST ; 
     all_aborts  += thread_data[i].Self->Aborts ; 
     all_misses  += thread_data[i].nMisses ; 

     all_inserts += thread_data[i].nInserts ; 
     all_updates += thread_data[i].nUpdates ; 
     all_lookups += thread_data[i].nLookups ; 
     all_deletes += thread_data[i].nDeletes ; 
     AggSum      += thread_data[i].kSum ; 
      
     if (Verbose && (i % 3) == 2) printf ("\n") ; 
  }
  if (Verbose) printf ("\n") ;
  nOps = all_inserts + all_updates + all_deletes + all_lookups ; 

  printf ("Post validation : ") ; 
  // Logical content validation ...
  // Crude - iterate over the key space to determine the population (cardinality)
  // of the RB-tree.  A better way would be to augment the recursive _verify()
  // routine to return the population.  
  int upop = 0 ; 
  int PostSum = 0 ; 
  Self->is_reader_non_abortable = 1;
  for (i = 0 ; i < arg_range ; i++) { 
    upop += kv_contains (Self, ht, i) != 0 ; 
	if (kv_contains (Self, ht, i)) PostSum += i ; 
  }
  Self->is_reader_non_abortable = 0;
  if ((PreSum + AggSum) != PostSum) { 
    printf ("ERROR!: Lightweight key integrity check failure %X+%X != %X\n",
      PreSum, AggSum, PostSum) ; 
    exit (1) ; 
  } else { 
    printf ("[pass] Content Integrity check: %X+%X=%X\n", PreSum, AggSum, PostSum) ; 
  }

  // Structural validation ...
  // Run a post-mortem verification pass.
  // There are no concurrent operations on the ht.
  // TODO: implement a non-transactional verify we can run when
  // the ht is stable and queisced. 
  // Verify post-conditions.
  int vfy = kv_verify (ht, 1) ; 
  if (vfy <= 0) {
    printf ("ERROR! Structural integrity failure %d\n", vfy) ; 
    exit (1) ; 
  }

  printf ("TEST: %s %dT %d msecs ins=%%%d del=%%%d ups=%%%d isize=%d (initpop=%d) range=%d\n", 
    TxDescribe(),
    _nThreads, _Duration*1000, arg_inserts, arg_deletes, arg_updates, arg_initial_size, 
    uniq, arg_range) ; 
  

  // uniq is the initial population.  
  // TODO: We should report the final population, too.  

  printf ("RESULTS: Dur=%d pop=%d U=%d I=%d D=%d L=%d (Misses=%d) SPREAD=%g TOTAL=%d\n", 
    (int)((end_time - start_time) / 1000000),
    upop,
    all_updates, all_inserts, all_deletes, all_lookups, all_misses, 
    DOUBLE(MaxCompleted)/DOUBLE(MinCompleted+1),
    nOps) ; 
  printf ("RESULTS: TxLDs=%lld TxSTs=%lld\n", lds, sts) ; 
  printf ("SUMMARY: %s %s %s %dT I%d D%d U%d L%d:%d (%d,%d) ABO=%d pop=%d -> %d Ops\n", 
    ExecutableName,
    Comment,
    TxDescribe(), 
    _nThreads, arg_inserts, arg_deletes, arg_updates,
    100 - (arg_inserts + arg_deletes + arg_updates),
    ReadKeyLength,
    arg_initial_size, arg_range, all_aborts, upop, nOps) ;
  
  struct utsname un ;
  uname (&un) ;       // consider: gethostname()
  printf ("SYNOPSIS: %s %s %s %dT %dC %dS I%d D%d U%d L%d Z%d R%d ABO=%d : %d Ops\n", 
    ExecutableName,
    Comment,
    un.nodename,
    _nThreads, ncpus, _Duration,
    arg_inserts, arg_deletes, arg_updates,
    100 - (arg_inserts + arg_deletes + arg_updates),
    InterTxnDelay, 
    arg_range,
    all_aborts,
    nOps) ; 
    
  if (0) { 
    printf ("INSERT(55)=%d "   , kv_insert (Self, ht, 55, 55)) ; 
    printf ("INSERT(55)=%d "   , kv_insert (Self, ht, 55, 55)) ; 
    printf ("CONTAINS(55)=%d " , kv_contains (Self, ht, 55)) ;
    printf ("DELETE(55)=%d "   , kv_delete (Self, ht, 55)) ; 
    printf ("DELETE(55)=%d "   , kv_delete (Self, ht, 55)) ; 
    printf ("CONTAINS(55)=%d " , kv_contains (Self, ht, 55)) ;
    printf ("VERIFY=%d\n"      , kv_verify(ht, 0)) ; 
  }
  TxShutdown() ; 

  // consider: enable MSA? 
  struct rusage ru ; 
  getrusage (RUSAGE_SELF, &ru) ;
  printf ("User=%ld System=%ld msecs\n", 
    ((ru.ru_utime.tv_sec * 1000000L) + ru.ru_utime.tv_usec)/1000L, 
    ((ru.ru_stime.tv_sec * 1000000L) + ru.ru_stime.tv_usec)/1000L) ; 

  // Check for possible interference from other processes competing for CPU time.
  // This could indicate the results are invalid and should be discarded.  
  // ggWe expect the system to be otherwise idle.
  // And in general we have a simplistic execution model where threads
  // are expected to be CPU-bound during their measurement interval.
  // TODO: snapshot rusage compute time at the point when we release
  // the threads and again at rendezvous-time and use the difference.  
  // Beware that we can see false+ warnings for the Mutex tests where threads actually block.
  int MaxParallelism = sysconf (_SC_NPROCESSORS_ONLN) ;
  if (_nThreads < MaxParallelism) MaxParallelism = _nThreads ;    // ideal speedup
  // Elapsed wall-clock time (duration)
  const int IntervalMsecs = (int)((end_time - start_time) / 1000000LL) ;                          
  // Aggregate compute time
  const int UserTimeMsecs = ((ru.ru_utime.tv_sec * 1000000LL) + ru.ru_utime.tv_usec)/1000LL ;     
  // Compare expected vs actual compute time
  if ((MaxParallelism * IntervalMsecs) > (UserTimeMsecs + 20)) {
    printf ("WARNING: MaxParallelism %d * Interval %d > UserTime %d -- possible invalid run\n",
      MaxParallelism, IntervalMsecs, UserTimeMsecs) ; 
  }
  
  unsigned long total_non_abortable = 0;
  unsigned long total_non_abortable_readers = 0;
  unsigned long total_combiners = 0;
  for (i = 0; i < _nThreads; i++) {
    total_non_abortable += thread_data[i].nNonAbortable;
	total_non_abortable_readers += thread_data[i].nNonAbortableReaders;
	total_combiners += thread_data[i].Self->n_combiners;
	printf("thread[%d][%llX] iter_num = %d\n", i, thread_data[i].Self, thread_data[i].Self->iter_num);
  }

  unsigned long total_reads_additional = 0;
  unsigned long total_writes_additional = 0;
  unsigned long total_na_writes_additional = 0;
  unsigned long total_num_of_waiting = 0;
  for (i = 0; i < _nThreads; i++) {
	
	printf("thread[%d] combiner_iter_num = %d, num_of_waiting=%u\n", i, thread_data[i].Self->combiner_iter_num, thread_data[i].Self->num_of_waiting);
	total_num_of_waiting += thread_data[i].Self->num_of_waiting;
	
	int j;
	for (j = 0; j < arg_shadow_thread_max_num; j++)
	{
		printf("  shadow_thread[%d] reads_num = %d writes_num = %d na_writes_num = %d\n", j, thread_data[i].Self->inner_reads_num[j], thread_data[i].Self->inner_writes_num[j], thread_data[i].Self->inner_na_writes_num[j]);
		total_reads_additional += thread_data[i].Self->inner_reads_num[j];
		total_writes_additional += thread_data[i].Self->inner_writes_num[j];
		total_na_writes_additional += thread_data[i].Self->inner_na_writes_num[j];
	}
  }
	
  printf("Total num of waiting: %u \n", total_num_of_waiting);
  printf("Total non abortable: %u na_lvl_1 = %u na_shadow = %u\n", total_non_abortable + total_na_writes_additional, total_non_abortable, total_na_writes_additional);
  printf("Total non abortable readers: %u \n", total_non_abortable_readers);
  printf("Total combiners: %u\n", total_combiners);
  printf("Total reads additional: %u\n", total_reads_additional);
  printf("Total writes additional: %u\n", total_writes_additional);
  printf("Total operations: %u\n", nOps + total_reads_additional + total_writes_additional);
  
#ifdef COUNTER  
  unsigned long long total_read_time = 0;
  unsigned long long total_write_time = 0;
  unsigned long long total_start_time = 0;
  unsigned long long total_commit_time = 0;
  unsigned long long total_time = 0;
  for (i = 0; i < _nThreads; i++) {
	total_read_time += thread_data[i].Self->read_total;
	total_write_time += thread_data[i].Self->write_total;
	total_start_time += thread_data[i].Self->start_total;
	total_commit_time += thread_data[i].Self->commit_total;
	
  }
  total_time = total_read_time + total_write_time + total_start_time + total_commit_time;
  printf("Statistics:\n");
/*
  printf("Total read time  : %.2f %llu\n", ((double)total_read_time / (double)total_time) * 100, total_read_time);
  printf("Total write time : %.2f %llu\n", ((double)total_write_time / (double)total_time) * 100, total_write_time);
  printf("Total start time : %.2f %llu\n", ((double)total_start_time / (double)total_time) * 100, total_start_time);
  printf("Total commit time: %.2f %llu\n", ((double)total_commit_time / (double)total_time) * 100, total_commit_time);
  printf("Total time       : %.2f %llu\n", ((double)total_time / (double)total_time) * 100, total_time);
  */
  printf("%.2f %llu\n", ((double)total_read_time / (double)total_time) * 100, total_read_time);
  printf("%.2f %llu\n", ((double)total_write_time / (double)total_time) * 100, total_write_time);
  printf("%.2f %llu\n", ((double)total_start_time / (double)total_time) * 100, total_start_time);
  printf("%.2f %llu\n", ((double)total_commit_time / (double)total_time) * 100, total_commit_time);
  printf("%.2f %llu\n", ((double)total_time / (double)total_time) * 100, total_time);
  
#endif

#ifdef PAPI
  long long total_L1_miss = 0;
  for (i = 0; i < _nThreads; i++) {
    total_L1_miss += g_values[i][0];
	//printf("[Thread %d] L1_DCM: %lld\n", i, g_values[i][0]);
    //printf("[Thread %d] L2_DCM: %lld\n", i, g_values[i][1]);
  }
  printf("L1_DCM: %lld\n", total_L1_miss);
#endif 
  
  return 0;
}

