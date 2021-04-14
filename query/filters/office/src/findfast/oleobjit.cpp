
/******************************************************************************

   OleObjectIterator 

*****************************************************************************/
#include "pp97rdr.h"
//KYLEP
#include "OleObjIt.h"
#include "zlib.h"
#include <winnls.h>
#include <assert.h>

//      OleObjectIterator Class

OleObjectIterator::OleObjectIterator(IStorage* iStore):m_iStore(iStore)
{
        m_pRefListHead = NULL;

        if(!Initialize())
                m_iStore = NULL;
        else
                m_iStore->AddRef();
}

OleObjectIterator::~OleObjectIterator()
{ 
        if(m_iStore)
                m_iStore->Release();
        delete m_pRefListHead;
}

// Make a list of all containers in the Powerpoint Document Stream.

BOOL OleObjectIterator::Initialize(void)
{
        // Find the offset to last edit.

        IStream *pStm = NULL;
        PSR_CurrentUserAtom currentUser;
        PSR_UserEditAtom        userEdit;
        LARGE_INTEGER li;
        ULARGE_INTEGER ul;
        HRESULT hr;
        RecordHeader rh;
        unsigned long   rd;

        PPT8Ref *pRef = NULL;

        unsigned long   Reference;  //  top 12 bits is the number of following, sequential offsets.
                                                //      lower 20 bits is the starting reference number
        unsigned long   numOfSeqOffsets;
        unsigned long   startRefNum;
        unsigned long   offset;
        LARGE_INTEGER liLast;
        BOOL fFirstLoop = TRUE;
        
        hr = m_iStore->OpenStream( CURRENT_USER_STREAM, NULL, STGM_READ | STGM_DIRECT | STGM_SHARE_EXCLUSIVE, NULL, &pStm );
        if( !SUCCEEDED(hr))
                return  FALSE;

        hr = pStm->Read(&rh, sizeof(rh), &rd);  // Read in the 8 bytes of the record header.
        if( !SUCCEEDED(hr) || rh.recType != PST_CurrentUserAtom)
        {
                pStm->Release();
                return  FALSE;
        }       
        hr = pStm->Read(&currentUser, sizeof(currentUser), &rd);
        pStm->Release();
        if( !SUCCEEDED(hr))
        {
                return  FALSE;
        }       

        // Open the Document Stream
        
        hr = m_iStore->OpenStream( DOCUMENT_STREAM, NULL, STGM_READ | STGM_DIRECT | STGM_SHARE_EXCLUSIVE, NULL, &m_pDocStream );
        if( !SUCCEEDED(hr))
                return  FALSE;

        li.LowPart = currentUser.offsetToCurrentEdit;
        li.HighPart = 0;
        hr = m_pDocStream->Seek(li,STREAM_SEEK_SET,&ul);        // Absolute seek to start of data.
        if(!SUCCEEDED(hr))
                goto LWrong;
        
        hr=m_pDocStream->Read(&rh, sizeof(rh), &rd);
        if(!SUCCEEDED(hr))
                goto LWrong;
        //assert( rh.recType == PST_UserEditAtom );
        if (rh.recType != PST_UserEditAtom)
                goto LWrong;
        //assert( rh.recLen == sizeof(PSR_UserEditAtom));
        if (rh.recLen != sizeof(PSR_UserEditAtom))
                goto LWrong;
        hr = m_pDocStream->Read(&userEdit, sizeof(userEdit), &rd);
        if( !SUCCEEDED(hr))
            goto LWrong;
        
        //Loop through all User Edits to gather all the Persist Directory Entries.
        pRef = new PPT8Ref(0,0);
        if (!pRef)
                goto LWrong;

        m_pRefListHead = new PPT8RefList(pRef);
        if (!m_pRefListHead)
        {
                delete pRef;
                goto LWrong;
        }
                
        while(1)  // Read and save all persist directory entries.
        {
                
                li.LowPart = userEdit.offsetPersistDirectory;
                li.HighPart = 0;
                if (!fFirstLoop && li.LowPart == liLast.LowPart && li.HighPart == liLast.HighPart)
                  break;
                liLast = li;
                if(fFirstLoop)
                        fFirstLoop = FALSE;
                hr = m_pDocStream->Seek(li,STREAM_SEEK_SET,&ul);        // Absolute seek to start of data.
        
                if( !SUCCEEDED(hr))
                        goto LWrong;
                                
                hr = m_pDocStream->Read(&rh, sizeof(rh), &rd);
                if(!SUCCEEDED(hr))
                        goto LWrong;
                //assert( rh.recType == PST_PersistPtrIncrementalBlock );                       
                if (rh.recType != PST_PersistPtrIncrementalBlock)
                        break;
                        
                for (unsigned long j=0; j<rh.recLen; )  // Read all the data in the Directory.
                {
                        hr = m_pDocStream->Read(&Reference, sizeof(Reference), &rd); // Read a reference.
                        if (!SUCCEEDED(hr))
                            goto LWrong;    // Exiting two blocks - destruction might be required at a later date
                        j+=  sizeof(Reference);
                        numOfSeqOffsets = Reference >> 20;
                        startRefNum = Reference & 0x000FFFFF;
                                                
                        for(unsigned long k=startRefNum; k<numOfSeqOffsets+startRefNum; k++) // Pick up the offsets.
                        {
                                if(m_pRefListHead->IsNewReference(k))          // If this is a new reference, get it.
                                {
                                        PPT8RefList *pRefListTemp;

                                        hr = m_pDocStream->Read(&offset, sizeof(offset), &rd);       // Read an offset.
                                        if(!SUCCEEDED(hr))
                                                goto LWrong;
                                        j+=  sizeof(offset);
                                        pRef = new PPT8Ref(k, offset);
                                        if (!pRef)
                                                break;
                                        pRefListTemp = new PPT8RefList(pRef);
                                        if (pRefListTemp)
                                                m_pRefListHead->AddToBack(pRefListTemp);                
                                        else
                                        {
                                                delete pRef;
                                                break;
                                        }
                                }
                                else
                                {
                                        hr = m_pDocStream->Read(&offset, sizeof(offset), &rd);       // Swallow duplicate entry.
                                        if (!SUCCEEDED(hr))
                                                goto LWrong;    // Exiting four blocks - destruction might be required at a later date

                                        j+=  sizeof(offset);
                                } // End if     
                                        
                        }// End for
                              
                } // End for
                
                if(!userEdit.offsetLastEdit) break;             // If no more User Edit Atoms, break.
                li.LowPart = userEdit.offsetLastEdit;
                li.HighPart = 0;
                hr = m_pDocStream->Seek(li,STREAM_SEEK_SET,&ul);        // Absolute seek to start of data.
                if( !SUCCEEDED(hr))
                {
                        m_pDocStream->Release();
                        return  FALSE;
                }
                                
                hr = m_pDocStream->Read(&rh, sizeof(rh), &rd);
                if(!SUCCEEDED(hr))
                        goto LWrong;    // Exiting a block - destruction might be required at a later date
                
                //assert( rh.recType == PST_UserEditAtom );
                if (rh.recType != PST_UserEditAtom)
                        break;
                //assert( rh.recLen == sizeof(PSR_UserEditAtom));
                if (rh.recLen != sizeof(PSR_UserEditAtom))
                        break;
                hr = m_pDocStream->Read(&userEdit, sizeof(userEdit), &rd);
                if( !SUCCEEDED(hr))
                        goto LWrong;    // Exiting a block
        } // End while
        
        m_pRefList = m_pRefListHead; 
        m_pDocStream->Release();
        m_pDocStream = 0;
        return TRUE;
LWrong:
        m_pDocStream->Release();
        m_pDocStream = 0;
        return FALSE;
}

