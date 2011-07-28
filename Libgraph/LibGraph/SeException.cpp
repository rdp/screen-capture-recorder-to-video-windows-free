#include "LibGraphStdAfx.h"

#define CASE_THROW( x ) case EXCEPTION_##x: throw Exception(0, #x );

//////////////////////////////////////////////////////////////////////////
// This function handles OS exceptions and re-throws a C++ exception 
// allowing us to use the C++ try catch to handle them.
//////////////////////////////////////////////////////////////////////////
void SeTranslator( UINT nSeCode, _EXCEPTION_POINTERS* pExcPointers )
{
    switch( nSeCode )    
    {   
        CASE_THROW( ACCESS_VIOLATION );
        CASE_THROW( DATATYPE_MISALIGNMENT );
        CASE_THROW( BREAKPOINT );
        CASE_THROW( SINGLE_STEP );
        CASE_THROW( ARRAY_BOUNDS_EXCEEDED );
        CASE_THROW( FLT_DENORMAL_OPERAND );
        CASE_THROW( FLT_DIVIDE_BY_ZERO );
        CASE_THROW( FLT_INEXACT_RESULT );
        CASE_THROW( FLT_INVALID_OPERATION );
        CASE_THROW( FLT_OVERFLOW );
        CASE_THROW( FLT_STACK_CHECK );
        CASE_THROW( FLT_UNDERFLOW );
        CASE_THROW( INT_DIVIDE_BY_ZERO );
        CASE_THROW( INT_OVERFLOW );
        CASE_THROW( PRIV_INSTRUCTION );
        CASE_THROW( IN_PAGE_ERROR );
        CASE_THROW( ILLEGAL_INSTRUCTION );
        CASE_THROW( NONCONTINUABLE_EXCEPTION );
        CASE_THROW( STACK_OVERFLOW );
        CASE_THROW( INVALID_DISPOSITION );
        CASE_THROW( GUARD_PAGE );
        CASE_THROW( INVALID_HANDLE );
    default:
        throw Exception(0,  "Unknown OS Exception" );
    }
}
