#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include <iostream>
#include <algorithm>

#include "Minet.h"
#include "tcpstate.h"


using std::cout;
using std::endl;
using std::cerr;
using std::string;
using std::cin;
using std::min;
using std::max;


//          ~~~~~ MACROS ~~~~~

#define MAX_TRIES 5
#define MSS 536
#define TIMEOUT 10
#define GBN MSS*16
#define RTT 5

// CHANGE THIS TOO
#define RECV_BUF_SIZE(state) (state.TCP_BUFFER_SIZE - state.RecvBuffer.GetSize()) //JUST USE GetRwnd(). A member function of a state
#define SEND_BUF_SIZE(state) (state.TCP_BUFFER_SIZE - state.SendBuffer.GetSize())


//           ~~~~~ HELPER ~~~~~

//helper function to make a packet
Packet MakePacket(Buffer data, Connection conn, unsigned int seq_n, unsigned int ack_n, size_t win_size, unsigned char flag)
{
  // Make Packet
  unsigned size = MIN_MACRO(IP_PACKET_MAX_LENGTH-TCP_HEADER_MAX_LENGTH, data.GetSize());
  Packet send_pack(data.ExtractFront(size));

  // Make and push IP header
  IPHeader sendIPheader;
  sendIPheader.SetProtocol(IP_PROTO_TCP);
  sendIPheader.SetSourceIP(conn.src);
  sendIPheader.SetDestIP(conn.dest);
  sendIPheader.SetTotalLength(size + TCP_HEADER_BASE_LENGTH + IP_HEADER_BASE_LENGTH);
  send_pack.PushFrontHeader(sendIPheader);

  // Make and push TCP header
  TCPHeader sendTCPheader;
  sendTCPheader.SetSourcePort(conn.srcport, send_pack);
  sendTCPheader.SetDestPort(conn.destport, send_pack);
  sendTCPheader.SetHeaderLen(TCP_HEADER_BASE_LENGTH/4, send_pack);
  sendTCPheader.SetFlags(flag, send_pack);
  sendTCPheader.SetWinSize(win_size, send_pack); // to fix
  sendTCPheader.SetSeqNum(seq_n, send_pack);
  if (IS_ACK(flag))
  {
    sendTCPheader.SetAckNum(ack_n, send_pack);
  }
  sendTCPheader.RecomputeChecksum(send_pack);
  send_pack.PushBackHeader(sendTCPheader);

  cerr << "*MAKING PACKET*" << endl;
  cerr << sendIPheader << endl;
  cerr << sendTCPheader << endl;
  cerr << send_pack << endl;

  return send_pack;
}

//helper function to make packet and send it using Minet
void SendPacket(MinetHandle handle, Buffer data, Connection conn, unsigned int seq_n, unsigned int ack_n, size_t win_size, unsigned char flag)
{
  Packet pack = MakePacket(data, conn, seq_n, ack_n, win_size, flag); // ack
  MinetSend(handle, pack);
}

//helper function to receive a packet from a handle using minet and save it to a packet
Packet ReceivePacket(MinetHandle handle) 
{
  Packet pack;
  MinetReceive(handle, pack);
  return pack;
}

//         ~~~~~ MAIN ~~~~~                

