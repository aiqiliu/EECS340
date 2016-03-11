#ifndef _table
#define _table


#include <iostream>

using namespace std;

#if defined(GENERIC)
class Table {
  // Students should write this class

 public:
  ostream & Print(ostream &os) const;
};
#endif


#if defined(LINKSTATE)
class Table {
  // Students should write this class
 	vector<unsigned> preds; 
 	vector<double> costs; 
 	vector<bool> visited;
 	deque<Link*> out_links;
 	deque<Node*> all_nodes;
 public:
 	vector<double> getCosts() ;
 	vector<unsigned> getPreds() ;
 	vector<bool> getVisited() ;
 	deque<Link*> getOutLinks();
 	// void setCosts(vector<double> updatedCosts);
 	// void setPreds(vector<unsigned> updatedPreds);
 	// void setVisited(vector<bool> updatedVisited);
 	// Table(const unsigned size);
 	Table(){};
    ostream & Print(ostream &os) const;
};
#endif

#if defined(DISTANCEVECTOR)

#include <deque>

class Table {
 public:
  ostream & Print(ostream &os) const;
};
#endif

inline ostream & operator<<(ostream &os, const Table &t) { return t.Print(os);}

#endif
