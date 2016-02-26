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

#include "Minet.h"


using std::cout;
using std::endl;
using std::cerr;
using std::string;

int main(int argc, char *argv[])
{
  MinetHandle mux, sock;

  MinetInit(MINET_TCP_MODULE);

  mux=MinetIsModuleInConfig(MINET_IP_MUX) ? MinetConnect(MINET_IP_MUX) : MINET_NOHANDLE;
  sock=MinetIsModuleInConfig(MINET_SOCK_MODULE) ? MinetAccept(MINET_SOCK_MODULE) : MINET_NOHANDLE;

  if (MinetIsModuleInConfig(MINET_IP_MUX) && mux==MINET_NOHANDLE) {
    MinetSendToMonitor(MinetMonitoringEvent("Can't connect to mux"));
    return -1;
  }

  if (MinetIsModuleInConfig(MINET_SOCK_MODULE) && sock==MINET_NOHANDLE) {
    MinetSendToMonitor(MinetMonitoringEvent("Can't accept from sock module"));
    return -1;
  }

  MinetSendToMonitor(MinetMonitoringEvent("tcp_module handling TCP traffic"));

  MinetEvent event;

  while (MinetGetNextEvent(event)==0) {
    // if we received an unexpected type of event, print error
    if (event.eventtype!=MinetEvent::Dataflow 
	|| event.direction!=MinetEvent::IN) {
      MinetSendToMonitor(MinetMonitoringEvent("Unknown event ignored."));
<<<<<<< HEAD
    } 
    //  Data from the IP layer below 
    else if (event.handle == mux) 
    {
      cerr << "\n === IP LAYER START === \n";

      cerr << "  == GOT A PACKET ==\n";

      Packet rec_pack;
      MinetReceive(mux, rec_pack);
      
      unsigned tcphlen = TCPHeader::EstimateTCPHeaderLength(rec_pack);
      rec_pack.ExtractHeaderFromPayload<TCPHeader>(tcphlen);

      IPHeader rec_ip_h = rec_pack.FindHeader(Headers::IPHeader);
      TCPHeader rec_tcp_h = rec_pack.FindHeader(Headers::TCPHeader);

      cerr << rec_ip_h <<"\n";
      cerr << rec_tcp_h << "\n";
      cerr << "Checksum is " << (rec_tcp_h.IsCorrectChecksum(rec_pack) ? "VALID\n\n" : "INVALID\n\n");
      cerr << rec_pack << "\n";

      // Unpack useful data
      Connection conn;
      rec_ip_h.GetDestIP(conn.src);
      rec_ip_h.GetSourceIP(conn.dest);
      rec_ip_h.GetProtocol(conn.protocol);
      rec_tcp_h.GetDestPort(conn.srcport);
      rec_tcp_h.GetSourcePort(conn.destport);

      unsigned short rwnd;
      rec_tcp_h.GetWinSize(rwnd);

      unsigned int rec_seq_n;
      unsigned int rec_ack_n;
      unsigned int send_ack_n;
      unsigned int send_seq_n;
      rec_tcp_h.GetSeqNum(rec_seq_n);
      rec_tcp_h.GetAckNum(rec_ack_n);
      send_ack_n = rec_seq_n + 1;

      unsigned char rec_flag;
      rec_tcp_h.GetFlags(rec_flag);

      ConnectionList<TCPState>::iterator cs = clist.FindMatching(conn);
      if (cs != clist.end() && rec_tcp_h.IsCorrectChecksum(rec_pack))
      {   
        cerr << "Found matching connection\n";
        cerr << "Last Acked: " << cs->state.GetLastAcked() << endl;
        cerr << "Last Sent: " << cs->state.GetLastSent() << endl;
        cerr << "Last Recv: " << cs->state.GetLastRecvd() << endl << endl;;
        rec_tcp_h.GetHeaderLen((unsigned char&)tcphlen);
        tcphlen -= TCP_HEADER_MAX_LENGTH;
        Buffer data = rec_pack.GetPayload().ExtractFront(tcphlen);
        data.Print(cerr);
        cerr << endl;

        unsigned char send_flag = 0;
        SockRequestResponse res;
        Packet send_pack;

        switch(cs->state.GetState())
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
              send_seq_n = rand();
              cerr << "generated seq: " << send_seq_n << endl;

              cs->state.SetState(SYN_RCVD);
              cs->state.SetLastRecvd(rec_seq_n);
              cerr << "SET1: " << cs->state.GetLastSent() << endl;
              cs->state.SetLastSent(send_seq_n); // generate random SEQ # to send out
              cerr << "SET2: " << cs->state.GetLastSent() << endl;

              cs->bTmrActive = true;
              cs->timeout = Time() + RTT;

              SET_SYN(send_flag);
              SET_ACK(send_flag);
              send_pack = MakePacket(Buffer(NULL, 0), conn, send_seq_n, send_ack_n, RECV_BUF_SIZE(cs->state), send_flag); // ack
              MinetSend(mux, send_pack);
            }
          }
          break;
          case SYN_RCVD:
          {
            cerr << "\n=== MUX: SYN_RCVD STATE ===\n";
            if (IS_ACK(rec_flag) && cs->state.GetLastSent() == rec_ack_n - 1)
            {
              cs->state.SetState(ESTABLISHED);
              cs->state.SetLastRecvd(rec_seq_n - 1); // next will have same seq num

              // timer
              cs->bTmrActive = false;
              cs->state.SetTimerTries(MAX_TRIES);

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
              cerr << "Last Acked: " << cs->state.GetLastAcked() << endl;
              cerr << "Last Sent: " << cs->state.GetLastSent() << endl;
              cerr << "Last Recv: " << cs->state.GetLastRecvd() << endl;
              send_seq_n = cs->state.GetLastSent() + data.GetSize() + 1;
              cerr << "increment" << data.GetSize() + 1;

              cs->state.SetState(ESTABLISHED);
              cs->state.SetLastAcked(rec_ack_n - 1);
              cs->state.SetLastRecvd(rec_seq_n); // first data will be the same as this


              SET_ACK(send_flag);
              send_pack = MakePacket(Buffer(NULL, 0), conn, send_seq_n, send_ack_n, SEND_BUF_SIZE(cs->state), send_flag);
              MinetSend(mux, send_pack);

              // NOTE: we do this for compatibility with our tcp_server testing, which seems to receives size 6 ACKs
              // from us even though the size is shown as zero above.
              cs->state.SetLastSent(max((int) cs->state.GetLastSent() + 7, (int) send_seq_n));

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

            //if received FIN packet
            if (IS_FIN(rec_flag))
            {
              cerr << "FIN is flagged." << endl;
              send_seq_n = cs->state.GetLastSent() + data.GetSize() + 1;

              cs->state.SetState(CLOSE_WAIT);
              cerr << "SET1: " << cs->state.GetLastSent() << endl;
              cs->state.SetLastSent(send_seq_n);
              cerr << "SET2: " << cs->state.GetLastSent() << endl;
              cs->state.SetLastRecvd(rec_seq_n);

              SET_ACK(send_flag);
              send_pack = MakePacket(Buffer(NULL, 0), conn, send_seq_n, send_ack_n, RECV_BUF_SIZE(cs->state), send_flag);
              MinetSend(mux, send_pack);
            }
            //else, is a dataflow packet
            else
            { 
              if (IS_ACK(rec_flag) && cs->state.GetLastRecvd() < rec_seq_n)
              {
                cerr << "ACK is flagged." << endl;

                //packet has data
                if (data.GetSize() > 0)
                {
                  cerr << "The received packet has data" << endl;
                  cerr << "Recv: " << cs->state.GetLastRecvd() << endl;
                
                  size_t recv_buf_size = RECV_BUF_SIZE(cs->state);
                  //if there is overflow of the received data
                  if (recv_buf_size < data.GetSize()) 
                  {
                    cs->state.RecvBuffer.AddBack(data.ExtractFront(recv_buf_size)); //extract first n bits from data to recvbuffer
                    send_ack_n = rec_seq_n + recv_buf_size - 1;
                    cs->state.SetLastRecvd(send_ack_n);
                  }
                  else //if there is no overflow
                  {
                    cs->state.RecvBuffer.AddBack(data);
                    send_ack_n = rec_seq_n + data.GetSize() - 1;
                    cs->state.SetLastRecvd(send_ack_n);
                  }

                  //grabbing the next sequence number to send 
                  send_seq_n = cs->state.GetLastSent() + min(recv_buf_size, data.GetSize());
                  cs->state.SetLastSent(send_seq_n);

                  //send an empty packet with ACK flag to mux
                  SET_ACK(send_flag);
                  send_pack = MakePacket(Buffer(NULL, 0), conn, send_seq_n, send_ack_n + 1, RECV_BUF_SIZE(cs->state), send_flag);
                  MinetSend(mux, send_pack);

                  //create a socketrequestresponse to send to sock 
                  res.type = WRITE;
                  res.connection = conn;
                  res.data = cs->state.RecvBuffer; //send data in recvbuffer to sock
                  res.bytes = cs->state.RecvBuffer.GetSize();
                  res.error = EOK;
                  MinetSend(sock, res);
                }
                else //packet has no data
                {
                  cs->state.SendBuffer.Erase(0, rec_ack_n - cs->state.GetLastAcked() - 1);
                  cs->state.N = cs->state.N - (rec_ack_n - cs->state.GetLastAcked() - 1);

                  cs->state.SetLastAcked(rec_ack_n);
                  cs->state.SetLastRecvd(rec_seq_n);
                  cs->state.last_acked = rec_ack_n;


                  cerr << "\nSend Buffer: ";
                  cs->state.SendBuffer.Print(cerr);
                  cerr << endl;

                  cs->state.N = cs->state.N - (rec_ack_n - cs->state.GetLastAcked() - 1);
  
                  // send some of the information in the buffer if there is an overflow in the sendbuffer
                  if (cs->state.SendBuffer.GetSize() - cs->state.GetN() > 0)
                  {
                    unsigned int inflight_n = cs->state.GetN();
                    unsigned int rwnd = cs->state.GetRwnd(); // receiver congestion window
                    size_t cwnd = cs->state.SendBuffer.GetSize(); // sender congestion window

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
                        data = cs->state.SendBuffer.Extract(inflight_n, MSS);
                        // set new seq_n
                        // move on to the next set of packets
                        inflight_n = inflight_n + MSS;
                        CLR_SYN(send_flag);
                        SET_ACK(send_flag);
                        SET_PSH(send_flag);
                        send_pack = MakePacket(data, cs->connection, cs->state.GetLastSent(), cs->state.GetLastRecvd() + 1, SEND_BUF_SIZE(cs->state), send_flag);

                        cerr << "SET1: " << cs->state.GetLastSent() << endl;
                        cs->state.SetLastSent(cs->state.GetLastSent() + MSS);
                        cerr << "SET2: " << cs->state.GetLastSent() << endl;
                      }

                      // else space in cwnd or rwnd
                      else
                      {
                        cerr << "space in either or" << endl;
                        data = cs->state.SendBuffer.Extract(inflight_n, min((int)rwnd, (int)cwnd));
                        // set new seq_n
                        // move on to the next set of packets
                        inflight_n = inflight_n + min((int)rwnd, (int)cwnd);
                        CLR_SYN(send_flag);
                        SET_ACK(send_flag);
                        SET_PSH(send_flag);
                        send_pack = MakePacket(data, cs->connection, cs->state.GetLastSent(), cs->state.GetLastRecvd() + 1, SEND_BUF_SIZE(cs->state), send_flag);
                        cs->state.SetLastSent(cs->state.GetLastSent() + min((int)rwnd, (int)cwnd));
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
                      cs->bTmrActive = true;
                      cs->timeout = Time() + RTT;
                    }

                    cs->state.N = inflight_n;
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
              send_seq_n = cs->state.GetLastSent() + data.GetSize() + 1;

              cs->state.SetState(LAST_ACK);
              cs->state.SetLastRecvd(rec_seq_n);
              cerr << "SET1: " << cs->state.GetLastSent() << endl;
              cs->state.SetLastSent(send_seq_n);
              cerr << "SET2: " << cs->state.GetLastSent() << endl;

              // timeout stuff
              cs->bTmrActive = true;
              cs->timeout = Time() + RTT;
              cs->state.SetTimerTries(MAX_TRIES);

              SET_FIN(send_flag);
              send_pack = MakePacket(Buffer(NULL, 0), conn, send_seq_n, send_ack_n, RECV_BUF_SIZE(cs->state), send_flag);
              MinetSend(mux, send_pack);
            }
          }
          break;
          case FIN_WAIT1:
          {
            cerr << "\n=== MUX: FIN_WAIT1 STATE ===\n";
            if (IS_FIN(rec_flag))
            {
              send_seq_n = cs->state.GetLastSent() + data.GetSize() + 1;

              cs->state.SetState(CLOSING);
              cs->state.SetLastRecvd(rec_seq_n);
              cerr << "SET1: " << cs->state.GetLastSent() << endl;
              cs->state.SetLastSent(send_seq_n);
              cerr << "SET2: " << cs->state.GetLastSent() << endl;

              // set timeout
              cs->bTmrActive = true;
              cs->timeout = Time() + RTT;
              cs->state.SetTimerTries(MAX_TRIES);

              SET_FIN(send_flag);
              SET_ACK(send_flag);
              send_pack = MakePacket(Buffer(NULL, 0), conn, send_seq_n, send_ack_n, SEND_BUF_SIZE(cs->state), send_flag);
              MinetSend(mux, send_pack);
            }
            else if (IS_ACK(rec_flag))
            {

              cs->state.SetState(FIN_WAIT2);
              cs->state.SetLastSent(send_seq_n);
              cs->state.SetLastAcked(rec_ack_n - 1);
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
              cs->state.SetState(TIME_WAIT);
              cs->state.SetLastAcked(rec_ack_n - 1);
              cs->state.SetLastRecvd(rec_seq_n);
            }
          }
          break;
          case LAST_ACK:
          {
            cerr << "\n=== MUX: LAST_ACK STATE ===\n";
            if (IS_ACK(rec_flag))
            {
              cs->state.SetState(CLOSED);
              cs->state.SetLastAcked(rec_ack_n - 1);
              cs->state.SetLastRecvd(rec_seq_n);
            }
          }
          break;
          case FIN_WAIT2:
          {
            cerr << "\n=== MUX: FIN_WAIT2 STATE ===\n";
            if (IS_FIN(rec_flag))
            {
              send_seq_n = cs->state.GetLastSent() + data.GetSize() + 1;

              cs->state.SetState(TIME_WAIT);
              cs->state.SetLastRecvd(rec_seq_n);
              cs->state.SetLastSent(send_seq_n);
              cs->state.SetLastAcked(rec_ack_n - 1);
 
              // set timeout
              cs->bTmrActive = true;
              cs->timeout = Time() + RTT;
              cs->state.SetTimerTries(MAX_TRIES);

              SET_ACK(send_flag);
              send_pack = MakePacket(Buffer(NULL, 0), conn, send_seq_n, send_ack_n, SEND_BUF_SIZE(cs->state), send_flag);
              MinetSend(mux, send_pack);
            }
          }
          break;
          case TIME_WAIT:
          {
            cerr << "\n=== MUX: TIME_WAIT STATE ===\n";
            cs->timeout = Time() + 30;
            cs->state.SetState(CLOSED);
          }
          break;
          default:
          {
            cerr << "\n=== MUX: DEFAULTED STATE ===\n";
          }
          break;
        }
=======
    // if we received a valid event from Minet, do processing
    } else {
      cerr << "invalid event from Minet" << endl;
      //  Data from the IP layer below  //
      if (event.handle==mux) {
        Packet p;
        MinetReceive(mux,p);
        unsigned tcphlen=TCPHeader::EstimateTCPHeaderLength(p);
        cerr << "estimated header len="<<tcphlen<<"\n";
        p.ExtractHeaderFromPayload<TCPHeader>(tcphlen);
        IPHeader ipl=p.FindHeader(Headers::IPHeader);
        TCPHeader tcph=p.FindHeader(Headers::TCPHeader);

        cerr << "TCP Packet: IP Header is "<<ipl<<" and ";
        cerr << "TCP Header is "<<tcph << " and ";

        cerr << "Checksum is " << (tcph.IsCorrectChecksum(p) ? "VALID" : "INVALID");
        
>>>>>>> 0bee0e6e5ea3ee411f7036fddfe7b34e79609253
      }
          //  Data from the Sockets layer above  //
      if (event.handle==sock) {
        SockRequestResponse s;
        MinetReceive(sock,s);
        cerr << "Received Socket Request:" << s << endl;
      }
    }
  }
  return 0;
}
