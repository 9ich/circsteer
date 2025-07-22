#define           SZ sizeof
#define           PI 3.141592653589793115997963468544185161590576171875
#define          TAU (2.0*PI)
#define       F(i,n) for(TYP(n) i=0;i<(n);++i)
#define          VEC F(k,2)
#define          TYP __typeof__
#define     MIN(x,y) ({TYP(x) x_=(x),y_=(y);x_<y_?x_:y_;})
#define     MAX(x,y) ({TYP(x) x_=(x),y_=(y);x_>y_?x_:y_;})
#define CLAMP(x,l,h) (MAX(MIN((x),(h)),(l)))
#define  SATURATE(x) (CLAMP((x),0.0,1.0))
#define  LERP(a,b,t) ({TYP(t) t_=(t);((1.0-t_)*(a) + t_*(b));})
#define       SGN(x) ({TYP(x) x_=(x);-(x_<0)+(x_>0);})
#define       LEN(a) (SZ(a)/SZ((a)[0]))
#define    SWAP(x,y) ({TYP(x) t_=(x);x=y;y=t_;})
#define        SQ(x) ((x)*(x))
#define   deg2rad(x) ((PI*0.005555555555555556)*(x))
#define   rad2deg(x) ((x)/(0.005555555555555556*PI))

enum{
	S_MAIN=0,S_BINDWAIT,S_BIND,    // states
	ES_NOPAD=1<<0,ES_NOJOY=1<<1,   // error states
	A_STEER=0,A_TRIGL,A_TRIGR,A_N  // axes
};

typedef struct V{
	int     state;
	int     errstate;
	int     bindaxis;
	double  bindfrac[A_N];
	double  bindosc;
	double  wheel;
	double  wlimit;
	double  winput[2];
	int     grabbed;
	double  grabthresh;
	double  out[A_N];
}V;
typedef struct UI{
	void  (*kill)(void);
	int   (*update)(void);
}UI;

extern V  v;

extern void        bindstart(void);
extern void        bindcancel(void);
extern void        cursesinit(UI *ui);
extern void        coninit(UI *ui);
extern const char  *axis2s(int a);
