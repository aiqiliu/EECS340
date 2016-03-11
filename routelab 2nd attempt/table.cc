#include "table.h"
#include "node.h"
#include "link.h"

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

Table::vector<double> Table::getCosts() {
    return costs;
}

Table::vector<unsigned> Table::getPreds() {
    return preds;
}

Table::vector<bool> Table::getVisited() {
    return visited;
}

Table::deque<Link*> getOutLinks() {
    return out_links;
}

Table::deque<Node*> getAllNodes() {
    return all_nodes;
}

Table::void addLink(Link* l) {
    out_links.push_back(l);
}

Table::void addNode(Node* n) {
    all_nodes.push_back(n);
}

Table::ostream & Table::Print(ostream &os) const
{
  for(unsigned i = 1;i<preds.size(); i++)
      os << "Node: "<< i << "Predecessor: " << preds[i] << "Cost: " << costs[i] << endl;
  return os;
}

#endif

#if defined(DISTANCEVECTOR)


#endif
