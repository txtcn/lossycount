#ifndef QDIGEST_h
#define QDIGEST_h

#include "prng.h"

#define QDWeight_t int
#define QDTime_t double

#define QD_FULL 125 // sets the accuracy: 0.01 --> 100 nodes per level
#define QD_LOGU 20 // sets the size of the domain
// may need to fix some data types from int to long if this is to go
// from 32 to 64
//#define QD_SIZE 1+QD_FULL*QD_LOGU*QDSCALE

// if QD_SIZE is defined, will perform static allocation, else will dynamically alloc memory at init time

#define QDSCALE 2 // scaling factor: allocate QDSCALE * slack space to 
// store all nodes.  Increase this if program quits with out of space 
// errors.  Do not decrease below 1, will probably not work if < 2
typedef struct qd_node_t QD_node;

struct qd_node_t {
  QD_node *kids[2];   // point to left and right children of the node
  QDWeight_t count;     // count for the node (integer)
  QDWeight_t wt;    // weight of the subtree routed at current node
                    // or a timestamp of when the node was last updated
                    // in the exponentially decayed case
};

typedef struct QD_admin{
  int qdsize;        // current size of the sketch
  int slack;         // derived from logu and eps (does not change)
  QDWeight_t thresh; // current (int) threshold for pruning
  QDWeight_t n;      // total weight of updates received
  int _new;       // counts down until threshold should be increased
  int logu;      // log of domain size (does not change)
  int maxn;      // for 2D quantiles, a pruning threshold
  int eager;     // for 2D quantiles, indicate eager merging
  int *freept;    // pointer integer index into freelist of pointers to next spare node
  int freep;  // integer index into freelist of pointers...
  int size;          // total size of sketch (does not change)
  int flags;   // store some flags for the structure
  double eps;    // epsilon parameter (does not change)
  QDTime_t ctime;     // timestamp associated with q-digest
  QDTime_t etime;     // timestamp associated with q-digest for value-division
  double lambda; // parameter for exponential decay (does not change) 
  QD_node *qhead;// pointer to first qdnode
  QD_node *bufhead;// pointer to first buffered item
#ifdef QD_SIZE
  QD_node *freelist[QD_SIZE];
#else
  QD_node **freelist;// list of pointers to unused nodes, for space management
#endif
} QD_admin;

typedef struct QD_type{
  QD_admin * a;
#ifdef QD_SIZE
  QD_admin aa;
  QD_node q[QD_SIZE];
#else
  QD_node * q;
#endif
} QD_type;

extern QD_type * QD_Init(double, int, int);  
// Initialize with epsilon, logu and no. of nodes to allocate (-1 for default)
extern void QD_Insert(QD_type *, size_t, QDWeight_t); // Insert item
extern void QD_InsertDecayed(QD_type *, size_t, double);
extern void QD_Compress(QD_type *); // Compress
extern void QD_CompressDecay(QD_type *); 
extern unsigned int QD_OutputQuantile(QD_type *, double);
extern void QD_Destroy(QD_type *); // Destroy
extern std::map<uint32_t, uint32_t> QD_FindHH(QD_type *, int);
// returns a list of heavy hitters above threshold

extern int QD_Size(QD_type *);  // output size of structure (in bytes)
extern int QD_Nodes(QD_type *);  // output size of structure (in nodes)
extern void QD_Merge(QD_type *, QD_type *);

extern void QD_ListShare(QD_type *, QD_type *); 
//extern void QD_Show(QD_type *, unsigned int, QD_node*, int);
// (debugging) show contents of data structure

// destructive merge: from right to left.  Right argument becomes empty

extern int QD_Slack(QD_admin * qda); // debugging routines.

/****************************************************************/

typedef struct qd2_node_t QD2_node;

struct qd2_node_t {
  QD2_node *kids[2];
  QDWeight_t count; 
  QDWeight_t weight;
  QD_type * qd;
};

typedef struct QD2_type{
  QD_admin * a;
  QD2_node * q;
} QD2_type;

extern QD2_type * QD2_Init(double, int, int, int, int );  
// Initialize with epsilon, logu, logw, maxn, eager (0 or 1)
extern void QD2_Insert(QD2_type *, unsigned int, unsigned int);
extern void QD2_Compress(QD2_type *); // Compress
extern int QD2_Size(QD2_type *); // return size in bytes
extern int QD2_Nodes(QD2_type *); // return the number of nodes

/****************************************************************/

typedef int duo [2];

#define BUFSIZE 100000

typedef struct QDSW_type{
  QD2_type ** qds;
  duo * buffer;
  int n, i, bufsize, bufpt;
} QDSW_type;

extern QDSW_type * QDSW_Init(double, int, int, int, int);
// initialize the sliding window case with epsilon, log U, log W, 
// a bound on the total number of items that will be stored, 
// and 0 for defer merge, 1 for eager merge

extern void QDSW_Insert(QDSW_type *, unsigned int, unsigned int);
// insert into the data structure an item and time value
extern void QDSW_Compress(QDSW_type *); // compress the data structure
extern int QDSW_Nodes(QDSW_type *); // return data structure size in nodes 
extern int QDSW_Size(QDSW_type *); // return the data structure size in bytes
extern void QDSW_Destroy(QDSW_type *); // destroy 

/***************************************************************/

#endif
