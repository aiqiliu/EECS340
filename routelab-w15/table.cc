#include "table.h"

#include <iostream>
#include <string>
#include <vector>

using namespace std;

#if defined(GENERIC)
ostream & Table::Print(ostream &os) const
{
  // WRITE THIS
  os << "Table()";
  return os;
}
#endif

#if defined(LINKSTATE)

Table::Table(unsigned size){
preds.assign(size+1, size +1);
costs.assign(size+1, 100000000.0);
visited.assign(size+1, false);
}
vector<double> Table::getCosts(){
    return costs;
}

vector<unsigned> Table::getPreds(){
    return preds;
}
vector<int> Table::Visited(){
    return visited;
}

void Table::setCosts(vector<double> updatedCosts){
    costs = updatedCosts;
}

void Table::setPreds(vector<unsigned> updatedPreds){
    preds = updatedPreds;
}
void Table::setVisited(vector<bool> updatedPreds){
    preds = updatedPredsVisited;
}

ostream & Table::Print(ostream &os) const
{
  for(int i = 1;i<preds.size(); i++)
      os << "Node: "<< i << "Predecessor: " << preds[i] << "Cost: " << costs[i] << endl;
  return os;
}
#endif

#if defined(DISTANCEVECTOR)
RoutingPath::RoutingPath(unsigned dest, unsigned next, double c) :
	destination(dest), next_node(next), cost(c)
{}

ostream & RoutingPath::Print(ostream &os) const
{
	os << "RoutingPath(dest=" << destination << ", next=" << next_node << ", cost=" << cost << ")";
	return os;
}

deque<RoutingPath> Table::GetRoutingPaths() {
    return contents;
}

deque<RoutingPath>::iterator Table::GetDestinationRoutingPath(unsigned dest) {
    for(deque<RoutingPath>::iterator routing_path = contents.begin(); routing_path != contents.end(); routing_path++){
      if(routing_path->destination == dest){
        return routing_path;
      } 
    }
    return contents.end();
}

RoutingPath* Table::GetRoutingPath(unsigned dest) {
  deque<RoutingPath>::iterator routing_path = GetDestinationRoutingPath(dest);
  if (routing_path != contents.end()) 
  {
    RoutingPath* res = new RoutingPath(routing_path->destination, routing_path->next_node, routing_path->cost);
    return res;
  } 
  else 
  {
    return NULL;
  }
}

void Table::EditRoutingPath(unsigned dest, RoutingPath new_routing_path) {
    deque<RoutingPath>::iterator routing_path = GetDestinationRoutingPath(dest);
    if (routing_path != contents.end()) {
      routing_path->destination = new_routing_path.destination;
      routing_path->next_node = new_routing_path.next_node;
      routing_path->cost = new_routing_path.cost;
    }
    else {
      contents.push_back(new_routing_path);
    }
}

ostream & Table::Print(ostream &os) const
{
  os << "Table(routing paths={";
  for (deque<RoutingPath>::const_iterator routing_path = contents.begin(); routing_path != contents.end(); routing_path++) { 
    os << *routing_path << ", ";
  }
  os << "})";
  return os;
}

#endif