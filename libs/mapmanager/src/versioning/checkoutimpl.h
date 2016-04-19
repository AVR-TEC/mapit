#ifndef CHECKOUTIMPL_H
#define CHECKOUTIMPL_H

#include "upns_globals.h"
#include "services.pb.h"
#include "modules/serialization/abstractentitydatastreamprovider.h"
#include "serialization/abstractmapserializer.h"
#include "entitydata.h"
#include "versioning/checkout.h"
#include "modules/versioning/checkoutraw.h"
#include "util.h"
#include <functional>
#include <QDateTime>

namespace upns
{

/**
 * @brief The CheckoutImpl class
 * Why?
 *
 * Checkout vs. CheckoutRaw
 * The Class "Checkout" is meant to be used by user/application. The Class "CheckoutRaw" is meant to be used by operators.
 * CheckoutRaw can edit objects and can not execute other operators (thus, recursive execution of operators is forbidden, it could break metadata).
 * Checkout can not edit objects, as the application can only do changes by executing operations!
 *
 * CheckoutCommon:
 * Both classes have similarities (e.g. all the read operations). These similarities are not repeated in both interfaces, but in "CheckoutCommon".
 *
 * CheckoutImpl:
 * When a Checkout is generated and given to an operator, the objects should not be copied. Thus, both of the classes ("Checkout" and "CheckoutRaw")
 * need to be one object with both implementations. The Implementation is in "CheckoutImpl". Instances of "CheckoutImpl" can be seen as "Chechout" for
 * the user/application and as "CheckoutRaw" fom an operator.
 *
 */
class CheckoutImpl : public Checkout, public CheckoutRaw /*, CheckoutCommon*/
{
public:
    /**
     * @brief Checkout Checkouts represent an editable state of a group of maps.
     * @param serializer
     * @param commitOrCheckoutId
     */
    CheckoutImpl(AbstractMapSerializer *serializer, upnsSharedPointer<CheckoutObj> checkoutCommit, upnsString name, const upnsString branchname = "");
    ~CheckoutImpl();

    virtual bool isInConflictMode();
    virtual upnsVec< upnsSharedPointer<Conflict> > getPendingConflicts();
    virtual upnsSharedPointer<Tree> getRoot();
    virtual upnsSharedPointer<Tree> getTree(const Path &path);
    virtual upnsSharedPointer<Entity> getEntity(const Path &path);
    virtual upnsSharedPointer<Tree> getTreeConflict(const ObjectId &objectId);
    virtual upnsSharedPointer<Entity> getEntityConflict(const ObjectId &objectId);
    virtual OperationResult doOperation(const OperationDescription &desc);

    virtual upnsSharedPointer<AbstractEntityData> getEntityDataReadOnly(const Path &path);
    virtual upnsSharedPointer<AbstractEntityData> getEntityDataReadOnlyConflict(const ObjectId &entityId);
    virtual upnsSharedPointer<AbstractEntityData> getEntityDataForReadWrite(const Path &path);

    virtual StatusCode storeTree(const Path &path, upnsSharedPointer<Tree> tree);
    virtual StatusCode storeEntity(const Path &path, upnsSharedPointer<Entity> entity);

    virtual void setConflictSolved(const Path &path, const ObjectId &oid);

    template <typename T>
    inline upnsString generateTransientOid(const upnsSharedPointer<T> &obj);

    StatusCode depthFirstSearch(std::function<bool(upnsSharedPointer<Commit>)> beforeCommit, std::function<bool(upnsSharedPointer<Commit>)> afterCommit,
                                std::function<bool(upnsSharedPointer<Tree>)> beforeTree, std::function<bool(upnsSharedPointer<Tree>)> afterTree,
                                std::function<bool(upnsSharedPointer<Entity>)> beforeEntity, std::function<bool(upnsSharedPointer<Entity>)> afterEntity);
private:

    ObjectId oidForChild(upnsSharedPointer<Tree> tree, const std::string &name);
    ObjectId oidForPath(const Path &path);
    Path preparePath(const Path &path);
    template <typename T>
    StatusCode createPath(const Path &path, upnsSharedPointer<T> createLeaf = upnsSharedPointer<T>(nullptr));
    template <typename T>
    StatusCode createObject(upnsSharedPointer<T> leafObject);
    template <typename T>
    StatusCode storeObject(upnsSharedPointer<T> leafObject);

    bool forEachPathSegment(const Path &path,
                                  std::function<bool(upnsString, size_t, bool)> before, std::function<bool(upnsString, size_t, bool)> after, const int start = 0);

    /**
     * Depth first search for Commit, Tree and Entity.
     * Does not work for branches. Does not visit EntityData (must be done manually).
     * If before return false, after will not be executed.
     */
    StatusCode depthFirstSearch(upnsSharedPointer<Commit> obj,
                                std::function<bool(upnsSharedPointer<Commit>)> beforeCommit, std::function<bool(upnsSharedPointer<Commit>)> afterCommit,
                                std::function<bool(upnsSharedPointer<Tree>)> beforeTree, std::function<bool(upnsSharedPointer<Tree>)> afterTree,
                                std::function<bool(upnsSharedPointer<Entity>)> beforeEntity, std::function<bool(upnsSharedPointer<Entity>)> afterEntity);

