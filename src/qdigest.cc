// qdigest alg
// uses a static array of sufficient size
// pointers within the array to left, right child
// implemented slow insert procedure: start at root
// alternate fast insert procedure: hash map -- not implemented

#include "qdigest.h"

#define QDBFFLAG 1
#define QDWTFLAG 2

/**************************************************************/
// Node manipulation routines

QD_node * QD_CleanNode(QD_node * qd){
	qd->count=(QDWeight_t) 0;  // works whether things are int or float
	qd->wt=(QDWeight_t) 0;
	qd->kids[0]=NULL;
	qd->kids[1]=NULL;
	return qd;
	// return pointer to the clean node
}

void QD_RemoveNode(QD_admin * qda, QD_node * qd){
	*qda->freept=(*qda->freept)-1;
	if (*qda->freept<0) {
		fprintf(stderr, "Error: freed too many nodes!\n");
		fprintf(stderr, "qda->size %d qda->slack %d\n",qda->size, qda->slack);
		exit(13);
	}
	qda->freelist[*qda->freept]=qd;
	qda->qdsize--;
}

QD_node * QD_GetNode(QD_admin * qda){
	QD_node * newnode;
	newnode=qda->freelist[*qda->freept];
	*qda->freept=(*qda->freept)+1;
	if (*qda->freept>qda->size) {
		printf(" freept = %d \n",*qda->freept);
		fprintf(stderr,
			"Error: could not get node (using all %d).  ",qda->size);
		fprintf(stderr, "Try increasing QDSCALE (currently %d).\n", QDSCALE);
		exit(12); // quit gracefully if overflow detected
	}
	qda->qdsize++; // update node count
	return newnode;
}

QD_node *QD_CreateNode(QD_admin * qda, QD_node *parent, int child) {
	// take node from freelist and return it (if available), slot it into tree
	QD_node *newnode;

	newnode=QD_CleanNode(QD_GetNode(qda)); // get an empty node off the list
	parent->kids[child]=newnode;
	// set default values, and link it into tree.
	return newnode;
}

QD_node * QD_Buffer(QD_type * qd, size_t item, QDWeight_t wt){
	// insert an item into a buffer stored as a linked list
	QD_node * npt;

	npt=QD_CleanNode(QD_GetNode(qd->a));
	npt->kids[0]= (QD_node *) item; // stash itemname in first kid
	// some abuse of types here
	npt->kids[1]= qd->a->bufhead; // and a next pointer in 2nd kid
	npt->wt=wt;
	qd->a->bufhead=npt;
	qd->a->n+=wt;
	return npt;
}

QD_node * QD_UnBuffer(QD_type * qd){
	// remove an item from the buffer
	QD_node *oldhead;

	oldhead=qd->a->bufhead;
	if (oldhead) { // NULL terminates the list
		qd->a->bufhead=oldhead->kids[1];
		qd->a->n-=oldhead->wt;
		QD_RemoveNode(qd->a,oldhead); // restore to list of free nodes
	}
	return oldhead;
}

/***************************************************************/
// Initialization routines

void QD_ListShare(QD_type * src, QD_type * dest){
	// share list of free nodes between multiple qdigests
	dest->q=src->q; // point to the same set of nodes
	dest->a->size=src->a->size; // all have the same overall max size
	dest->a->freept=src->a->freept; // share a pointer to an index
	dest->a->freelist=src->a->freelist; // share a list
}

void QD_FreeListInit(QD_type * qd, int dim){
	// initialize a list of free nodes to avoid doing memory allocation online
	int i;
	QD_admin * qda;
	QD_node* q1 = NULL;
	QD2_node* q2 = NULL;

	qda=qd->a;
	if (qda->size>0){
		if (dim==1) {
			q1=(QD_node *) calloc(1+qda->size,sizeof(QD_node));
			qd->q=q1;
			qda->freelist=(QD_node **) calloc(1+qda->size,sizeof(QD_node*));
		} else {
			q2=(QD2_node *) calloc(1+qda->size,sizeof(QD2_node));
			qd->q=(QD_node*) q2;
			qda->freelist=(QD_node **) calloc(1+qda->size,sizeof(QD2_node*));
		}

		if (qd->q && qda->freelist){ // if the allocations worked
			for (i=0; i<qda->size; i++)
				if (dim==1) {
					qda->freelist[i]=(QD_node*) &q1[i];
				} else qda->freelist[i]=(QD_node*) &q2[i];
			qda->freelist[qda->size]=NULL; // put a null pointer at end of list
			qda->freep=0; // integer index into freelist
			qda->freept=&qda->freep; // use a pointer to allow sharing
		} else {
			fprintf(stderr,"Out of memory error allocating %d items\n", qda->size);
			exit (1);
		}
	}
}

