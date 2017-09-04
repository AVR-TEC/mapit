#ifndef CHECKOUTIMPL_H
#define CHECKOUTIMPL_H

#include <upns/typedefs.h>
#include <upns/logging.h>
#include <mapit/msgs/services.pb.h>
#include <upns/operators/serialization/abstractentitydataprovider.h>
#include "serialization/abstractserializer.h"
#include <upns/entitydata.h>
#include <upns/versioning/checkout.h>
#include <upns/operators/versioning/checkoutraw.h>
#include "util.h"
#include <functional>

using namespace mapit::msgs;

namespace upns
{

// PathInternal contains the checkout name at the beginning
// The corresponing types should be used. However there is no "typechecking" here.
typedef Path PathInternal;
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
    CheckoutImpl(std::shared_ptr<AbstractSerializer> serializer, std::shared_ptr<CheckoutObj> checkoutCommit, std::string name, const std::string branchname = "");

    virtual bool isInConflictMode();
    virtual std::vector< std::shared_ptr<Conflict> > getPendingConflicts();
    virtual std::shared_ptr<Tree> getRoot();
    virtual std::shared_ptr<Tree> getTree(const Path &path);
    virtual std::shared_ptr<Entity> getEntity(const Path &path);
    virtual MessageType typeOfObject(const Path &path);
    virtual std::shared_ptr<Tree> getTreeConflict(const ObjectId &objectId);
    virtual std::shared_ptr<Entity> getEntityConflict(const ObjectId &objectId);
    virtual OperationResult doOperation(const OperationDescription &desc);
    virtual OperationResult doUntraceableOperation(const OperationDescription &desc, std::function<upns::StatusCode(OperationEnvironment *)> operate);

    virtual std::shared_ptr<AbstractEntitydata> getEntitydataReadOnly(const Path &path);
    virtual std::shared_ptr<AbstractEntitydata> getEntitydataReadOnlyConflict(const ObjectId &entityId);
    virtual std::shared_ptr<AbstractEntitydata> getEntitydataForReadWrite(const Path &path);

    virtual StatusCode storeTree(const Path &path, std::shared_ptr<Tree> tree);
    virtual StatusCode storeEntity(const Path &path, std::shared_ptr<Entity> entity);

    virtual void setConflictSolved(const Path &path, const ObjectId &oid);

    virtual std::shared_ptr<Branch> getParentBranch();
    virtual std::vector<CommitId> getParentCommitIds();

    template <typename T>
    inline std::string generateTransientOid(const std::shared_ptr<T> &obj);

    StatusCode depthFirstSearch(std::function<bool(std::shared_ptr<Commit>, const ObjectReference&, const Path&)> beforeCommit, std::function<bool(std::shared_ptr<Commit>, const ObjectReference&, const Path&)> afterCommit,
                                std::function<bool(std::shared_ptr<Tree>, const ObjectReference&, const Path&)> beforeTree, std::function<bool(std::shared_ptr<Tree>, const ObjectReference&, const Path&)> afterTree,
                                std::function<bool(std::shared_ptr<Entity>, const ObjectReference&, const Path&)> beforeEntity, std::function<bool(std::shared_ptr<Entity>, const ObjectReference&, const Path&)> afterEntity);
    std::shared_ptr<CheckoutObj> getCheckoutObj();
    const std::string& getName() const;
private:

    std::shared_ptr<Tree> getTree(const ObjectReference &ref);
    std::shared_ptr<Entity> getEntity(const ObjectReference &ref);
    MessageType typeOfObject(const ObjectReference &ref);
    /**
     * @brief objectReferenceOfChild Used to get reference of children
     * @param tree parent
     * @param name of the child (path segment, without slashes)
     * @return Oid or empty oid
     */
    ObjectReference objectReferenceOfChild(std::shared_ptr<Tree> tree, const std::string &name);

    /**
     * @brief oidForPath Used to convert path to oids.
     * @param path beginning with root dir of checkout. Checkout must not be part of path. Can have leading or trailing slashes.
     * @return Oid or empty oid
     */
    ObjectReference objectReferenceForPath(const Path &path);

