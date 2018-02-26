#include "zmqprotobufnode.h"
#include <mapit/msgs/transport.pb.h>
#include <algorithm>

#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <stdlib.h>        // random()  RAND_MAX

//  Provide random number from 0..(num-1)
#define within(num) (int) ((float) (num) * random () / (RAND_MAX + 1.0))

void my_free(void *data, void *hint)
{
    free (data);
}

ZmqProtobufNode::ZmqProtobufNode(bool reply)
{
  context_ = new zmq::context_t(1);
//  socket_ = new zmq::socket_t(*context_, reply ? ZMQ_ROUTER : ZMQ_REQ);
//  socket_ = new zmq::socket_t(*context_, reply ? ZMQ_ROUTER : ZMQ_DEALER);
  socket_ = new zmq::socket_t(*context_, reply ? ZMQ_REP : ZMQ_REQ);
//  if(!reply) {
//    std::stringstream ss;
//    ss << std::hex << std::uppercase
//       << std::setw(4) << std::setfill('0') << within (0x10000) << "-"
//       << std::setw(4) << std::setfill('0') << within (0x10000);
//    identity_ = ss.str();
//    socket_->setsockopt(ZMQ_IDENTITY, ss.str().c_str(), ss.str().length());
//  }
//#if (defined (WIN32))
//    s_set_id(socket_);//, (intptr_t)args);
//#else
//    s_set_id(socket_);          //  Set a printable identity
//#endif
  connected_ = false;
  isReply_ = reply;
}

ZmqProtobufNode::~ZmqProtobufNode()
{
  delete(socket_);
  if(connected_)
  {
    delete(context_);
  }
}

void
ZmqProtobufNode::connect(std::string com)
{
  if (connected_) {
    std::string msg = "connect called, but node is already connected";
    throw std::runtime_error(msg);
  }

  try {
    socket_->connect(com);
    address_ = com;
    connected_ = true;
    //const int receiveTimeout = 5000;
    //socket_->setsockopt(ZMQ_RCVTIMEO, &receiveTimeout, sizeof(receiveTimeout)); //Set Timeout before recv returns with EAGAIN
  } catch (zmq::error_t e) {
    log_error("Can't connect to server " + e.what());
    connected_ = false;
  }
}

void
ZmqProtobufNode::bind(std::string com)
{
  if (connected_) {
    std::string msg = "bind called, but node is already connected";
    throw std::runtime_error(msg);
  }

  socket_->bind(com);
  connected_ = true;
}

void
ZmqProtobufNode::send_pb_single(std::unique_ptr< ::google::protobuf::Message> msg, int flags)
{
  if ( ! connected_) {
    std::string msg = "send protobuf called, but node is not connected";
    throw std::runtime_error(msg);
  }
  assert(!has_more());

  int size = msg->ByteSize();
  zmq::message_t msg_zmq( size );
  msg->SerializeToArray(msg_zmq.data(), size);

  socket_->send(msg_zmq, flags);
}

void
ZmqProtobufNode::send(std::unique_ptr< ::google::protobuf::Message> msg, bool sndmore)
{
  if ( ! connected_) {
    std::string msg = "send called, but node is not connected";
    throw std::runtime_error(msg);
  }
  assert(!has_more());

  // check for COMP_ID and MSG_TYPE
  const google::protobuf::Descriptor *desc = msg->GetDescriptor();
  KeyType key = key_from_desc(desc);
  int comp_id = key.first;
  int msg_type = key.second;

  // send header
  std::unique_ptr<Header> h(new Header);
  h->set_comp_id(comp_id);
  h->set_msg_type(msg_type);
  send_pb_single(std::move(h), ZMQ_SNDMORE);

  // send msg
  send_pb_single(std::move(msg), sndmore ? ZMQ_SNDMORE : 0);
}

// Does not add header information. For multipart binary frames
void
ZmqProtobufNode::send_raw_body(const unsigned char* data, size_t size, int flags)
{
  if ( ! connected_) {
    std::string msg = "send raw called, but node is not connected";
    throw std::runtime_error(msg);
  }
  assert(!has_more());

  zmq::message_t msg(size);
  memcpy(msg.data(), data, size);
  socket_->send(msg, flags);
}