// Starting from the last container read, look for an OLE object.
  
HRESULT OleObjectIterator::GetNextEmbedding(IStorage ** ppstg)
{
//              Get the next object 
        HRESULT hr = STG_E_UNKNOWN;
        unsigned long   myRef;
        unsigned long   myOffset;
        LARGE_INTEGER li;
        ULARGE_INTEGER ul;
        unsigned long rd; 
        RecordHeader rh;

        *ppstg = NULL;
        if(!m_iStore)   // If storage pointer is null, return error
                return hr;

        hr = m_iStore->OpenStream( DOCUMENT_STREAM, NULL, STGM_READ | STGM_DIRECT | STGM_SHARE_EXCLUSIVE, NULL, &m_pDocStream );
        if( !SUCCEEDED(hr))
                return  hr;

        while(1)
        {
                m_pRefList = m_pRefList->GetNext();
                if(!m_pRefList) // No more containers to seek.
                {
                        m_pDocStream->Release();
                        m_pDocStream=0;
                        return hr;
                }
                myRef = m_pRefList->GetRef()->GetRefNum();
                myOffset = m_pRefList->GetRef()->GetOffset();

                // For each reference see if there is an OLE container

                if(myRef)
                {
                        li.LowPart = myOffset;
                        li.HighPart = 0;
                        hr = m_pDocStream->Seek(li,STREAM_SEEK_SET,&ul);        // Absolute seek to start of data.
                        if( !SUCCEEDED(hr))
                        {
                                m_pDocStream->Release();
                                m_pDocStream = 0;
                                return  hr;
                        }

                        hr = m_pDocStream->Read(&rh, sizeof(rh), &rd);  // Read the header
                        if( !SUCCEEDED(hr))
                        {
                                m_pDocStream->Release();
                                m_pDocStream = 0;
                                return  hr;
                        }

                        if(PST_ExOleObjStg==rh.recType)   // OLE Object?
                        {
                                // Get the OLE data and return an IStorage
                                hr = ReadOLEData(ppstg, rh);
                                m_pDocStream->Release();
                                m_pDocStream = 0;
                                return hr;
                        }
                } // End if (myRef)
        }       // End while
}