void QD_DefaultVals(QD_admin * qda){
	// set a bunch of values to default values (zero)
	// to be used to reset a qdigest for reuse
	qda->n=(QDWeight_t) 0;
	qda->ctime=(QDTime_t) 0;
	qda->thresh=(QDWeight_t) 0;
	qda->maxn=(QDWeight_t) 0;
	// and reset everything...
	qda->lambda=0.0;
	qda->qdsize=0; // number of nodes used
	qda->_new=qda->slack+4; // when to compress
	qda->qhead=NULL;  // pointer to first node in tree
	qda->bufhead=NULL; // pointer to first node in buffer
	qda->flags=QDBFFLAG; // indicate that we should be buffering
}

QD_admin * QD_InitAdmin(double eps, int logu) {
	// intialize the bookkeeping data, for a given epsilon/log u pair

	QD_admin* qda=(QD_admin *) calloc(1,sizeof(QD_admin));
	qda->logu=logu;
	qda->eps = eps;
	qda->slack =(QDWeight_t) (1.0 + qda->logu*(1.0/qda->eps));
	// compute the slack, which determines when to compress etc.
	qda->size = 20 + QDSCALE*qda->slack;

	QD_DefaultVals(qda);
	return qda;
}

QD_type * QD_Init(double eps, int logu, int freel) {
	// Initialize the data structure

	QD_type* qd=(QD_type *) calloc(1,sizeof(QD_type));

	qd->a=QD_InitAdmin(eps,logu);
	if (freel>=0) 
		qd->a->size=freel; // overwrite the computed size if instructured
	QD_FreeListInit(qd,1);  // also sets all nodes to zero.
	return qd;
}

void QD_Destroy(QD_type * qd){
	// remove freelist, and self

	if (qd) {
		if (qd->a->freelist){
			free(qd->a->freelist);
			qd->a->freelist=NULL;
		}
		if (qd->q){
			free(qd->q);
			qd->q=NULL;
		}
		free(qd->a);
		free (qd);
	}
}

/*************************************/
// Return various useful statistics

int QD_Slack(QD_admin * qda){
	// return the amount of spare space when sharing a buffer
	int i;
	i=qda->size-*qda->freept;
	return i;
}

QDWeight_t QD_ComputeWeights(QD_node * qdq) {
	// recursively compute the weight of the subtree at the current point
	// and store in the wt portion of the node
	int i;
	QDWeight_t wt;

	if (!qdq) return (QDWeight_t) 0;
	wt=qdq->count;
	for (i=0;i<=1;i++)
		wt+=QD_ComputeWeights(qdq->kids[i]);
	qdq->wt=wt;
	return wt;
}

int QD_ComputeHeight(QD_node *qdq) {
	// debugging routine: compute the height of the subtree at current point
	int h=1, i;

	if (!qdq) return 0;
	for (i=0; i<=1; i++)
		h=(std::max)(h,1+QD_ComputeHeight(qdq->kids[i]));
	return h;
}

int QD_Size(QD_type * qd) {  // output size in bytes
	return (sizeof(QD_type) + qd->a->qdsize*(sizeof(QD_node*) + sizeof(QD_node))
		+ sizeof(QD_admin)); // compute size of dynamic structure
}

int QD_Nodes(QD_type * qd){ // return the number of nodes stored
	return (qd)?qd->a->qdsize:0;
}

/*************************************/
// Node management -- compression and insertion 

QD_node * QD_KillKids(QD_admin * qda, QD_node * q) {
	// recursively delete all children of the indicated node
	// assumes the node exists.
	// if called with point =0, will annihilate entire tree
	int i;

	for (i=0; i<=1; i++)
		if (q->kids[i]>0)
			q->kids[i]=QD_KillKids(qda,q->kids[i]);
	QD_RemoveNode(qda,q);
	q->count=0;
	// return the node to the list of free nodes, and update bookkeeping
	return NULL;
}

QD_node *QD_CompressTree(QD_admin*qda, QD_node*q, QDWeight_t debt, int depth){
	// recursively compress the tree
	// based on ideas from CKMS06...

	QD_node *left, *rght;
	QDWeight_t wl;
	QDWeight_t thresh;

	thresh=qda->thresh;
	if (depth==0)
		// stop when we reach the leaves
		q->count-=debt;
	else{
		left=q->kids[0];
		rght=q->kids[1];
		if (q->wt-debt>thresh) {
			// if the adjusted weight of the subtree is large, update and recurse
			debt=debt+thresh-q->count;
			q->count=thresh;
			wl=(left)?left->wt:0;
			q->kids[0]=(left)? QD_CompressTree(qda,left,(std::min)(debt,wl),depth-1):NULL;
			q->kids[1]=(rght)? QD_CompressTree(qda,rght,(std::max)(debt-wl,0),depth-1):NULL;
		}
		else
		{	// can update current node and remove subtree
			q->count=q->wt-debt;
			if (q->count==0) {
				return QD_KillKids(qda,q);
				// return NULL pointer if node itself gets deleted
			} else {
				if (left) q->kids[0]=QD_KillKids(qda,left);
				if (rght) q->kids[1]=QD_KillKids(qda,rght);
			}
		}
	}
	return q; // else return pointer to self
}

