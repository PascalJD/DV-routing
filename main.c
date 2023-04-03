#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Simulation structs */
struct event
{
  int eventity; /* entity (node) where event occurs */
  struct event *prev;
  struct event *next;
};
struct route
{
  int *path;
  int has_cycle;
  int length;
};

/* Global variables*/
struct event *evlist = NULL;
int **dt;
int **dt_new;
int **lc;
int **lc_new;
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
void init_lc_new();
int has_changed(int node);
void dtupdate(int node);
void lcupdate(struct route *rt, int n_pkts);
void route(int src, int dst, struct route *rt);
int r_file(char *path, int n_col, int n_max, int ***matrix);
void parse_args(char *argv[]);
int are_neighbors(int node1, int node2);
void cpy_matrix(int ***m1, int **m2);
void print_dt();
void print_route(int src, int dst, int n_pkts, struct route *rt);
void print_lc();

/* Simulation */
int main(int argc, char *argv[])
{
  struct event *eventptr;

  parse_args(argv);

  dt = (int **)malloc(num_nodes * sizeof(int *));
  for (int i = 0; i < num_nodes; i++)
    init(i);

  while (k < k_max)
  {
    k++;
    cpy_matrix(&dt_new, dt); /* sets dt_new = copy of dt */
    init_lc_new();           /* null matrix */

    while (evlist != NULL)
    {
      eventptr = evlist;            /* get next event */
      evlist = evlist->next;        /* remove this event from event list */
      dtupdate(eventptr->eventity); /* update the distance vector */
      free(eventptr);               /* free memory for event struct   */
    }

    printf("k=%d\n", k);
    for (int i = 0; i < v; i++)
    {
      int src = traffic[i][0];
      int dst = traffic[i][1];
      int n_pkts = traffic[i][2];
      struct route *rt;
      rt = (struct route *)malloc(sizeof(struct route));
      route(src, dst, rt); /* route  pkts*/
      print_route(src, dst, n_pkts, rt);
      lcupdate(rt, n_pkts);
    }

    for (int i = 0; i < num_nodes; i++) /* detect change */
    {
      if (has_changed(i))
        send2neighbors(i);
    }

    free(dt);
    cpy_matrix(&dt, dt_new); /* sets dt = copy of dt_new */
    free(dt_new);
    free(lc);
    cpy_matrix(&lc, lc_new);
    free(lc_new);
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
    if (dt[node][i] != dt_new[node][i] || lc[node][i] != lc_new[node][i])
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

void init_lc_new()
{
  lc_new = (int **)malloc(num_nodes * sizeof(int *));
  for (int i = 0; i < num_nodes; i++)
  {
    lc_new[i] = (int *)malloc(num_nodes * sizeof(int));
    for (int j = 0; j < num_nodes; j++)
    {
      if (lc[i][j] == -1)
        lc_new[i][j] = -1;
      else
        lc_new[i][j] = 0;
    }
  }
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

void lcupdate(struct route *rt, int n_pkts)
{
  if (rt->has_cycle) /* drop the traffic */
    return;
  for (int i = 0; i < rt->length - 1; i++)
  {
    int current_hop = rt->path[i];
    int next_hop = rt->path[i + 1];
    lc_new[current_hop][next_hop] += n_pkts;
    lc_new[next_hop][current_hop] += n_pkts;
  }
}

void route(int src, int dst, struct route *rt)
{
  rt->has_cycle = 0;
  rt->length = -1;
  if (dt_new[src][dst] == -1) /* not reachable */
    return;

  int hop = src;
  int *path = (int *)malloc(num_nodes * sizeof(int));
  path[0] = hop;
  rt->length = 1;

  while (hop != dst)
  {
    int min_cost = -1;
    int next_hop = -1;
    for (int i = 0; i < num_nodes; i++) /* implicitly choose the smallest node index */
    {
      int cost_hop_i = lc[hop][i];
      int cost_i_dst = dt[i][dst]; // ;)
      int cost = cost_hop_i + cost_i_dst;
      if (are_neighbors(hop, i) && cost_i_dst != -1)
      {
        if (min_cost == -1 || cost < min_cost)
        {
          min_cost = cost;
          next_hop = i;
        }
        if (cost == min_cost && i == dst) /* pick the destination */
        {
          next_hop = i;
        }
      }
    }
    for (int i = 0; i < rt->length; i++)
    {
      if (next_hop == path[i]) /* path is a cycle */
      {
        rt->has_cycle = 1;
        break;
      }
    }
    hop = next_hop;
    path[rt->length] = hop;
    rt->length++;
    if (rt->has_cycle)
      break;
  }

  rt->path = (int *)malloc(rt->length * sizeof(int));
  for (int i = 0; i < rt->length; i++)
    rt->path[i] = path[i];
  free(path);
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

  char *path_traffic = argv[3];
  v = r_file(path_traffic, 3, v_max, &traffic);
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

void print_lc()
{
  printf("LC k=%d:\n", k);
  for (int i = 0; i < num_nodes; i++)
  {
    for (int j = 0; j < num_nodes - 1; j++)
    {
      int cost = lc[i][j];
      printf("%d ", cost);
    }
    printf("%d\n", lc[i][num_nodes - 1]);
  }
}

void print_route(int src, int dst, int n_pkts, struct route *rt)
{
  printf("%d %d %d ", src, dst, n_pkts);
  if (rt->length == -1)
  {
    printf("null");
    return;
  }
  for (int i = 0; i < rt->length - 1; i++)
    printf("%d>", rt->path[i]);
  if (rt->has_cycle)
    printf("%d(drop)\n", rt->path[rt->length - 1]);
  else
    printf("%d\n", rt->path[rt->length - 1]);
}