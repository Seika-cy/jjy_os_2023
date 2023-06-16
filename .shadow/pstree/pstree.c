#include <assert.h>
#include <getopt.h>
#include <stdio.h>

typedef struct TreeNode{
  char name[256];
  unsigned int pid;
  unsigned int ppid;
}TreeNode;

struct children_table
{

};

int main(int argc, char *argv[]) {
  // for (int i = 0; i < argc; i++) {
  //   assert(argv[i]);
  //   printf("argv[%d] = %s\n", i, argv[i]);
  // }
  // assert(!argv[argc]);
  struct option opts[] = {{"show-pids", 0, NULL, 'p'},
                          {"numeric-sort", 0, NULL, 'n'},
                          {"version", 0, NULL, 'V'}};
  char c;
  int show_pids_flag = 0;
  int numeric_sort_flag = 1;
  while ((c = getopt_long(argc, argv, "pnV", opts, NULL)) != EOF) {
    switch (c) {
    case 'p':
      show_pids_flag = 1;
      break;
    case 'n':
      numeric_sort_flag = 1;
      break;
    case 'V':
      fprintf(stderr, "pstree 0.0.1\n");
      return 0;
    }
  }
  return 0;
}
