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
 void setCosts(vector<int> updatedCosts);
 void setPreds(vector<unsigned> updatedPreds);
 void setVisited(vector<bool> updatedVisited);
 Table(const unsigned size);
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