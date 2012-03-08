/**************
 lao-native.c 
**************/

/*
 plain mode: $gcc -o lao-plain.cgi lao-native.c ; chmod +x lao-plain.cgi
 graph mode: $gcc -lm -DGRAPHMODE -o lao-table lao-native.c ; chmod +x lao-table
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/syslimits.h>
#include <grp.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <time.h>

#ifdef GRAPHMODE
#include <math.h>
#endif

#define HOME "/home"
#define GROUP "users"
#define HOSTLEN 30
#define TIMELEN 10

void error(const char *szError);
int getusercount(void);

int main(void)
{
	/* declare */
	char szHostName[HOSTLEN+1];
	int nUserCount = 0;
	double nLoadAvg[]={-1,-1,-1,-1};
	int mib[2],nUptime;
	struct timeval tvBoottime,tvCurrent;
	struct timezone tz;
	size_t len=0;
	struct tm *tmCurrent;
	char szTime[TIMELEN+1];
#ifdef GRAPHMODE
	int nGraph = 0;
#endif
	
	
	/* put content-header */
#ifndef GRAPHMODE
	puts("Content-Type:text/plain\n");
#endif
	/* get usercount */
	nUserCount = getusercount();
	
	/* get hostname */ 
	if(gethostname(szHostName,HOSTLEN)){
		error("gethostname");
	}
	
	/* get load average */
	if(getloadavg(nLoadAvg,3) == -1){
		error("getloadavg");
	}

	/* get boot epoch to get uptime(BSD) */
	mib[0] = CTL_KERN;
	mib[1] = KERN_BOOTTIME;
	len = sizeof(tvBoottime);
	if(sysctl(mib, 2, &tvBoottime, &len, NULL, 0)){
		error("sysctl");
	}
	
	/*
	get uptime at Linux ... read "/proc/uptime" first val
	
	$cat /proc/uptime
	[uptime] [idletime]
	*/
	
	if(gettimeofday(&tvCurrent,&tz)){
		error("getttimeofday");
	}

	nUptime = (tvCurrent.tv_sec - tvBoottime.tv_sec)/86400; /* 86400 = 60*60*24 */

	/* get current time */
	
	if((tmCurrent = localtime(&(tvCurrent.tv_sec))) == NULL){
		error("localtime");
    }

    if (strftime(szTime, sizeof(szTime), "%l:%M%p", tmCurrent) == 0) {
    	error("strftime");
	}

	/* output */
#ifdef GRAPHMODE
	/* code from lao-table.cgi */
	if (nLoadAvg[1] < 1) {
		nGraph = (int)rint(nLoadAvg[1]*4+0.5);
	} else {
		nGraph = (int)rint(log(nLoadAvg[1])/log(10)*8+4+0.5);
	}
	
	if (nGraph > 20) {
		nGraph = 20;
	}

	puts("<table><tr><th>Server Name</th><th colspan=\"2\">Load Average (5min)</th><th>Time</th><th>Day</th><th>User</th></tr>");
	printf("<tr><td>%s</td><td>%.2f</td><td><img src=\"./lao/bar-%d.png\" alt=\"#\"></td><td>%s</td><td>%d</td><td>%d</td></tr></table>\n",
		szHostName, nLoadAvg[1] , nGraph, szTime, nUptime, nUserCount);
#else
	printf("%s,%d,%s,%d, %.2f, %.2f\n",
		szHostName,nUserCount,szTime,nUptime,nLoadAvg[1],nLoadAvg[2]);
#endif
	return 0;
}

/* put error */
void error(const char *szError)
{
	puts(szError);
	exit(-1);
}

/* get user count */
int getusercount(void)
{
	DIR* pDir=NULL;
	struct dirent * pDE=NULL;
	int nUserCount = 0;
	struct stat ST;
	char szPath[PATH_MAX+1];
	struct group *pGRP;
	
	/* open directory */
	if((pDir = opendir(HOME)) == NULL){
		error("opendir");
	}
	
	/* scan directory */
	while ((pDE = readdir(pDir)) != NULL){
		
		/* get directory's group */
		if(snprintf(szPath,PATH_MAX,"%s/%s",HOME,pDE->d_name) < 0){
			closedir(pDir);
			error("sprintf");
		}
		if(stat(szPath,&ST)){
			closedir(pDir);
			error("stat");
		}
		if((pGRP = getgrgid(ST.st_gid)) == NULL){
			closedir(pDir);
			error("getgrgid");
		}

		if(!strcmp(pGRP->gr_name,GROUP)){
			nUserCount++;
		}
	};
	closedir(pDir);	
	
	return nUserCount;
}

