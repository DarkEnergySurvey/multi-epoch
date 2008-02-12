typedef struct {
	unsigned long *icod,*ncod,*left,*right,nch,nodemax;
} huffcode;

void hufdec(ich,code,lcode,nb,hcode)
huffcode *hcode;
unsigned char *code;
unsigned long *ich,*nb,lcode;
{
	long nc,node;
	static unsigned char setbit[8]={0x1,0x2,0x4,0x8,0x10,0x20,0x40,0x80};

	node=hcode->nodemax;
	for (;;) {
		nc=(*nb >> 3);
		if (++nc > lcode) {
			*ich=hcode->nch;
			return;
		}
		node=(code[nc] & setbit[7 & (*nb)++] ?
			hcode->right[node] : hcode->left[node]);
		if (node <= hcode->nch) {
			*ich=node-1;
			return;
		}
	}
}
