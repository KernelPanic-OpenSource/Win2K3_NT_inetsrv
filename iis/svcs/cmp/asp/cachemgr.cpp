/*-----------------------------------------------------------------------------
Microsoft Denali

Microsoft Confidential
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Template Cache Manager

File: CacheMgr.cpp

Owner: DGottner

Template cache manager implementation
-----------------------------------------------------------------------------*/
#include "denpre.h"
#pragma hdrstop

#include "perfdata.h"
#include "memchk.h"

CTemplateCacheManager   g_TemplateCache;
CIncFileMap             g_IncFileMap;

BOOL  CTemplateCacheManager::m_fFailedToInitPersistCache = FALSE;
char  CTemplateCacheManager::m_szPersistCacheDir[MAX_PATH];

DWORD   g_nScavengedPersisted = 0;
DWORD   g_nScavengedPurged = 0;
DWORD   g_nScavengedPersistFailed = 0;

extern BOOL g_fUNCChangeNotificationEnabled;

/*===================================================================
ZapTemplate

Decrement the ref. count of a template to remove it from cache.
If the template is global.asa, that's all we do because application
manager has the last reference.  Otherwise, we Release the template
by calling CTemplate::End() to also free references to it from the
debugger.

Parameters: pTemplate - template pointer to Release() from cache

Returns: new ref. count
===================================================================*/

static inline
ULONG ZapTemplate(CTemplate *pTemplate)
    {
    if (! pTemplate->FGlobalAsa())
        return pTemplate->End();

    else
        return pTemplate->Release();
    }


/*  ****************************************************************************
    CCacheManager member functions
*/

/*===================================================================
CTemplateCacheManager::CTemplateCacheManager

Parameters: N/A

Returns: N/A
===================================================================*/

CTemplateCacheManager::CTemplateCacheManager()
{
    m_pHashTemplates = NULL;
    m_szPersistCacheDir[0] = '\0';
    m_fFailedToInitPersistCache = FALSE;
    m_dwTemplateCacheTag = 0;
    m_hOnInitCleanupThread = NULL;
    m_cCleanupThreads = 0;

    ZeroMemory (&m_hCleanupThreads, sizeof(m_hCleanupThreads));
}


/*===================================================================
CTemplateCacheManager::~CTemplateCacheManager

Parameters: N/A

Returns: N/A
===================================================================*/

CTemplateCacheManager::~CTemplateCacheManager()
{
    if (m_hOnInitCleanupThread != NULL) {
        WaitForSingleObject(m_hOnInitCleanupThread, INFINITE);
        CloseHandle(m_hOnInitCleanupThread);
        m_hOnInitCleanupThread = NULL;
    }

    if (!m_fFailedToInitPersistCache) {
        RemoveDirectoryA(m_szPersistCacheDir);
    }
}


/*===================================================================
CTemplateCacheManager::Init

Init the template cache manager - phase 1 - that which can be done
with just default values in Glob.

Parameters: None

Returns: Completion Status
===================================================================*/

HRESULT CTemplateCacheManager::Init()
    {
    HRESULT hrInit;
    ErrInitCriticalSection(&m_csUpdate, hrInit);
    if (FAILED(hrInit))
        return(hrInit);

    // allocate the initial CTemplateHashTable

    m_pHashTemplates = new CTemplateHashTable;

    // Initialize the Cache Tag.
   	m_dwTemplateCacheTag = GetTickCount();

    return S_OK;
    }

/*  ****************************************************************************
    CTemplateCacheManager member functions
*/

/*===================================================================
CTemplateCacheManager::UnInit

Parameters: N/A

Returns: Completion status
===================================================================*/

HRESULT CTemplateCacheManager::UnInit()
{

    if (m_pHashTemplates) {
        while (! m_pHashTemplates->FMemoryTemplatesIsEmpty()) {
		    CTemplate *pTemplate = static_cast<CTemplate *>(m_pHashTemplates->MemoryTemplatesBegin());
		    m_pHashTemplates->RemoveTemplate(pTemplate);
		    ZapTemplate(pTemplate);
		}

        while (! m_pHashTemplates->FPersistTemplatesIsEmpty()) {
		    CTemplate *pTemplate = static_cast<CTemplate *>(m_pHashTemplates->PersistTemplatesBegin());
		    m_pHashTemplates->RemoveTemplate(pTemplate);
		    ZapTemplate(pTemplate);
		}
    }
    delete m_pHashTemplates;
    m_pHashTemplates = NULL;

    // give any flush threads a chance to finish.  This is necessary
    // to prevent an AV by LKRHash.

    LockTemplateCache();

    if (m_cCleanupThreads)
    {
        WaitForMultipleObjects(
                 m_cCleanupThreads,
                 m_hCleanupThreads,
                 TRUE,         // wait for ALL event
                 INFINITE);    // Wait for as long as it takes.

        while( m_cCleanupThreads ) {
            CloseHandle(m_hCleanupThreads[ --m_cCleanupThreads ]);
        }
    }

    UnLockTemplateCache();

#ifndef PERF_DISABLE
    g_PerfData.Zero_MEMORYTEMPLCACHE();
    g_PerfData.Zero_TEMPLCACHE();
#endif
    DeleteCriticalSection(&m_csUpdate);

    return S_OK;
}

/*===================================================================
CTemplateCacheManager::FindCached

    Get a template from the cache only

NOTE: FindCached just checks for the existance of the template. It will not check if the files that make up the
template are valid or not like Load does. Please keep this assumption in mind while using this Method.

Parameters:
    szFile       - file to find in the cache
    ppTemplate   - [out] template object found

Returns:
    HRESULT (S_OK if found, S_FALSE if noe found)
===================================================================*/

HRESULT CTemplateCacheManager::FindCached(const TCHAR *szFile, DWORD dwInstanceID, CTemplate **ppTemplate)
{

    Assert(IsNormalized(szFile));
    if (!ppTemplate)
    	return E_POINTER;

    LockTemplateCache();

    m_pHashTemplates->FindTemplate(CTemplateKey(szFile, dwInstanceID), ppTemplate);
    if (*ppTemplate)
        {
        if (!(*ppTemplate)->m_fReadyForUse)
            *ppTemplate = NULL;         // not ready - as if nor found
        else
            (*ppTemplate)->AddRef();    // addref inside critical section
        }

    UnLockTemplateCache();

    return *ppTemplate? S_OK : S_FALSE;
}

/*===================================================================
CTemplateCacheManager::Load

    Get a template from the cache, or load it into cache

Parameters:
    szFile - file to load into the cache

Returns: N/A
===================================================================*/

