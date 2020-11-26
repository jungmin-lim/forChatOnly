#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <utmp.h>
#include <stdlib.h>
#define	is_delim(x) ((x)==' '||(x)=='\t')

void do_ls (char[]);
void dostat(char*);
void show_file_info(char*, struct stat*);
void mode_to_letters(int, char[]);
char* uid_to_name(uid_t);
char* gid_to_name(gid_t);
void get_total(char*);
char** splitline(char* line);
void show_info(struct utmp* utbufp);
void showtime(long);
char* newstr(char* s, int l);
void freelist(char** list);
void* emalloc(size_t n);
void* erealloc(void* p, size_t n);

long total=0;
#define BUF_LENGTH 1024
int main(){
	char command[BUF_LENGTH],**arglist;

	scanf("%s", command);
	arglist = splitline(command);
	if (strcmp(arglist[0], "who")) {
		struct utmp* utbufp;
		struct utmp* utmp_next();

		if (utmp_open(UTMP_FILE) == -1) {
			perror(UTMP_FILE);
			exit(1);
		}

		while ((utbufp = utmp_next()) != ((struct utmp*)NULL))
			show_info(utbufp);

		utmp_close();
	}
	if (strcmp(arglist[0], "ls")) {
		do_ls(".");
	}
	/*if(argc ==1){
		do_ls(".");
	}
	else{
		while(--argc){
			printf("%s:\n", *++argv);
			do_ls(*argv);
		}
	}*/
}

void do_ls(char dirname[]){
	DIR *dir_ptr,*dir_ptr2;
	struct dirent *direntp,*direntp2;
	if((dir_ptr=opendir(dirname))==NULL)
		fprintf(stderr, "ls2: cannot open %s\n", dirname);
	else{
		dir_ptr2=opendir(dirname);
		while((direntp2=readdir(dir_ptr2))!=NULL)
			get_total(direntp2->d_name);
		printf("total : %ld\n",total/2);
		while((direntp=readdir(dir_ptr))!=NULL)
			dostat(direntp->d_name);
		closedir(dir_ptr);
	}
}
void dostat(char* filename){
	struct stat info;
	if(stat(filename,&info)==-1)
		perror(filename);
	else
		show_file_info(filename, &info);
}

void get_total(char* filename){
	struct stat info;
	if(stat(filename,&info)==-1)
		perror(filename);
	else
		total+=(info.st_blocks);
}
void show_file_info(char* filename, struct stat* info_p){
	char *uid_to_name(), *ctime(), *gid_to_name(), *filemode();
	void mode_to_letters();
	char modestr[11];

	mode_to_letters(info_p->st_mode, modestr);
	printf("%s", modestr);
	printf("%4d", (int) info_p->st_nlink);
	printf("%-8s ", uid_to_name(info_p->st_uid));
	printf("%-8s ", gid_to_name(info_p->st_gid));
	printf("%-8ld ", (long)info_p->st_size);
	printf("%.12s ", 4+ctime(&info_p->st_mtime));
	printf("%s \n", filename);
}

void mode_to_letters(int mode, char str[]){
	strcpy(str, "----------");

	if(S_ISDIR(mode)) str[0] = 'd';
	if(S_ISCHR(mode)) str[0] = 'c';
	if(S_ISBLK(mode)) str[0] = 'b';
	if(mode & S_IRUSR) str[1] = 'r';
	if(mode & S_IWUSR) str[2] = 'w';
	if(mode & S_IXUSR) str[3] = 'x';

	if(mode & S_IRGRP) str[4] = 'r';
	if(mode & S_IWGRP) str[5] = 'w';
	if(mode & S_IXGRP) str[6] = 'x';

	if(mode&S_IROTH) str[7] = 'r';
	if(mode &S_IWOTH) str[8]= 'w';
	if(mode&S_IXOTH) str[9] = 'x';
}

char* uid_to_name(uid_t uid){
	struct passwd *getpwuid(), *pw_ptr;
	static char numstr[10];

	if((pw_ptr = getpwuid(uid))==NULL){
		sprintf(numstr, "%d", uid);
		return numstr;
	}
	else
		return pw_ptr->pw_name;
}

char* gid_to_name (gid_t gid){
	struct group *getdrid(), *grp_ptr;
	static char numstr[10];
	if((grp_ptr=getgrgid(gid))==NULL){
		sprintf(numstr, "%d", gid);
		return numstr;
	}
	else
		return grp_ptr->gr_name;
}



void show_info(struct utmp* utbufp) {
	if (utbufp->ut_type != USER_PROCESS)
		return;
	printf("%-8.8s", utbufp->ut_name);
	printf(" ");

	printf("%-8.8s", utbufp->ut_line);
	printf(" ");

	showtime(utbufp->ut_time);
	printf(" ");

	printf(" (%s)", utbufp->ut_host);
	printf("\n");
}

void showtime(long timeval) {
	char* cp;
	cp = ctime(&timeval);

	printf("%12.12s", cp + 4);
}

char** splitline(char* line) {
	char* newstr();
	char** args;
	int	spots = 0;
	int	bufspace = 0;
	int	argnum = 0;
	char* cp = line;
	char* start;
	int	len;

	if (line == NULL)
		return NULL;

	args = emalloc(BUF_LENGTH);
	bufspace = BUF_LENGTH;
	spots = BUF_LENGTH / sizeof(char*);

	while (*cp != '\0')
	{
		while (is_delim(*cp))
			cp++;
		if (*cp == '\0')
			break;
		if (argnum + 1 >= spots) {
			args = erealloc(args, bufspace + BUF_LENGTH);
			bufspace += BUF_LENGTH;
			spots += (BUF_LENGTH / sizeof(char*));
		}
		start = cp;
		len = 1;
		while (*++cp != '\0' && !(is_delim(*cp)))
			len++;
		args[argnum++] = newstr(start, len);
	}
	args[argnum] = NULL;
	return args;
}


char* newstr(char* s, int l) {
	char* rv = emalloc(l + 1);

	rv[l] = '\0';
	strncpy(rv, s, l);
	return rv;
}

void freelist(char** list) {
	char** cp = list;
	while (*cp)
		free(*cp++);
	free(list);
}

void* emalloc(size_t n) {
	void* rv;
	if ((rv = malloc(n)) == NULL)
		fatal("out of memory", "", 1);
	return rv;
}
void* erealloc(void* p, size_t n) {
	void* rv;
	if ((rv = realloc(p, n)) == NULL)
		fatal("realloc() failed", "", 1);
	return rv;
}