    // helper, used to ensure slashes/no slashes at beginning and end
    Path preparePath(const Path &path);
    Path preparePathFilename(const Path &path);

    /**
     * @brief createPath If checkout wants to write to a path, it must be created. The leaf can be a tree or entity. This is not a trivial function, but should be easy to use from the outside.
     * @param path path to create in the checkout.
     * @param createLeaf null, if the leaf exists as non-exclusive tree/entity. Can also be a new entity. Can also be a tree (e.g. copy/move operation).
     */
    template <typename T>
    StatusCode createPath(const Path &path, std::shared_ptr<T> createLeaf = std::shared_ptr<T>(nullptr));

    /**
     * @brief Stores all kind of objects at a path.
     */
    template <typename T>
    std::pair<StatusCode, PathInternal> storeObject(std::shared_ptr<T> leafObject, const PathInternal &pathInternal);

    bool forEachPathSegment(const Path &path,
                                  std::function<bool(std::string, size_t, bool)> before, std::function<bool(std::string, size_t, bool)> after, const int start = 0);

//    /**
//     * @brief Depth first search for Commit, Tree and Entity.
//     * Does not work for branches. Does not visit Entitydata (must be done manually).
//     * If "before" returns false, "after" will not be executed.
//     */
//    StatusCode depthFirstSearch(std::shared_ptr<Commit> obj, const ObjectId& oid, const Path &path,
//                                std::function<bool(std::shared_ptr<Commit>, const ObjectReference &)> beforeCommit, std::function<bool(std::shared_ptr<Commit>, const ObjectReference &)> afterCommit,
//                                std::function<bool(std::shared_ptr<Tree>, const ObjectReference &)> beforeTree, std::function<bool(std::shared_ptr<Tree>, const ObjectReference &)> afterTree,
//                                std::function<bool(std::shared_ptr<Entity>, const ObjectReference &)> beforeEntity, std::function<bool(std::shared_ptr<Entity>, const ObjectReference &)> afterEntity);

//    StatusCode depthFirstSearch(std::shared_ptr<Tree> obj, const ObjectId& oid, const Path &path,
//                                std::function<bool(std::shared_ptr<Commit>, const ObjectReference &)> beforeCommit, std::function<bool(std::shared_ptr<Commit>, const ObjectReference &)> afterCommit,
//                                std::function<bool(std::shared_ptr<Tree>, const ObjectReference &)> beforeTree, std::function<bool(std::shared_ptr<Tree>, const ObjectReference &)> afterTree,
//                                std::function<bool(std::shared_ptr<Entity>, const ObjectReference &)> beforeEntity, std::function<bool(std::shared_ptr<Entity>, const ObjectReference &)> afterEntity);

//    StatusCode depthFirstSearch(std::shared_ptr<Entity> obj, const ObjectId& oid, const Path &path,
//                                std::function<bool(std::shared_ptr<Commit>, const ObjectReference &)> beforeCommit, std::function<bool(std::shared_ptr<Commit>, const ObjectReference &)> afterCommit,
//                                std::function<bool(std::shared_ptr<Tree>, const ObjectReference &)> beforeTree, std::function<bool(std::shared_ptr<Tree>, const ObjectReference &)> afterTree,
//                                std::function<bool(std::shared_ptr<Entity>, const ObjectReference &)> beforeEntity, std::function<bool(std::shared_ptr<Entity>, const ObjectReference &)> afterEntity);

    std::shared_ptr<AbstractSerializer> m_serializer;

    // Branch, the checkout is based on, if any
    std::string m_branchname;

    // Name of the checkout
    std::string m_name;

    std::shared_ptr<CheckoutObj> m_checkout;

    std::vector<ObjectId> m_transientOids;
    ObjectId m_nextTransientOid;
};