HRESULT CTemplateCacheManager::Load(BOOL fRunGlobalAsp, const TCHAR *szFile, DWORD dwInstanceID, CHitObj *pHitObj, CTemplate **ppTemplate, BOOL *pfTemplateInCache)
    {
    HRESULT hr = S_OK;                   // return value
    HRESULT (CTemplate::*pmAction)(CHitObj *);  // do we need to compile a new template or deliver an existing one?
    BOOL    fNeedsCheck = FALSE;

    Assert(IsNormalized(szFile));

    BOOL fLocked = FALSE;

    // If this is the GLOBAL.ASA we can pick up
    // template directly from the application
    if (fRunGlobalAsp && pHitObj->PAppln()->PGlobalTemplate())
        {
        *ppTemplate = pHitObj->PAppln()->PGlobalTemplate();
        }
    // see if we already have looked up the template on the I/O thread...
    else if (!fRunGlobalAsp && pHitObj->GetTemplate())
        {
        *ppTemplate = pHitObj->GetTemplate();
        pHitObj->SetTemplate(NULL);
        }
    else
    // Otherwise we have to look for it in the cache
        {
        LockTemplateCache();
        fLocked = TRUE;
        m_pHashTemplates->FindTemplate(CTemplateKey(szFile, dwInstanceID), ppTemplate,&fNeedsCheck);
        }

    if (*ppTemplate != NULL)
        {
        // Template found in cache -> use it
        (*ppTemplate)->AddRef();
        *pfTemplateInCache = TRUE;

        (*ppTemplate)->IncrUseCount();

        if (fLocked)    // Global.Asa from App - no lock
            UnLockTemplateCache();

        pmAction = CTemplate::Deliver;
        }
    else
        {
        *pfTemplateInCache = FALSE;

        Assert(fLocked); // only could get here if not found in the hash table
        UnLockTemplateCache();

        // Create and init new template outside of crirical section

        CTemplate *pNewTemplate = new CTemplate;

        if (!pNewTemplate)
            hr = E_OUTOFMEMORY;

        if (SUCCEEDED(hr))
            hr = pNewTemplate->Init(pHitObj, !!fRunGlobalAsp, CTemplateKey(szFile, dwInstanceID));

        if (SUCCEEDED(hr))
            {
            LockTemplateCache();

            // Try to find if inserted by another thread
            m_pHashTemplates->FindTemplate(CTemplateKey(szFile, dwInstanceID), ppTemplate,&fNeedsCheck);

            if (*ppTemplate != NULL)
                {
                // Template found in cache -> use it
                (*ppTemplate)->AddRef();
                (*ppTemplate)->IncrUseCount();
                UnLockTemplateCache();
                pmAction = CTemplate::Deliver;
                }
            else
                {

                // since we are creating a new template, call FlushCache to make
                // sure that no script engines are cached with this name

                g_ScriptManager.FlushCache(szFile);

                // Insert the newly created template

                *ppTemplate = pNewTemplate;
                pNewTemplate = NULL; // not to be deleted later

                m_pHashTemplates->InsertTemplate(*ppTemplate);
                (*ppTemplate)->AddRef();

                if (Glob(dwScriptFileCacheSize) == 0) {
                    // This is special case when a valid template
                    // does not get added to the cache
                    // Don't attach such templates to debugger
                    (*ppTemplate)->m_fDontAttach = TRUE;
                }

                UnLockTemplateCache();

                pmAction = CTemplate::Compile;
                }
            }

        // cleanup new template if created but unused
        if (pNewTemplate)
            pNewTemplate->Release();
        }

    if (FAILED(hr))
        return hr;


    // init succeeded: compile or deliver the template, as required
    hr = ((*ppTemplate)->*pmAction)(pHitObj);

    if (pmAction == CTemplate::Compile && (*ppTemplate)->m_fDontCache)
        {
        /*  We were compiling and the compiler alerted us not to cache the failed template.
            Typically, this occurs when compile failure was caused by something other than
            bad template syntax (permissions failure, bad include file reference, etc.).

            We need to roll back to where the template did not exist.
        */

        // de-cache and release the template
        // NOTE we don't nullify template ptr, because we want ExecuteRequest to do the final release

        LockTemplateCache();
        if (m_pHashTemplates->RemoveTemplate(*ppTemplate) == LK_SUCCESS)
            ZapTemplate(*ppTemplate);
        UnLockTemplateCache();

		(*ppTemplate)->Release();
		*ppTemplate = NULL;
        }

    if (SUCCEEDED(hr) && fNeedsCheck && *ppTemplate != NULL)
    {
        if (!(*ppTemplate)->ValidateSourceFiles(pHitObj->PIReq()))
        {
            // Template is invalid (out of date)
            LockTemplateCache();

            if (m_pHashTemplates->RemoveTemplate(*ppTemplate) == LK_SUCCESS)
                ZapTemplate (*ppTemplate);

            UnLockTemplateCache();
        }
    }

    LockTemplateCache();

    BOOL    bTemplateRemoved = FALSE;

    // Remove old scripts from cache

    while (!m_pHashTemplates->FMemoryTemplatesIsEmpty()
           && (m_pHashTemplates->InMemoryTemplates() > Glob(dwScriptFileCacheSize))) {
        Assert (!m_pHashTemplates->FMemoryTemplatesIsEmpty());
        CTemplate *pOldTemplate = static_cast<CTemplate *>(m_pHashTemplates->MemoryTemplatesEnd());

        // don't call ScavengePersistCache in this call.  We'll call it once at the end

        m_pHashTemplates->RemoveTemplate(pOldTemplate, TRUE, FALSE);

        bTemplateRemoved = TRUE;

        // flush the corresponding script engines.  But only if the template
        // is valid.

        if (pOldTemplate->FIsValid()) {
            g_ScriptManager.FlushCache(pOldTemplate->GetSourceFileName());
        }

        // Only Zap the template if it is not persisted.  The result of the above
        // call to RemoveTemplate is that the template may have been moved from the
        // memory cache to the persist cache.  In which case, the template is still
        // effectively cached.

        if (pOldTemplate->FIsPersisted() == FALSE) {

            ZapTemplate(pOldTemplate);
        }
    }

    // call ScavengePersistCache() once here...

    if (bTemplateRemoved)
        m_pHashTemplates->ScavengePersistCache();

    UnLockTemplateCache();

    // Store a pointer to the template with the application
    // if we haven't already done so
    if (SUCCEEDED(hr) && *ppTemplate && fRunGlobalAsp && pHitObj->PAppln()->PGlobalTemplate() == NULL)
        pHitObj->PAppln()->SetGlobalTemplate(*ppTemplate);

    // If we are shutting down, don't request change notification

    if (!IsShutDownInProgress() && *ppTemplate)
        {
        // If running on NT, and we just compiled the template
        // register all the directories used by this template
        // for change notification
        if (pmAction == CTemplate::Compile && SUCCEEDED(hr)) {
            if (!RegisterTemplateForChangeNotification(*ppTemplate, pHitObj->PAppln())) {
                LockTemplateCache();
                if (m_pHashTemplates->RemoveTemplate(*ppTemplate) == LK_SUCCESS)
                    ZapTemplate(*ppTemplate);
                UnLockTemplateCache();
            }

            // also create the services config object

            hr = (*ppTemplate)->CreateTransServiceConfig(pHitObj->PAppln()->QueryAppConfig()->fTrackerEnabled());
        }

        // If running on NT, this is a new application, and the template is a global.asa
        // register this application for file change notifications
        if (SUCCEEDED(hr) && (*ppTemplate)->m_fGlobalAsa && pHitObj->FStartApplication())
            {
            RegisterApplicationForChangeNotification(*ppTemplate, pHitObj->PAppln());
            }
        }

    return hr;
    }



/*===================================================================
CTemplateCacheManager::Flush

Parameters:
    szFile - the file to remove from cache

Returns:
    None
===================================================================*/