// Read the data from the container, uncompress if necessary, and save to an ILockByte buffer.

// Maximum embedded limit - arbitrary 1 GB
#define MAX_EMBEDDED_DATA 0x40000000Lu

HRESULT OleObjectIterator::ReadOLEData(IStorage ** ppstg, RecordHeader rh)
{
        HRESULT hr = STG_E_UNKNOWN;
        unsigned long expandedSize;
        HGLOBAL   hOleData=0;
        void* oleCompData;
        unsigned long rd;
        int result = 0;

        *ppstg = NULL;
        STATSTG sDocStat;
        HRESULT h = m_pDocStream->Stat(&sDocStat, STATFLAG_NONAME);

        if ( FAILED( h ) )
            return h;

        ULONGLONG uliMaxDocSize = (sDocStat.cbSize.QuadPart < MAX_EMBEDDED_DATA ? sDocStat.cbSize.QuadPart : MAX_EMBEDDED_DATA);

        if (rh.recLen > uliMaxDocSize)
            return E_INVALIDARG;

        if(rh.recInstance == 0) // No compression of OLE data
        {
                expandedSize = rh.recLen;
                hOleData = GlobalAlloc(GMEM_MOVEABLE, expandedSize);

                if(hOleData)
                {
                        void *pOleData = GlobalLock(hOleData);
                        if (0 == pOleData)
                        {
                            GlobalFree(hOleData);
                            return HRESULT_FROM_WIN32(GetLastError());
                        }
                        hr = m_pDocStream->Read(pOleData, expandedSize, &rd );  // Read the OLE data.
                        GlobalUnlock(hOleData);
                        if (!SUCCEEDED(hr))
                        {
                            GlobalFree(hOleData);
                            return hr; // Not cleaning up - this is not the initializer
                        }
                }
                else
                        return E_OUTOFMEMORY;
        }       

        else if(rh.recInstance == 1)    // Data is compressed
        {
                hr = m_pDocStream->Read(&expandedSize, sizeof(expandedSize), &rd);  // Read the decompressed size.
                if (!SUCCEEDED(hr))
                    return hr;

                if ( expandedSize >= MAX_EMBEDDED_DATA )
                    return E_INVALIDARG;
                
                // If the above read fails, we could do something very bad here...
                oleCompData = new BYTE[rh.recLen - sizeof(expandedSize)];
                hOleData = GlobalAlloc(GMEM_MOVEABLE, expandedSize);
                // Remember to clean up the above, on failure
                
                if(oleCompData && hOleData)
                {
                    void *pOleData = GlobalLock(hOleData);
                    if (0 == pOleData)
                    {
                        GlobalFree(hOleData);
                        return HRESULT_FROM_WIN32(GetLastError());
                    }
                    hr = m_pDocStream->Read(oleCompData, rh.recLen - sizeof(expandedSize), &rd);  // Read the compressed OLE data.
                    if (!SUCCEEDED(hr))
                    {
                        delete [] oleCompData;
                        GlobalFree(hOleData);
                        return hr;
                    }
                    result = uncompress( (BYTE *)pOleData, &expandedSize, (BYTE*)oleCompData, rh.recLen - sizeof(expandedSize) );
                    GlobalUnlock(hOleData);
                }
                else
                {
                        if (hOleData)
                                GlobalFree(hOleData);
                        if (oleCompData)
                                delete [] oleCompData;
                        return E_OUTOFMEMORY;
                }
                delete [] oleCompData;
        }
        else
                return hr;

        //assert(result ==0);   
        if(result != 0)
        {
                GlobalFree(hOleData);
                return hr;
        }

        ILockBytes *pLockBytes;
        hr=CreateILockBytesOnHGlobal(hOleData, TRUE, &pLockBytes);
        if (FAILED(hr))
        {
            GlobalFree(hOleData);
            return hr;
        }
        // From here on, hOleData is owned by pLockBytes

        hr = StgOpenStorageOnILockBytes(pLockBytes, NULL, STGM_READ | STGM_SHARE_DENY_WRITE, NULL, 0, ppstg);
        pLockBytes->Release();  // The IStorage holds a refcount, so Release()ing it,
                                // triggers a Release() on pLockBytes, which cleans up the hOleData
        pLockBytes = 0;
        if (FAILED(hr))
        {
                *ppstg = NULL;
        }
        return hr;
}