    StatusCode depthFirstSearch(upnsSharedPointer<Tree> obj,
                                std::function<bool(upnsSharedPointer<Commit>)> beforeCommit, std::function<bool(upnsSharedPointer<Commit>)> afterCommit,
                                std::function<bool(upnsSharedPointer<Tree>)> beforeTree, std::function<bool(upnsSharedPointer<Tree>)> afterTree,
                                std::function<bool(upnsSharedPointer<Entity>)> beforeEntity, std::function<bool(upnsSharedPointer<Entity>)> afterEntity);

    StatusCode depthFirstSearch(upnsSharedPointer<Entity> obj,
                                std::function<bool(upnsSharedPointer<Commit>)> beforeCommit, std::function<bool(upnsSharedPointer<Commit>)> afterCommit,
                                std::function<bool(upnsSharedPointer<Tree>)> beforeTree, std::function<bool(upnsSharedPointer<Tree>)> afterTree,
                                std::function<bool(upnsSharedPointer<Entity>)> beforeEntity, std::function<bool(upnsSharedPointer<Entity>)> afterEntity);

    inline upnsString transientOid(const upnsString &path);
    AbstractMapSerializer* m_serializer;

    // Rolling Commit, id is random and not yet the hash of commit. This commit is exclusive for this checkout, this checkout is based on as "parents"
    // TODO: maybe leave id out in every object
    //upnsSharedPointer<Commit>  m_commit;

    // Branch, the checkout is based on, if any
    upnsString m_branchname;

    // Name of the checkout
    upnsString m_name;

    upnsSharedPointer<CheckoutObj> m_checkout;