void CTemplateCacheManager::Flush(const TCHAR *szFile, DWORD dwInstanceID)
    {
    LockTemplateAndIncFileCaches();

    Assert (IsNormalized(szFile));
    CTemplate *pTemplate;
    m_pHashTemplates->FindTemplate(CTemplateKey(szFile, dwInstanceID), &pTemplate);

    while (pTemplate != NULL)
        {
#ifndef PERF_DISABLE
        g_PerfData.Incr_TEMPLFLUSHES();
#endif

		m_pHashTemplates->RemoveTemplate(pTemplate);

        // Make sure anyone using this template can tell it is obsolete
        pTemplate->Zombify();

        // Don't flush engines if this is a global.asa file
        // We'll need the engines to run Application_OnEnd
        // The application will flush the engine from the cache
        // when it unints
        if (!FIsGlobalAsa(szFile))
            {
            g_ScriptManager.FlushCache(szFile);
            }

        ZapTemplate(pTemplate);

        // If wildcard was specified in Flush for Instance ID, there may be
        // more templates to remove.
        m_pHashTemplates->FindTemplate(CTemplateKey(szFile, dwInstanceID), &pTemplate);
        }

    UnLockTemplateAndIncFileCaches();
    }

/*===================================================================
CTemplateCacheManager::FlushAll

    Completely empties the template cache

Parameters:
    None

Returns:
    None
===================================================================*/

void CTemplateCacheManager::FlushAll(BOOL fDoLazyFlush)
{

    if (fDoLazyFlush)
	{
		m_dwTemplateCacheTag = GetTickCount();
		
		DBGPRINTF ((DBG_CONTEXT,"Using new Cache Tag to Invalidate Template\n"));
		
		return;
	}

    LockTemplateAndIncFileCaches();

    CTemplateHashTable  *pNewTable = NULL;
    HANDLE              hnd;

    // note that all of the following logic works on the premise that any
    // error causes the code to fall into the old mechanism of flushing
    // the hash table in place...

    // allocate a new table

    if (pNewTable = new CTemplateHashTable)
    {
        //
        // Create a thread to clean up the old table
        //
        DWORD nThreadIndex;

        //
        // if no threads started yet, use the first slot
        //
        if (m_cCleanupThreads) {
            nThreadIndex = 0;
            goto create_new_thread;
        }

        //
        // see if there is a thread that terminated
        //
        nThreadIndex = WaitForMultipleObjects(
                                    m_cCleanupThreads,
                                    m_hCleanupThreads,
                                    FALSE, // wait for any event
                                    0);    // return immediately

        Assert(nThreadIndex == WAIT_TIMEOUT);

        if (m_cCleanupThreads < MAX_CLEANUP_THREADS)
        {
            //
            // just get the next index
            //
            nThreadIndex = m_cCleanupThreads;
            goto create_new_thread;
        }

        goto Cleanup;

    create_new_thread:

        hnd = CreateThread(NULL, 0, CTemplateCacheManager::FlushHashTableThread, m_pHashTemplates, 0, NULL);

        if (hnd)
        {
            //
            // close the previous handle if we are reusing an entry
            //
            if (nThreadIndex < m_cCleanupThreads)
            {
                //
                // we are reusing a slot from a terminated thread
                //
                CloseHandle(m_hCleanupThreads[ nThreadIndex ]);

            } else {
                //
                // we are using a new slot
                //
                Assert(nThreadIndex == m_cCleanupThreads);

                m_cCleanupThreads++;
            }

            m_hCleanupThreads[ nThreadIndex ] = hnd;

            // all the above was successful, so note that the new table is the
            // current table in the cache, cleanup and exit.

            DBGPRINTF((DBG_CONTEXT, "[CTemplateCacheManager] Flushing entire cache on another thread.\n"));

            m_pHashTemplates = pNewTable;

            UnLockTemplateAndIncFileCaches();

            return;

        }
    }

Cleanup:
    // delete the new table if something above failed.

    if (pNewTable)
        delete pNewTable;

    DBGPRINTF((DBG_CONTEXT, "[CTemplateCacheManager] Flushing entire cache in place\n"));

    FlushHashTable(m_pHashTemplates);

    UnLockTemplateAndIncFileCaches();

    return;
}

/*===================================================================
CTemplateCacheManager::FlushHashTableThread

    Thread spun up by CTemplateCacheMgr::FlushAll() to flush all
    templates in the cache but not while under the critical section
    on the notification thread.  Prevents unwanted contention on the
    cache.

Parameters:
    None

Returns:
    None
===================================================================*/

DWORD CTemplateCacheManager::FlushHashTableThread(VOID  *pArg)
{
    CTemplateHashTable  *pTable = (CTemplateHashTable *)pArg;

    Assert(pTable);

    FlushHashTable(pTable);

    delete pTable;

    return S_OK;
}

/*===================================================================
CTemplateCacheManager::FlushHashTable

    Does the actual work of flushing the templates.

    This routine may or may not be under the global cache manager
    crit sec.  It will if the flush is happening on the notification
    thread.  It won't be if it's happening on the FlushHashTableThread.

Parameters:
    None

Returns:
    None
===================================================================*/

void CTemplateCacheManager::FlushHashTable(CTemplateHashTable  *pTable)
{
    // Delete templates from the cache until there are no more

    while (!pTable->FMemoryTemplatesIsEmpty()) {
        CTemplate *pTemplate = static_cast<CTemplate *>(pTable->MemoryTemplatesEnd());

        // Remove the template from its various data structures
		pTable->RemoveTemplate(pTemplate);

        // Make sure anyone using this template can tell it is obsolete
        pTemplate->Zombify();

        // Flush the engine for this template from the script engine cache
		//   (use hash key, in case template was previously a zombie.)
        g_ScriptManager.FlushCache(pTemplate->ExtractHashKey()->szPathTranslated);

        ZapTemplate(pTemplate);

    }

    // Delete templates from the cache until there are no more

    while (!pTable->FPersistTemplatesIsEmpty()) {
        CTemplate *pTemplate = static_cast<CTemplate *>(pTable->PersistTemplatesEnd());

        // Remove the template from its various data structures
		pTable->RemoveTemplate(pTemplate);

        // Make sure anyone using this template can tell it is obsolete
        pTemplate->Zombify();

        // Flush the engine for this template from the script engine cache
		//   (use hash key, in case template was previously a zombie.)
        g_ScriptManager.FlushCache(pTemplate->ExtractHashKey()->szPathTranslated);

        ZapTemplate(pTemplate);

    }
}


/*===================================================================
CTemplateCacheManager::FlushFiles

    Empties template cache of files that match a prefix

Parameters:
    None

Returns:
    None
===================================================================*/

