#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

#include <semaphore.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>



enum docTip {File,Dir};
enum obrada {DONE, INPROCESS, DIRR,REMOVED,NEEDED, EMBRYO};  // Embriyo = u toku //



typedef struct node_{
	char ime[100];
	int index;	
	int roditelj;
	enum docTip tip;

	time_t vremObr;	
	enum obrada stanje;

	pthread_mutex_t mutex;
	int vrednost;
}node;
void fixAddingSame(node *nod);
char * pripremiArg(char* str);
int contains(char * arg, char * fun);
node * makeaNode(char * ime,int dir);
void izbirsiMe(node *tren);
void add_dir(char *ime,int roditelj);
void add_file(char *ime, int roditelj, struct stat buf);
void printRez();
node *findNode( char *nodeIme);
int generateSize();
void osveziRoditelje(int roditelj, int oduzeti, int minus);
int checkIfExists(node *tren);
void printerino(int index,int depth,int sz);
int decauradu(node tren,int sz);
int primarius(int br);
int ispitatDalJeFile(char *path);

///konobar sluzi za dodavanje u stablo kako bi izbegli probleme pri menjanu size-a
pthread_mutex_t konobar;
node *stablo= NULL;
int size=-1;
sem_t doFiles;

//korisceno za referencu 
//https://stackoverflow.com/questions/30877110/how-to-determine-directory-or-file-in-linux-in-c/30877241

//za dodavanje 
void* worker_thread(void *_args){
while(1){
	sem_wait(&doFiles);
	node *tren;
	int sz=generateSize(),index=-1;
	for(int i=0 ; i< sz ; i++){
		if(stablo[i].tip ==Dir)continue;		
		pthread_mutex_lock(&stablo[i].mutex);
		if(stablo[i].stanje== NEEDED){
			stablo[i].stanje=INPROCESS;
			tren=&stablo[i];
			
			pthread_mutex_unlock(&stablo[i].mutex);	 
			break;		
		}
		
		pthread_mutex_unlock(&stablo[i].mutex);	
	}
	int sum=0,num=0;
	int nflag=0,sflag=0;

	char buf[1000],c;
	FILE *f;
	f=fopen(tren->ime, "r");
	if(!f){
		printf("lose otvoren file %s\n", tren->ime);
		pthread_exit(NULL);
	}
	int vr=-1;
	pthread_mutex_lock(&tren->mutex);
	vr=tren->vrednost;
	tren->vrednost=0;
	pthread_mutex_unlock(&tren->mutex);	
	//printf(" krecemo da  brisemo %d za pocenti index %d \n", vr, tren->index);

	osveziRoditelje(tren->roditelj, vr, 1);
	//printRez();
	memset(buf, '~', 1000);
	while (fread(buf, 1, sizeof buf, f) > 0){
		
		
		for( int i=0 ; i< 1000; i++){
			c=buf[i];
			//printf("car je %c  za %s \n", c, tren->ime);
			if(c == '\0' || c=='~'){
				nflag=0;
				if(primarius(num))				
					sum+=num;
				//printf(" sima : %d\n", sum);
				break;			
			};
			if( c >='0' && c<= '9'){
				nflag=1;
				num=(num*10 +  (c-'0') );	
				//printf("num je sad %d\n", num);		
			}else{
				nflag=0;
				if(primarius(num))				
					sum+=num;
				
				num=0;			
				if(sflag){
					sflag=0;
					pthread_mutex_lock(&tren->mutex);
					tren->vrednost=sum;
					pthread_mutex_unlock(&tren->mutex);				
				}			
			} 
		}
		if(nflag){
			sflag=1;					
			continue;
		}
		
		pthread_mutex_lock(&tren->mutex);
		tren->vrednost=sum;
		pthread_mutex_unlock(&tren->mutex);
		osveziRoditelje(tren->roditelj, sum, 0);
		//sleep(8); //da bi se lakse video parcijalni rezultati koristiti parci folder 		
    }
	pthread_mutex_lock(&tren->mutex);
	if(tren->stanje == REMOVED){
		pthread_mutex_unlock(&tren->mutex);		
		continue;
	}
	//tren->vrednost=sum;
	tren->stanje= DONE;
	pthread_mutex_unlock(&tren->mutex);
	
	
	//printRez();		
	
}
	pthread_exit(NULL);
}


