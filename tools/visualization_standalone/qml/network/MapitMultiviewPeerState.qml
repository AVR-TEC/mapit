import QtQuick 2.9

Item {
    // identifier given on the first request by the server (secret). Only set for own state
    property string sessionId: ""

    // identifier given on the first request by the server (also for other peers to identify peers)
    property string ident

    // human readable name of the peer, can be freely chosen
    property string peername

    // list of path to objects, the peer currently sees (and works with and wants other to see)
    property var visibleObjects

    // list of objects that are not part of the collaborative workspace, exculsive to the peer
    //property list<RealtimeObject> realtimeObjects
    property list<RealtimeObject> realtimeObjects

    property var timestamp: Date.now()
}