void QD_Compress(QD_type * qd) {
	// compress!

	QD_admin * qda = qd->a;
	// don't attempt to compress if not enough nodes...
	qda->thresh=qda->n/qda->slack;
	QD_ComputeWeights(qda->qhead);
	// make sure the weights are up to date
	if (qda->flags & QDWTFLAG)
		qda->flags-=QDWTFLAG; // set flag that it is clean
	if (qda->qhead)
		QD_CompressTree(qda,qda->qhead,0,qda->logu);
}

/*************************************/

void QD_InsertR(QD_admin * qda, size_t item, QDWeight_t weight) {
	// recursively insert a node
	// will create dummy nodes if thresh==0 until it reaches leaves
	// assumes that the root node is there, qdhead is allowed to be zero
	QD_node * point;
	unsigned int thresh, mask, b, i;

	qda->n+=weight;
	point=qda->qhead;
	if (!point) {
		qda->qhead=QD_CleanNode(QD_GetNode(qda));
		point=qda->qhead;
	}
	mask=1<<(qda->logu-1);
	thresh=qda->thresh;
	for(i=qda->logu; i>=0; i--) {
		if (point->count<thresh || i==0) { 
			// if there is room at current node, or have reached leaf, insert
			point->count+=weight;
			break;
		} else { // use mask to probe bit value
			b=((item&mask)==0)?0:1;
			if (!point->kids[b]) { // create child if not already there
				QD_CreateNode(qda,point,b);
			}
			mask>>=1;
			point=point->kids[b];
			// recurse into appropriate child
		}
	}
}

void QD_MergeBuf(QD_type * fqd, QD_type * qd) {
	// merge when qd is buffering:
	//  use insert routine to insert buffered items into fqd
	QD_node *pt;
	while (qd->a->bufhead) {
		pt=QD_UnBuffer(qd);
		QD_Insert(fqd,(size_t) pt->kids[0],pt->wt);
		// retrieve stuffed data from buffer
	}
}

void QD_ConvertFromBuffer(QD_type * qd) {
	// if we have been buffering, convert from buffer to qd

	if (qd->a->flags&QDBFFLAG) { // check that it is buffering
		qd->a->thresh=qd->a->n/qd->a->slack;
		qd->a->flags-=QDBFFLAG; // indicate no longer buffering
		QD_MergeBuf(qd,qd);
	}
}

void QD_Insert(QD_type * qd, size_t item, QDWeight_t wt) {
	// try to insert at node
	// if count >= thresh
	// got to appropriate child
	// if child does not exist, create,
	// until reach root
	QD_admin * qda;

	qda=qd->a;
	qda->_new--;
	if (qda->_new==0) {
		// if it is time to update the threshold
		if (qda->bufhead)  // if we are buffering
			QD_ConvertFromBuffer(qd);
		else
			QD_Compress(qd);
		qda->_new=qda->slack;
	}
	if (qda->flags & QDBFFLAG) // insert into buffer if flags is set
		QD_Buffer(qd,item,wt);
	else {
		QD_InsertR(qda,item,wt);
		qda->flags|=QDWTFLAG; // indicate we have touched the structure
		if (qda->qdsize>qda->size-100) // hard code constant 100
			QD_Compress(qd);
		// if data structure is getting dangerously full, compress
	}
}

void QD_Reset(QD_admin * qda) {
	// reset a qdidgest: remove all children, set root to zero, reset values
	if (qda->qhead)
		qda->qhead=QD_KillKids(qda,qda->qhead);
	QD_DefaultVals(qda);
}

void QD_Copy(QD_type * fqd, QD_node *fpt, QD_node*pt, int c, int depth){
	// if nodes exist in qd but not in fqd, copy them
	// create new node, link to parent
	QD_node *newpt;
	int i;

	newpt=QD_CreateNode(fqd->a,fpt,c);
	newpt->count+=pt->count;
	fqd->a->n+=pt->count;
	for (i=0; i<=1; i++)
		if (pt->kids[i])
			QD_Copy(fqd, newpt, pt->kids[i], i, depth-1);
	// call recursively on children
}