void
ZmqProtobufNode::receive_and_dispatch(int milliseconds)
{
    log_info("Server DBG: receive_and_dispatch");
  if ( ! connected_) {
    std::string msg = "Receive called, but node is not connected";
    throw std::runtime_error(msg);
  }

  socket_->setsockopt(ZMQ_RCVTIMEO, &milliseconds, sizeof(milliseconds));

  assert(isReply_);

  log_info("Server DBG: receive_and_dispatch: prepareForwardComChannel");
  prepareForwardComChannel();

  log_info("Server DBG: receive_and_dispatch: socket_->recv( &msg_h )");
  // receive header
  Header h;
  zmq::message_t msg_h;
  bool status = socket_->recv( &msg_h );
  if(!status) {
      log_info("Server DBG: ERROR: receive_and_dispatch: Nothing received");
      return; // hopefully timeout
  }
  h.ParseFromArray(msg_h.data(), msg_h.size());

  log_info("Server DBG: receive_and_dispatch: socket_->recv( &msg_zmq )");
  // receive msg
  zmq::message_t msg_zmq;
  socket_->recv( &msg_zmq );

  // dispatch msg
  ReceiveRawDelegate handler = get_handler_for_message(h.comp_id(), h.msg_type());
  log_info("Server DBG: receive_and_dispatch: dispatching");
  if(!handler)
  {
      log_error("Remote seems to speak another language. Could not dispatch message.");
      return;
  }
  try {
      log_info("Server DBG: receive_and_dispatch: prepareBackComChannel");
    prepareBackComChannel();
    log_info("Server DBG: receive_and_dispatch: handler()");
    handler(msg_zmq.data(), msg_zmq.size());
    log_info("Server DBG: receive_and_dispatch: Done");
  }
  catch(zmq::error_t err)
  {
      log_error("DBG: SERVER ERROR: " + err.what());
      try {
        discard_more();
      }
      catch(...) {}
  }
  catch(std::runtime_error err)
  {
      log_error("DBG: SERVER ERROR: " + err.what());
      try {
        discard_more();
      }
      catch(...) {}
  }
  catch(...)
  {
      log_error("Unknown Server error");
      try {
        discard_more();
      }
      catch(...) {}
  }

  // Make sure the handler received everything.
  try {
      assert(!has_more());
  } catch(...) {}
}

void ZmqProtobufNode::discard_more()
{
    while(has_more())
    {
        zmq::message_t msg;
        socket_->recv( &msg );
    }
}

size_t
ZmqProtobufNode::receive_raw_body(void* data, size_t size)
{
    //zmq::message_t msg(data, size); //TODO: zero copy would be nice
    zmq::message_t msg;
    socket_->recv( &msg );
    size_t len = msg.size();
    memcpy(data, msg.data(), std::min(size, len));
    return len;
}

zmq::message_t*
ZmqProtobufNode::receive_raw_body()
{
    //zmq::message_t msg(data, size); //TODO: zero copy would be nice
    zmq::message_t *msg(new zmq::message_t);
    socket_->recv( msg );
    return msg;
}

bool ZmqProtobufNode::has_more()
{
    int64_t more;
    size_t more_size = sizeof (more);
    socket_->getsockopt(ZMQ_RCVMORE, &more, &more_size);
    return more != 0;
}

void ZmqProtobufNode::prepareForwardComChannel()
{
//  if ( ! connected_ ) {
//    std::string msg = "send called, but node is not connected";
//    throw std::runtime_error(msg);
//  }
//  if(has_more())
//  {
//      log_warn("Starting Communication but has more data");
//  }
//  //assert(!has_more());
//  log_info("STARTCOM " + (isReply_?"REPL":"REQ"));
//  if(isReply_) {
//      // ROUTER
//      char ident[256];
//      log_info("1 REPL: recv ident start");
//      int status = socket_->recv( static_cast<void*>(ident), 256 );
//      log_info("1 REPL: recv ident: " + ident);
////      if(status == -1) {
////          log_error("Disconnecting, error in message frames (1)");
////        socket_->disconnect(address_);
////        connected_ = false;
////        return;
////      }
//      char buf[256];
//      log_info("2 REPL: recv empty frame start");
//      status = socket_->recv( static_cast<void*>(buf), 256 );
//      log_info("2 REPL: recv empty frame: " + buf);
//      if(!has_more()) {
//        log_error("Message had frames but no content");
//      }
////      if(status == -1) {
////        log_error("Disconnecting, error in message frames (2)");
////        socket_->disconnect(address_);
////        connected_ = false;
////        return;
////      }
//  } else {
//      // DEALER
//////      socket_->send("", 0, ZMQ_SNDMORE);
//////      log_info("SEND empty frame REP");
//////      socket_->send(identity_.c_str(), identity_.length(), ZMQ_SNDMORE);
//////      log_info("SEND identiy REQ: " + identity_);
////      log_info("4 REQ: send empty frame");
////      socket_->send("", 0, ZMQ_SNDMORE);
//  }
}

