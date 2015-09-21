#ifndef __MESSAGESTEST_H
#define __MESSAGESTEST_H

#include <QTest>
#include "upns_interface/services.pb.h"

class MessagesInterface : public QObject
{
    Q_OBJECT
private slots:
    void init();
    void cleanup();

    void testGetMaps();
    void testGetLayer();


};

#endif