static int fl=1;
void* watcher_thread(void *_args){
	
	node *tren=((node *) _args);
	DIR *d;
	struct dirent *dir;
	int id=tren->index;

	//printf("USLI U WATCHER ZA %d %s \n", tren->index, tren->ime);
  	int dir_len = strlen(tren->ime)-fl;
  	char* path = malloc(dir_len + NAME_MAX + 2); // +2, 1 for '/' and 1 for '\0'
 	
	if(path == NULL) {
    	fprintf(stderr, "malloc failed\n");
    	pthread_exit(NULL);
  	}
  	strcpy(path, tren->ime);
 	if(path[dir_len-fl] != '/') {
  		 path[dir_len] = '/';
   		 dir_len++;
  	}
	fl=0;
	strcpy(tren->ime,path);
	
	
	int oduzeti=0;
	while(1){
		tren=&stablo[id];
		pthread_mutex_lock(&tren->mutex);
		if(tren->stanje==REMOVED){
		pthread_mutex_unlock(&tren->mutex);
			izbirsiMe(tren);
			pthread_exit(NULL);
		}
		pthread_mutex_unlock(&tren->mutex);
		
		d = opendir(tren->ime);
		if(d == NULL){
			//printf("neuspeh u otvranju dir-a %s\n", tren->ime);
			//izbirsiMe(tren);
					
			pthread_exit(NULL);		
		}
		
		while ((dir = readdir(d)) != NULL){
				//printf("ucitan file %s a\n", dir->d_name);
			if (!strcmp (dir->d_name, "."))
        	    		continue;
        		if (!strcmp (dir->d_name, ".."))    
        	   		 continue;			
		
		struct stat buf;

      		strcpy(&path[dir_len], dir->d_name);
		
      		if(stat(path, &buf) < 0) {
        		fprintf(stderr, "error\n");
				break;      		
		}
			//printf("ucitan file %s \n", path);
		
			node *dete;	
			if((dete=findNode(path))==NULL){
				tren=&stablo[id]; // kako ne bi doslo do falticnog tren-a
				if(S_ISDIR(buf.st_mode)){
					add_dir(path, tren->index);
					
				}else{
					add_file(path, tren->index, buf);
					sem_post(&doFiles);
					
				}
				//printRez();		
			}
			
		
		
			if((dete=findNode(path))!=NULL && dete->tip==File && dete->stanje == DONE){
					//dete->mutex
					if(dete->vremObr!=buf.st_mtime){
						pthread_mutex_lock(&dete->mutex);						
						dete->vremObr=buf.st_mtime;
						dete->stanje=NEEDED;
						pthread_mutex_unlock(&dete->mutex);
						//printf("		DETE U PITANJU %d\n", dete->index);
						sem_post(&doFiles);
						//printRez();
						
					}		
							
			}
			 		
		
			
			
   	 	}

    	closedir(d);
		
		tren=&stablo[id]; // potrebo jer kako mislim pri realloc-ku moze da dodje da faltickog pokazivaca tj da pokazuje na nest drugo
		checkIfExists(tren);

				
	sleep(10);
			
	}
	pthread_exit(NULL);

}


void* printer_thread(void *_args){
	int sz= generateSize();	
	node tren;	
	for(int i= 0; i<sz; i++){
		tren=stablo[i];
		pthread_mutex_lock(&tren.mutex);
		if(tren.stanje== REMOVED){
		 	pthread_mutex_unlock(&tren.mutex);
			continue;		
		}
		pthread_mutex_unlock(&tren.mutex);	
		if(tren.roditelj==-1){
			int flag=decauradu(tren,sz);
		
			pthread_mutex_lock(&tren.mutex);
			int dp=0;	
			int duzina=strlen(tren.ime);
			for(int k=0; k <duzina; k++){
				if(tren.ime[k]== ' ')break;			
				printf("%c",tren.ime[k]);	
			}
			printf(": %d",tren.vrednost);
			if(flag)
				printf("*");
			printf("\n");
			
			pthread_mutex_unlock(&tren.mutex);
					
			printerino(tren.index,0, sz);		
		}
	}
		

}

