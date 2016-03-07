#include "node.h"
#include "context.h"
#include "error.h"
#include "table.h"
#include <deque>


Node::Node(const unsigned n, SimulationContext *c, double b, double l) : 
    number(n), context(c), bw(b), lat(l) 
{}

Node::Node() 
{ throw GeneralException(); }

Node::Node(const Node &rhs) : 
  number(rhs.number), context(rhs.context), bw(rhs.bw), lat(rhs.lat) {}

Node & Node::operator=(const Node &rhs) 
{
  return *(new(this)Node(rhs));
}

void Node::SetNumber(const unsigned n) 
{ number=n;}

unsigned Node::GetNumber() const 
{ return number;}

void Node::SetLatency(const double l)
{ lat=l;}

double Node::GetLatency() const 
{ return lat;}

void Node::SetBW(const double b)
{ bw=b;}

double Node::GetBW() const 
{ return bw;}

Node::~Node()
{}

// Implement these functions  to post an event to the event queue in the event simulator
// so that the corresponding node can recieve the ROUTING_MESSAGE_ARRIVAL event at the proper time
void Node::SendToNeighbors(const RoutingMessage *m)
{
}

void Node::SendToNeighbor(const Node *n, const RoutingMessage *m)
{

}

deque<Node*> *Node::GetNeighbors()
{
  return context->GetNeighbors(this);
}

void Node::SetTimeOut(const double timefromnow)
{
  context->TimeOut(this,timefromnow);
}


bool Node::Matches(const Node &rhs) const
{
  return number==rhs.number;
}


#if defined(GENERIC)
void Node::LinkHasBeenUpdated(const Link *l)
{
  cerr << *this << " got a link update: "<<*l<<endl;
  //Do Something generic:
  SendToNeighbors(new RoutingMessage);
}


void Node::ProcessIncomingRoutingMessage(const RoutingMessage *m)
{
  cerr << *this << " got a routing messagee: "<<*m<<" Ignored "<<endl;
}


void Node::TimeOut()
{
  cerr << *this << " got a timeout: ignored"<<endl;
}

Node *Node::GetNextHop(const Node *destination) const
{//consult the table.get the 
  return 0;
}

Table *Node::GetRoutingTable() const
{
  return new Table;
}


ostream & Node::Print(ostream &os) const
{
  os << "Node(number="<<number<<", lat="<<lat<<", bw="<<bw<<")";
  return os;
}

#endif

#if defined(LINKSTATE)
Table dRoute;
int static Node::number_of_nodes; //might need to create helper functions to get these values since they are private
deque<int> static Node::list_of_node_nums; 