void CTemplateCacheManager::FlushFiles(const TCHAR *szFilePrefix)
{
    LockTemplateAndIncFileCaches();
    BOOL    fDoingMemoryTemplates = TRUE;

    // Delete templates from the cache until there are no more

	CDblLink *pLink = m_pHashTemplates->MemoryTemplatesBegin();
	while (! (fDoingMemoryTemplates
                ? m_pHashTemplates->FMemoryTemplatesDblLinkAtEnd(pLink)
                : m_pHashTemplates->FPersistTemplatesDblLinkAtEnd(pLink))) {

		CDblLink *pNextLink = pLink->PNext();
		CTemplate *pTemplate = static_cast<CTemplate *>(pLink);

		if (_tcsncmp(pTemplate->ExtractHashKey()->szPathTranslated, szFilePrefix, _tcslen(szFilePrefix)) == 0) {
#if UNICODE
			DBGPRINTF((DBG_CONTEXT, "FlushFiles: flushing %S\n", pTemplate->ExtractHashKey()->szPathTranslated));
#else
            DBGPRINTF((DBG_CONTEXT, "FlushFiles: flushing %s\n", pTemplate->ExtractHashKey()->szPathTranslated));
#endif
			// Remove the template from its various data structures
			m_pHashTemplates->RemoveTemplate(pTemplate);

			// Make sure anyone using this template can tell it is obsolete
			pTemplate->Zombify();

			// Flush the engine for this template from the script engine cache
			//   (use hash key, in case template was previously a zombie.)
			g_ScriptManager.FlushCache(pTemplate->ExtractHashKey()->szPathTranslated);

			ZapTemplate(pTemplate);

#ifndef PERF_DISABLE
			g_PerfData.Incr_TEMPLFLUSHES();
#endif
        }

		pLink = pNextLink;

        if (fDoingMemoryTemplates && m_pHashTemplates->FMemoryTemplatesDblLinkAtEnd(pLink)) {
            fDoingMemoryTemplates = FALSE;
            pLink = m_pHashTemplates->PersistTemplatesBegin();
        }
    }

    UnLockTemplateAndIncFileCaches();
}


/*===================================================================
CTemplateCacheManager::AddApplicationToDebuggerUI

    Loop through the template cache, and create doc nodes for
    all templates that belong to the application

Parameters:
    pAppln - pointer to application to attach to.

Returns: N/A
===================================================================*/

void CTemplateCacheManager::AddApplicationToDebuggerUI(CAppln *pAppln)
    {
    CDblLink *pLink;
    for (pLink = m_pHashTemplates->MemoryTemplatesBegin(); !m_pHashTemplates->FMemoryTemplatesDblLinkAtEnd(pLink); pLink = pLink->PNext())
        {
        // Bug  92070:
        //   Determine if the template is a member of pAppln by comparing
        //   the virtual path of the template to the application's virtual
        //   path (previously compared physical paths)  Since a template
        //   can have multiple virtual paths, only the first instance wins.
        //   Thus the template will only appear in the application that first
        //   loaded it.

		CTemplate *pTemplate = static_cast<CTemplate *>(pLink);
        if (_tcscmp(pAppln->GetApplnPath(SOURCEPATHTYPE_VIRTUAL), pTemplate->GetApplnPath(SOURCEPATHTYPE_VIRTUAL)) == 0)
            pTemplate->AttachTo(pAppln);
        }
    }



/*===================================================================
CTemplateCacheManager::RemoveApplicationFromDebuggerUI

    Loop through the template cache, and remove doc nodes for
    all templates that belong to the application

Parameters:
    pAppln - pointer to application to detach from
             if pAppln is NULL, detach from ALL applications


Returns: N/A
===================================================================*/

void CTemplateCacheManager::RemoveApplicationFromDebuggerUI(CAppln *pAppln)
    {
    CDblLink *pLink;
    for (pLink = m_pHashTemplates->MemoryTemplatesBegin();
         !m_pHashTemplates->FMemoryTemplatesDblLinkAtEnd(pLink);
         pLink = pLink->PNext())
        {
		CTemplate *pTemplate = static_cast<CTemplate *>(pLink);
        if (pAppln != NULL)
            pTemplate->DetachFrom(pAppln);
        else
            pTemplate->Detach();
        }
    }

/*===================================================================
void CTemplateCacheManager::RegisterTemplateForChangeNotification

    Request to watch template directories for file changes

Parameters:
    A pointer to the template

Returns:
    BOOL    True if successfully registered for change notification
===================================================================*/
BOOL CTemplateCacheManager::RegisterTemplateForChangeNotification(CTemplate *pTemplate, CAppln  *pApplication)
{
    DWORD   dwValue = 0;

    // If the template has a UNC include or main file & we the registry key is absent (default) or the key is present and disabled.
    // Then we will use the flag this file one that requires monitoring to be performed by ASP.
    if (pTemplate->FIsUNC() && !g_fUNCChangeNotificationEnabled)
    {
            pTemplate->m_fNeedsMonitoring = TRUE;
            pTemplate->m_dwLastMonitored = GetTickCount ();
            return TRUE;
    }

    //else   (Local File or UNC file with ChangeNotification Enabled)

    STACK_BUFFER( tempPath, MAX_PATH );

    for (DWORD i = 0; i < pTemplate->m_cFilemaps; i++) {
        // Check if this directory is already registered for change notification

        // Pick out the directory portion of the path
        TCHAR *szEndOfPath = _tcsrchr(pTemplate->m_rgpFilemaps[i]->m_szPathTranslated, _T('\\'));
        size_t cch = DIFF(szEndOfPath - pTemplate->m_rgpFilemaps[i]->m_szPathTranslated)+1;

        if (tempPath.Resize((cch * sizeof(TCHAR)) + sizeof(TCHAR)) == FALSE) {
            // if failure to resize, unless disabled through the registry, flag the file for manual monitoring after releasing previously registered DME's
            if (SUCCEEDED(g_AspRegistryParams.GetFileMonitoringEnabled(&dwValue)) && dwValue == 0)
                continue;

            if (i > 0) {
                while (--i) {
                    pTemplate->m_rgpFilemaps[i]->m_pDME->Release();
                    pTemplate->m_rgpFilemaps[i]->m_pDME = NULL;
                }
            }

            pTemplate->m_fNeedsMonitoring = TRUE;
            pTemplate->m_dwLastMonitored = GetTickCount ();
            return TRUE;

        }
        TCHAR *szPath = (TCHAR *) tempPath.QueryPtr();
        _tcsncpy(szPath, pTemplate->m_rgpFilemaps[i]->m_szPathTranslated, cch);
        szPath[cch] = 0;

        // if the template is within the application's physical path, then it is
        // already being monitored.

        CASPDirMonitorEntry *pDME = NULL;

        if (pDME = pApplication->FPathMonitored(szPath)) {
            pDME->AddRef();
            pTemplate->m_rgpFilemaps[i]->m_pDME= pDME;
            continue;
        }

        if (RegisterASPDirMonitorEntry(szPath, &pDME)) {
            Assert(pDME);
            pTemplate->m_rgpFilemaps[i]->m_pDME= pDME;
        }
        else {
            // the current file failed to register.  Release all previous DMEs
            // and return FALSE...

            if (i > 0) {
                while (--i) {

                    pTemplate->m_rgpFilemaps[i]->m_pDME->Release();
                    pTemplate->m_rgpFilemaps[i]->m_pDME = NULL;
                }
            }

            // Without the monitoring magic we would return FALSE here but we give
            // it one more chance to live, except if the registry parameter is set to disable it.

            // if the registry setting asks for this to be disabled disable it.
            if (SUCCEEDED(g_AspRegistryParams.GetFileMonitoringEnabled(&dwValue)) && dwValue == 0)
                return FALSE;

            pTemplate->m_fNeedsMonitoring = TRUE;
            pTemplate->m_dwLastMonitored = GetTickCount ();
            return TRUE;

        }
    }
    return TRUE;
}

