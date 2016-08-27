

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.00.0603 */
/* at Sat Aug 27 23:28:36 2016
 */
/* Compiler settings for LDViewThumbs.idl:
    Oicf, W1, Zp8, env=Win32 (32b run), target_arch=X86 8.00.0603 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __LDViewThumbs_h__
#define __LDViewThumbs_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ILDViewThumbExtractor_FWD_DEFINED__
#define __ILDViewThumbExtractor_FWD_DEFINED__
typedef interface ILDViewThumbExtractor ILDViewThumbExtractor;

#endif 	/* __ILDViewThumbExtractor_FWD_DEFINED__ */


#ifndef __LDViewThumbExtractor_FWD_DEFINED__
#define __LDViewThumbExtractor_FWD_DEFINED__

#ifdef __cplusplus
typedef class LDViewThumbExtractor LDViewThumbExtractor;
#else
typedef struct LDViewThumbExtractor LDViewThumbExtractor;
#endif /* __cplusplus */

#endif 	/* __LDViewThumbExtractor_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __ILDViewThumbExtractor_INTERFACE_DEFINED__
#define __ILDViewThumbExtractor_INTERFACE_DEFINED__

/* interface ILDViewThumbExtractor */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ILDViewThumbExtractor;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0F7171EC-663E-4C01-9777-5B6496E3DD91")
    ILDViewThumbExtractor : public IUnknown
    {
    public:
    };
    
    
#else 	/* C style interface */

    typedef struct ILDViewThumbExtractorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ILDViewThumbExtractor * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ILDViewThumbExtractor * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ILDViewThumbExtractor * This);
        
        END_INTERFACE
    } ILDViewThumbExtractorVtbl;

    interface ILDViewThumbExtractor
    {
        CONST_VTBL struct ILDViewThumbExtractorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ILDViewThumbExtractor_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ILDViewThumbExtractor_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ILDViewThumbExtractor_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ILDViewThumbExtractor_INTERFACE_DEFINED__ */



#ifndef __LDVIEWTHUMBSLib_LIBRARY_DEFINED__
#define __LDVIEWTHUMBSLib_LIBRARY_DEFINED__

/* library LDVIEWTHUMBSLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_LDVIEWTHUMBSLib;

EXTERN_C const CLSID CLSID_LDViewThumbExtractor;

#ifdef __cplusplus

class DECLSPEC_UUID("FA993BF4-40AF-49C1-BD0B-035A275757C1")
LDViewThumbExtractor;
#endif
#endif /* __LDVIEWTHUMBSLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