int decauradu(node tren,int sz){

	int index=tren.index;
		
	for(int i=0;i< sz; i++){
		if(stablo[i].roditelj!=index)continue;

		pthread_mutex_lock(&stablo[i].mutex);
		if(stablo[i].stanje==REMOVED){
			pthread_mutex_unlock(&stablo[i].mutex);
			continue;		
		}
		pthread_mutex_unlock(&stablo[i].mutex);
		//da bi proverili dal neki poddir ima decu koja i dalje rade
		if(stablo[i].tip==Dir){  	
			if(decauradu(stablo[i],sz)){
				return 1;			
			}
		}
		
		pthread_mutex_lock(&stablo[i].mutex);	
		
		if(stablo[i].stanje== INPROCESS || stablo[i].stanje== EMBRYO || stablo[i].stanje== NEEDED){
		
			pthread_mutex_unlock(&stablo[i].mutex);
			return 1;
		}
			
		pthread_mutex_unlock(&stablo[i].mutex);
	}
	
//	printf("nema dece u radu  index %d", index);
	return 0;

}

//ideja: https://stackoverflow.com/questions/230062/whats-the-best-way-to-check-if-a-file-exists-in-c
int checkIfExists(node *tren){
		node *primerak=stablo;
		int sz=generateSize(),oduzeti=-1;
		node *prim;
		for(int i=0; i<sz; i++){
			prim=&stablo[i];
			if(tren->index != prim->roditelj || prim->stanje==REMOVED) continue;
			if( access( prim->ime, R_OK ) !=0 ){
				//printf("usli smo u dete %s \n", prim->ime);
						//ovo treba da osvezi i roditelja ovog dir tako da ovo treba da ide u rekurziju nadodati kasnije
					pthread_mutex_lock(&prim->mutex);
					prim->stanje=REMOVED;
					oduzeti=prim->vrednost;
					pthread_mutex_unlock(&prim->mutex);
					
					pthread_mutex_lock(&tren->mutex);
					tren->vrednost-=oduzeti;
					pthread_mutex_unlock(&tren->mutex);
					osveziRoditelje(tren->roditelj, oduzeti, 1);
					//logika za brisanje fajlova				
					
				
			}
				
		}





	return 1;
}


void osveziRoditelje(int roditelj, int oduzeti, int minus){
	if(roditelj == -1)
		return;
	int rodo=-1;
	int sz=generateSize();
	for(int i= 0 ; i < sz; i++){
		node *tren=&stablo[i];
		//printf(" PSVEZO RPDOTELJE tren %d a rodi %d\n ", tren->index, roditelj);
		pthread_mutex_lock(&tren->mutex);
		if(tren->stanje!=REMOVED && tren->index==roditelj){
				tren->vrednost= minus >= 1 ? tren->vrednost-oduzeti : tren->vrednost + oduzeti;	
				//printf(" OSVEZI RODITELJE vrednost trena %d \n",tren->vrednost);		
			rodo=tren->roditelj;
			pthread_mutex_unlock(&tren->mutex);
			break;
		}
		pthread_mutex_unlock(&tren->mutex);
	}
	osveziRoditelje(rodo,oduzeti, minus);
}


pthread_mutex_t thkonobar;
pthread_t *threads=NULL;
int tSize=1;

void add_dir(char *ime,int roditelj){

	
	char ptr[50];
	strcpy(ptr,ime);
	int len=strlen(ptr);
	ptr[len-1]='/';
	ptr[len]='\0';
	node *nod=findNode(ptr);
	
	if(nod!=NULL){

		pthread_mutex_lock(&nod->mutex);	
		if(nod->stanje==DIRR){
			pthread_mutex_unlock(&nod->mutex);
			printf("vec prisutan u sistemu %s\n", nod->ime);
			return;			
		}
		pthread_mutex_unlock(&nod->mutex);
		fixAddingSame(nod);
		nod->vrednost=0;
		
	}
	
	
	if(nod==NULL)
		nod=makeaNode(ime,1);
	
	
		
	nod->roditelj=roditelj; 
	nod->stanje =DIRR;	

	pthread_mutex_lock(&thkonobar);
	pthread_t *watcher=malloc(sizeof(pthread_t));
	if(threads ==NULL ){
		threads=malloc(sizeof(pthread_t));	
	}else{
		threads=(pthread_t *) realloc(threads,tSize*sizeof(pthread_t));
	}		
	threads[tSize-1]=*watcher;
	tSize++;
		
	pthread_create(&threads[tSize-2], NULL, watcher_thread, (void*)(&stablo[nod->index]));

	pthread_mutex_unlock(&thkonobar);

	
}