void QD_MergeR(QD_type * fqd, QD_node *fpt, QD_node *pt, int depth) {
	int i;
	// recursively merge two q-digests together
	fpt->count+=pt->count;
	fqd->a->n+=pt->count;
	// sum the counts of the node
	for (i=0;i<=1;i++)
		if (pt->kids[i]) {
			// recurse on qd: if child is present, merge, else copy over
			if (fpt->kids[i])
				QD_MergeR(fqd, fpt->kids[i],pt->kids[i], depth-1);
			else
				QD_Copy(fqd, fpt, pt->kids[i], i, depth-1);
		}
}

void QD_Swap(QD_type * fqd, QD_type * qd) {
	// swap over two data structure
	//   (which is not too hard, since they are pointer based)
	QD_type tmp;

	tmp.a=fqd->a;
	tmp.q=fqd->q;
	fqd->a=qd->a;
	fqd->q=qd->q;
	qd->a=tmp.a;
	qd->q=tmp.q;
}

void QD_SetThresh(QD_admin * qda) {
	// after a merge, recompute the threshold based on the current count n
	qda->thresh=qda->n/qda->slack;
	qda->_new=qda->slack-(qda->n - (qda->thresh*qda->slack));
}

void QD_Merge(QD_type * fqd, QD_type * qd) {
	// logic: if qd is buffering, insert whole buffer into fqd
	// if fqd is buffering, and qd is not, do reverse and then copy

	QD_Compress(fqd);
	QD_Compress(qd);

	if (qd->a->bufhead) // if buffering first
		QD_MergeBuf(fqd,qd);
	else
		if (fqd->a->bufhead) { // if buffering second
			QD_MergeBuf(qd,fqd);
			QD_Swap(qd,fqd);
		} else {
			QD_MergeR(fqd, fqd->a->qhead, qd->a->qhead, qd->a->logu);
		}
		QD_Reset(qd->a);
		QD_SetThresh(fqd->a);
		QD_Compress(fqd);
}

/*************************************/
/*         Debugging                 */
/*************************************/

void QD_Show(QD_type *qd, unsigned int id, QD_node * point, int depth) {
	// debugging routine to show contents of data structure
	int i;
	for (i=0; i<depth; i++)
		printf(" ");
	printf("%d [%d--%d] ct = %f \n",(size_t) point,id,depth, (double) point->count);
	for (i=0;i<=1; i++)
		if (point->kids[i])
			QD_Show(qd,(id<<1)+i,point->kids[i],depth-1);
}

void QD_PrintHH(QD_type *qd, int id, QD_node *point, int depth, int thresh, std::map<uint32_t, uint32_t>& res)
{
	// debugging routine, to use qdigest to search for heavy hitters
	int i;
	if (depth==0 && point->count + qd->a->thresh*qd->a->logu > thresh)
	{
		res.insert(std::pair<uint32_t, uint32_t>(id, point->count + qd->a->thresh*qd->a->logu));
	}
	else
	{
		for (i=0;i<=1; ++i)
			if (point->kids[i])
				QD_PrintHH(qd,(id<<1)+i,point->kids[i],depth-1, thresh, res);
	}
}

std::map<uint32_t, uint32_t> QD_FindHH(QD_type * qd, int thresh)
{
	// output a list of heavy hitters (leaves) found in the data structure
	std::map<uint32_t, uint32_t> res;

	QD_PrintHH(qd,0,qd->a->qhead,qd->a->logu,thresh,res);
	return res;
}

QDWeight_t QD_LBoundRank(QD_type * qd, size_t item)
{
	// return a lower bound on the rank of item
	unsigned int depth, mask, b;
	QD_node * point;
	QDWeight_t lwt, twt;
	twt=0;
	point=qd->a->qhead;
	mask=1<<(qd->a->logu-1);
	for (depth=qd->a->logu-1; depth>0; depth--) {
		b=((item&mask)==0)?0:1;
		if (b!=0 && point->kids[0])
			lwt=point->kids[0]->wt;
		else lwt=0;
		twt+=lwt;
		mask>>=1;
		if (!point->kids[b])
			return twt;
		point=point->kids[b];
	}
	return twt; // control should never reach here
}

QDWeight_t QD_UBoundRank(QD_type *qd, int item){
	// return an upper bound on the rank of item
	QD_node * point;
	unsigned int depth,  mask, b;
	QDWeight_t lwt, twt;

	twt=0;
	point=qd->a->qhead;
	mask=1<<(qd->a->logu-1);
	for (depth=qd->a->logu-1; depth>0; depth--) {
		b=((item&mask)==0)?0:1;
		if (b!=1 && point->kids[1])
			lwt=point->kids[1]->wt;
		else lwt=0;
		twt+=lwt;
		mask>>=1;
		if (!point->kids[b])
			return qd->a->n-twt;
		point=point->kids[b];
	}
	return twt;  // control should never reach here
}

