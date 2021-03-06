#ifndef _table
#define _table


#include <iostream>
#include <string>
#include <vector>

using namespace std;

#if defined(GENERIC)
class Table {
  // Students should write this class

 public:
  ostream & Print(ostream &os) const;
};
#endif


#if defined(LINKSTATE)
// Djistra's Algorithm
class Table {
 vector<unsigned> preds; 
 vector<double> costs; 
 vector<bool> visited;
 public:
 vector<double> getCosts();
 vector<unsigned> getPreds();
 vector<bool> getVisited();
 void setCosts(vector<double> updatedCosts);
 void setPreds(vector<unsigned> updatedPreds);
 void setVisited(vector<bool> updatedVisited);
 Table(const unsigned size);
 Table(){};
 ostream & Print(ostream &os) const;
};
#endif

#if defined(DISTANCEVECTOR)

#include <deque>

// Each RoutingPath is the shortest path to the destination node that's
// not a direct neighbor.
struct RoutingPath {
	unsigned destination; // destination node
	unsigned next_node; // node to immediately forward to 
	double cost;
	ostream & Print(ostream &os) const;
	RoutingPath(unsigned dest, unsigned next, double c);
};

inline ostream & operator<<(ostream &os, const RoutingPath &e) { return e.Print(os);}

class Table {
private:
	deque<RoutingPath> contents;
public:
	deque<RoutingPath> GetRoutingPaths();
	deque<RoutingPath>::iterator GetDestinationRoutingPath(unsigned);
	RoutingPath* GetRoutingPath(unsigned);
	void EditRoutingPath(unsigned, RoutingPath);
	ostream & Print(ostream &os) const;
};
#endif

inline ostream & operator<<(ostream &os, const Table &t) { return t.Print(os);}

#endif