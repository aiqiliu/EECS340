#include "table.h"

#include <new>
#include <string>
#include <vector>
#include <iostream>
#include <deque>

#if defined(GENERIC)
ostream & Table::Print(ostream &os) const
{
  // WRITE THIS
  os << "Table()";
  return os;
}
#endif

#if defined(LINKSTATE)

Table::Table() {
	preds.assign(10000, 100000000);
	costs.assign(10000, 100000000.0);
	visited.assign(10000, false);
	out_links(0);
 	all_nodes(0);
}

vector<double> Table::getCosts() {
    return costs;
}

vector<unsigned> Table::getPreds() {
    return preds;
}
vector<bool> Table::getVisited() {
    return visited;
}

deque<Link*> getOutLinks() {
	return out_links;
}

ostream & Table::Print(ostream &os) const
{
  for(unsigned i = 1;i<preds.size(); i++)
      os << "Node: "<< i << "Predecessor: " << preds[i] << "Cost: " << costs[i] << endl;
  return os;
}

#endif

#if defined(DISTANCEVECTOR)


#endif