unsigned int QD_OutputQuantile(QD_type * qd, double phi) {
	// returns the phi quantile
	QDWeight_t thresh, lwt;
	unsigned int depth=0;
	QD_node * point;
	unsigned int id;

	QD_ConvertFromBuffer(qd);  // if we are buffering
	if (qd->a->flags&QDWTFLAG) { // if we have recently touched the structure
		QD_ComputeWeights(qd->a->qhead); // if the weights are not up to date.
		qd->a->flags-=QDWTFLAG;
	}
	id=0;
	point=qd->a->qhead;
	thresh=(QDWeight_t) (phi*qd->a->n);
	// compute the weight we are looking for
	for (depth=qd->a->logu-1; depth>0; depth--) {
		if (point->kids[0]) // compute how much weight resides in the left subtree
			lwt=point->kids[0]->wt;
		else lwt=0;

		if (lwt<=thresh) { // recurse right, and remove the left subtree's weight
			id+=((unsigned int) 1) <<depth;
			thresh-=lwt;
			point=point->kids[1];
			if (!point)
				break;
		} else // recurse left
			point=point->kids[0];
	}
	return id;
}

QDWeight_t QD_OutputWeight(QD_type * qd, int item) {
	// estimate the weight of a given item
	QD_node * point;
	unsigned int mask, depth, b;

	point=qd->a->qhead;
	mask=1<<(qd->a->logu-1);
	for (depth=qd->a->logu-1; depth>=0; depth--) {
		b=((item&mask)==0)?0:1;
		if (!point->kids[b])
			return 0;
		else point=point->kids[b];
		mask>>=1;
	}
	return point->count; // should underestimate
}

///////////////////////////////////////////////////////////////////
// Routines for exponential decay combined with qdigest

void QD_SetTime(QD_node* qd, QDTime_t t) {
	// set time at every node to t
	if (qd) {
		qd->wt=t;
		QD_SetTime(qd->kids[0],t);
		QD_SetTime(qd->kids[1],t);
	}
}

double QD_DecayWeight(QD_node * pt, QDTime_t ctime, double lambda) {
	// compute the decayed weight of a node, and store time in wt

	if (pt->wt<ctime && lambda>0)
	{ // for lambda <= 0, do not do decay.  also, do not decay if
		// the point is from the future (this shouldn't happen)
		pt->count=pt->count*exp(lambda*(pt->wt-ctime));
		// do the decay
		pt->wt=ctime;
	}
	return pt->count;
}

double QD_ComputeDecayedWeights(QD_node * fqdq, QDTime_t ctime, double lambda){
	double wt;
	int i;
	// recursively compute the weight of a subtree

	wt=QD_DecayWeight(fqdq,ctime,lambda);
	// first, decay the weight of the current node
	for (i=0;i<=1;i++)
		wt+=(fqdq->kids[i])?
		QD_ComputeDecayedWeights(fqdq->kids[i],ctime,lambda):0;
	fqdq->wt=wt;   // overwrite the current time stamp with the weight
	return wt;
}

void QD_CompressDecay(QD_type * qd) {
	// compress: compute the current (decayed) weights
	QD_admin * qda = qd->a;

	qda->n=QD_ComputeDecayedWeights(qda->qhead,qda->ctime,qda->lambda);
	QD_SetThresh(qda);  // compute the current, correct threshold
	QD_CompressTree(qda,qda->qhead,0,qda->logu);
	QD_SetTime(qda->qhead, qda->ctime); // mark all nodes as up to date
}

void QD_InsertDecayedR(QD_admin * qda, size_t item, double decwt) {
	unsigned int mask, b;
	double thresh, ctime, lambda, lwt;
	int i;
	QD_node * pt;

	qda->n+=decwt;
	pt=qda->qhead; 
	if (!pt) {
		qda->qhead=QD_CleanNode(QD_GetNode(qda));
		pt=qda->qhead;
	}

	mask=1<<(qda->logu-1);
	ctime=qda->ctime;
	lambda=qda->lambda;
	//QD_SetThresh(qda); // recompute current threshold on every update
	thresh=qda->thresh;
	for(i=qda->logu; i>=0; i--) {
		// decay count to current time
		lwt=QD_DecayWeight(pt,ctime,lambda);
		if (lwt+decwt<thresh || i==0)
		{ // if all of the count is used up, or we hit the leaves
			pt->count+=decwt;
			break;
		} else { // put as much weight as possible at the current node, recurse
			decwt=decwt-(thresh-lwt);
			pt->count=thresh;
			b=((item&mask)==0)?0:1;
			if (pt->kids[b]==0) {
				pt=QD_CreateNode(qda,pt,b);
				pt->wt=qda->ctime; // use w/t to store timestamps
			} else
				pt=pt->kids[b];
			mask>>=1;
		}
	}
}