int main(int argc, char *argv[])
{
  MinetHandle mux, sock;
  srand(time(NULL)); // generate seeds
  ConnectionList<TCPState> connect_list;

  MinetInit(MINET_TCP_MODULE);

  mux = MinetIsModuleInConfig(MINET_IP_MUX) ? MinetConnect(MINET_IP_MUX) : MINET_NOHANDLE;
  sock = MinetIsModuleInConfig(MINET_SOCK_MODULE) ? MinetAccept(MINET_SOCK_MODULE) : MINET_NOHANDLE;

  if (MinetIsModuleInConfig(MINET_IP_MUX) && mux == MINET_NOHANDLE) 
  {
    MinetSendToMonitor(MinetMonitoringEvent("Can't connect to mux"));
    return -1;
  }

  if (MinetIsModuleInConfig(MINET_SOCK_MODULE) && sock == MINET_NOHANDLE) 
  {
    MinetSendToMonitor(MinetMonitoringEvent("Can't accept from sock module"));
    return -1;
  }

  MinetSendToMonitor(MinetMonitoringEvent("tcp_module handling TCP traffic"));

  MinetEvent event;

  while (MinetGetNextEvent(event, TIMEOUT) == 0) 
  {

    if (event.eventtype == MinetEvent::Timeout)
    {
      //loop through all the connections in the connections list
      for (ConnectionList<TCPState>::iterator cxn = connect_list.begin(); cxn != connect_list.end(); cxn++)
      {
        //if connection is closed, erase it from the connections list
        if (cxn->state.GetState() == CLOSED)
        {
          connect_list.erase(cxn);
        }

        // check for active timers
        Time curr_time = Time();
        if (cxn->bTmrActive == true && cxn->timeout < curr_time)
        {
          // CAN GET RID OF THIS?
          // if there are no more timer tries for this state
          if (cxn->state.ExpireTimerTries()) //true if no more timer tries
          {
            //closes connection if the number of time outs reaches a threshold
            SockRequestResponse res;
            res.type = CLOSE;
            res.connection = cxn->connection;
            res.error = EOK;
            MinetSend(sock, res);
          }
          // else handle each case of timeout
          else
          {
            Packet send_pack;
            unsigned char send_flag;
            switch(cxn->state.GetState())
            {
              case SYN_RCVD:
              {
                cerr << "TIMEOUT: SYN_RCVD - SEND SYN ACK" << endl;
                SET_SYN(send_flag);
                SET_ACK(send_flag);
                //MAKING PACKET BUT NOT SENDING IT!!!
                //MakePacket(Buffer(NULL, 0), cxn->connection, cxn->state.GetLastSent(), cxn->state.GetLastRecvd(), RECV_BUF_SIZE(cxn->state), send_flag);
                SendPacket(mux, Buffer(NULL, 0), cxn->connection, cxn->state.GetLastSent(), cxn->state.GetLastRecvd(), RECV_BUF_SIZE(cxn->state), send_flag);
              }
              break;
              case SYN_SENT:
              {
                cerr << "TIMEOUT: SYN_SENT - SEND SYN" << endl;
                SET_SYN(send_flag);
                //MAKING PACKET BUT NOT SENDING IT!!!
                //MakePacket(Buffer(NULL, 0), cxn->connection, cxn->state.GetLastSent(), cxn->state.GetLastRecvd(), SEND_BUF_SIZE(cxn->state), send_flag); 
                SendPacket(mux, Buffer(NULL, 0), cxn->connection, cxn->state.GetLastSent(), cxn->state.GetLastRecvd(), SEND_BUF_SIZE(cxn->state), send_flag);
              } 
              break;
              case ESTABLISHED:
              {
                cerr << "TIMEOUT: ESTABLISHED - SEND DATA" << endl;
                // use GBN to resend data
                if (cxn->state.N > 0)
                {
                  cerr << "TIMEOUT: ESTABLISHED - SEND DATA - GBN" << endl;

                  unsigned int inflight_n = cxn->state.GetN();
                  unsigned int rwnd = cxn->state.GetRwnd(); // receiver congestion window
                  size_t cwnd = cxn->state.SendBuffer.GetSize(); // sender congestion window

                  Buffer data;
                  while(inflight_n < GBN && cwnd != 0 && rwnd != 0)
                  {
                    cerr << "\n inflight_n: " << inflight_n << endl;
                    cerr << "\n rwnd: " << rwnd << endl;
                    cerr << "\n cwnd: " << cwnd << endl;
                    send_flag = 0;

                    // if MSS < rwnd and MSS < cwnd
                    // space in rwnd and cwnd
                    if(MSS < rwnd && MSS < cwnd)
                    {
                      cerr << "space in rwnd and cwnd" << endl;
                      data = cxn->state.SendBuffer.Extract(inflight_n, MSS);

                      inflight_n = inflight_n + MSS;
                      CLR_SYN(send_flag);
                      SET_ACK(send_flag);
                      send_pack = MakePacket(data, cxn->connection, cxn->state.GetLastSent(), cxn->state.GetLastRecvd() + 1, SEND_BUF_SIZE(cxn->state), send_flag);

                      cerr << "Last last sent: " << cxn->state.GetLastSent() << endl;
                      cxn->state.SetLastSent(cxn->state.GetLastSent() + MSS);
                      cerr << "Last sent: " << cxn->state.GetLastSent() << endl;
                    }

                    // else space in cwnd or rwnd
                    else
                    {
                      cerr << "space in either or" << endl;
                      data = cxn->state.SendBuffer.Extract(inflight_n, min((int)rwnd, (int)cwnd));

                      inflight_n = inflight_n + min((int)rwnd, (int)cwnd);
                      CLR_SYN(send_flag);
                      SET_ACK(send_flag);
                      send_pack = MakePacket(data, cxn->connection, cxn->state.GetLastSent(), cxn->state.GetLastRecvd() + 1, SEND_BUF_SIZE(cxn->state), send_flag);
                      cxn->state.SetLastSent(cxn->state.GetLastSent() + min((int)rwnd, (int)cwnd));
                    }

                    MinetSend(mux, send_pack);

                    if ((rwnd < rwnd - inflight_n) && (cwnd < cwnd - inflight_n)) 
                    {
                      break;
                    }
                    else
                    {
                      rwnd = rwnd - inflight_n;
                      cwnd = cwnd - inflight_n;                
                    }

                    cerr << "\n inflight_n: " << inflight_n << endl;
                    cerr << "rwnd: " << rwnd << endl;
                    cerr << "cwnd: " << cwnd << endl;

                    // set timeout
                    cxn->bTmrActive = true;
                    cxn->timeout = Time() + RTT;
                  }


                  cxn->state.N = inflight_n;

                }
                // otherwise just need to resend ACK
                else
                {
                  cerr << "TIMEOUT: ESTABLISHED - SEND DATA - SEND ACK" << endl;
                  SET_ACK(send_flag);
                  MakePacket(Buffer(NULL, 0), cxn->connection, cxn->state.GetLastSent(), cxn->state.GetLastRecvd() + 1, RECV_BUF_SIZE(cxn->state), send_flag);
                }
              }
              break;
              case CLOSE_WAIT:
              {
                cerr << "TIMEOUT: CLOSE_WAIT - SEND ACK" << endl;
                SET_ACK(send_flag);
                MakePacket(Buffer(NULL, 0), cxn->connection, cxn->state.GetLastSent(), cxn->state.GetLastRecvd(), RECV_BUF_SIZE(cxn->state), send_flag);
              }
              break;
              case FIN_WAIT1:
              {
                cerr << "TIMEOUT: FIN_WAIT1 - SEND FIN" << endl;
                SET_FIN(send_flag);
                MakePacket(Buffer(NULL, 0), cxn->connection, cxn->state.GetLastSent(), cxn->state.GetLastRecvd(), SEND_BUF_SIZE(cxn->state), send_flag);
              }
              break;
              case CLOSING:
              {
                cerr << "TIMEOUT: CLOSING - SEND ACK" << endl;
                SET_ACK(send_flag);
                MakePacket(Buffer(NULL, 0), cxn->connection, cxn->state.GetLastSent(), cxn->state.GetLastRecvd(), RECV_BUF_SIZE(cxn->state), send_flag);
              }
              break;
              case LAST_ACK:
              {
                cerr << "TIMEOUT: LAST_ACK - SEND FIN" << endl;
                SET_FIN(send_flag);
                MakePacket(Buffer(NULL, 0), cxn->connection, cxn->state.GetLastSent(), cxn->state.GetLastRecvd(), SEND_BUF_SIZE(cxn->state), send_flag);
              }
              break;
              case TIME_WAIT:
              {
                cerr << "TIMEOUT: TIME_WAIT - SEND ACK" << endl;
                SET_ACK(send_flag);
                MakePacket(Buffer(NULL, 0), cxn->connection, cxn->state.GetLastSent(), cxn->state.GetLastRecvd(), RECV_BUF_SIZE(cxn->state), send_flag);
              }
            }
            MinetSend(mux, send_pack);
            cxn->timeout = Time() + RTT;
          }
        }
      }
    }
    // Unexpected event type, signore
    else if (event.eventtype != MinetEvent::Dataflow || event.direction != MinetEvent::IN)
    {
      MinetSendToMonitor(MinetMonitoringEvent("Unknown event ignored."));
    } 
    //  Data from the IP layer below 
    else if (event.handle == mux) 
    {
      cerr << "\n === IP LAYER START === \n";

      cerr << "  == GOT A PACKET ==\n";

      Packet rec_pack = ReceivePacket(mux);
      // MinetReceive(mux, rec_pack);

      unsigned tcphlen = TCPHeader::EstimateTCPHeaderLength(rec_pack);
      rec_pack.ExtractHeaderFromPayload<TCPHeader>(tcphlen);

      IPHeader rec_ip_h = rec_pack.FindHeader(Headers::IPHeader);
      TCPHeader rec_tcp_h = rec_pack.FindHeader(Headers::TCPHeader);

      // cerr << rec_ip_h <<"\n";
      // cerr << rec_tcp_h << "\n";
      // cerr << "Checksum is " << (rec_tcp_h.IsCorrectChecksum(rec_pack) ? "VALID\n\n" : "INVALID\n\n");
      // cerr << rec_pack << "\n";

      // Unpack useful data
      Connection conn;
      // what's the connection class?
      rec_ip_h.GetDestIP(conn.src);
      rec_ip_h.GetSourceIP(conn.dest);
      rec_ip_h.GetProtocol(conn.protocol);
      rec_tcp_h.GetDestPort(conn.srcport);
      rec_tcp_h.GetSourcePort(conn.destport);

      unsigned short rwnd; // cpuld change it to int rwnd
      rec_tcp_h.GetWinSize(rwnd);

      unsigned int rec_seq_n;
      unsigned int rec_ack_n;
      unsigned int send_ack_n;
      unsigned int send_seq_n;
      rec_tcp_h.GetSeqNum(rec_seq_n);
      rec_tcp_h.GetAckNum(rec_ack_n);
      // Since ack_n denotes the next packet expected
      send_ack_n = rec_seq_n + 1;

      unsigned char rec_flag;
      rec_tcp_h.GetFlags(rec_flag);

      ConnectionList<TCPState>::iterator cxn = connect_list.FindMatching(conn);
      if (cxn != connect_list.end() && rec_tcp_h.IsCorrectChecksum(rec_pack))
      {   
        // cerr << "Found matching connection\n";
        // cerr << "Last Acked: " << cxn->state.GetLastAcked() << endl;
        // cerr << "Last Sent: " << cxn->state.GetLastSent() << endl;
        // cerr << "Last Recv: " << cxn->state.GetLastRecvd() << endl << endl;;
        rec_tcp_h.GetHeaderLen((unsigned char&)tcphlen); // what is the GetHeaderLen () ?
        tcphlen -= TCP_HEADER_MAX_LENGTH;
        Buffer data = rec_pack.GetPayload().ExtractFront(tcphlen); // getting the contents as long as tcphlen from the payload
        data.Print(cerr); // why?
        cerr << endl;

        unsigned char send_flag = 0; // SET_SYN function needs a parameter of type unsigned char which is then ORed with 10 (bitwise OR)
        SockRequestResponse res;
        Packet send_pack;

        switch(cxn->state.GetState())
        {
          case CLOSED:
          {
            cerr << "MUX: CLOSED STATE\n";
          }
          break;
          case LISTEN:
          {
            cerr << "\n=== MUX: LISTEN STATE ===\n";
            // coming from ACCEPT in socket layer
            if (IS_SYN(rec_flag))
            {
              send_seq_n = rand(); // As the sequence number for the packet sent back is randomly chosen
              //cerr << "generated seq: " << send_seq_n << endl;

              cxn->state.SetState(SYN_RCVD);
              cxn->state.SetLastRecvd(rec_seq_n);
             // cerr << "SET1: " << cxn->state.GetLastSent() << endl;
              cxn->state.SetLastSent(send_seq_n); 
              // cerr << "SET2: " << cxn->state.GetLastSent() << endl;

              cxn->bTmrActive = true; // bTmrActive is ??? To denote that the timer is now active
              cxn->timeout = Time() + RTT; // To set the timeout interval to be that of 1 RTT starting the current time. This sets a timeout for receiving an ACK from remote side.
              SET_SYN(send_flag);
              SET_ACK(send_flag);
              SendPacket(mux, Buffer(NULL, 0), conn, send_seq_n, send_ack_n, RECV_BUF_SIZE(cxn->state), send_flag);
            }
          }
          break;
          case SYN_RCVD:
          {
            cerr << "\n=== MUX: SYN_RCVD STATE ===\n";
            if (IS_ACK(rec_flag) && cxn->state.GetLastSent() == rec_ack_n - 1)
            {
              cxn->state.SetState(ESTABLISHED);
              cxn->state.SetLastRecvd(rec_seq_n - 1); // next will have same seq num

              // timer
              cxn->bTmrActive = false;
              cxn->state.SetTimerTries(MAX_TRIES);

              // create res to send to sock
              res.type = WRITE;
              res.connection = conn;
              res.bytes = 0;
              res.error = EOK;
              MinetSend(sock, res);
            }
          }
          break;
          case SYN_SENT:
          {
            cerr << "\n=== MUX: SYN_SENT STATE ===\n";
            if (IS_SYN(rec_flag) && IS_ACK(rec_flag))
            {
              cerr << "Last Acked: " << cxn->state.GetLastAcked() << endl;
              cerr << "Last Sent: " << cxn->state.GetLastSent() << endl;
              cerr << "Last Recv: " << cxn->state.GetLastRecvd() << endl;
              send_seq_n = cxn->state.GetLastSent() + data.GetSize() + 1;
              cerr << "increment" << data.GetSize() + 1;

              cxn->state.SetState(ESTABLISHED);
              cxn->state.SetLastAcked(rec_ack_n - 1);
              cxn->state.SetLastRecvd(rec_seq_n); // first data will be the same as this


              SET_ACK(send_flag);
              SendPacket(mux, Buffer(NULL, 0), conn, send_seq_n, send_ack_n, SEND_BUF_SIZE(cxn->state), send_flag);

              // NOTE: we do this for compatibility with our tcp_server testing, which seems to receives size 6 ACKs
              // from us even though the size is shown as zero above.
              cxn->state.SetLastSent(max((int) cxn->state.GetLastSent() + 7, (int) send_seq_n));

              res.type = WRITE;
              res.connection = conn;
              res.bytes = 0;
              res.error = EOK;
              MinetSend(sock, res);
            }
          }
          break;
          case SYN_SENT1:
          {
            cerr << "\n=== MUX: SYN_SENT1 STATE ===\n";
            // NO IMPLEMENTATION NEEDED
          }
          break;
          case ESTABLISHED:
          {
            cerr << "\n=== MUX: ESTABLISHED STATE ===\n";

            if (IS_FIN(rec_flag))
            { // part16. activate close
              cerr << "FIN flagged.\n";
              send_seq_n = cxn->state.GetLastSent() + data.GetSize() + 1;

              cxn->state.SetState(CLOSE_WAIT); //passive close
              cerr << "Last last sent: " << cxn->state.GetLastSent() << endl;
              cxn->state.SetLastSent(send_seq_n);
              cerr << "Last sent: " << cxn->state.GetLastSent() << endl;
              cxn->state.SetLastRecvd(rec_seq_n);

              SET_ACK(send_flag);
              SendPacket(mux, Buffer(NULL, 0), conn, send_seq_n, send_ack_n, RECV_BUF_SIZE(cxn->state), send_flag);
            }
            //else, is a dataflow packet
            else
            { 
              if (IS_ACK(rec_flag) && cxn->state.GetLastRecvd() < rec_seq_n)
              {
                cerr << "ACK is flagged." << endl;

                //packet has data
                if (data.GetSize() > 0)
                {
                  cerr << "The received packet has data" << endl;
                  cerr << "Recv: " << cxn->state.GetLastRecvd() << endl;
                
                  size_t recv_buf_size = RECV_BUF_SIZE(cxn->state);
                  //if there is an overflow of the received data
                  if (recv_buf_size < data.GetSize()) 
                  {
                    cxn->state.RecvBuffer.AddBack(data.ExtractFront(recv_buf_size)); //extract first n bits from data to recvbuffer
                    send_ack_n = rec_seq_n + recv_buf_size - 1;
                    cxn->state.SetLastRecvd(send_ack_n);
                  }
                  else //if there is no overflow
                  {
                    cxn->state.RecvBuffer.AddBack(data);
                    send_ack_n = rec_seq_n + data.GetSize() - 1;
                    cxn->state.SetLastRecvd(send_ack_n);
                  }

                  //grabbing the next sequence number to send 
                  send_seq_n = cxn->state.GetLastSent() + min(recv_buf_size, data.GetSize());
                  cxn->state.SetLastSent(send_seq_n);

                  //send an empty packet with ACK flag to mux to acknowledge the last received packet
                  SET_ACK(send_flag);
                  SendPacket(mux, Buffer(NULL, 0), conn, send_seq_n, send_ack_n + 1, RECV_BUF_SIZE(cxn->state), send_flag);

                  //create a socketrequestresponse to send to sock (its a write request to the socket)
                  res.type = WRITE;
                  res.connection = conn;
                  res.data = cxn->state.RecvBuffer; //send data in recvbuffer to sock
                  res.bytes = cxn->state.RecvBuffer.GetSize();
                  res.error = EOK;
                  MinetSend(sock, res);
                }
                else //packet has no data (WHY WOULD THAT EVER BE THE CASE?)
                {
                  cxn->state.SendBuffer.Erase(0, rec_ack_n - cxn->state.GetLastAcked() - 1);
                  cxn->state.N = cxn->state.N - (rec_ack_n - cxn->state.GetLastAcked() - 1);

                  cxn->state.SetLastAcked(rec_ack_n);
                  cxn->state.SetLastRecvd(rec_seq_n);
                  cxn->state.last_acked = rec_ack_n;


                  cerr << "\nSend Buffer: ";
                  cxn->state.SendBuffer.Print(cerr);
                  cerr << endl;

                  cxn->state.N = cxn->state.N - (rec_ack_n - cxn->state.GetLastAcked() - 1);
  
                  // send some of the information in the buffer if there is an overflow in the sendbuffer
                  if (cxn->state.SendBuffer.GetSize() - cxn->state.GetN() > 0)
                  {
                    //WHAT IS STATE.GETN() or STATE.N???
                    unsigned int inflight_n = cxn->state.GetN(); //packets in flight
                    unsigned int rwnd = cxn->state.GetRwnd(); //receiver congestion window
                    size_t cwnd = cxn->state.SendBuffer.GetSize(); //sender congestion window

                    while(inflight_n < GBN && cwnd != 0 && rwnd != 0) //GBN is a macro defined at the top
                    {
                      cerr << "\n inflight_n: " << inflight_n << endl;
                      cerr << "\n rwnd: " << rwnd << endl;
                      cerr << "\n cwnd: " << cwnd << endl;
                      send_flag = 0;

                      // if MSS < rwnd and MSS < cwnd
                      // space in rwnd and cwnd
                      if(MSS < rwnd && MSS < cwnd)
                      {
                        cerr << "There is still space in the receiver window and sender window" << endl; 
                        data = cxn->state.SendBuffer.Extract(inflight_n, MSS); //extract data of size MSS from send buffer (offset of # packets inflight)
                        // set new sequence number
                        // move on to the next set of packets
                        inflight_n = inflight_n + MSS;
                        CLR_SYN(send_flag);
                        SET_ACK(send_flag);
                        send_pack = MakePacket(data, cxn->connection, cxn->state.GetLastSent(), cxn->state.GetLastRecvd() + 1, SEND_BUF_SIZE(cxn->state), send_flag);

                        cerr << "Last last sent: " << cxn->state.GetLastSent() << endl;
                        cxn->state.SetLastSent(cxn->state.GetLastSent() + MSS); //adjust LastSent to account for the MSS just sent
                        cerr << "Last sent: " << cxn->state.GetLastSent() << endl;
                      }
                      // else there space is not enough space in sender window or receiver window
                      else
                      {
                        cerr << "Limited space in either sender window or receiver window" << endl;
                        data = cxn->state.SendBuffer.Extract(inflight_n, min((int)rwnd, (int)cwnd)); //extract data of size min(windows) from send buffer
                        // set new sequence number
                        // move on to the next set of packets
                        inflight_n = inflight_n + min((int)rwnd, (int)cwnd);
                        CLR_SYN(send_flag);
                        SET_ACK(send_flag);
                        send_pack = MakePacket(data, cxn->connection, cxn->state.GetLastSent(), cxn->state.GetLastRecvd() + 1, SEND_BUF_SIZE(cxn->state), send_flag);
                        cxn->state.SetLastSent(cxn->state.GetLastSent() + min((int)rwnd, (int)cwnd));
                      }

                      MinetSend(mux, send_pack); //send the packet to mus

                      if ((rwnd < rwnd - inflight_n) && (cwnd < cwnd - inflight_n)) // CAN GET RID OF THIS FIRST IF?
                      {
                        break;
                      }
                      else
                      {
                        rwnd = rwnd - inflight_n;
                        cwnd = cwnd - inflight_n;                
                      }

                      cerr << "\n inflight_n: " << inflight_n << endl;
                      cerr << "rwnd: " << rwnd << endl;
                      cerr << "cwnd: " << cwnd << endl;
                      // set timeout LOOK IN THIS TIMER THING
                      cxn->bTmrActive = true;
                      cxn->timeout = Time() + RTT;
                    }

                    cxn->state.N = inflight_n;
                  }
                  
                }
              }
            }
          }
          break;
          case SEND_DATA:
          {
            cerr << "\n=== MUX: SEND_DATA STATE ===\n";
            // NO IMPLEMENTATION NEEDED
          }
          break;
          case CLOSE_WAIT:
          {
            cerr << "\n=== MUX: CLOSE_WAIT STATE ===\n";
            if (IS_FIN(rec_flag))
            {
              // send a fin ack back
              send_seq_n = cxn->state.GetLastSent() + data.GetSize() + 1;

              cxn->state.SetState(LAST_ACK);
              cxn->state.SetLastRecvd(rec_seq_n);
              cerr << "Last last sent: " << cxn->state.GetLastSent() << endl;
              cxn->state.SetLastSent(send_seq_n);
              cerr << "Last sent: " << cxn->state.GetLastSent() << endl;

              // timeout stuff
              cxn->bTmrActive = true;
              cxn->timeout = Time() + RTT;
              cxn->state.SetTimerTries(MAX_TRIES);

              SET_FIN(send_flag);
              SendPacket(mux, Buffer(NULL, 0), conn, send_seq_n, send_ack_n, RECV_BUF_SIZE(cxn->state), send_flag);
            }
          }
          break;
          case FIN_WAIT1: //this state is after the client actively sent a FIN to the other user and is waiting for a response
          {
            cerr << "\n=== MUX: FIN_WAIT1 STATE ===\n";
            if (IS_FIN(rec_flag)) //if other user sends a FIN back, we close the connection
            {
              send_seq_n = cxn->state.GetLastSent() + data.GetSize() + 1;

              cxn->state.SetState(CLOSING); //set state to closing
              cxn->state.SetLastRecvd(rec_seq_n); //can replace these code with helper 
              cerr << "Last last sent: " << cxn->state.GetLastSent() << endl;
              cxn->state.SetLastSent(send_seq_n);
              cerr << "Last sent: " << cxn->state.GetLastSent() << endl;

              // set timeout
              cxn->bTmrActive = true;
              cxn->timeout = Time() + RTT;
              cxn->state.SetTimerTries(MAX_TRIES);

              SET_FIN(send_flag); 
              SET_ACK(send_flag); 
              SendPacket(mux, Buffer(NULL, 0), conn, send_seq_n, send_ack_n, SEND_BUF_SIZE(cxn->state), send_flag);
            }
            else if (IS_ACK(rec_flag)) //received an ACK back after first sending a FIN, so set state to FIN_WAIT2
            {

              cxn->state.SetState(FIN_WAIT2);
              cxn->state.SetLastSent(send_seq_n);
              cxn->state.SetLastAcked(rec_ack_n - 1);
            }
          }
          break;
          case CLOSING:
          {
            cerr << "\n=== MUX: CLOSING STATE ===\n";
            if (IS_ACK(rec_flag))
            {
              // done, not sending any other packets
              // move to time-wait
              cxn->state.SetState(TIME_WAIT);
              cxn->state.SetLastAcked(rec_ack_n - 1);
              cxn->state.SetLastRecvd(rec_seq_n);
            }
          }
          break;
          case LAST_ACK:
          { //start of sending the second fin
            cerr << "\n=== MUX: LAST_ACK STATE ===\n";
            if (IS_ACK(rec_flag))
            {
              cxn->state.SetState(CLOSED);
              cxn->state.SetLastAcked(rec_ack_n - 1);
              cxn->state.SetLastRecvd(rec_seq_n);
            }
          }
          break;
          case FIN_WAIT2: //waiting for a FIN from the server, and then will send an ACK back and change to time_wait state
          {
            cerr << "\n=== MUX: FIN_WAIT2 STATE ===\n";
            if (IS_FIN(rec_flag)) //if receive FIN from server, will send an ACK back to server and change to time_wait state
            {
              send_seq_n = cxn->state.GetLastSent() + data.GetSize() + 1;

              cxn->state.SetState(TIME_WAIT);
              cxn->state.SetLastRecvd(rec_seq_n);
              cxn->state.SetLastSent(send_seq_n);
              cxn->state.SetLastAcked(rec_ack_n - 1);
 
              // set timeout
              cxn->bTmrActive = true;
              cxn->timeout = Time() + RTT;
              cxn->state.SetTimerTries(MAX_TRIES);

              //send ACK back to server after receiving their FIN
              SET_ACK(send_flag); 
              SendPacket(mux, Buffer(NULL, 0), conn, send_seq_n, send_ack_n, SEND_BUF_SIZE(cxn->state), send_flag);
            }
          }
          break;
          case TIME_WAIT:
          {
            cerr << "\n=== MUX: TIME_WAIT STATE ===\n";
            cxn->timeout = Time() + 30;
            cxn->state.SetState(CLOSED);
          }
          break;
          default:
          {
            cerr << "\n=== MUX: DEFAULTED STATE ===\n";
          }
          break;
        }
      }
      // else there is no open connection
      else
      {
        cerr << "Could not find matching connection\n";
      }
      cerr << "\n === IP LAYER DONE === \n";
    }

    //  Data from the Sockets layer above  //
    else if (event.handle == sock) 
    {
      cerr << "\n === SOCK LAYER START === \n";
      SockRequestResponse req;
      SockRequestResponse res;
      MinetReceive(sock, req);
      Packet send_pack;
      unsigned char send_flag;
      cerr << "Received Socket Request:" << req << endl;

      switch(req.type)
      {
        case CONNECT:
        {
          // cerr << "\n=== SOCK: CONNECT ===\n";

          unsigned int init_seq = rand(); // Can make this a specific wierd value rather than the rand() function.
          TCPState connect_conn(init_seq, SYN_SENT, MAX_TRIES); //state is SYN_SENT
          connect_conn.N = 0; // number of packets allowed in flight
          ConnectionToStateMapping<TCPState> new_conn(req.connection, Time(), connect_conn, true); // sets properties for connection, timeout, state, timer
          connect_list.push_front(new_conn); // Add this new connection to the list of connections
         
          res.type = STATUS;
          res.error = EOK; 
          MinetSend(sock, res); //send an ok status to sock

          SET_SYN(send_flag);
          // Packet send_pack = MakePacket(Buffer(NULL, 0), new_conn.connection, init_seq, 0, SEND_BUF_SIZE(new_conn.state), send_flag);
          // MinetSend(mux, send_pack); //send a SYN packet to mux
          SendPacket(mux, Buffer(NULL, 0), new_conn.connection, init_seq, 0, SEND_BUF_SIZE(new_conn.state), send_flag);
          
          // cerr << "\n=== SOCK: END CONNECT ===\n";
        }
        break;
        case ACCEPT:
        {
          // passive open
          cerr << "\n=== SOCK: ACCEPT ===\n";

          TCPState acceptConnection(rand(), LISTEN, MAX_TRIES);
          acceptConnection.N = 0;
          ConnectionToStateMapping<TCPState> newConnection(req.connection, Time(), acceptConnection, false); //new connection with accept connection state
          connect_list.push_front(newConnection); //push newconnection to connection list
         
          res.type = STATUS;
          res.connection = req.connection;
          res.bytes = 0;
          res.error = EOK;
          MinetSend(sock, res); //send sockrequestresponse to sock

          cerr << "\n=== SOCK: END ACCEPT ===\n";
        }
        break;
        case WRITE: 
        {
          cerr << "\n=== SOCK: WRITE ===\n";

          ConnectionList<TCPState>::iterator cxn = connect_list.FindMatching(req.connection);
          if (cxn != connect_list.end() && cxn->state.GetState() == ESTABLISHED)
          {
            cerr << "\n=== SOCK: WRITE: CONNECTION FOUND ===\n";

            size_t send_buffer_size = SEND_BUF_SIZE(cxn->state);
            //if there is overflow in the sendbuffer
            if (send_buffer_size < req.bytes)
            {
              //add to some of the data from req to the sendbuffer
              cxn->state.SendBuffer.AddBack(req.data.ExtractFront(send_buffer_size));

              res.bytes = send_buffer_size; //size of res.data is the size of the send buffer
              res.error = EBUF_SPACE; //error to indicate lack fo buffer space
            }
            //otherwise if there is no overflow
            else
            {
              cxn->state.SendBuffer.AddBack(req.data);

              res.bytes = req.bytes;
              res.error = EOK;
            }
            
            cxn->state.SendBuffer.Print(cerr);
            cerr << endl;

            res.type = STATUS;
            res.connection = req.connection;
            MinetSend(sock, res);

            // send data from buffer using "Go Back N"
            unsigned int inflight_n = 0; // packets in flight
            unsigned int rwnd = cxn->state.GetRwnd(); // receiver congestion window
            size_t cwnd = cxn->state.SendBuffer.GetSize(); // sender congestion window

            cerr << "\n outside of gbn loop\n";
            cerr << "inflight_n: " << inflight_n << endl;
            cerr << "rwnd: " << rwnd << endl;
            cerr << "cwnd: " << cwnd << endl;
            cerr << "last SENT: " << cxn->state.GetLastSent() << endl;
            cerr << "last ACKED: " << cxn->state.GetLastAcked() << endl;
            
            // iterate through all the packets
            Buffer data;
            while(inflight_n < GBN && cwnd != 0 && rwnd != 0) //GBN is a macro defined at the top (16)
            {
              cerr << "\n=== SOCK: WRITE: GBN LOOP ===\n";
              // if MSS < rwnd and MSS < cwnd
              // space in rwnd and cwnd
              if(MSS < rwnd && MSS < cwnd)
              {
                cerr << "There is still space in the receiver window and sender window" << endl;
                data = cxn->state.SendBuffer.Extract(inflight_n, MSS); //extract data of size MSS from send buffer (offset of # packets inflight)
                // set new seq_n
                // move on to the next set of packets
                inflight_n = inflight_n + MSS;
                CLR_SYN(send_flag);
                SET_ACK(send_flag);
                send_pack = MakePacket(data, cxn->connection, cxn->state.GetLastSent(), cxn->state.GetLastRecvd() + 1, SEND_BUF_SIZE(cxn->state), send_flag);

                cerr << "Last last sent: " << cxn->state.GetLastSent() << endl; 
                cxn->state.SetLastSent(cxn->state.GetLastSent() + MSS); //adjust LastSent to account for the MSS just sent
                cerr << "Last sent: " << cxn->state.GetLastSent() << endl;
              }
              // else there space is not enough space in sender window or receiver window
              else
              {
                cerr << "Limited space in either sender window or receiver window" << endl;
                data = cxn->state.SendBuffer.Extract(inflight_n, min((int)rwnd, (int)cwnd)); //extract data of size min(windows) from send buffer
                // set new seq_n
                // move on to the next set of packets
                inflight_n = inflight_n + min((int)rwnd, (int)cwnd);
                CLR_SYN(send_flag);
                SET_ACK(send_flag);
                send_pack = MakePacket(data, cxn->connection, cxn->state.GetLastSent(), cxn->state.GetLastRecvd() + 1, SEND_BUF_SIZE(cxn->state), send_flag);
                cxn->state.SetLastSent(cxn->state.GetLastSent() + min((int)rwnd, (int)cwnd));
              }

              MinetSend(mux, send_pack); //send the packet to mux

              if ((rwnd < rwnd - inflight_n) && (cwnd < cwnd - inflight_n)) // CAN GET RID OF THIS FIRST IF?
              {
                break;
              }
              else
              {
                rwnd = rwnd - inflight_n;
                cwnd = cwnd - inflight_n;                
              }

              cerr << "\n inflight_n: " << inflight_n << endl;
              cerr << "rwnd: " << rwnd << endl;
              cerr << "cwnd: " << cwnd << endl;
              // set timeout
              //NEED TO SET TIMEOUT FOR THIS TOO???
            }

            cxn->state.N = inflight_n;
          }
          else
          {
            cerr << "\n=== SOCK: WRITE: NO CONNECTION FOUND ===\n";
            res.connection = req.connection;
            res.type = STATUS;
            res.bytes = req.bytes;
            res.error = ENOMATCH;
          }
          
          cerr << "\n=== SOCK: END WRITE ===\n";
        }
        break;
        case FORWARD:
        {
          cerr << "\n=== SOCK: FORWARD ===\n";
          
          cerr << "\n=== SOCK: END FORWARD ===\n";
        }
        break;
        case CLOSE:
        {
          cerr << "\n=== SOCK: CLOSE ===\n";
          ConnectionList<TCPState>::iterator cxn = connect_list.FindMatching(req.connection);
          if (cxn->state.GetState() == ESTABLISHED) //established connection, now call for close 
          {
            unsigned char send_flag;
            Packet send_pack;
            cxn->state.SetState(FIN_WAIT1);
            SET_FIN(send_flag); //send fin to activate closing
            SendPacket(mux, Buffer(NULL, 0), cxn->connection, cxn->state.GetLastSent(), cxn->state.GetLastRecvd() + 1, RECV_BUF_SIZE(cxn->state), send_flag);
          }
          cerr << "\n=== SOCK: END CLOSE ===\n";
        }
        break; 
        case STATUS: //MIGHT HAVE TO CHANGE THIS!!! PERSONALIZE IT!!!
        {
          cerr << "\n=== SOCK: STATUS ===\n";
          ConnectionList<TCPState>::iterator cxn = connect_list.FindMatching(req.connection);
          if (cxn->state.GetState() == ESTABLISHED)
          {
            //ISNT RECVBUFFER THE AMOUNT THAT CAN BE RECEIVED? NOT THE AMOUNT THAT IS ACTUALLY RECEIVED.
            cerr << req.bytes << " out of " << cxn->state.RecvBuffer.GetSize() << " bytes read" << endl;
            // if all data read
            if (req.bytes == cxn->state.RecvBuffer.GetSize())
            {
              cxn->state.RecvBuffer.Clear();
            }
            // if some data still in buffer
            else
            {
              cxn->state.RecvBuffer.Erase(0, req.bytes);

              //resend the WRITE request to the sock
              res.type = WRITE;
              res.connection = req.connection;
              res.data = cxn->state.RecvBuffer;
              res.bytes = cxn->state.RecvBuffer.GetSize();
              res.error = EOK;

              MinetSend(sock, res);
            }
          }
          cerr << "\n=== SOCK: END STATUS ===\n";
        }
        break;
        default:
        {
          cerr << "\n=== SOCK: DEFAULT ===\n";
          cerr << "\n=== SOCK: END DEFAULT ===\n";
        } 
          // TODO: responsd to request with
        break;

      }

      cerr << "\n === SOCK LAYER DONE === \n";

    }
  }
  return 0;
}