void remove_dir(char *ime){
	char ptr[50];
	strcpy(ptr,ime);
	int len=strlen(ptr);
	ptr[len-1]='/';
	ptr[len]='\0';
	node *tren=findNode(ptr);
	int oduzeti=0;	

	if(tren == NULL){
		printf("data putanja jos nije dodata %s\n", ime);
		return;			
	}

	pthread_mutex_lock(&tren->mutex);
	oduzeti=tren->vrednost;
	if(tren->stanje==REMOVED){
		pthread_mutex_unlock(&tren->mutex);
		printf("dir je vec izbacen! dir: %s\n",tren->ime);
		return;	
	}
	pthread_mutex_unlock(&tren->mutex);
	if(tren->tip==File){
		printf("data dadoteka je file samo dir moze : %s\n", tren->ime);
		return;
	}	
	
	pthread_mutex_lock(&tren->mutex);
	tren->stanje=REMOVED;	
	pthread_mutex_unlock(&tren->mutex);
	izbirsiMe(tren);
	if(tren->roditelj!=-1)
		osveziRoditelje(tren->roditelj,oduzeti,1);
	


}
void result(char *path){
	char ptr[50];
	strcpy(ptr,path);
	int len=strlen(ptr);
	ptr[len-1]='/';
	ptr[len]='\0';
	node *tren= ispitatDalJeFile(path) ? findNode(path) : findNode(ptr);
	if(tren==NULL){
		printf("dati path nije lepo prosledjen ako je upitanju dir mugce treba na kraju  /  : %s \n",path);
		return;	
	}
	pthread_mutex_lock(&tren->mutex);
	
	if(tren->stanje==REMOVED){
		pthread_mutex_unlock(&tren->mutex);
		printf("dadoteka je izbacena : %s\n",tren->ime);
		return;	
	}
	pthread_mutex_unlock(&tren->mutex);
	int duzina=strlen(tren->ime);
	if(tren->tip ==File){
		pthread_mutex_lock(&tren->mutex);
		for(int k=0; k <duzina; k++){
		if(tren->ime[k]== ' ')break;			
			printf("%c",tren->ime[k]);	
		}
		printf(": %d",tren->vrednost);
		if(tren->stanje==INPROCESS)
			printf("*");
		printf("\n");	
		pthread_mutex_unlock(&tren->mutex);
		return;
	}
	int flag=decauradu(stablo[tren->index],generateSize());
		
	pthread_mutex_lock(&tren->mutex);
	int dp=0;	
	
	for(int k=0; k <duzina; k++){
		if(tren->ime[k]== ' ')break;			
			printf("%c",tren->ime[k]);	
	}
	printf(": %d",tren->vrednost);
	if(flag)
		printf("*");
	printf("\n");
	pthread_mutex_unlock(&tren->mutex);
					
	printerino(tren->index,0, generateSize());	

}

int ispitatDalJeFile(char* path){
	char *p=path;
	for(int i=0; i< strlen(p);i++){
		if(*p=='.'){
			return 1;		
		}
	}
	return 0;
}
void fixAddingSame(node *nod){
	pthread_mutex_unlock(&nod->mutex);
	int sz=generateSize(),pm=0;
	for(int i=0 ; i < sz; i++){
		pthread_mutex_lock(&stablo[i].mutex);
		pm=1;
		if( stablo[i].roditelj == nod->index){
			pm=0;
			
			
			//printf("ime za %d je %s", stablo[i].index, stablo[i].ime);
			pthread_mutex_unlock(&stablo[i].mutex);
			if(stablo[i].tip==Dir){		
				fixAddingSame(&stablo[i]);	;				
			}
			stablo[i].index=-50;
			stablo[i].roditelj=-2;
			stablo[i].ime[0]='`';
			stablo[i].ime[1]='+';
			stablo[i].ime[2]='\0';
			stablo[i].tip=File;									
								
		}
		
		if(pm)
			pthread_mutex_unlock(&stablo[i].mutex);					
	}
	
}