void QD_InsertDecayed(QD_type * qd, size_t item, double itime) {
	// note: we never buffer in this case

	QDWeight_t decwt;
	QD_admin * qda;

	qda=qd->a;
	if (itime>qda->ctime)
	{ // if newitem is from the future
		// notionally decay everything else by updating the weight
		if (qda->lambda>0) 
			qda->n=qda->n*(exp(qda->lambda*(qda->ctime-itime)));
		qda->ctime=itime;
		// set the current time to be the new (later) time
		decwt=1.0;
	}
	else
		// else, item is from the past, set its decayed weight accordingly
		decwt=exp(qda->lambda*(itime-qda->ctime));
	QD_InsertDecayedR(qda,item, decwt);
	if (qda->qdsize>qda->size-2*qda->logu)
		QD_CompressDecay(qd); // if it is getting full, compress
}

/*******************************************************************/

QD2_type * QD2_Init(double eps, int logu, int logw, int maxn, int eager){
	QD2_type * qd;
	int i, t;

	qd=(QD2_type *) calloc(1,sizeof(QD2_type));
	qd->a=QD_InitAdmin(eps,logw);
	QD_FreeListInit((QD_type *) qd,2); 
	// create a bunch of spare two dimensional qds 
	if (maxn>0) {
		qd->a->maxn=maxn;
		qd->a->_new=-1; // means threshold never changes
		qd->a->thresh=(QDWeight_t) (eps*maxn/(logu));
		if (qd->a->thresh<1) qd->a->thresh=1;
		// avoid a zero threshold
	}
	if (eager==0)
		i=1000+(int) (logu*logw*QDSCALE/(eps*eps));
	else
		i=(int) (logu*logw*logw*QDSCALE/(eps));
	i=(std::min)(i,(QDSCALE*maxn*(1 + eager*logu)));

	qd->q[0].qd=QD_Init(eps,logu,i); 
	// set up a root note all ready at zero with i spare nodes
	qd->a->qhead=NULL; 

	// keep a long list of qdigests, each covering a time stamp range
	for (i=1;i<=qd->a->size;i++) {
		qd->q[i].qd=QD_Init(eps,logu,0);
		QD_ListShare(qd->q[0].qd,qd->q[i].qd); // share the nodes among the qds
	}
	if (eager==1) {
		qd->a->eager=1;
		t=(int) maxn*eps/(logu * logw);
		if (t==0) t=1;
		for (i=0;i<=qd->a->size;i++) {
			qd->q[i].qd->a->_new=-1; // ensure that we never change threshold
			qd->q[i].qd->a->thresh=t;
			qd->q[i].qd->a->eager=1;
		}
	}
	return qd;
}

void QD2_Destroy(QD2_type * qd){
	int i;

	// kill the shared secondary qd nodes
	QD_Destroy(qd->q[0].qd);
	// remove the other secondary qds
	for (i=1; i<=qd->a->size;i++){
		free(qd->q[i].qd);
	}
	// remove the primary qd nodes
	// remove the primary qd
	QD_Destroy((QD_type *) qd);
}

QD2_node* QD2_KillKids(QD2_type * qd, QD_type * dest, QD2_node *point) {
	// recursively delete all children of the indicated node
	// assumes the node exists.
	// if called with point =0, will annihilate entire tree
	int i;

	for (i=0; i<=1; i++)
		if (point->kids[i])
			point->kids[i]=QD2_KillKids(qd,dest,point->kids[i]);
	if (qd->a->eager==0) {
		QD_Merge(dest,point->qd);
		// merge together the nodes into one -- if we are defer merging
	} else 
		QD_Reset(point->qd->a); // free up the nodes, but leave it as a valid qd
	point->count=0;
	QD_RemoveNode(qd->a,(QD_node *) point); // mess around with pointers
	// return the node to the list of free nodes, and update bookkeeping
	return NULL;
}

/*******************************************************************/
// Various counting routines

int QD2_NodesR(QD2_node * point){
	int wt=0, i;

	if (point) {
		wt=QD_Nodes(point->qd); // count how many nodes we have here
		for (i=0;i<=1;i++) // plus recursively down
			wt+=QD2_NodesR(point->kids[i]);
		return wt;
	} else return 0;
}

int QD2_Nodes(QD2_type * qd){
	int d;
	d=QD2_NodesR((QD2_node *) qd->a->qhead);
	return (qd->a->qdsize+d);
}

int QD2_Size(QD2_type * qd) {
	int i;
	// output size in bytes
	i=sizeof(QD2_type) + sizeof(QD_admin);
	// start with size of admin data

	if (qd->a->thresh>0)  {
		i+=qd->a->qdsize*(sizeof(int) + sizeof(QD2_node));
		// if we are not buffering, compute number of nodes times node+overhead
		i+=QD2_NodesR((QD2_node *) qd->a->qhead)*sizeof(QD_node);
		// need to get size of each node in turn, with a walk over tree.
	} else
		i+=qd->a->n*2*sizeof(unsigned int);
	// if we are buffering, count how much of the buffer is used.
	return i;
}

