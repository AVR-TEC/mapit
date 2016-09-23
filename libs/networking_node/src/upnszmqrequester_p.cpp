#include "upnszmqrequester_p.h"
#include "services.pb.h"
#include <functional>

upns::ZmqRequesterPrivate::ZmqRequesterPrivate(std::__cxx11::string urlOutgoingRequests)
    :ZmqNode( false )
{
    add_receivable_message_type<ReplyCheckout>();
    add_receivable_message_type<ReplyEntitydata>();
    add_receivable_message_type<ReplyHierarchy>();
    add_receivable_message_type<ReplyListCheckouts>();
    add_receivable_message_type<ReplyOperatorExecution>();
    add_receivable_message_type<ReplyStoreEntity>();
}