void add_file(char *ime, int roditelj, struct stat buf){
	
	node *nod=makeaNode(ime,0);
	
	nod->roditelj=roditelj;
	nod->vremObr=buf.st_mtime;
	nod->stanje=NEEDED;

	int sz = generateSize();
	for(int i =0 ;i < sz; i++){
		//printf("		index: %d ime  a stanje %d %s \n",stablo[i].index, stablo[i].stanje,stablo[i].ime);	
		
	}
}

int generateSize(){
	int n;
	pthread_mutex_lock(&konobar);
	n=size;
	pthread_mutex_unlock(&konobar);
	return n;
}

int imecmp2(char *ime, char *nodeIme){
	char *str=ime, *str1=nodeIme;
	int n = abs(strlen(ime) - strlen(str1));
	if(n > 1 || n== 0 )return 0;
	int m=strlen(str) <  strlen(str1) ? strlen(str) : strlen(str1);
	
	for(int i=0; i< m; i++ ){
		if(*str != *str1) return 0;
		str++;
		str1++;
	}
		
	return 1;	
}

node *findNode(char *nodeIme){
	node* pr=stablo;
	int sz=generateSize();
	

	node* p;
	for(int i=0; i< sz;i++){
		p=&pr[i];
		pthread_mutex_lock(&p->mutex);
		if((strcmp(p->ime,nodeIme)== 0 && p->stanje !=REMOVED && p->tip ==File) || (strcmp(p->ime,nodeIme)== 0 && p->tip== Dir)){
			pthread_mutex_unlock(&p->mutex);
			return p;
		}
		pthread_mutex_unlock(&p->mutex);
			
		if(imecmp2(p->ime,nodeIme)){
			return p;
		}
		
	}

	
	
	//printf("neuspeh u pronalazenju nod-a pod imenom %s\n",nodeIme);	

	return NULL;
}


int main(int argc, char **argv){
	int n;
	char* p;
	if( argc <=1){
		printf("Nema dovoljno parametara");
	}else{
		long conv = strtol(argv[1], &p, 10);
		if(*p!='\0'){
			printf("nije lepo unesen broj za workere ! \n");
			exit(1);		
		}		
		n=conv;
		printf("n je %d\n", n);
	}
	pthread_t workers[n];
	for(int i = 0; i < n; i++)
	{
		pthread_create(&workers[i], NULL, worker_thread, NULL);
	}
	
	
	sem_init(&doFiles, 0, 0 ); // sad oke
	pthread_mutex_init(&thkonobar, NULL);
	pthread_mutex_init(&konobar, NULL);
	char arg[50];
	while(1){

		
		//scanf("%s", arg);
		 fgets(arg, 50, stdin);
		if( contains(arg, "add_dir")){
			fl=1;
			add_dir(pripremiArg(arg),-1);
			continue;
		}
		
		if(contains(arg,"remove_dir")){
			remove_dir(pripremiArg(arg));
			continue;		
		}
		if(contains(arg,"result")){
			result(pripremiArg(arg));
			continue;		
		}
		
		if(contains(arg, "exit"))break;
	}
	if(stablo!=NULL)
		free(stablo);
	if(threads!=NULL)	
		free(threads);
	pthread_mutex_destroy(&thkonobar);
	pthread_mutex_destroy(&konobar);
	sem_destroy(&doFiles);

	return 1;
	}

int contains(char *arg,char *func){
	char *prim=arg;
	char str[15],c;
	int j=0;
	for( int i =0; i < strlen(arg); i++){
		c=prim[i];
		if(c == ' ')	
			break;		
		str[j++]=c;		
	}
	str[j]='\0';
	
	
	if (strcmp(str,func)==0)
		return 1;
	int sz=strlen(func);
	for(int i=0; i<strlen(str); i++){	
		if( i < sz && func[i] != str[i])	
			return 0;		
		
		if(i>= sz && !(str[i] >= 'a' && str[i] <='z')){
			return 1;		
		}else{
			if(i >= sz)
				return 0;		
		}
	}

	
return 0;
}