/*===================================================================
void CTemplateCacheManager::RegisterApplicationForChangeNotification

    Request to watch template directories for file changes

Parameters:
    A pointer to the template

Returns:
    BOOL    True if successfully registered for change notification
===================================================================*/
BOOL CTemplateCacheManager::RegisterApplicationForChangeNotification(CTemplate *pTemplate, CAppln *pApplication)
    {

    STACK_BUFFER( tempPath, MAX_PATH );

    // Start with 1 to skip GLOBAL.ASA that is always added
    // in hitobj.cpp when new application gets created

    for (DWORD i = 1; i < pTemplate->m_cFilemaps; i++)
        {

        // Add to list of file-application mappings
        g_FileAppMap.AddFileApplication(pTemplate->m_rgpFilemaps[i]->m_szPathTranslated, pApplication);

        // Check if this directory is already registered for change notification
        // Pick out the directory portion of the path
        TCHAR *szEndOfPath = _tcsrchr(pTemplate->m_rgpFilemaps[i]->m_szPathTranslated, _T('\\'));
        size_t cch = DIFF(szEndOfPath - pTemplate->m_rgpFilemaps[i]->m_szPathTranslated) + 1;

        if (tempPath.Resize((cch*sizeof(TCHAR)) + sizeof(TCHAR)) == FALSE) {

            // if failure, continue registering anyway...
            continue;
        }
        TCHAR *szPath = (TCHAR *) tempPath.QueryPtr();
        _tcsncpy(szPath, pTemplate->m_rgpFilemaps[i]->m_szPathTranslated, cch);
        szPath[cch] = 0;

        // if the template is within the application's physical path, then it is
        // already being monitored.

        if (pApplication->FPathMonitored(szPath)) {

            continue;
        }

        // Register directory for monitoring
        CASPDirMonitorEntry *pDME = NULL;
        if (RegisterASPDirMonitorEntry(szPath, &pDME))
            {
            Assert(pDME);
            pApplication->AddDirMonitorEntry(pDME);
            }
        }


        return TRUE;

    }


/*===================================================================
BOOL CTemplateCacheManager::ShutdownCacheChangeNotification

    Turn off change notification for changes to files in the cache

Parameters:
    None

Returns:
    Nothing
===================================================================*/
BOOL CTemplateCacheManager::ShutdownCacheChangeNotification()
    {

    BOOL fDoingMemoryTemplates = TRUE;

    LockTemplateCache();

    CTemplate *pTemplate = static_cast<CTemplate *>(m_pHashTemplates->MemoryTemplatesBegin());
    while (fDoingMemoryTemplates
             ? !m_pHashTemplates->FMemoryTemplatesDblLinkAtEnd(pTemplate)
             : !m_pHashTemplates->FPersistTemplatesDblLinkAtEnd(pTemplate)) {

        if(pTemplate->m_rgpFilemaps)
            {
            for(UINT i = 0; i < pTemplate->m_cFilemaps; i++)
                {
                // Give up our ref count on the directory monitor entry
                if (pTemplate->m_rgpFilemaps[i]->m_pDME)
                    {
                    pTemplate->m_rgpFilemaps[i]->m_pDME->Release();
                    pTemplate->m_rgpFilemaps[i]->m_pDME = NULL;
                    }
                }
            }

        pTemplate = static_cast<CTemplate *>(pTemplate->PNext());
        if (fDoingMemoryTemplates && m_pHashTemplates->FMemoryTemplatesDblLinkAtEnd(pTemplate)) {
            fDoingMemoryTemplates = FALSE;
            pTemplate = static_cast<CTemplate *>(m_pHashTemplates->PersistTemplatesBegin());
        }
    }

    UnLockTemplateCache();
    return TRUE;
    }

/*  ****************************************************************************
    CIncFileMap member functions
*/

/*===================================================================
CIncFileMap::CIncFileMap

Parameters: N/A

Returns: N/A
===================================================================*/

CIncFileMap::CIncFileMap()
    {
    }


/*===================================================================
CIncFileMap::~CIncFileMap

Parameters: N/A

Returns: N/A
===================================================================*/

CIncFileMap::~CIncFileMap()
    {
    }



/*===================================================================
CIncFileMap::Init

Parameters: None

Returns: Completion Status
===================================================================*/

HRESULT CIncFileMap::Init()
    {
    HRESULT hr;
    ErrInitCriticalSection(&m_csUpdate, hr);
    if (FAILED(hr))
        return(hr);
    return m_mpszIncFile.Init(CINCFILEBUCKETS);
    }


/*===================================================================
CIncFileMap::GetIncFile

    Get an inc-file from the cache, first storing it into cache if it is not yet there.

Parameters:
    szIncFile   - file name
    ppIncFile   - ptr-to-ptr to inc-file (out-parameter)

Returns: HRESULT
===================================================================*/

HRESULT CIncFileMap::GetIncFile(const TCHAR *szFile, CIncFile **ppIncFile)
    {
    HRESULT hrInit = S_OK;           // return value

    LockIncFileCache();

    Assert(IsNormalized(szFile));
    *ppIncFile = static_cast<CIncFile *>(m_mpszIncFile.FindElem(szFile, _tcslen(szFile)*sizeof(TCHAR)));

    // if we have a cached inc-file at this stage, it must be "reliable," so we use it.
    // else, if we have no cached inc-file, create a new one.
    if (*ppIncFile == NULL)
        {
        if ((*ppIncFile = new CIncFile) == NULL)
            {
            UnLockIncFileCache();
            return E_OUTOFMEMORY;
            }

        if (SUCCEEDED(hrInit = (*ppIncFile)->Init(szFile)))
            {
            // The hash table will hold a reference to the inc file
            (*ppIncFile)->AddRef();
            m_mpszIncFile.AddElem(*ppIncFile);
            }
        else
            {
            //
            // Init can fail with E_OUTOFMEMORY on call to SmallAlloc or due to a failure to init critical section.
            // Cleanup allocated memory for ppIncFile if that is the case.
            //
            delete *ppIncFile;
            *ppIncFile = NULL;
            }

        }

    if (SUCCEEDED(hrInit))
        {
        // The caller will hold a reference to the inc file
        (*ppIncFile)->AddRef();
        }

    UnLockIncFileCache();

    return hrInit;
    }



/*===================================================================
CIncFileMap::UnInit

Parameters: N/A

Returns: Completion status
===================================================================*/

HRESULT CIncFileMap::UnInit()
    {
    CIncFile *pNukeIncFile = static_cast<CIncFile *>(m_mpszIncFile.Head());
    while (pNukeIncFile != NULL)
        {
        CIncFile *pNext = static_cast<CIncFile *>(pNukeIncFile->m_pNext);
        pNukeIncFile->OnIncFileDecache();
        pNukeIncFile->Release();
        pNukeIncFile = pNext;
        }
    DeleteCriticalSection(&m_csUpdate);
    return m_mpszIncFile.UnInit();
    }



/*===================================================================
CIncFileMap::Flush

Parameters:
    szFile - the file to remove from cache

Returns:
    None
===================================================================*/

