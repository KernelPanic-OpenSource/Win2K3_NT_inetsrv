/*++

   Copyright    (c) 1997-2002    Microsoft Corporation

   Module  Name :
       LKR-apply.cpp

   Abstract:
       Apply, ApplyIf, and DeleteIf

   Author:
       George V. Reilly      (GeorgeRe)     1998

   Project:
       LKRhash

--*/

#include "precomp.hxx"


#ifndef LIB_IMPLEMENTATION
# define DLL_IMPLEMENTATION
# define IMPLEMENTATION_EXPORT
#endif // !LIB_IMPLEMENTATION

#include <lkrhash.h>

#include "i-lkrhash.h"


#ifndef __LKRHASH_NO_NAMESPACE__
namespace LKRhash {
#endif // !__LKRHASH_NO_NAMESPACE__

#ifdef LKR_APPLY_IF

//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::Apply
// Synopsis:
// Returns:
//------------------------------------------------------------------------

DWORD
CLKRLinearHashTable::Apply(
    LKR_PFnRecordAction pfnAction,
    void*               pvState,
    LK_LOCKTYPE         lkl)
{
    if (!IsUsable())
        return static_cast<DWORD>(LK_UNUSABLE);

    if (lkl == LKL_WRITELOCK)
        this->WriteLock();
    else
        this->ReadLock();

    // Must call IsValid inside a lock to ensure that none of the state
    // variables change while it's being evaluated
    IRTLASSERT(IsValid());

    LK_PREDICATE lkp = LKP_PERFORM;
    DWORD dw = _ApplyIf(_PredTrue, pfnAction, pvState, lkl, lkp);

    if (lkl == LKL_WRITELOCK)
        this->WriteUnlock();
    else
        this->ReadUnlock();

    return dw;
} // CLKRLinearHashTable::Apply



//------------------------------------------------------------------------
// Function: CLKRHashTable::Apply
// Synopsis:
// Returns:
//------------------------------------------------------------------------

DWORD
CLKRHashTable::Apply(
    LKR_PFnRecordAction pfnAction,
    void*               pvState,
    LK_LOCKTYPE         lkl)
{
    if (!IsUsable())
        return static_cast<DWORD>(LK_UNUSABLE);

    DWORD dw = 0;
    LK_PREDICATE lkp = LKP_PERFORM;

    for (DWORD i = 0;  i < m_cSubTables;  ++i)
    {
        if (lkl == LKL_WRITELOCK)
            m_palhtDir[i]->WriteLock();
        else
            m_palhtDir[i]->ReadLock();

        // Must call IsValid inside a lock to ensure that none of the state
        // variables change while it's being evaluated
        IRTLASSERT(m_palhtDir[i]->IsValid());

        if (m_palhtDir[i]->IsValid())
        {
            dw += m_palhtDir[i]->_ApplyIf(CLKRLinearHashTable::_PredTrue,
                                          pfnAction, pvState, lkl, lkp);
        }

        if (lkl == LKL_WRITELOCK)
            m_palhtDir[i]->WriteUnlock();
        else
            m_palhtDir[i]->ReadUnlock();

        if (lkp == LKP_ABORT
            ||  lkp == LKP_PERFORM_STOP
            ||  lkp == LKP_DELETE_STOP)
        {
            break;
        }
    }

    return dw;
} // CLKRHashTable::Apply



//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::ApplyIf
// Synopsis:
// Returns:
//------------------------------------------------------------------------

DWORD
CLKRLinearHashTable::ApplyIf(
    LKR_PFnRecordPred   pfnPredicate,
    LKR_PFnRecordAction pfnAction,
    void*               pvState,
    LK_LOCKTYPE         lkl)
{
    if (!IsUsable())
        return static_cast<DWORD>(LK_UNUSABLE);

    DWORD dw = 0;
    LK_PREDICATE lkp = LKP_PERFORM;

    if (lkl == LKL_WRITELOCK)
        this->WriteLock();
    else
        this->ReadLock();

    // Must call IsValid inside a lock to ensure that none of the state
    // variables change while it's being evaluated
    IRTLASSERT(IsValid());

    if (IsValid())
    {
        dw = _ApplyIf(pfnPredicate, pfnAction, pvState, lkl, lkp);
    }

    if (lkl == LKL_WRITELOCK)
        this->WriteUnlock();
    else
        this->ReadUnlock();

    return dw;
} // CLKRLinearHashTable::ApplyIf



//------------------------------------------------------------------------
// Function: CLKRHashTable::ApplyIf
// Synopsis:
// Returns:
//------------------------------------------------------------------------

DWORD
CLKRHashTable::ApplyIf(
    LKR_PFnRecordPred   pfnPredicate,
    LKR_PFnRecordAction pfnAction,
    void*               pvState,
    LK_LOCKTYPE         lkl)
{
    if (!IsUsable())
        return static_cast<DWORD>(LK_UNUSABLE);

    DWORD dw = 0;
    LK_PREDICATE lkp = LKP_PERFORM;

    for (DWORD i = 0;  i < m_cSubTables;  ++i)
    {
        if (lkl == LKL_WRITELOCK)
            m_palhtDir[i]->WriteLock();
        else
            m_palhtDir[i]->ReadLock();
        
        // Must call IsValid inside a lock to ensure that none of the state
        // variables change while it's being evaluated
        IRTLASSERT(m_palhtDir[i]->IsValid());
        
        if (m_palhtDir[i]->IsValid())
        {
            dw += m_palhtDir[i]->_ApplyIf(pfnPredicate, pfnAction,
                                          pvState, lkl, lkp);
        }
        
        if (lkl == LKL_WRITELOCK)
            m_palhtDir[i]->WriteUnlock();
        else
            m_palhtDir[i]->ReadUnlock();
        
        if (lkp == LKP_ABORT
            ||  lkp == LKP_PERFORM_STOP
            ||  lkp == LKP_DELETE_STOP)
        {
            break;
        }
    }

    return dw;
} // CLKRHashTable::ApplyIf



//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::DeleteIf
// Synopsis:
// Returns:
//------------------------------------------------------------------------

DWORD
CLKRLinearHashTable::DeleteIf(
    LKR_PFnRecordPred   pfnPredicate,
    void*               pvState)
{
    if (!IsUsable())
        return static_cast<DWORD>(LK_UNUSABLE);

    DWORD dw = 0;
    LK_PREDICATE lkp = LKP_PERFORM;

    this->WriteLock();

    // Must call IsValid inside a lock to ensure that none of the state
    // variables change while it's being evaluated
    IRTLASSERT(IsValid());

    if (IsValid())
        dw = _DeleteIf(pfnPredicate, pvState, lkp);

    this->WriteUnlock();

    return dw;
} // CLKRLinearHashTable::DeleteIf



//------------------------------------------------------------------------
// Function: CLKRHashTable::DeleteIf
// Synopsis:
// Returns:
//------------------------------------------------------------------------

DWORD
CLKRHashTable::DeleteIf(
    LKR_PFnRecordPred   pfnPredicate,
    void*               pvState)
{
    if (!IsUsable())
        return static_cast<DWORD>(LK_UNUSABLE);

    DWORD dw = 0;
    LK_PREDICATE lkp = LKP_PERFORM;

    for (DWORD i = 0;  i < m_cSubTables;  ++i)
    {
        m_palhtDir[i]->WriteLock();
        
        // Must call IsValid inside a lock to ensure that none of the state
        // variables change while it's being evaluated
        IRTLASSERT(m_palhtDir[i]->IsValid());
        
        if (m_palhtDir[i]->IsValid())
            dw += m_palhtDir[i]->_DeleteIf(pfnPredicate, pvState, lkp);
        
        m_palhtDir[i]->WriteUnlock();
        
        if (lkp == LKP_ABORT
            ||  lkp == LKP_PERFORM_STOP
            ||  lkp == LKP_DELETE_STOP)
        {
            break;
        }
    }

    return dw;
} // CLKRHashTable::DeleteIf



//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_ApplyIf
// Synopsis:
// Returns:  Number of successful actions
//------------------------------------------------------------------------

DWORD
CLKRLinearHashTable::_ApplyIf(
    LKR_PFnRecordPred   pfnPredicate,
    LKR_PFnRecordAction pfnAction,
    void*               pvState,
    LK_LOCKTYPE         lkl,
    LK_PREDICATE&       rlkp)
{
    INCREMENT_OP_STAT(ApplyIf);

    if (!IsUsable())
        return static_cast<DWORD>(LK_UNUSABLE);

    IRTLASSERT(lkl == LKL_WRITELOCK  ?  IsWriteLocked()  :  IsReadLocked());
    IRTLASSERT(pfnPredicate != NULL  &&  pfnAction != NULL);

    if ((lkl == LKL_WRITELOCK  ?  !IsWriteLocked()  :  !IsReadLocked())
            ||  pfnPredicate == NULL  ||  pfnAction == NULL)
        return 0;

    DWORD cActions = 0;

    for (DWORD iBkt = 0;  iBkt < m_cActiveBuckets;  ++iBkt)
    {
        PBucket const pbkt = _BucketFromAddress(iBkt);
        IRTLASSERT(pbkt != NULL);

        if (_UseBucketLocking())
        {
            if (lkl == LKL_WRITELOCK)
                pbkt->WriteLock();
            else
                pbkt->ReadLock();
        }

        for (PNodeClump pncCurr = &pbkt->m_ncFirst, pncPrev = NULL;
                        pncCurr != NULL;
                        pncPrev = pncCurr, pncCurr = pncCurr->m_pncNext)
        {
            FOR_EACH_NODE_DECL(i)
            {
                if (pncCurr->IsEmptySlot(i))
                {
                    IRTLASSERT(pncCurr->NoMoreValidSlots(i));
                    IRTLASSERT(0 == _IsBucketChainCompact(pbkt));
                    goto unlock;
                }
                else
                {
                    rlkp = (*pfnPredicate)(pncCurr->m_pvNode[i], pvState);

                    switch (rlkp)
                    {
                    case LKP_ABORT:
                        if (_UseBucketLocking())
                        {
                            if (lkl == LKL_WRITELOCK)
                                pbkt->WriteUnlock();
                            else
                                pbkt->ReadUnlock();
                        }
                        
                        return cActions;
                        break;

                    case LKP_NO_ACTION:
                        // nothing to do
                        break;

                    case LKP_DELETE:
                    case LKP_DELETE_STOP:
                        if (lkl != LKL_WRITELOCK)
                        {
                            if (_UseBucketLocking())
                                pbkt->ReadUnlock();

                            return cActions;
                        }

                        // fall through

                    case LKP_PERFORM:
                    case LKP_PERFORM_STOP:
                    {
                        LK_ACTION lka;

                        if (rlkp == LKP_DELETE  ||  rlkp == LKP_DELETE_STOP)
                        {
                            _DeleteNode(pbkt, pncCurr, pncPrev, i,
                                        LKAR_APPLY_DELETE);

                            ++cActions;
                            lka = LKA_SUCCEEDED;
                        }
                        else
                        {
                            lka = (*pfnAction)(pncCurr->m_pvNode[i], pvState);

                            switch (lka)
                            {
                            case LKA_ABORT:
                                if (_UseBucketLocking())
                                {
                                    if (lkl == LKL_WRITELOCK)
                                        pbkt->WriteUnlock();
                                    else
                                        pbkt->ReadUnlock();
                                }
                                
                                return cActions;
                                
                            case LKA_FAILED:
                                // nothing to do
                                break;
                                
                            case LKA_SUCCEEDED:
                                ++cActions;
                                break;
                                
                            default:
                                IRTLASSERT(! "Unknown LK_ACTION in ApplyIf");
                                break;
                            }
                        }

                        if (rlkp == LKP_PERFORM_STOP
                            ||  rlkp == LKP_DELETE_STOP)
                        {
                            if (_UseBucketLocking())
                            {
                                if (lkl == LKL_WRITELOCK)
                                    pbkt->WriteUnlock();
                                else
                                    pbkt->ReadUnlock();
                            }
                            
                            return cActions;
                        }

                        break;
                    }

                    default:
                        IRTLASSERT(! "Unknown LK_PREDICATE in ApplyIf");
                        break;
                    }
                }
            }
        }

      unlock:
        if (_UseBucketLocking())
        {
            if (lkl == LKL_WRITELOCK)
                pbkt->WriteUnlock();
            else
                pbkt->ReadUnlock();
        }
    }

    return cActions;
} // CLKRLinearHashTable::_ApplyIf



//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_DeleteIf
// Synopsis: Deletes all records that match the predicate
// Returns:  Count of successful deletions
//------------------------------------------------------------------------

DWORD
CLKRLinearHashTable::_DeleteIf(
    LKR_PFnRecordPred   pfnPredicate,
    void*               pvState,
    LK_PREDICATE&       rlkp)
{
    INCREMENT_OP_STAT(DeleteIf);

    if (!IsUsable())
        return static_cast<DWORD>(LK_UNUSABLE);

    IRTLASSERT(IsWriteLocked());
    IRTLASSERT(pfnPredicate != NULL);

    if (!IsWriteLocked()  ||  pfnPredicate == NULL)
        return 0;

    DWORD cActions = 0;

    for (DWORD iBkt = 0;  iBkt < m_cActiveBuckets;  ++iBkt)
    {
        PBucket const pbkt = _BucketFromAddress(iBkt);
        IRTLASSERT(pbkt != NULL);

        if (_UseBucketLocking())
            pbkt->WriteLock();

        for (PNodeClump pncCurr = &pbkt->m_ncFirst, pncPrev = NULL;
                        pncCurr != NULL;
                        pncPrev = pncCurr, pncCurr = pncCurr->m_pncNext)
        {
            FOR_EACH_NODE_DECL(i)
            {
                if (pncCurr->IsEmptySlot(i))
                {
                    IRTLASSERT(pncCurr->NoMoreValidSlots(i));
                    IRTLASSERT(0 == _IsBucketChainCompact(pbkt));
                    goto unlock;
                }
                else
                {
                    rlkp = (*pfnPredicate)(pncCurr->m_pvNode[i], pvState);

                    switch (rlkp)
                    {
                    case LKP_ABORT:
                        if (_UseBucketLocking())
                            pbkt->WriteUnlock();

                        return cActions;
                        break;

                    case LKP_NO_ACTION:
                        // nothing to do
                        break;

                    case LKP_PERFORM:
                    case LKP_PERFORM_STOP:
                    case LKP_DELETE:
                    case LKP_DELETE_STOP:
                    {
                        _DeleteNode(pbkt, pncCurr, pncPrev, i,
                                    LKAR_DELETEIF_DELETE);

                        ++cActions;

                        if (rlkp == LKP_PERFORM_STOP
                            ||  rlkp == LKP_DELETE_STOP)
                        {
                            if (_UseBucketLocking())
                                pbkt->WriteUnlock();

                            return cActions;
                        }

                        break;
                    }

                    default:
                        IRTLASSERT(! "Unknown LK_PREDICATE in DeleteIf");
                        break;
                    }
                }
            }
        }

      unlock:
        if (_UseBucketLocking())
            pbkt->WriteUnlock();
    }

    return cActions;
} // CLKRLinearHashTable::_DeleteIf

#endif // LKR_APPLY_IF


#ifndef __LKRHASH_NO_NAMESPACE__
};
#endif // !__LKRHASH_NO_NAMESPACE__
