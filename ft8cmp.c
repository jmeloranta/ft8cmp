#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CTY_DAT "cty.dat"  // cty file
#define FREQ_TOL 3 // Hz
// #define REQUIRE_BOTH     // Requre decoded line to be in both files

typedef struct {
  int date;
  int time;
  int db;
  int freq;
  char call[25];
} log_entry;

#define MAX_PREFIX 32768
#define MAX_PREFLEN 16

char prefixes[MAX_PREFIX][MAX_PREFLEN];
int nprefixes = 0;

#ifdef CTY_DAT
void read_cty(char *file) {

  FILE *fp;
  char buf[512];
  int i, j, prev_j, state = 0;

  if(!(fp = fopen(CTY_DAT, "r"))) {
    fprintf(stderr, "Can't open %s.\n", file);
    exit(1);
  }
  while (1) {
    if(fgets(buf, sizeof(buf), fp) == NULL) break;
    if(buf[0] == '#') continue;
    prev_j = 0;
    switch(state) {
    case 0:
      // no relevant info on this line
      state = 1;
      break;
   case 1:
      for (j = 0; ; j++) {
        if(buf[j] == ' ' || buf[j] == '=') {
          prev_j = j + 1;
          continue;
        }
        if(buf[j] == ',' || buf[j] == ';' || buf[j] == '(' || buf[j] == '[') {
          if(nprefixes == MAX_PREFIX) {
            fprintf(stderr, "Increase MAX_PREFIX.\n");
            exit(1);
          }
          strncpy(prefixes[nprefixes], &buf[prev_j], j - prev_j);
          prefixes[nprefixes][j - prev_j + 1] = '\0';
//          printf("%d %s\n", nprefixes, prefixes[nprefixes]);fflush(stdout);
          nprefixes++;
          if(buf[j] == '(') {
            for ( ; buf[j] != ')'; j++);
            j++;
          }
          if(buf[j] == '[') {
            for ( ; buf[j] != ']'; j++);
            j++;
          }
          prev_j = j + 1;
        }
        if(buf[j] == ';') {
          state = 0;
          break;
        }
        if(buf[j] == ',' && buf[j+1] == '\r') {
          state = 1;
          break;
        }
      }
    }
  }
  fprintf(stderr, "Number of prefixes read = %d\n", nprefixes);
  fclose(fp);
}
#endif

int check_prefix(char *call) {  // 0 = not wanted, 1 = wanted

  int i;

  for (i = 0; i < nprefixes; i++) {
    if(!strncmp(prefixes[i], call, strlen(prefixes[i]))) return 1;
  }
  return 0;
}

void read_log(char *file, log_entry *log, int *nbr) {

  *nbr = 0;
  char tmp1[64], tmp2[64], tmp3[64], tmp4[4], buf[256];
  FILE *fp;

  if(!(fp = fopen(file, "r"))) {
    fprintf(stderr, "Can't open file %s\n", file);
    exit(1);
  }
  while(1) {
    tmp1[0] = tmp2[0] = tmp3[0] = tmp4[0] = '\0';
    if(fgets(buf, sizeof(buf), fp) == NULL) break;
    sscanf(buf, "%d_%d %*f %*s %*s %d %*f %d %s %s %s %s", &(log[*nbr].date),
	   &(log[*nbr].time), &(log[*nbr].db), &(log[*nbr].freq), tmp1, tmp2, tmp3, tmp4);
    if(tmp4[0] == '\0' || tmp4[0] == 'a') strcpy(log[*nbr].call, tmp2);
    else strcpy(log[*nbr].call, tmp3);
#ifdef CTY_DAT
    if(check_prefix(log[*nbr].call)) *nbr += 1; // otherwise ignore
#else
    *nbr += 1;
#endif
  }
  fclose(fp);
}

int main(int argc, char **argv) {

  log_entry log1[2048], log2[2048];
  int i, j, n1, n2;
  
  if(argc != 3) {
    fprintf(stderr, "Usage: ft8cmp ALL1.DAT ALL2.DAT\n");
    exit(1);
  }

#ifdef CTY_DAT
  read_cty(CTY_DAT);
#endif

  read_log(argv[1], log1, &n1);
  fprintf(stderr, "%d lines read from %s (unwanted prefixed excluded).\n", n1, argv[1]);
  read_log(argv[2], log2, &n2);
  fprintf(stderr, "%d lines read from %s (unwanted prefixed excluded).\n", n2, argv[2]);

  // search for every ALL1.TXT entries
  for(i = 0; i < n1; i++) {
    for (j = 0; j < n2; j++)
      if(log1[i].date == log2[j].date && log1[i].time == log2[j].time &&
	 abs(log1[i].freq - log2[j].freq) <= FREQ_TOL && !strcmp(log1[i].call, log2[j].call)) {
        printf("# %s (%d-%d)\n", log1[i].call, log1[i].date, log1[i].time);
	printf("%d %d\n", log1[i].db, log2[j].db);
	break;
      }
#ifndef REQUIRE_BOTH
    if(j && j == n2) {
      printf("# %s (%d-%d)\n", log1[i].call, log1[i].date, log1[i].time);
      printf("%d -26\n", log1[i].db); // not received in ALL2.TXT
    }
#endif
  }
	 
#ifndef REQUIRE_BOTH
  // search for every ALL2.TXT entries - only entries that are missing in ALL1.TXT
  for(j = 0; j < n2; j++) {
    for (i = 0; i < n1; i++)
      if(log1[i].date == log2[j].date && log1[i].time == log2[j].time &&
	 abs(log1[i].freq - log2[j].freq) <= FREQ_TOL && !strcmp(log1[i].call, log2[j].call))
	break;
    if(i && i == n1) {
      printf("# %s (%d-%d)\n", log2[j].call, log2[j].date, log2[j].time);
      printf("-26 %d\n", log2[j].db); // not received in ALL1.TXT
    }
  }
#endif
}