    upnsVec<ObjectId> m_transientOids;
    ObjectId m_nextTransientOid;
};

template <typename T>
StatusCode CheckoutImpl::createPath(const Path &path, upnsSharedPointer<T> createLeaf)
{
    std::cout << "\nPRE\n\n";
    m_serializer->debugDump();
    StatusCode s;
    Path p = preparePath(path);
    upnsVec<upnsPair<upnsSharedPointer<Tree>, bool> > exclusiveTreePath;
    ObjectId oid;
    upnsSharedPointer<Tree> current;
    bool rootMissing = m_checkout->rollingcommit().root().empty();
    bool rootNotExclusive = m_checkout->transientoidstoorigin().count(m_checkout->rollingcommit().root()) == 0;
    // if there is no root directory, checkout must be empty and have nothing transient
    assert(!rootMissing || rootMissing && m_checkout->transientoidstoorigin_size() == 0);
    if(rootMissing || rootNotExclusive)
    {
        // create/copy root for checkout
        ::std::string origin;
        if(rootMissing)
        {
            // create new root
            current = upnsSharedPointer<Tree>(new Tree);
            origin = "";
        }
        else
        {
            // copy to make exclusive
            ObjectId rootOid(m_checkout->rollingcommit().root());
            current = m_serializer->getTree(rootOid);
            origin = rootOid;
        }
        ::std::string toid = transientOid("");
        current->set_id(toid);
        m_checkout->mutable_rollingcommit()->set_root(toid);
        m_checkout->mutable_transientoidstoorigin()
                ->insert(::google::protobuf::MapPair< ::std::string, ::std::string>(m_checkout->rollingcommit().root(), origin));
        oid = toid;
        exclusiveTreePath.push_back(upnsPair<upnsSharedPointer<Tree>, bool>(current, true));
    }
    else
    {
        // root exists and can exclusively be altered
        oid = m_checkout->rollingcommit().root();
        current = m_serializer->getTree(oid);
        exclusiveTreePath.push_back(upnsPair<upnsSharedPointer<Tree>, bool>(current, false));
    }

    std::cout << "\nROOT\n\n";
    m_serializer->debugDump();
    bool leafWasStored = false;
    forEachPathSegment(p,
    [&](upnsString seg, size_t idx, bool isLast)
    {
        if(current == NULL) return false; // can not go futher
        if(seg.empty()) return false; // "//" not allowed
        oid = oidForChild(current, seg);
        ObjectId nextOid = transientOid(path.substr(0, idx));
        // 6 cases:
        // 1+2) no oid yet          -> create tree/entity, put in vector to update and store. (for leaf: create directly)
        // 3+4) exclusive oids      -> put in vector to update and store (for leaf: create directly)
        // 5+6) non-exclusive oids  -> put copy in vector to update and store (for leaf: create directly)
        if(oid.empty())
        {
            // create new
            if(!isLast || (isLast && createLeaf == NULL))
            {
                // append tree
                upnsSharedPointer<Tree> tree(new Tree);
                tree->set_id(nextOid);
                exclusiveTreePath.push_back(upnsPair<upnsSharedPointer<Tree>, bool>(tree, true));
                m_checkout->mutable_transientoidstoorigin()
                        ->insert(::google::protobuf::MapPair< ::std::string, ::std::string>(nextOid, ""));
                current = tree;
            }
            else
            {
                // create Leaf. This is executed once. Next step is after(...).
                ObjectId oldId = createLeaf->id();
                createLeaf->set_id(nextOid);
                s = createObject(createLeaf);
                if(!upnsIsOk(s)) return false;
                m_checkout->mutable_transientoidstoorigin()
                        ->insert(::google::protobuf::MapPair< ::std::string, ::std::string>(nextOid, oldId));

                leafWasStored = true;
            }
        }
        else
        {
            // use or copy existing
            bool segExclusive = m_checkout->transientoidstoorigin().count(oid) != 0;
            if(segExclusive)
            {
                // use existing
                if(!isLast || (isLast && createLeaf == NULL))
                {
                    // put tree in vector and do nothing
                    upnsSharedPointer<Tree> tree(m_serializer->getTree(oid));
                    exclusiveTreePath.push_back(upnsPair<upnsSharedPointer<Tree>, bool>(tree, false));
                    current = tree;
                }
                else
                {
                    // overwrite Leaf. This is executed once. Next step is after(...).
                    // Note: here might be two transient ids in the pair
                    m_checkout->mutable_transientoidstoorigin()
                            ->insert(::google::protobuf::MapPair< ::std::string, ::std::string>(nextOid, createLeaf->id()));
                    createLeaf->set_id(nextOid);
                    s = storeObject(createLeaf);
                    if(!upnsIsOk(s)) return false;
                    leafWasStored = true;
                }
            }
            else
            {
                // copy
                if(!isLast || (isLast && createLeaf == NULL))
                {
                    // copy existing tree under transient oid
                    upnsSharedPointer<Tree> tree(m_serializer->getTree(oid));
                    assert(tree);
                    m_checkout->mutable_transientoidstoorigin()
                            ->insert(::google::protobuf::MapPair< ::std::string, ::std::string>(nextOid, oid));
                    tree->set_id(nextOid);
                    exclusiveTreePath.push_back(upnsPair<upnsSharedPointer<Tree>, bool>(tree, true));
                    current = tree;
                }
                else
                {
                    // overwrite Leaf with copy. This is executed once. Next step is after(...).
                    m_checkout->mutable_transientoidstoorigin()
                            ->insert(::google::protobuf::MapPair< ::std::string, ::std::string>(nextOid, createLeaf->id()));
                    createLeaf->set_id(nextOid);
                    s = createObject(createLeaf);
                    if(!upnsIsOk(s)) return false;
                    leafWasStored = true;
                }
            }

        }
        return true; // continue thru path
    },
    [&](upnsString seg, size_t idx, bool isLast)
    {
        std::cout << "DBG: " << seg << ", last: " << isLast << ", i: " << exclusiveTreePath.size() << std::endl;
        if(leafWasStored && isLast) // leaf already created in "before"
        {
            oid = createLeaf->id();
            assert(!oid.empty());
        }
        else
        {
            upnsPair<upnsSharedPointer<Tree>, bool> obj = exclusiveTreePath.back();
            std::cout << "Stack id: " << obj.first->id() << std::endl;
            exclusiveTreePath.pop_back();
            current = obj.first;
            oid = current->id();
            assert(!oid.empty());
            if(obj.second)
            {
                s = m_serializer->createTree(current, true);
            }
            else
            {
                s = m_serializer->storeTree(current, true);
            }
            if(!upnsIsOk(s)) return false; // must never happen. leads to inconsistent data. TODO: rollback
            if(exclusiveTreePath.empty())
            {
                assert(idx == 0);
                return false;
            }
        }
        upnsSharedPointer<Tree> parent = exclusiveTreePath.back().first;
        std::cout << "Parent id: " << parent->id() << std::endl;
        std::cout << "Oid: " << oid << std::endl;
        std::cout << "seg: " << seg << std::endl;
        std::cout << "Done" << std::endl;
        ObjectReference oref;
        oref.set_id(oid);
        oref.set_lastchange(QDateTime::currentDateTime().toMSecsSinceEpoch());
        assert(parent);
        parent->mutable_refs()
                ->insert( ::google::protobuf::MapPair< ::std::string, ::upns::ObjectReference>( seg, oref));
        return true;
    });

    std::cout << "\nPOST\n\n";
    m_serializer->debugDump();
    // store/create root
    upnsPair<upnsSharedPointer<Tree>, bool> obj = exclusiveTreePath.back();
    exclusiveTreePath.pop_back();
    if(obj.second)
    {
        s = m_serializer->createTree(obj.first, true);
    }
    else
    {
        s = m_serializer->storeTree(obj.first, true);
    }
    if(!upnsIsOk(s)) return s;
    std::cout << "\nPREFIN\n\n";
    m_serializer->debugDump();

    // update checkout commit
    s = m_serializer->storeCheckoutCommit(m_checkout, m_name);
    std::cout << "\nFIN\n\n";
    m_serializer->debugDump();
    return s;
}

}
#endif
