#ifndef _node
#define _node

#include <new>
#include <string>
#include <vector>
#include <iostream>
#include <deque>


class RoutingMessage;
class Table;
class Link;
class SimulationContext;

#include "table.h"

using namespace std;

class Node {
 private:
  unsigned number;
  SimulationContext    *context;
  double   bw;
  double   lat;
  bool     visited;
#if defined(DISTANCEVECTOR)
  Table table;
#endif

 public:
  Node(const unsigned n, SimulationContext *c, double b, double l);
  Node();
  Node(const Node &rhs);
  Node & operator=(const Node &rhs);
  virtual ~Node();

  virtual bool Matches(const Node &rhs) const;

  virtual void SetNumber(const unsigned n);
  virtual unsigned GetNumber() const;

  virtual void SetLatency(const double l);
  virtual double GetLatency() const;
  virtual void SetBW(const double b);
  virtual double GetBW() const;

  virtual void SendToNeighbors(const RoutingMessage *m);
  virtual void SendToNeighbor(const Node *n, const RoutingMessage *m);
  virtual deque<Node*> *GetNeighbors();
  virtual void SetTimeOut(const double timefromnow);

#if defined(DISTANCEVECTOR)
  void UpdatesFromNeighbors();
#endif

  //
  // Students will WRITE THESE
  //
  virtual void LinkHasBeenUpdated(const Link *l);
  virtual void ProcessIncomingRoutingMessage(const RoutingMessage *m);
  virtual void TimeOut();
  virtual Node *GetNextHop(const Node *destination) const;
  virtual Table *GetRoutingTable() const;

  virtual ostream & Print(ostream &os) const;

#if defined(LINKSTATE)
  //static variables

  unsigned number_of_nodes;
  deque<unsigned> list_of_node_nums; 
  deque<Node*> list_of_nodes;
  Table mytable;
  Table *GetRoutingTable();
  void GetAllNodes();
  //Node* nexthop;
  void Djistras();
  void modifyTable(Node* root, vector<unsigned> preds_vec, vector<double> costs_vec, vector<bool> visited_vec);

#endif
};
inline ostream & operator<<(ostream &os, const Node &n) { return n.Print(os);}


#endif