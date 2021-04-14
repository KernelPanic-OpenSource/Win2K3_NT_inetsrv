/*++


Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    MBSchemaWriter.cpp

Abstract:

    Implementation of the class that writes schema extensions.
    If there are schema extensions, then, we create an extensions file
    (MD_SCHEMA_EXTENSION_FILE_NAMEW), that contains the schema extension
    descriptions and we then compile it into the schema bin format.

Author:

    Varsha Jayasimha (varshaj)        30-Nov-1999

Revision History:


--*/

#include "precomp.hxx"

typedef  CMBCollectionWriter*   LP_CMBCollectionWriter;

#define  MAX_COLLECTIONS        50

/***************************************************************************++
Routine Description:

    Constructor for CMBSchemaWriter.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/
CMBSchemaWriter::CMBSchemaWriter(CWriter* i_pcWriter):
m_apCollection(NULL),
m_cCollection(0),
m_iCollection(0),
m_pCWriter(NULL)
{
    //
    // Assumption: i_pcWriter will be valid for the
    // lifetime of the schema writer object.
    //

    m_pCWriter = i_pcWriter;

} // CMBSchemaWriter::CMBSchemaWriter


/***************************************************************************++
Routine Description:

    Destructor for CMBSchemaWriter.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/
CMBSchemaWriter::~CMBSchemaWriter()
{
    if(NULL != m_apCollection)
    {
        for(ULONG i=0; i<m_iCollection; i++)
        {
            if(NULL != m_apCollection[i])
            {
                delete m_apCollection[i];
                m_apCollection[i] = NULL;
            }
        }

        delete [] m_apCollection;
        m_apCollection = NULL;
    }

    m_cCollection = 0;
    m_iCollection = 0;

} // CMBSchemaWriter::~CMBSchemaWriter


/***************************************************************************++
Routine Description:

    Creates a new collection writer and saves it in its list

Arguments:

    [in]  Collection name.
    [in]  Bool to indicate container
    [in]  Container list, if container
    [out] New collection writer object

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CMBSchemaWriter::GetCollectionWriter(LPCWSTR                i_wszCollection,
                                             BOOL                   i_bContainer,
                                             LPCWSTR                i_wszContainerClassList,
                                             CMBCollectionWriter**  o_pMBCollectionWriter)
{
    CMBCollectionWriter*    pCMBCollectionWriter = NULL;
    HRESULT                 hr                   = S_OK;

    *o_pMBCollectionWriter = NULL;

    if(m_iCollection == m_cCollection)
    {
        hr = ReAllocate();

        if(FAILED(hr))
        {
            return hr;
        }
    }

    pCMBCollectionWriter = new CMBCollectionWriter();
    if(NULL == pCMBCollectionWriter)
    {
        return E_OUTOFMEMORY;
    }

    pCMBCollectionWriter->Initialize(i_wszCollection,
                                     i_bContainer,
                                     i_wszContainerClassList,
                                     m_pCWriter);

    m_apCollection[m_iCollection++] = pCMBCollectionWriter;

    *o_pMBCollectionWriter = pCMBCollectionWriter;

    return S_OK;

} // CMBSchemaWriter::GetCollectionWriter


/***************************************************************************++
Routine Description:

    ReAllocates its list of collection writers.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CMBSchemaWriter::ReAllocate()
{
    CMBCollectionWriter** pSav = NULL;

    pSav = new LP_CMBCollectionWriter[m_cCollection + MAX_COLLECTIONS];
    if(NULL == pSav)
    {
        return E_OUTOFMEMORY;
    }
    memset(pSav, 0, (sizeof(LP_CMBCollectionWriter))*(m_cCollection + MAX_COLLECTIONS));

    if(NULL != m_apCollection)
    {
        memcpy(pSav, m_apCollection, (sizeof(LP_CMBCollectionWriter))*(m_cCollection));
        delete [] m_apCollection;
        m_apCollection = NULL;
    }

    m_apCollection = pSav;
    m_cCollection = m_cCollection + MAX_COLLECTIONS;

    return S_OK;

} // CMBSchemaWriter::ReAllocate


/***************************************************************************++
Routine Description:

    Wites the schema.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CMBSchemaWriter::WriteSchema()
{
    HRESULT hr = S_OK;

    for(ULONG i=0; i<m_iCollection; i++)
    {
        hr = m_apCollection[i]->WriteCollection();

        if(FAILED(hr))
        {
            return hr;
        }
    }

    return hr;

} // CMBSchemaWriter::WriteSchema