char* pripremiArg(char *arg){
	char *prim=arg;
	while(*prim != ' ' && prim != NULL)
		prim++;	
		
	prim++;	


	if(prim !=NULL)
		return prim;


return arg;
}

void izbirsiMe(node *tren){
	int sz =generateSize(),index=tren->index;
	pthread_mutex_lock(&tren->mutex);
	tren->stanje=REMOVED;
	pthread_mutex_unlock(&tren->mutex);
	for(int i=0;i<sz;i++){
		node *pr=&stablo[i];
		if(pr->roditelj== index){
			pthread_mutex_lock(&pr->mutex);
			pr->stanje=REMOVED;
			pthread_mutex_unlock(&pr->mutex);		
		}	
	}
	return;

}





node * makeaNode(char * ime, int dir){
	node *nod=malloc(sizeof(node));
	if(nod == NULL){
		printf("no more mem for nodes\n");
		exit(1);	
	}
	
	strcpy(nod->ime,ime);
	//printf("sada je im u embryo %s \n", nod->ime);
	nod->index=-1;
	nod->roditelj=-1;
	if(dir)
		nod->tip=Dir;
	else
		nod->tip=File;
	
	nod->vrednost=0;
	pthread_mutex_init(&nod->mutex, NULL);
	if(dir)
		nod->stanje=DIRR;
	else 
		nod->stanje=EMBRYO;	
	// vreme faili?
	nod->vremObr=(time_t) (-1);

	pthread_mutex_lock(&konobar);
	
//dodavanje u stablo
	if(stablo==NULL){
	 	stablo=malloc(sizeof(node));
	
		size=1;
	}else{
		size++;
		stablo=realloc(stablo,size*sizeof(node));
	}


	nod->index=size-1;
	stablo[size-1]=*nod;
	
	pthread_mutex_unlock(&konobar);
	return &stablo[size-1];
}

void printRez(){

	pthread_mutex_lock(&thkonobar);

	
	pthread_t *prikazt=malloc(sizeof(pthread_t));
	if(threads ==NULL ){
		threads=malloc(sizeof(pthread_t));	
	}else{
		threads=(pthread_t *) realloc(threads,tSize*sizeof(pthread_t));
	}		
	threads[tSize-1]=*prikazt;
	tSize++;
	
	pthread_create(&threads[tSize-2], NULL, printer_thread, NULL);
	pthread_mutex_unlock(&thkonobar);
	pthread_join(threads[tSize-2],NULL);


}

void printerino(int index,int depth, int sz){
	

	for(int i=0;i< sz; i++){
		if(stablo[i].roditelj!=index || stablo[i].index== index)continue;

		pthread_mutex_lock(&stablo[i].mutex);
		if(stablo[i].stanje==REMOVED){
			pthread_mutex_unlock(&stablo[i].mutex);
			continue;		
		}
		pthread_mutex_unlock(&stablo[i].mutex);
		
		pthread_mutex_lock(&stablo[i].mutex);	
		int dp=depth;
		while(dp-- >=0)
			printf("-");
		//printanje imena
		int duzina=strlen(stablo[i].ime);
		for(int k=0; k <duzina; k++){
			if(stablo[i].ime[k]== ' ')break;			
			printf("%c",stablo[i].ime[k]);	
		}
		//print vrednosti;

		printf(": %d", stablo[i].vrednost);	
		if(stablo[i].stanje== INPROCESS || stablo[i].stanje== EMBRYO || stablo[i].stanje== NEEDED){
			printf("*");
		}	
		
		pthread_mutex_unlock(&stablo[i].mutex);
		if( stablo[i].tip==Dir && decauradu(stablo[i], sz))
			printf("*");
		printf("\n");
		if(stablo[i].tip==Dir){
			printerino(stablo[i].index,depth+1,sz);
		}
			
	}	
	


}

int primarius(int n){
	

	int i, flag = 0;
 
	for (i = 2; i <= n / 2; ++i) {
    		// condition for non-prime
    		if (n % i == 0) {
      			flag = 1;
      			break;
    		}
  	}

  	if (n == 1 || flag) 
    		return 0;
	if(!flag) return 1;

  return 0;
	
}