//TODO: This method could be implemented shorter with less branching and duplication in code.
template <typename T>
StatusCode CheckoutImpl::createPath(const Path &path, std::shared_ptr<T> createLeaf)
{
    Path p = preparePath(path);
    std::vector< std::shared_ptr<Tree> > exclusiveTreePath;
    std::shared_ptr<Tree> current;
    bool rootMissing = !m_checkout->rollingcommit().has_root() || m_checkout->rollingcommit().root().id().empty() && m_checkout->rollingcommit().root().path().empty();
    // root is not transient and must be copied to edit.
    bool rootNotExclusive = (!rootMissing) && (!m_checkout->rollingcommit().root().id().empty() && m_checkout->rollingcommit().root().path().empty());
    //bool rootNotExclusive = m_checkout->transientoidstoorigin().count(m_checkout->rollingcommit().root()) == 0;
    assert(rootMissing || (rootNotExclusive == (m_checkout->transientoidstoorigin().count(m_checkout->rollingcommit().root().path()) == 0)));
    // if there is no root directory, checkout must be empty and have nothing transient
    assert(!rootMissing || rootMissing && m_checkout->transientoidstoorigin_size() == 0);
    if(rootMissing || rootNotExclusive)
    {
        // create/copy root for checkout
        ::std::string originOid;
        if(rootMissing)
        {
            // create new root
            originOid = "";
            current = std::shared_ptr<Tree>(new Tree);
        }
        else
        {
            // copy to make exclusive
            originOid = m_checkout->rollingcommit().root().id();
            current = m_serializer->getTree(originOid);
        }
        m_checkout->mutable_rollingcommit()->mutable_root()->set_path(m_name + "/");
        m_checkout->mutable_transientoidstoorigin()
                ->insert(::google::protobuf::MapPair< ::std::string, ::std::string>(m_checkout->rollingcommit().root().path(), originOid));
        exclusiveTreePath.push_back( current );
    }
    else
    {
        // root exists and can exclusively be altered
        PathInternal path = m_checkout->rollingcommit().root().path();
        current = m_serializer->getTreeTransient(path);
        exclusiveTreePath.push_back( current );
    }

    bool leafWasStored = false;
    PathInternal leafPathIntenal;
    forEachPathSegment(p,
    [&](std::string seg, size_t idx, bool isLast)
    {
        if(current == NULL) return false; // can not go futher
        if(seg.empty()) return false; // "//" not allowed
        ObjectReference ref = objectReferenceOfChild(current, seg);
        PathInternal pathInternal = m_name + "/" + p.substr(0,idx);
        // 6 cases:
        // 1+2) no oid yet          -> create tree/entity, put in vector to update and store. (for leaf: create directly)
        // 3+4) exclusive oids      -> put in vector to update and store (for leaf: create directly)
        // 5+6) non-exclusive oids  -> put copy in vector to update and store (for leaf: create directly)
        assert(ref.id().empty() || ref.path().empty()); // both must not be set at the same time.
        bool segNotExistant = ref.id().empty() && ref.path().empty();
        if(segNotExistant)
        {
            // create new
            if(!isLast || (isLast && createLeaf == NULL))
            {
                // append tree
                std::shared_ptr<Tree> tree(new Tree);
                exclusiveTreePath.push_back( tree );
                m_checkout->mutable_transientoidstoorigin()
                        ->insert(::google::protobuf::MapPair< ::std::string, ::std::string>(pathInternal, ""));
                current = tree;
            }
            else
            {
                // create Leaf. This is executed once. Next step is after(...).
                std::pair<StatusCode, PathInternal> pathStatus = storeObject(createLeaf, pathInternal);
                leafPathIntenal = pathStatus.second;
                if(!upnsIsOk(pathStatus.first)) return false;
                m_checkout->mutable_transientoidstoorigin()
                        ->insert(::google::protobuf::MapPair< ::std::string, ::std::string>(pathStatus.second, "")); //TODO: history of createLeaf lost, if ther was one

                leafWasStored = true;
            }
        }
        else
        {
            // use or copy existing
            bool segExclusive = ref.id().empty() && !ref.path().empty();
            //bool segExclusive = m_checkout->transientoidstoorigin().count(oid) != 0;
            assert(segExclusive == (m_checkout->transientoidstoorigin().count(ref.path()) != 0));
            if(segExclusive)
            {
                // use existing
                if(!isLast || (isLast && createLeaf == NULL))
                {
                    // put tree in vector and do nothing
                    std::shared_ptr<Tree> tree(m_serializer->getTreeTransient(ref.path()));
                    if(tree == NULL)
                    {
                        log_error("Segment of path was not a tree or path was wrong");
                        return false;
                    }
                    exclusiveTreePath.push_back( tree );
                    current = tree;
                }
                else
                {
                    // overwrite Leaf. This is executed once. Next step is after(...).
                    // Note: here might be two transient ids in the pair
                    m_checkout->mutable_transientoidstoorigin()
                            ->insert(::google::protobuf::MapPair< ::std::string, ::std::string>(pathInternal, "")); //TODO: history of createLeaf lost, if ther was one
                    std::pair<StatusCode, PathInternal> statusPath = storeObject(createLeaf, pathInternal);
                    if(!upnsIsOk(statusPath.first)) return false;
                    leafPathIntenal = statusPath.second;
                    leafWasStored = true;
                }
            }
            else
            {
                // copy
                assert(!ref.id().empty() && ref.path().empty());
                if(!isLast || (isLast && createLeaf == NULL))
                {
                    // copy existing tree under transient oid
                    std::shared_ptr<Tree> tree(m_serializer->getTree(ref.id()));
                    if(tree == NULL)
                    {
                        log_error("Segment of path was not a tree or corrupt oids");
                        return false;
                    }
                    m_checkout->mutable_transientoidstoorigin()
                            ->insert(::google::protobuf::MapPair< ::std::string, ::std::string>(pathInternal, ref.id()));
                    exclusiveTreePath.push_back( tree );
                    current = tree;
                }
                else
                {
                    // overwrite Leaf with copy. This is executed once. Next step is after(...).
                    m_checkout->mutable_transientoidstoorigin()
                            ->insert(::google::protobuf::MapPair< ::std::string, ::std::string>(pathInternal, ref.id()));
                    //createLeaf->set_id(nextOid);
                    std::pair<StatusCode, PathInternal> status_path = storeObject(createLeaf, pathInternal);
                    if(!upnsIsOk(status_path.first)) return false;
                    leafPathIntenal = status_path.second;
                    leafWasStored = true;
                }
            }

        }
        return true; // continue thru path
    },
    [&](std::string seg, size_t idx, bool isLast)
    {
        PathInternal pathInternal;
        if(leafWasStored && isLast) // leaf already created in "before"
        {
            assert(!leafPathIntenal.empty());
            pathInternal = leafPathIntenal;
        }
        else
        {
            current = exclusiveTreePath.back();
            exclusiveTreePath.pop_back();
            pathInternal = m_name + "/" + p.substr(0, idx);
            std::pair<StatusCode, PathInternal> status_path = m_serializer->storeTreeTransient(current, pathInternal);
            pathInternal = status_path.second;
            if(!upnsIsOk(status_path.first)) { assert(false); return false;} // must never happen. leads to inconsistent data. TODO: rollback
            if(exclusiveTreePath.empty())
            {
                assert(idx == 0);
                return false;
            }
        }
        std::shared_ptr<Tree> parent = exclusiveTreePath.back();
        ObjectReference oref;
        oref.set_path(pathInternal);
        //oref.set_lastchange(QDateTime::currentDateTime().toMSecsSinceEpoch()); //< breaks hashing
        assert(parent);
        parent->mutable_refs()
                ->insert( ::google::protobuf::MapPair< ::std::string, ::mapit::msgs::ObjectReference>( seg, oref));
        return true;
    });

    // store/create root
    std::shared_ptr<Tree> obj = exclusiveTreePath.back();
    exclusiveTreePath.pop_back();
    std::pair<StatusCode, PathInternal> status_path = m_serializer->storeTreeTransient(obj, m_name + "/");
    if(!upnsIsOk(status_path.first)) return status_path.first;

    // update checkout commit
    StatusCode s = m_serializer->storeCheckoutCommit(m_checkout, m_name);
    return s;
}

}
#endif