#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_PATH 4096
#define MAX_FILENAME 256
#define INT_MAX 0x7FFFFFFF

typedef struct Process {
  char *name;
  int pid;
  int ppid;
  struct ChildNode *children;
} Process;

typedef struct ChildNode {
  Process *val;
  struct ChildNode *next;
} ChildNode, *Children;

typedef struct ListNode {

  Process val;
  struct ListNode *next;
} ListNode_pro, *List;

// int is_numerice(const char str[]) {
//   for (size_t i = 0; i < strlen(str); i++) {
//     if (!isdigit(str[i])) {
//       return 0;
//     }
//   }
//   return 1;
// }

void get_processes(List l) {
  assert(l != NULL);
  DIR *dir;
  struct dirent *entry;
  char path[MAX_PATH];
  dir = opendir("/proc");
  if (!dir) {
    fprintf(stderr, "/proc,error opening.");
  }
  ListNode_pro *cur = l;
  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_type == DT_DIR) {
      char *endptr;
      long pid = strtol(entry->d_name, &endptr, 10);
      if (*endptr != '\0') {
        continue;
      }
      snprintf(path, MAX_PATH, "/proc/%ld/stat", pid);
      FILE *fp = fopen(path, "r");
      if (fp) {
        char name[MAX_FILENAME + 2] = {0};
        int ppid = 0;
        fscanf(fp, "%ld %s %*c %d", &pid, name, &ppid);
        cur->next = (ListNode_pro *)malloc(sizeof(ListNode_pro));
        cur->next->next = NULL;
        cur = cur->next;
        strncpy(cur->val.name, name + 1, strlen(name) - 2);
        cur->val.pid = pid;
        cur->val.ppid = ppid;
        cur->val.children = NULL;
      } else {
        fprintf(stderr, "pid: %ld file,error opening.", pid);
      }
    }
  }
}

void set_children(List l) {
  assert(l != NULL);
  ListNode_pro *cur_i = l;
  while (cur_i->next != NULL) {
    cur_i = cur_i->next;
    ListNode_pro *cur_j = l;
    while (cur_j->next != NULL) {
      cur_j = cur_j->next;
      if (cur_i == cur_j) {
        continue;
      }
      if (cur_i->val.pid == cur_j->val.ppid) {
        Children children = (Children)malloc(sizeof(ChildNode));
        children->next = cur_i->val.children;
        children->val = &(cur_j->val);
        cur_i->val.children = children;
      }
    }
  }
}

void sort_by_name(Children);
void sort_by_pid(Children);
void sort(List l, int numeric_sort_flag) {
  assert(l != NULL);
  ListNode_pro *cur = l;
  while (cur->next != NULL) {
    cur = cur->next;
    if (!cur->val.children) {
      continue;
    }
    ChildNode head;
    head.next = cur->val.children;
    if (numeric_sort_flag) {
      sort_by_pid(&head);
    } else {
      sort_by_name(&head);
    }
    cur->val.children = head.next;
  }
}

void sort_by_name(Children c) {
  ChildNode *curr = c->next;
  while (curr != NULL) {
    ChildNode *next = curr->next;
    while (next != NULL) {
      if (strcmp(curr->val->name, next->val->name) > 0) {
        Process *temp = curr->val;
        curr->val = next->val;
        next->val = temp;
      }
      next = next->next;
    }
    curr = curr->next;
  }
}
void sort_by_pid(Children c) {
  ChildNode *curr = c->next;
  while (curr != NULL) {
    ChildNode *next = curr->next;
    while (next != NULL) {
      if (curr->val->pid > next->val->pid) {
        Process *temp = curr->val;
        curr->val = next->val;
        next->val = temp;
      }
      next = next->next;
    }
    curr = curr->next;
  }
}

void free_children(Children c) {
  ChildNode *curr = c;
  while (curr != NULL) {
    ChildNode *next = curr->next;
    free(curr);
    curr = next;
  }
}
void free_list(List l) {
  ListNode_pro *curr = l;
  while (curr != NULL) {
    ListNode_pro *next = curr->next;
    free_children(curr->val.children);
    free(curr);
    curr = next;
  }
}

Process *find_init(List l) {
  ListNode_pro *cur = l;
  while (cur->next != NULL) {
    cur = cur->next;
    if (cur->val.ppid == 0) {
      return &(cur->val);
    }
  }
  return NULL;
}
void print_pstree_helper(Process *p, int show_pids_flag, int depth) {
  for (int i = 0; i < depth; i++) {
    printf("|   ");
  }
  if (depth > 0) {
    printf("+-- ");
  }
  printf("%s", p->name);
  if (show_pids_flag) {
    printf("(%d)", p->pid);
  }
  printf("\n");
  Children children = p->children;
  while (children != NULL) {
    print_pstree_helper(children->val, show_pids_flag, depth + 1);
    children = children->next;
  }
}

void print_pstree(Process *init, int show_pids_flag) {
  assert(init != NULL);
  printf("%s", init->name);
  if (show_pids_flag) {
    printf("(%d)", init->pid);
  }
  printf("\n");
  Children children = init->children;
  while (children != NULL) {
    print_pstree_helper(children->val, show_pids_flag, 1);
    children = children->next;
  }
}
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
  int numeric_sort_flag = 0;
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
  List l = (List)malloc(sizeof(ListNode_pro));
  l->val.children = NULL;
  l->next = NULL;

  get_processes(l);
  Process *init = find_init(l);
  if (!init) {
    fprintf(stderr, "no find init.");
  }
  set_children(l);
  sort(l, numeric_sort_flag);
  print_pstree(init, show_pids_flag);
  free_list(l);

  return 0;
}
