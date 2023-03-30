#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Simulation struct */
struct event
{
  int eventity; /* entity (node) where event occurs */
  struct event *prev;
  struct event *next;
};

/* Global variables*/
struct event *evlist = NULL;
int **dt;
int **dt_new;
int **lc;
int **traffic;
int num_nodes_max = 10;
int num_nodes;
int k_max = 0;
int k = 0;
int v_max = 99;
int v = 0;

/* Routines */
void insertevent(struct event *p);
void send2neighbor(int node);
void send2neighbors(int node);
void init(int node);
int has_changed(int node);
void dtupdate(int node);
int r_file(char *path, int n_col, int n_max, int ***matrix);
void parse_args(char *argv[]);
int are_neighbors(int node1, int node2);
void cpy_matrix(int ***m1, int **m2);
void print_dt();

/* Simulation */
int main(int argc, char *argv[])
{
  struct event *eventptr;

  parse_args(argv);

  dt = (int **)malloc(num_nodes * sizeof(int *));
  for (int i = 0; i < num_nodes; i++)
    init(i);
  print_dt();

  while (k < k_max)
  {
    k++;
    cpy_matrix(&dt_new, dt); /* sets dt_new = copy of dt */

    while (evlist != NULL)
    {
      eventptr = evlist;            /* get next event */
      evlist = evlist->next;        /* remove this event from event list */
      dtupdate(eventptr->eventity); /* update the distance vector */
      free(eventptr);               /* free memory for event struct   */
    }

    for (int i = 0; i < num_nodes; i++) /* detect change */
    {
      if (has_changed(i))
        send2neighbors(i);
    }
    free(dt);
    cpy_matrix(&dt, dt_new); /* sets dt = copy of dt_new */
    free(dt_new);
    print_dt();
  }
}

/* Event handling*/
void insertevent(struct event *p)
{
  struct event *q, *qold;

  q = evlist; /* q points to header of list in which p struct inserted */
  if (q == NULL)
  { /* list is empty */
    evlist = p;
    p->next = NULL;
    p->prev = NULL;
  }
  else
  {
    if (q == NULL)
    { /* end of list */
      qold->next = p;
      p->prev = qold;
      p->next = NULL;
    }
    else if (q == evlist)
    { /* front of list */
      p->next = evlist;
      p->prev = NULL;
      p->next->prev = p;
      evlist = p;
    }
    else
    { /* middle of list */
      p->next = q;
      p->prev = q->prev;
      q->prev->next = p;
      q->prev = p;
    }
  }
}

int has_changed(int node)
{
  for (int i = 0; i < num_nodes; i++)
  {
    if (dt[node][i] != dt_new[node][i])
      return 1;
  }
  return 0;
}

void send2neighbor(int node)
{
  struct event *evptr;
  evptr = (struct event *)malloc(sizeof(struct event));
  evptr->eventity = node;
  insertevent(evptr);
}

void send2neighbors(int node)
{
  for (int i = 0; i < num_nodes; i++)
  {
    if (are_neighbors(node, i))
      send2neighbor(i);
  }
}

void init(int node)
{
  dt[node] = (int *)malloc(num_nodes * sizeof(int));
  for (int i = 0; i < num_nodes; i++)
    dt[node][i] = lc[node][i];
  send2neighbors(node);
}

void dtupdate(int node)
{
  for (int i = 0; i < num_nodes; i++) /* node -> j -> i*/
  {
    if (node == i)
      continue;
    int min_cost = -1;
    for (int j = 0; j < num_nodes; j++)
    {
      if (are_neighbors(node, j))
      {
        int cost_node_j = lc[node][j];
        int cost_j_i = dt[j][i];
        int cost = cost_node_j + cost_j_i;
        if (cost_j_i != -1) /* can pass through j to reach i from node */
        {
          if (min_cost == -1 || cost < min_cost)
            min_cost = cost;
        }
      }
    }
    dt_new[node][i] = min_cost;
  }
}

/* Helpers */
int are_neighbors(int node1, int node2)
{
  if (lc[node1][node2] == -1 || node1 == node2)
    return 0;
  else
    return 1;
}

void parse_args(char *argv[])
{
  k_max = atoi(argv[1]);

  char *path_topo = argv[2];
  num_nodes = r_file(path_topo, -1, num_nodes_max, &lc);
}

int r_file(char *path, int n_col, int n_max, int ***matrix)
{
  FILE *f;
  if ((f = fopen(path, "r")) == NULL)
  {
    perror("read file topo");
    exit(1);
  }
  int m[n_max][n_max];
  char line[128];
  char *n;
  int i = 0;
  while (fgets(line, sizeof line, f) != NULL)
  {
    int j = 0;
    n = strtok(line, " ");
    while (n != NULL)
    {
      m[i][j] = atoi(n);
      n = strtok(NULL, " ");
      j++;
    }
    i++;
  }
  int n_row = i;
  if (n_col == -1)
    n_col = n_row;
  *matrix = (int **)malloc(n_row * sizeof(int *));
  for (int i = 0; i < n_row; i++)
  {
    (*matrix)[i] = (int *)malloc(n_col * sizeof(int));
    for (int j = 0; j < n_col; j++)
      (*matrix)[i][j] = m[i][j];
  }
  return n_row;
}

void cpy_matrix(int ***m1, int **m2)
{
  *m1 = (int **)malloc(num_nodes * sizeof(int *));
  for (int i = 0; i < num_nodes; i++)
  {
    (*m1)[i] = (int *)malloc(num_nodes * sizeof(int));
    for (int j = 0; j < num_nodes; j++)
      (*m1)[i][j] = m2[i][j];
  }
}

void print_dt()
{
  if (k < 5 || k % 10 == 0)
  {
    printf("k=%d:\n", k);
    for (int i = 0; i < num_nodes; i++)
    {
      printf("node-%d: ", i);
      for (int j = 0; j < num_nodes - 1; j++)
      {
        int cost = dt[i][j];
        printf("%d ", cost);
      }
      printf("%d\n", dt[i][num_nodes - 1]);
    }
  }
}