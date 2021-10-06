/************************************************************************************************************
* @internal
* @remark     Winbond Electronics Corporation - Confidential
* @copyright  Copyright (c) 2019 by Winbond Electronics Corporation . All rights reserved
* @endinternal
*
* @file       qlib_debug.h
* @brief      This file contains debugging utility macros
*
* ### project qlib
*
************************************************************************************************************/
#ifndef __QLIB_DEBUG_H__
#define __QLIB_DEBUG_H__
// clang-format off

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                             ERROR CHECKING                                              */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#ifndef __QLIB_H__
#error "This internal header file should not be included directly. Please include `qlib.h' instead"
#endif

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                            DEPENDENCY CHECK                                             */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#ifndef __QLIB_PLATFORM_INCLUDED__
#error "This file should not be included directly. Use qlib.h"
#endif

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                 INCLUDES                                                */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                             VERBOSITY LEVELS                                            */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#define QLIB_VERBOSE_NONE    0
#define QLIB_VERBOSE_FATAL   1
#define QLIB_VERBOSE_ERROR   2
#define QLIB_VERBOSE_WARNING 3
#define QLIB_VERBOSE_INFO    4
#define QLIB_VERBOSE_TRACE   5
#define QLIB_VERBOSE_ALL     6

#define QLIB_VERBOSE_DYNAMIC 0xFF

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                         ALLOW TO FAIL HANDLING                                          */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#ifdef QLIB_VERBOSE
    #ifdef __QLIB_C__
        int QLIB_globalAllowToFail = 0;
    #else
        extern int QLIB_globalAllowToFail;
    #endif

    #define QLIB_ALLOW_TO_FAIL__START()     {QLIB_globalAllowToFail = 1;}
    #define QLIB_ALLOW_TO_FAIL__END()       {QLIB_globalAllowToFail = 0;}
    #define QLIB_ALLOW_TO_FAIL__VAL()       QLIB_globalAllowToFail
#else
    #define QLIB_ALLOW_TO_FAIL__START()
    #define QLIB_ALLOW_TO_FAIL__END()
    #define QLIB_ALLOW_TO_FAIL__VAL()       (0)
#endif

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                       DEFAULT VERBOSITY SELECTION                                       */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#ifndef QLIB_VERBOSE
#define QLIB_VERBOSE QLIB_VERBOSE_NONE
#endif

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                       DYNAMIC VERBOSITY HANDLING                                        */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#if (QLIB_VERBOSE == QLIB_VERBOSE_DYNAMIC)
    #ifdef __QLIB_C__
        int QLIB_globalVerbose = 0;
    #else
        extern int QLIB_globalVerbose;
    #endif

    #define QLIB_SET_VERBOSE(v)             QLIB_globalVerbose = v

    #undef QLIB_VERBOSE
    #define QLIB_VERBOSE                    QLIB_globalVerbose
#else
    #define QLIB_SET_VERBOSE(v)
#endif

#define QLIB_DECLARE_GLOBAL_VERBOSE()       // For backward compatibility


/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                        PRINTING / LOGGING UTILS                                         */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#ifdef QLIB_ASSERT_ON_ERROR
    #define QLIB_DEBUG_CHECK_FOR_ERROR(verbose) \
    {                                           \
        if (QLIB_VERBOSE_ERROR == verbose)      \
        {                                       \
            ASSERT(0);                          \
        }                                       \
    }
#else // QLIB_ASSERT_ON_ERROR
    #define QLIB_DEBUG_CHECK_FOR_ERROR(verbose)
#endif // QLIB_ASSERT_ON_ERROR

    #ifdef QLIB_PRINT_CMD
        #undef QLIB_PRINT_CMD
    #endif // QLIB_PRINT_CMD
    #define QLIB_PRINT_CMD(...)

#ifdef QLIB_PRINT_CODE_INFO
    #define QLIB_PRINT(...)                                    \
    {                                                          \
        QLIB_PRINT_CMD("< %s(%d) > ", __FUNCTION__, __LINE__); \
        QLIB_PRINT_CMD(__VA_ARGS__);                           \
        QLIB_PRINT_CMD("\r\n");                                \
    }
#else
    #define QLIB_PRINT(...)          \
    {                                \
        QLIB_PRINT_CMD(__VA_ARGS__); \
        QLIB_PRINT_CMD("\r\n");      \
    }
#endif

#define QLIB_DEBUG_PRINT(verbose, ...)       \
{                                            \
    if (!QLIB_ALLOW_TO_FAIL__VAL())          \
    {                                        \
        if (QLIB_VERBOSE >= verbose)         \
        {                                    \
            QLIB_PRINT(__VA_ARGS__);         \
        }                                    \
        QLIB_DEBUG_CHECK_FOR_ERROR(verbose); \
    }                                        \
}

// clang-format on
#endif // __QLIB_DEBUG_H__
