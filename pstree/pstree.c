#include <assert.h>
#include <dirent.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Process {
  char name[NAME_MAX];
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
} ListNode, *List;

void free_children(Children c) {
  ChildNode *cur = c;
  while (cur != NULL) {
    ChildNode *next = cur->next;
    free(cur);
    cur = next;
  }
}

void free_list(List l) {
  ListNode *cur = l->next;
  while (cur != NULL) {
    ListNode *next = cur->next;
    free_children(cur->val.children);
    free(cur);
    cur = next;
  }
}

void get_processes(List l) {
  assert(l != NULL);
  DIR *dir;
  struct dirent *entry;
  char path[PATH_MAX];
  dir = opendir("/proc");
  if (!dir) {
    fprintf(stderr, "error: open dir /proc");
    exit(0);
  }
  ListNode *cur = l;

  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_type == DT_DIR) {
      char *endptr;
      long pid = strtol(entry->d_name, &endptr, 10);
      if (*endptr != '\0') {
        continue;
      }
      snprintf(path, PATH_MAX, "/proc/%ld/stat", pid);
      FILE *fp = fopen(path, "r");
      if (fp) {
        char name[NAME_MAX + 2] = {0};
        int ppid = 0;
        fscanf(fp, "%ld %s %*c %d", &pid, name, &ppid);
        ListNode *t = (ListNode *)calloc(1, sizeof(ListNode));
        strncpy(t->val.name, name + 1, strlen(name) - 2);
        if (t == NULL) {
          fprintf(stderr, "error: malloc fault");
          free_list(l);
          exit(0);
        }
        t->val.pid = pid;
        t->val.ppid = ppid;
        t->val.children = NULL;
        t->next = NULL;

        cur->next = t;
        cur = cur->next;
        fprintf(stderr, "Added process: %s (pid: %d, ppid: %d)\n",
                cur->val.name, cur->val.pid, cur->val.ppid);
      } else {
        fprintf(stderr, "error: open pid: %ld file", pid);
      }
      fclose(fp);
    }
  }
  closedir(dir);
}

void set_children(List l) {
  assert(l != NULL);
  ListNode *cur_i = l->next;
  while (cur_i != NULL) {
    ListNode *cur_j = l->next;
    while (cur_j != NULL) {
      if (cur_i->val.pid == cur_j->val.ppid) {
        Children children = (Children)calloc(1, sizeof(ChildNode));
        if (children == NULL) {
          fprintf(stderr, "error: malloc fault");
          free_list(l);
          exit(0);
        }
        children->next = cur_i->val.children;
        children->val = &(cur_j->val);
        cur_i->val.children = children;
      }
      cur_j = cur_j->next;
    }
    cur_i = cur_i->next;
  }
}

void sort_by_name(Children);
void sort_by_pid(Children);
void sort(List l, int numeric_sort_flag) {
  assert(l != NULL);
  ListNode *cur = l->next;
  while (cur != NULL) {
    if (cur->val.children) {
      if (numeric_sort_flag) {
        sort_by_pid(cur->val.children);
      } else {
        sort_by_name(cur->val.children);
      }
    }
    cur = cur->next;
  }
}

void sort_by_name(Children c) {
  ChildNode *cur = c;
  while (cur != NULL) {
    ChildNode *next = cur->next;
    while (next != NULL) {
      if (strcmp(cur->val->name, next->val->name) > 0) {
        Process *temp = cur->val;
        cur->val = next->val;
        next->val = temp;
      }
      next = next->next;
    }
    cur = cur->next;
  }
}
void sort_by_pid(Children c) {
  ChildNode *cur = c;
  while (cur != NULL) {
    ChildNode *next = cur->next;
    while (next != NULL) {
      if (cur->val->pid > next->val->pid) {
        Process *temp = cur->val;
        cur->val = next->val;
        next->val = temp;
      }
      next = next->next;
    }
    cur = cur->next;
  }
}

Process *find_init(List l) {
  ListNode *cur = l;
  while (cur->next != NULL) {
    cur = cur->next;
    if (cur->val.ppid == 0) {
      return &(cur->val);
    }
  }
  return NULL;
}
void print_pstree_helper(Process *p, int show_pids_flag, int depth) {
  assert(p != NULL);
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

  ListNode head;
  head.next = NULL;
  List l = &head;

  get_processes(l);
  Process *init = find_init(l);
  if (!init) {
    fprintf(stderr, "error: no find init");
    exit(0);
  }
  set_children(l);
  sort(l, numeric_sort_flag);
  print_pstree(init, show_pids_flag);

  free_list(l);

  return 0;
}