// function to get all nodes
void Node::GetAllNodes()
{
  cerr << "Getting all nodes in the network" << endl;
  deque<Node*> unvisited_nodes = GetNeighbors();
  deque<Node*> visited_nodes;
  list_of_node_nums.push_back(this->GetNumber()); //add this node's number to list_of_node_nums
  number_of_nodes++; //adds one to the number_of_nodes
  visited_nodes.push_back(this);
  while (!unvisited_nodes.empty()) //while unvisited nodes are empty
  {
    for(deque<Node*>::iterator curr_unvisited=unvisited_nodes.begin(); curr_unvisited!=unvisited_nodes.end(); curr_unvisited++) //loop through unvisited nodes
    {
      if(std::find(list_of_node_nums.begin(), list_of_node_nums.end(), curr_unvisited->GetNumber()) == list_of_node_nums.end()) //if current unvisited node's number is not in list_of_nodes_nums
      {
        list_of_node_nums.push_back(curr_unvisited->GetNumber()); //add current unvisited neighbor's number to list_of_node_nums
        number_of_nodes++; //increment number_of_nodes counter by one
      }
      deque<Node*> curr_neighbors = curr_unvisited->GetNeighbors(); //get neighbors of current node
      for(deque<Node*>::iterator neighbor=curr_neighbors.begin(); neighbor!=curr_neighbors.end(); neighbor++) //loop through neighbors of current neighbor
      {
        if(std::find(unvisited_nodes.begin(), unvisited_nodes.end(), neighbor) == unvisited_nodes.end()) //if neighbor is not in unvisited_nodes 
        {
          if(std::find(visited_nodes.begin(), visited_nodes.end(), neighbor) == visited_nodes.end()) //if neighbor is not in visited_nodes
          {
            unvisited_nodes.push_back(&(*neighbor); //add a pointer of neighbor to unvisited_nodes
          }
        }
      }
      visited_nodes.push_back(&(*curr_unvisited)); //add current unvisited node to visited nodes
      unvisited_nodes.erase(&(*curr_unvisited)); //erase current unvisited node from unvisited nodes
    }
  }
  cerr << number_of_nodes << " nodes found in the network." << endl;
}


void Node::LinkHasBeenUpdated(const Link *l)
{
  cerr << *this<<": Link Update: "<<*l<<endl;
}


void Node::ProcessIncomingRoutingMessage(const RoutingMessage *m)
{
  cerr << *this << " Routing Message: "<<*m;
}

void Node::TimeOut()
{
  cerr << *this << " got a timeout: ignored"<<endl;
}

Node *Node::GetNextHop(const Node *destination) const
{
  // WRITE
  return 0;
}

Table *Node::GetRoutingTable() const
{
  // WRITE
  Table routingTable = dRoute;
  return routingTable;
}


void Djistras(const Node* root){
  unsigned int neighbor_num;
  unsigned int srcIndex = root.GetNumber();
  unsigned int destIndex;
  double linkCost, nodeLatency;

  vector<int> preds_vec = dRoute.getPreds();
  vector<int> costs_vec = dRoute.getCosts();
  // initializing the values for the root to be 0 
  preds_vec[srcIndex] = srcIndex;
  costs_vec[srcIndex] = 0;
  root -> SetVisited(true);
  root->SetLatency(0.0);

  Topology dTopology;

  deque<Node*> myneighbors = dTopology.GetNeighbors(root)
  deque<Link*> outLinks = dTopology.GetOutgoingLinks(root);
  //Link* currLink = outLinks.front();
  for(deque<Link*>::iterator currLink = outLinks.begin(); currLink!= outLinks.end(); currLink++){

    destIndex = (*currLink) -> GetDest();
    linkCost = (*currLink) -> GetLatency();

    nodeLatency = root -> GetLatency();
    newCost = nodeLatency + linkCost;

     // for this link I need its destn which would give me the the node which would give me its number (i.e index) amd its 
    // latency which would be used to modify the costs vector
    // once we get the number that would be the index to modify both the arrays and the pred array will get srcIndex
    
    // if(costs_vec[destIndex] == 100000000){
    //   preds_vec[destIndex] = srcIndex;
    //   costs_vec[destIndex] = newCost;
    // }

    // else 
    if(newCost < costs_vec[destIndex]){
      preds_vec[destIndex] = srcIndex;
      costs_vec[destIndex] = newCost;
      
    }

  }
  // Now we need to find the min value in this deque
  int min_value = 10000000000000;
  int min_index = 0;
  
  for(int i = 0; i< costs_vec.size(); i++){
    // if(any of my neighbors are unvisited then)
    if(costs_vec[i] < min_value){
      min_value = costs_vec[i];
      min_index = i;
    }
  }



}

ostream & Node::Print(ostream &os) const
{
  os << "Node(number="<<number<<", lat="<<lat<<", bw="<<bw<<")";
  return os;
}

#endif


#if defined(DISTANCEVECTOR)


void Node::LinkHasBeenUpdated(const Link *l)
{
  // update our table
  // send out routing mesages
  cerr << *this<<": Link Update: "<<*l<<endl;
}


void Node::ProcessIncomingRoutingMessage(const RoutingMessage *m)
{

}

void Node::TimeOut()
{
  cerr << *this << " got a timeout: ignored"<<endl;
}


Node *Node::GetNextHop(const Node *destination) const
{
}

Table *Node::GetRoutingTable() const
{
}


ostream & Node::Print(ostream &os) const
{
  os << "Node(number="<<number<<", lat="<<lat<<", bw="<<bw;
  return os;
}
#endif