void CIncFileMap::Flush(const TCHAR *szFile)
    {
    LockTemplateAndIncFileCaches();

    Assert(IsNormalized(szFile));
    CIncFile *pIncFile = static_cast<CIncFile *>(m_mpszIncFile.FindElem(szFile, _tcslen(szFile)*sizeof(TCHAR)));

    if (pIncFile != NULL)
        {
        if (pIncFile->FlushTemplates())
            {
            // Remove from hash table
            m_mpszIncFile.DeleteElem(szFile, _tcslen(szFile)*sizeof(TCHAR));
            // The hash table gave up its reference
            // to the incfile
            pIncFile->OnIncFileDecache();
            pIncFile->Release();
            }
        }

    UnLockTemplateAndIncFileCaches();
    }



/*===================================================================
CIncFileMap::FlushFiles

Parameters:
    szFile - the file prefix to search for in cache

Returns:
    None
===================================================================*/

void CIncFileMap::FlushFiles(const TCHAR *szFilePrefix)
    {
    LockTemplateAndIncFileCaches();

    Assert(IsNormalized(szFilePrefix));
    CIncFile *pIncFile = static_cast<CIncFile *>(m_mpszIncFile.Head());

    while (pIncFile != NULL)
        {
		CIncFile *pNextFile = static_cast<CIncFile *>(pIncFile->m_pNext);

		int cchFilePrefix = _tcslen(szFilePrefix);
		if (pIncFile->m_cbKey >= (cchFilePrefix*(int)sizeof(TCHAR)) &&
		    _tcsncmp(reinterpret_cast<TCHAR *>(pIncFile->m_pKey), szFilePrefix, cchFilePrefix) == 0)
			{
#if UNICODE
			DBGPRINTF((DBG_CONTEXT, "FlushFiles: flushing %S\n", pIncFile->m_pKey));
#else
            DBGPRINTF((DBG_CONTEXT, "FlushFiles: flushing %s\n", pIncFile->m_pKey));
#endif
            if (pIncFile->FlushTemplates())
				{
				// Remove from hash table
				m_mpszIncFile.DeleteElem(pIncFile->m_pKey, pIncFile->m_cbKey);
				// The hash table gave up its reference
				// to the incfile
				pIncFile->OnIncFileDecache();
				pIncFile->Release();
				}
			}

		pIncFile = pNextFile;
        }

    UnLockTemplateAndIncFileCaches();
    }



/*  ****************************************************************************
    Non-class support functions
*/

/*===================================================================
FFileChangedSinceCached
Has the file changed since it was cached?

Parameters:
    szFile          - file name
    ftPrevWriteTime - the file's "previous write time"
                      (its last-write-time value when the file was cached)

Returns:
    TRUE or FALSE
===================================================================*/
BOOL FFileChangedSinceCached(const TCHAR *szFile, HANDLE hFile, FILETIME& ftPrevWriteTime)
    {
    BOOL           fRet = FALSE;   // return value
    FILETIME        ftLastWriteTime;

    if (FAILED(AspGetFileAttributes(szFile, hFile, &ftLastWriteTime)))
        {
        // assume file was changed if get file attributes failed
        fRet = TRUE;
        }

    if( 0 != CompareFileTime( &ftPrevWriteTime, &ftLastWriteTime) )
        {
        // file was changed if file times differ
        fRet = TRUE;
        }

    return fRet;
    }

/*===================================================================
 CTemplateCacheManager::CTemplateHashTable::TrimPersistCache

Parameters:
    dwTrimCount - the number of templates to trim from the cache

Returns:
    TRUE - if dwTrimCount was actually trimmed
    FALSE - if exited before dwTrimCount was met

===================================================================*/

BOOL CTemplateCacheManager::CTemplateHashTable::TrimPersistCache(DWORD  dwTrimCount)
{
    // enter a while loop to trim until the count is reached

    while(dwTrimCount--) {

        // if there isn't anything else to trim, we're done.  Return FALSE
        // to indicate that dwTrimCount was not met.

        if (m_dwPersistedTemplates == 0) {
            return(FALSE);
        }
        else {

            CTemplate   *pTemplate;

            // get the oldest template from the list

            pTemplate = static_cast<CTemplate *>(PersistTemplatesEnd());

            // remove the template.

            RemoveTemplate(pTemplate);

            ZapTemplate(pTemplate);

        }
    }

    // return TRUE to indicate that the TrimCount was met.
    return(TRUE);
}

/*===================================================================
 CTemplateCacheManager::CTemplateHashTable::ScavengePersistCache

Parameters:
    <NONE>

Returns:
    VOID

===================================================================*/
VOID CTemplateCacheManager::CTemplateHashTable::ScavengePersistCache()
{
    CTemplate   *pTemplate;
    CTemplate   *pTemplateNext;

    // enter a for loop to look at all persisted templates to see if
    // any memory can be freed.  It's memory can be freed only if the
    // ref count is 1 (the sole ref count is for the cache).  Also note
    // that the list is re-ordered to move templates to the head of the
    // list that can't have their memory freed at this time because of
    // the ref count.

    for (pTemplate = static_cast<CTemplate *>(PersistTemplatesBegin());
         (pTemplate != static_cast<CTemplate *>(&m_listPersistTemplates)) && (pTemplate->m_pbStart != NULL);
         pTemplate = pTemplateNext) {

        pTemplateNext = static_cast<CTemplate *>(pTemplate->PNext());

        // this check should be safe.  The only risk is that we miss a release
        // of the template from 2 to 1, in which case will miss it this time
        // but get it the next time through.  AddRef from 1 to 2 is impossible
        // to interrupt because it couldn't be on this list when it gets AddRef'd
        // from 1 to 2 and moving it from this list is protected by the template
        // cache lock which we should be under.

        if (pTemplate->m_cRefs == 1) {

            BOOL    fDeleteRecord = FALSE;

            if (pTemplate->m_cUseCount == 1) {

                // not going to continue to cache or persist a template that
                // was used just once.

                fDeleteRecord = TRUE;

                g_nScavengedPurged++;
            }
            else if (pTemplate->PersistData(m_szPersistCacheDir) != S_OK) {

                // a failure will result in the record being deleted.

                fDeleteRecord = TRUE;

                g_nScavengedPersistFailed++;

            }
            else {

                g_nScavengedPersisted++;

                // remove the memory

                CTemplate::LargeFree(pTemplate->m_pbStart);
                pTemplate->m_pbStart = NULL;
            }

            if (fDeleteRecord) {

                if (RemoveTemplate(pTemplate, FALSE, FALSE) == LK_SUCCESS)
                    ZapTemplate(pTemplate);
            }
        }
        else {

            // if some is still using it, move the template to the head of the
            // list so that we'll check again later.

            pTemplate->PrependTo(m_listPersistTemplates);
        }
    }
}

/*===================================================================
    GetAggregatedTemplCounter()

    Returns the Template Perf Counter.  To do this, initializes a private
    copy of the perfmainblock and aggregates the stats into it.

===================================================================*/
static DWORD GetAggregatedTemplCounter()
{

    CPerfMainBlock  perfSharedBlk;
    DWORD           pdwCounters[C_PERF_PROC_COUNTERS];
    BOOL            bInited = FALSE;

    memset(pdwCounters, 0, sizeof(pdwCounters));

    if (!(bInited = (perfSharedBlk.Init() == S_OK)));

    else {

        perfSharedBlk.GetStats(pdwCounters);
    }

    if (bInited)
        perfSharedBlk.UnInit();

    return(pdwCounters[ID_TEMPLCACHE]);
}

