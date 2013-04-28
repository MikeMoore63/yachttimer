#include <stdio.h>
#include <time.h>
#include "yachtimermodel.h"


YachtTimer test;

void dumpPbl(PblTm *output);

int main(int ac, char*av)
{
	PblTm testMe;
	char testName[10];
	int i=0;
	time_t tests[] = { 0, /* check if 0 works */
			   1, /* 1 millisecond should still be 0 */
			  1000, /* 1 second we should have something */
			  59 , /* 59 seconds  */
			  60 , /* 1 minute */
			  61 , /* 1 minute and 1 second */
			  59 * 60 , /* 59 minutes  */
			  (59 * 60 ) + (59 ) , /* 59 minutes 59 seconds   */
			  (60 * 60 )  , /* 1 hour    */
			  (60 * 60 ) + (1 )  , /* 1 hour and 1 second    */
			  (60 * 60 ) + (60 )  , /* 1 hour and 1 minute    */
			  (23 *   60 * 60 ) + (59 )  , /* 23 hours and 59 seconds    */
			  (23 *  60 * 60 ) + (59 * 60 ) + (59 )  , /* 23 hours and 59 minutes and 59 seconds    */
			  (24  * 60 * 60 )   , /* 1 day */
			  (2 * 24 * 60 * 60 )   , /* 2 day */
			  (3 * 24 * 60 * 60 )   , /* 3 day */
			  (4 * 24 * 60 * 60 )   , /* 4 day */
			  (5 * 24 * 60 * 60 )   , /* 5 day */
			  (6 * 24 * 60 * 60 )   , /* 6 day */
			  (7 * 24 * 60 * 60 )   , /* 7 day or a week  */
			  (10 * 24 * 60 * 60 )   , /* 10 day or a week  */
			  (20 * 24 * 60 * 60 )   , /* 20 day or a week  */
			  /* Integer overflow */
			  (30 * 24 * 60 * 60 )   , /* 30 day or a week  */
			  (31 * 24 * 60 * 60 )   , /* end of jan  */
			  ((31+28)  * 24 * 60 * 60 )   , /* end of feb no leap year */
			  ((31+29)  * 24 * 60 * 60 )   , /* end of feb with leap year 1st macrh with  */
			  ((31+30)  * 24 * 60 * 60 )   , /* end 2nd march if non leap year  */
			  ((31+29+30)  * 24 * 60 * 60 )   }; /* end of marchfeb  */

	printf("Starting yachtimer tests\n");	
	for(i=0;i<(sizeof(tests)/sizeof(time_t));i++)
	{
		yachtimer_setPblTime(&testMe,tests[i] );
		printf("test number %d\n",i);
		dumpPbl(&testMe);
	}
	printf("Completing  yachtimer tests\n");	
			

}

// Dump test
void dumpPbl(PblTm *output)
{
	printf("===================================\n");
	printf("output->tm_sec=%d\n",output->tm_sec);
	printf("output->tm_min=%d\n",output->tm_min);
	printf("output->tm_hour=%d\n",output->tm_hour);
	printf("output->tm_mday=%d\n",output->tm_mday);
	printf("output->tm_mon=%d\n",output->tm_mon);
	printf("output->tm_year=%d\n",output->tm_year);
	printf("output->tm_wday=%d\n",output->tm_wday);
	printf("output->tm_yday=%d\n",output->tm_yday);
	printf("output->tm_isdst=%d\n",output->tm_isdst);
	printf("===================================\n");
}

// shim this out for the minute
void get_time(PblTm *setMe)
{
	
}