// Utility classes

PPT8RefList::~PPT8RefList()
{
        delete  m_ref;
        m_ref = 0;
        delete  m_nextRef;
        m_nextRef = 0;
} 

void PPT8RefList::AddToBack(PPT8RefList* refList)
{
        if(!m_nextRef)
                m_nextRef = refList;    // If first added Ref, point to it.
        else
        {
                PPT8RefList     *m_pRefList=this;
                while(m_pRefList->GetNext())
                        m_pRefList = m_pRefList->GetNext();     // Find end of the line.
                refList->SetNext(0);
                m_pRefList->SetNext(refList);
        }
}


//      When there are more than one User Edit, there may be duplicated references.
//      Check here for these duplicates. Return FALSE if found.

BOOL    PPT8RefList::IsNewReference(unsigned long ref)
{
        PPT8RefList     *m_pRefList=this;
        while(m_pRefList = m_pRefList->GetNext()) // Loop until we get top the end of the list.
                if(m_pRefList->m_ref->GetRefNum() == ref) return(FALSE);
        
        return(TRUE);
}

BOOL    PPT8RefList::GetOffset(unsigned long ref, unsigned long& offset)        // Returns the offset for a given reference. 
{
        PPT8RefList     *m_pRefList=this;
        while(m_pRefList = m_pRefList->GetNext()) // Loop until we get top the end of the list.
                if(m_pRefList->m_ref->GetRefNum() == ref)
                {
                        offset = m_pRefList->m_ref->GetOffset();
                        return(TRUE);
                }
        return(FALSE);
} 