/*===================================================================
 CTemplateCacheManager::OnInitCleanup

Parameters:
    [none]

Returns:
    BOOL to indicate if the init was successful

===================================================================*/

DWORD    CTemplateCacheManager::OnInitCleanup(VOID *p)
{
    // we are using a single character buffer to construct all directory and file names we use.
    CHAR             szDirBuffer[ MAX_PATH + 32];

    // first build the template for the cache directories
    INT iLen = _snprintf(szDirBuffer,
                         MAX_PATH,
                         "%s\\PID*.TMP",
                         Glob(pszPersistTemplateDir),
                         GetCurrentProcessId());

    if ((iLen <= 0) || (iLen >= MAX_PATH)) {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    // outer loop: find all directories, retrieve PID, if process not exist, go to inner
    // inner loop: delete all ASP cache files

    WIN32_FIND_DATAA RootDir_FindData;
    HANDLE hRootDir = FindFirstFileA( szDirBuffer, &RootDir_FindData );

    WIN32_FIND_DATAA FileDir_FindData;
    HANDLE hFileDir;

    if ( hRootDir != INVALID_HANDLE_VALUE ) {

        // point to where the cache subdirectory begins
        CHAR *pDir = szDirBuffer + strlen(Glob(pszPersistTemplateDir)) + 1;
        CHAR *pFile;
        CHAR *pc;
        DWORD pid;
        HANDLE hProcess;

        do {
            // validate file name is what we expect and extract the PID
            // we know the first letters are 'PID' as that was the search criteria

            pc = RootDir_FindData.cFileName + 3;

            for(pid = 0; isdigit(*pc); pid = pid*10 + *pc++ - '0');

            if (pid == 0) {
                continue;
            }

            hProcess = OpenProcess(SYNCHRONIZE, FALSE, pid);

            if (hProcess) {

                // the process is alive, so leave the directory alone.

                CloseHandle( hProcess );
                continue;
            }

            if (GetLastError() != ERROR_INVALID_PARAMETER) {

                // we got an error other than "no such process"

                Assert( GetLastError() == ERROR_ACCESS_DENIED);

                continue;
            }

            // The process does not exist, so clean up the directory
            // Add the directory name, append a '\', append the file template, and point pFile
            // to the position where the file name starts

            strcpy( pDir, RootDir_FindData.cFileName );
            pFile = pDir + strlen(pDir);
            *pFile++ = '\\';
            strcpy( pFile, "ASP*.TMP" );

            // search the subdirectory for asp template cache files

            hFileDir = FindFirstFileA( szDirBuffer, &FileDir_FindData );

            if ( hFileDir != INVALID_HANDLE_VALUE ) {
                do {
                    // append the file name and delete the file
                    strcpy( pFile, FileDir_FindData.cFileName);

                    DeleteFileA( szDirBuffer );

                } while (FindNextFileA( hFileDir, &FileDir_FindData ));

                FindClose( hFileDir );
            }

            // Now zap the last part of the path and delete the directory

            *--pFile = '\0';
            RemoveDirectoryA( szDirBuffer );

        } while (FindNextFileA( hRootDir, &RootDir_FindData ));

        FindClose( hRootDir );
    }

    return S_OK;
}

/*===================================================================
 CTemplateCacheManager::InitPersistCache

Parameters:
    [none]

Returns:
    BOOL to indicate if the init was successful

===================================================================*/

BOOL    CTemplateCacheManager::InitPersistCache(CIsapiReqInfo *pIReq)
{
    HANDLE      hImpersonationToken = NULL;
    BOOL        fRevertedToSelf = FALSE;
    DWORD       dirAttribs;
    INT         iLen;
    UINT        uiEventSubId = 0;
    DWORD       dwError = ERROR_SUCCESS;

    if (OpenThreadToken( GetCurrentThread(),
                         TOKEN_READ | TOKEN_IMPERSONATE,
                         TRUE,
                         &hImpersonationToken )) {

       RevertToSelf();
       fRevertedToSelf = TRUE;
    }

    // build the cache directory name

    iLen = _snprintf(m_szPersistCacheDir,
                     sizeof(m_szPersistCacheDir),
                     "%s\\PID%d.TMP",
                     Glob(pszPersistTemplateDir),
                     GetCurrentProcessId());

    if ((iLen <= 0) || (iLen >= sizeof(m_szPersistCacheDir))) {

        uiEventSubId = IDS_CACHE_DIR_NAME_TOO_LONG;

        goto LExit;
    }

    // If directory exists, rename it to something else (must be an old leftover

    dirAttribs = GetFileAttributesA(m_szPersistCacheDir);

    if (dirAttribs != INVALID_FILE_ATTRIBUTES) {

        // recreate the same dir name with a leading 0 for the pid...

        CHAR szNewDirName[ sizeof(m_szPersistCacheDir) ];
        iLen = _snprintf(szNewDirName,
                         sizeof(szNewDirName),
                         "%s\\PID0%d.TMP",
                         Glob(pszPersistTemplateDir),
                         GetCurrentProcessId());

        if ((iLen <= 0) || (iLen >= sizeof(szNewDirName))) {

            uiEventSubId = IDS_CACHE_DIR_NAME_TOO_LONG;

            goto LExit;
        }

        if (!MoveFileA(m_szPersistCacheDir, szNewDirName)) {

            uiEventSubId = IDS_CACHE_SUBDIR_CREATION_FAILED;
            dwError = GetLastError();

            goto LExit;
        }
    }

    // Now create it

    if (CreateDirectoryA(m_szPersistCacheDir, NULL)) {

        dirAttribs = GetFileAttributesA(m_szPersistCacheDir);
    } else {

        uiEventSubId = IDS_CACHE_SUBDIR_CREATION_FAILED;
        dwError = GetLastError();

        goto LExit;
    }

    if ((dirAttribs == INVALID_FILE_ATTRIBUTES)
        || !(dirAttribs & FILE_ATTRIBUTE_DIRECTORY)) {

        uiEventSubId = IDS_CACHE_SUBDIR_MISSING;

        goto LExit;
    }

    // start the scavenger thread

    m_hOnInitCleanupThread = CreateThread(NULL,
                                      0,
                                      CTemplateCacheManager::OnInitCleanup,
                                      NULL,
                                      0,
                                      NULL);


LExit:

    if (uiEventSubId != 0) {
        //
        // an initialization error has occured. log it and indicate initialization fail.
        //

        MSG_Error(IDS_CACHE_INIT_FAILED, pIReq->QueryPszAppPoolIdA(), uiEventSubId, dwError);

        m_fFailedToInitPersistCache = TRUE;
    }

    if (fRevertedToSelf) {
        SetThreadToken(NULL, hImpersonationToken);
        CloseHandle(hImpersonationToken);
    }

    return(!m_fFailedToInitPersistCache);
}

/*===================================================================
 CTemplateCacheManager::CTemplateHashTable::CanPersistTemplate

Parameters:
    pTemplate - The template to test for persistability

Returns:
    BOOL to indicate if template can be persisted.

===================================================================*/

BOOL CTemplateCacheManager::CTemplateHashTable::CanPersistTemplate(CTemplate  *pTemplate)
{

    // if MaxFiles is zero, then the persist cache is disabled

    if (Glob(dwPersistTemplateMaxFiles) == 0)
    {
        return(FALSE);
    }

    // can't persist if the persist cache failed to init

    if (m_fFailedToInitPersistCache == TRUE)
    {
        return(FALSE);
    }

    // can't persist templates that are marked as debuggable.  The
    // script engines need access to the memory.

    if (pTemplate->FDebuggable())
    {
        return(FALSE);
    }

    //
    // Cannot write an Encrypted file to disk as we would be writing it in plaintext.
    //
    if (pTemplate->FIsEncrypted())
    {
        return(FALSE);
    }

    // at this point, we're going to return true.  The next part of the code
    // trims the cache as necessary.

    if (m_dwPersistedTemplates >= Glob(dwPersistTemplateMaxFiles))
    {
        //
        // Ignore the value returned from TrimPersistCache as we are not concerned if we met the
        // trim count or not. In anycase, there should be 1 available location to add the new template.
        // Optimization may be to free up couple of places (configurable through the metabase) to achieve performance.
        //
        TrimPersistCache(m_dwPersistedTemplates - Glob(dwPersistTemplateMaxFiles) + 1);
    }

    return(TRUE);
}

/*===================================================================
 CTemplateCacheManager::CTemplateHashTable::InsertTemplate

Parameters:
    pTemplate - Template to insert into the memory cache

Returns:
    LK_RETCODE indicating the success of the insertion

===================================================================*/

LK_RETCODE CTemplateCacheManager::CTemplateHashTable::InsertTemplate(CTemplate *pTemplate)
{
	LK_RETCODE rcode = InsertRecord(pTemplate, true);

	if (rcode == LK_SUCCESS) {
#ifndef PERF_DISABLE
        g_PerfData.Incr_MEMORYTEMPLCACHE();
        g_PerfData.Incr_TEMPLCACHE();
#endif
        m_dwInMemoryTemplates++;
		pTemplate->PrependTo(m_listMemoryTemplates);
        pTemplate->SetHashTablePtr(this);
    }

    ScavengePersistCache();

	return rcode;
}

/*===================================================================
 CTemplateCacheManager::CTemplateHashTable::RemoveTemplate

Parameters:
    pTemplate - Template to remove from cache
    fPersist - indicate if memory template is a candidate for persist

Returns:
    LK_RETCODE indicating the success of the removal

===================================================================*/

LK_RETCODE CTemplateCacheManager::CTemplateHashTable::RemoveTemplate(CTemplate *pTemplate,
                                                                     BOOL fPersist,
                                                                     BOOL fScavengePersistCache)
{
	LK_RETCODE rcode = LK_SUCCESS;
#if DBG_PERSTEMPL
    DBGPRINTF((DBG_CONTEXT,
              "RemoveTemplate entered.\n\tTemplate = %s.\n\tfPersist = %d.\n\tFIsPersisted = %d\n",
              pTemplate->GetSourceFileName(),
              fPersist,
              pTemplate->FIsPersisted()));
#endif

    // if the template isn't in the cache, or if the template isn't on this
    // particular hash table, then just bail.  Nothing to
    // do here.  It may not be on this particular hash table because the entire
    // table may have been torn off the global cache manager and scheduled for
    // cleanup on the flush thread.  In this case, we're checking the wrong
    // table.  The flush thread will eventually clean this one up.

    if (pTemplate->FIsEmpty() || (pTemplate->GetHashTablePtr() != this)) {

        return LK_NO_SUCH_KEY;
    }

    // no matter what, this template is going to be unlinked from it's
    // current CDblLink

    pTemplate->UnLink();

    // update the appropriate counter

    if (pTemplate->FIsPersisted() == FALSE) {

        // decrement the number of InMemoryTemplates...

#ifndef PERF_DISABLE
        g_PerfData.Decr_MEMORYTEMPLCACHE();
#endif
        m_dwInMemoryTemplates--;
    }
    else {
        m_dwPersistedTemplates--;
    }

    // if asked to be persisted, see if it's a candidate to be persisted.

    if (fPersist && CanPersistTemplate(pTemplate)) {

        pTemplate->m_fIsPersisted = TRUE;

        // if successfully persisted, then add to the list of
        // persisted templates

        pTemplate->PrependTo(m_listPersistTemplates);

        m_dwPersistedTemplates++;

    }
    else {

#ifndef PERF_DISABLE
            g_PerfData.Decr_TEMPLCACHE();
#endif
        // if not asked to persist, then delete the record.

        rcode = DeleteRecord(pTemplate);
    }

    if (fScavengePersistCache)

        ScavengePersistCache();

	return rcode;
}

/*===================================================================
 CTemplateCacheManager::CTemplateHashTable::FindTemplate

Parameters:
    rTemplate - the key for the template being looked up

Returns:
    LK_RETCODE indicating the success of the look up

===================================================================*/

LK_RETCODE  CTemplateCacheManager::CTemplateHashTable::FindTemplate(const CTemplateKey &rTemplateKey, CTemplate **ppTemplate, BOOL *pfNeedsCheck)
 {

#if DBG_PERSTEMPL

    DBGPRINTF((DBG_CONTEXT,
              "FindTemplate entered\n\tLooking for %s\n",
              rTemplateKey.szPathTranslated));
#endif

#ifndef PERF_DISABLE
    g_PerfData.Incr_MEMORYTEMPLCACHETRYS();
    g_PerfData.Incr_TEMPLCACHETRYS();
#endif

    LK_RETCODE rcode = FindKey(&rTemplateKey, ppTemplate);

    // see if we found it.

	if (rcode == LK_SUCCESS) {

#if DBG_PERSTEMPL

        DBGPRINTF((DBG_CONTEXT,
                   "Template found\n\tfPersisted = %d\n",
                   (*ppTemplate)->FIsPersisted()));
#endif

#ifndef PERF_DISABLE
            g_PerfData.Incr_TEMPLCACHEHITS();
#endif
        // found it.  Is it persisted?

        if ((*ppTemplate)->FIsPersisted()) {

            // It is persisted.  Unlink it from the persisted list.

            (*ppTemplate)->UnLink();

            m_dwPersistedTemplates--;

            // unpersist it

            if ((*ppTemplate)->UnPersistData() != S_OK) {

                // error occurred

                // get the template out of the cache

                DeleteRecord(*ppTemplate);

                // release the reference that the cache had on the template

                (*ppTemplate)->Release();

                // NULL out *ppTemplate so that the caller doesn't think they
                // got a valid template

                *ppTemplate = NULL;

#ifndef PERF_DISABLE
                g_PerfData.Decr_TEMPLCACHE();
#endif
                // return NO_SUCH_KEY so that a new template will be built

                return(LK_NO_SUCH_KEY);
            }

            // bump the number of in memory templates

#ifndef PERF_DISABLE
            g_PerfData.Incr_MEMORYTEMPLCACHE();
#endif
            m_dwInMemoryTemplates++;
        }
        else {
#ifndef PERF_DISABLE
            g_PerfData.Incr_MEMORYTEMPLCACHEHITS();
#endif
        }

        // add it to, or move it to the top of, the memory templates

		(*ppTemplate)->PrependTo(m_listMemoryTemplates);
    }


    if (pfNeedsCheck && *ppTemplate && (*ppTemplate)->FNeedsValidation() && (*ppTemplate)->FIsValid())
        *pfNeedsCheck = TRUE;

    ScavengePersistCache();

	return rcode;
}

