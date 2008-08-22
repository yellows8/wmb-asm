#ifdef BUILDING_SDK
    
    #ifndef WMB_ASM_SDK_INTERNAL
    #define WMB_ASM_SDK_INTERNAL
        
        #ifdef SDK_MAIN
            volatile struct Nds_data **sdk_nds_data = NULL;
            bool *SDK_DEBUG = NULL;
            FILE **SDK_Log = NULL;
            struct sAsmSDK_Config *SDK_CONFIG = NULL;
        #endif
        
        #ifndef SDK_MAIN
            extern volatile struct Nds_data **sdk_nds_data;
            extern bool *SDK_DEBUG;
            extern FILE **SDK_Log;
            extern struct sAsmSDK_Config *SDK_CONFIG;
        #endif
        
    #endif
    
#endif