void ZmqProtobufNode::prepareBackComChannel()
{
//    // Starts backchannel for Dealer/Router Communication
//  if ( ! connected_ ) {
//    return;
//  }
//  log_info("ENDCOM " + (isReply_?"REPL":"REQ"));
//  if(isReply_) {
//      // ROUTER
//      log_info("6 REPL: END: send identiy");
//      socket_->send(identity_.c_str(), identity_.length(), ZMQ_SNDMORE);
//      log_info("6 REPL: END: send empty frame");
//      socket_->send("", 0, ZMQ_SNDMORE);
//  } else {
//      // DEALER
////    char buf[256];
////    log_info("7 REQ: END: recv empty frame start");
////    bool status = socket_->recv( static_cast<void*>(buf), 256 );
////    log_info("7 REQ: END: recv empty frame: " + buf);
////    if(status == -1) {
////      socket_->disconnect(address_);
////      connected_ = false;
////      return;
////    }
//  }
}

ZmqProtobufNode::KeyType
ZmqProtobufNode::key_from_desc(const google::protobuf::Descriptor *desc)
{
  const google::protobuf::EnumDescriptor *enumdesc = desc->FindEnumTypeByName("CompType");
  if (! enumdesc) {
    throw std::logic_error("Message does not have CompType enum");
  }
  const google::protobuf::EnumValueDescriptor *compdesc =
    enumdesc->FindValueByName("COMP_ID");
  const google::protobuf::EnumValueDescriptor *msgtdesc =
    enumdesc->FindValueByName("MSG_TYPE");
  if (! compdesc || ! msgtdesc) {
    throw std::logic_error("Message CompType enum hs no COMP_ID or MSG_TYPE value");
  }
  int comp_id = compdesc->number();
  int msg_type = msgtdesc->number();
  if (comp_id < 0 || comp_id > std::numeric_limits<uint16_t>::max()) {
    throw std::logic_error("Message has invalid COMP_ID");
  }
  if (msg_type < 0 || msg_type > std::numeric_limits<uint16_t>::max()) {
    throw std::logic_error("Message has invalid MSG_TYPE");
  }
  return KeyType(comp_id, msg_type);
}

ZmqProtobufNode::ReceiveRawDelegate
ZmqProtobufNode::get_handler_for_message(uint16_t component_id, uint16_t msg_type)
{
  KeyType key(component_id, msg_type);

  if (delegate_by_comp_type_.find(key) == delegate_by_comp_type_.end()) {
    std::string msg = "Message type " + std::to_string(component_id) + ":" + std::to_string(msg_type) + " not registered";
    log_error("Remote seems to speak another language. " + msg);
    throw std::runtime_error(msg);
  }

  ZmqProtobufNode::ReceiveRawDelegate delegate = delegate_by_comp_type_[key];
  return delegate;
}

ZmqProtobufNode::ConcreteTypeFactory
ZmqProtobufNode::get_factory_for_message(uint16_t component_id, uint16_t msg_type)
{
  KeyType key(component_id, msg_type);

  if (factory_by_comp_type_.find(key) == factory_by_comp_type_.end()) {
    std::string msg = "Message type " + std::to_string(component_id) + ":" + std::to_string(msg_type) + " not registered";
    log_error("Remote seems to speak another language. " + msg);
    throw std::runtime_error(msg);
  }

  ZmqProtobufNode::ConcreteTypeFactory delegate = factory_by_comp_type_[key];
  return delegate;
}