/*******************************************************************/

QD2_node *QD2_CompressTree(QD2_type * qd, QD2_node *point, QD2_node * par, int debt, int depth) {
	// recursively compress the tree
	// based on ideas from CKMS06...

	int wl, thresh;
	QD2_node * qdq, *left, *right;

	qdq=qd->q;
	thresh=qd->a->thresh;
	if (depth==0)
		// stop when we reach the leaves
		point->count-=debt;
	else{
		left=point->kids[0];
		right=point->kids[1];
		if (point->weight-debt>thresh) {
			// if the adjusted weight of the subtree is large, update and recurse
			debt=debt+thresh-point->count;
			point->count=thresh;
			wl=(left)?left->weight:0;
			point->kids[0] = (left) ?
				QD2_CompressTree(qd,left,point,(std::min)(debt,wl),depth-1):0;
			point->kids[1] = (right) ?
				QD2_CompressTree(qd,right,point,(std::max)(debt-wl,0),depth-1):0;
		} else{
			// can update current node and remove subtree
			point->count=point->weight-debt;
			if (point->count==0)
			{
				if (par)
					QD2_KillKids(qd,par->qd,point);	// put qdigest into parent
				return 0; // return 0 if node itself gets deleted
			} else {
				if (left) point->kids[0]=QD2_KillKids(qd,point->qd,left);
				if (right) point->kids[1]=QD2_KillKids(qd,point->qd,right);
			}
		}
		//    if (qd->a->eager==1) ;// in the eager merge case, do something
		QD_Compress(point->qd);
		// compress the qdigest on the second dimension
	}
	return point; // else return pointer to self
}

int QD2_PruneR(QD2_type * qd, QD2_node * point, int countin, int N){
	// prune away nodes corresponding to things older than N
	QD2_node * left, *right;
	left=point->kids[0];
	right=point->kids[1];
	if (right)
		countin=QD2_PruneR(qd,right,countin,N);
	if (left){
		if (countin>N) {
			qd->a->n-=left->weight;
			// assumes weights are up to date, reduce n
			point->kids[0]=QD2_KillKids(qd,point->qd,left);
		} else
			countin=QD2_PruneR(qd,left,countin,N);
	}
	countin+=point->count;
	return countin;
	// make sure to keep n up to date
}

void QD2_Compress(QD2_type * qd) {
	// compress! and also prune nodes with rank greater than maxn
	// set N to zero to not do this
	if (qd->a->n>qd->a->slack) {
		// don't attempt to compress if not enough nodes...
		QD_ComputeWeights(qd->a->qhead); // make sure the weights are up to date
		if (qd->a->maxn>0 && qd->a->n>qd->a->maxn)// if there is any scope to prune
			QD2_PruneR(qd, (QD2_node *) qd->a->qhead, 0, qd->a->maxn);
		QD2_CompressTree(qd, (QD2_node *) qd->a->qhead,NULL, 0,qd->a->logu);
	}
}

QD2_node * QD2_Buffer(QD2_type * qd, unsigned int xitem, unsigned int yitem){
	QD2_node *npt;

	npt=(QD2_node *) QD_GetNode(qd->a);
	npt->weight=xitem;
	npt->count=yitem;
	npt->kids[1]=(QD2_node *) qd->a->bufhead;
	npt->kids[0]=NULL;  // mark as a buffer node...
	qd->a->bufhead=(QD_node *) npt;
	return npt;
}

QD2_node * QD2_UnBuffer(QD2_type * qd, int * rval){

	QD2_node * oldhead;
	oldhead=(QD2_node *) qd->a->bufhead;
	if (oldhead==NULL) return NULL;
	rval[0]=oldhead->weight;
	rval[1]=oldhead->count;
	qd->a->bufhead=(QD_node *) oldhead->kids[1];
	QD_RemoveNode(qd->a,(QD_node *) oldhead);
	return oldhead;
}

void QD2_InsertR(QD2_type * qd, unsigned int xitem, unsigned int yitem) {
	// recursively insert a node
	// will create dummy nodes if thresh==0 until it reaches leaves
	QD2_node * point; 
	QDWeight_t thresh;
	unsigned int mask, b, i;

	point=(QD2_node *) qd->a->qhead;
	if (!point) {  // need to check if point is null...
		qd->a->qhead=QD_CleanNode(QD_GetNode(qd->a));
		point=(QD2_node *) qd->a->qhead;
	}
	mask=1<<(qd->a->logu-1);
	thresh=qd->a->thresh;
	for(i=qd->a->logu; i>=0; i--) {
		if (point->count<thresh || i==0) { 
			// if there is room at current node, or have reached leaf, insert
			point->count++;
			QD_Insert(point->qd,yitem,1);
			break;
		} else { // use mask to probe bit value
			if (qd->a->eager==1) // in eager merge, also insert here
				QD_Insert(point->qd,yitem,1);
			b=((xitem&mask)==0)?0:1;
			if (point->kids[b]==0) // create child if not already there
				QD_CreateNode(qd->a,(QD_node *) point,b); // some pointer casting
			mask>>=1;
			point=point->kids[b]; // recurse into appropriate child
		}
	}
}

