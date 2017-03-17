#ifndef QMLTREE
#define QMLTREE

#include <upns/typedefs.h>
#include <QtCore>
#include <upns/services.pb.h>
#include <QQmlListProperty>

class QmlTree : public QObject
{
    Q_OBJECT

public:
    QmlTree(QObject *parent = nullptr);
    QmlTree(upns::upnsSharedPointer<upns::Tree> &tree, QObject *parent = nullptr);

    Q_INVOKABLE QStringList getRefs();
    Q_INVOKABLE QString oidOfRef(QString name);

    Q_INVOKABLE bool isValid() const;
protected:
    upns::upnsSharedPointer<upns::Tree> m_tree;
};

#endif
