#ifdef BUILDING_SDK
    
    #ifndef WMB_ASM_SDK_INTERNAL
    #define WMB_ASM_SDK_INTERNAL
        
        #ifdef SDK_MAIN
            Nds_data *sdk_nds_data = NULL;
            bool *SDK_DEBUG = NULL;
            FILE **SDK_Log = NULL;
            sAsmSDK_Config *SDK_CONFIG = NULL;
        #endif
        
        #ifndef SDK_MAIN
            extern Nds_data *sdk_nds_data;
            extern bool *SDK_DEBUG;
            extern FILE **SDK_Log;
            extern sAsmSDK_Config *SDK_CONFIG;
        #endif
        
    #endif
    
#endif