void QD2_Insert(QD2_type * qd, unsigned int xitem, unsigned int yitem) {
	// insert into the 2D structure
	QD_admin * qda;
	unsigned int getbuf[2];

	qda=qd->a;
	qda->_new--;

	if (qda->_new ==0) { // if it is time to update the threshold
		qda->thresh++;
		qda->_new=qda->slack;
		if (qda->thresh==1)
			// if we have been buffering, convert from buffer to qd
			while (qd->a->bufhead) {
				// extract both x and y items from buffer...
				QD2_UnBuffer(qd,(int*) getbuf);
				QD2_InsertR(qd,getbuf[0],getbuf[1]);
			} else QD2_Compress(qd);
	}
	if (qda->thresh==0) // stow both x and y items in buffer
		QD2_Buffer(qd,xitem,yitem);
	else {
		// need a way for the kids to indicate they are getting full...
		// kids slack = qda->q[0]->freept
		QD2_InsertR(qd,xitem,yitem);
		if (QD_Slack(qda) < qda->logu ||
			QD_Slack(qd->q[0].qd->a)<qd->q[0].qd->a->slack)
			QD2_Compress(qd);
		// if data structure is getting dangerously full, compress
	}
	qda->n++;
}

/////////////////////////////////////////////////////////////////////////

QDSW_type * QDSW_Init(double eps, int logu, int logw, int N, int eager){
	QDSW_type * sw;
	int i,j, k;
	double localeps;
	int buffy;

	sw=(QDSW_type *) calloc(1,sizeof(QDSW_type));
	k=logw/eps;

	sw->bufsize=BUFSIZE;
	sw->buffer=(duo *) calloc(sw->bufsize,sizeof(duo));
	sw->bufpt=sw->bufsize;
	buffy=5*sw->bufsize/6;
	j=0;
	i=N;
	while (i>buffy) {i/=2; j++;}
	sw->n=j;
	sw->qds=(QD2_type **) calloc(j,sizeof(QD2_type *));
	i=N;
	j=0;
	while (i>buffy) {
		localeps=eps*0.5/(1.0 - (double)buffy/(double) i);
		sw->qds[j]=QD2_Init(localeps,logu,logw, i-buffy,eager);
		i/=2;
		j++;
	}
	return sw;
}

void QDSW_Destroy(QDSW_type * sw){
	int i;
	for (i=0;i<sw->n;i++)
		QD2_Destroy(sw->qds[i]);
	free(sw);
}

int dcmp(const void *x, const void *y) {
	const unsigned int * h1= (unsigned int*) x;
	const unsigned int * h2= (unsigned int*) y;
	if ((*h1)>(*h2))
		return 1;
	else if ((*h1)<(*h2))
		return -1;
	else return 0;
}

void QDSW_Insert(QDSW_type * sw, size_t item, unsigned int time){
	int i,j;

	sw->i++;
	sw->bufpt--;
	sw->buffer[sw->bufpt][0]=time;
	sw->buffer[sw->bufpt][1]=item;

	if (sw->bufpt<=0) {
		qsort(sw->buffer,sw->bufsize,sizeof(duo),dcmp);
		//sort buffer on reverse time
		sw->bufpt=sw->bufsize/6;
		for (j=sw->bufpt-1;j>=0;j--){
			// insert last sixth into structures
			for(i=0;i<sw->n;i++)
				QD2_Insert(sw->qds[i],sw->buffer[j][0],sw->buffer[j][1]);
		}
	}
}

void QDSW_Compress(QDSW_type * sw){
	int i;
	for(i=0;i<sw->n;i++)
		QD2_Compress(sw->qds[i]);
}

int QDSW_Nodes(QDSW_type * sw){
	int i, j=0, nodes;

	j=sw->bufsize-sw->bufpt;
	for (i=0; i<sw->n;i++){
		nodes=QD2_Nodes(sw->qds[i]);
		j+=nodes;
	}
	return j;
}

int QDSW_Size(QDSW_type * sw){
	int i,s;
	s=sizeof(QDSW_type)+(sw->bufsize-sw->bufpt)*2*sizeof(unsigned int);
	for (i=0;i<sw->n;i++)
		s+=QD2_Size(sw->qds[i]);
	return s;
}
