#ifndef __SSLCONFIGPIPECLIENT__HXX_
#define __SSLCONFIGPIPECLIENT__HXX_
/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :
     sslconfigprovclient.hxx

   Abstract:
     SSL CONFIG PROV client

     Client provides easy way of retrieving SSL related parameters
     through named pipes.
     Only one pipe connection is currently supported and 
     all client threads have to share it 
     ( exclusive access is maintained by locking )

     
     Client side is guaranteed not to use any COM stuff.
     Not using COM was requirement from NT Security folks
     to enable HTTPFilter be hosted in lsass
 
   Author:
     Jaroslav Dunajsky      April-24-2001

   Environment:
     Win32 - User Mode

   Project:
     Stream Filter Worker Process
--*/


#include <sslconfigcommon.hxx>
#include <sslconfigpipe.hxx>

class SSL_CONFIG_PROV_CLIENT: protected SSL_CONFIG_PIPE
{
public:
    
    SSL_CONFIG_PROV_CLIENT()
        :_pSslConfigChangeCallback( NULL ),
        _pSslConfigChangeCallbackParameter( NULL )
    {}
    
    ~SSL_CONFIG_PROV_CLIENT()
    {}
    
    HRESULT
    Initialize(
        IN SSL_CONFIG_CHANGE_CALLBACK * pSslConfigChangeCallback,
        IN OPTIONAL PVOID  pvParam = NULL
        );
    
    HRESULT
    Terminate(
        VOID
        );
    
    HRESULT
    MaintainPipeConnection(
        VOID
        ); 

    VOID
    CancelPendingCalls(
        VOID
        )
    {
        _SslConfigChangeProvClient.StopListeningForChanges();
        SSL_CONFIG_PIPE::PipeCancel();
    }
    
    HRESULT 
    GetOneSiteSecureBindings(
        IN  DWORD     dwSite,
        OUT MULTISZ*  pSecureBindings
        );

    HRESULT
    StartAllSitesSecureBindingsEnumeration(
        VOID
        );

    HRESULT
    StopAllSitesSecureBindingsEnumeration(
        VOID
        );

    HRESULT
    GetNextSiteSecureBindings( 
        OUT DWORD *   pdwId,
        OUT MULTISZ * pSecureBindings
        );

    HRESULT
    GetOneSiteSslConfiguration(
        IN  DWORD dwSiteId,
        OUT SITE_SSL_CONFIGURATION * pSiteSslConfiguration
        );

private:
    SSL_CONFIG_PROV_CLIENT( const SSL_CONFIG_PROV_CLIENT& );
    SSL_CONFIG_PROV_CLIENT& operator=( const SSL_CONFIG_PROV_CLIENT& );

    HRESULT 
    ReceiveOneSiteSecureBindings(
        OUT DWORD *   pdwSiteId,
        OUT MULTISZ * pSecureBinding
    );
   

    SSL_CONFIG_CHANGE_PROV_CLIENT _SslConfigChangeProvClient;

    //
    // parameters used for config change provider
    //
    SSL_CONFIG_CHANGE_CALLBACK * _pSslConfigChangeCallback;
    PVOID                        _pSslConfigChangeCallbackParameter;


};

#endif
