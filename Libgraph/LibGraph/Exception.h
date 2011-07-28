#ifndef __EXCEPTION_H__
#define __EXCEPTION_H__


//////////////////////////////////////////////////////////////////////////
//  Wrapper exception class, constructor uses printf style arguments
//////////////////////////////////////////////////////////////////////////
class Exception
{
public:
    //static bool s_bShowGraphDlg;
    std::string m_what;
    long m_hr;
    Exception(long hResult, const char* fmt, ...)
    {
        try // what if the ... has nasty stuff in it?
        {
            // MessageBox to be removed in final version, maybe graph can be dumped to logfile
//            if(s_bShowGraphDlg)
//                MessageBox(0, "If you need to view the graph from graphedit do so now and click OK when done!","Transcoder error", MB_OK );
//            
            // Print the arguments to a buffer
            m_hr = hResult;
            char pBuffer[1024] = {0};
            va_list args;
            va_start(args, fmt);
            _vsnprintf(pBuffer, 1023, fmt, args);
            va_end(args);
            m_what = pBuffer;
            
            // Convert windows error code to a Message
            if(hResult) 
            {
                LPSTR lpMsgBuf;
                sprintf(pBuffer, "\nError code : %X\n", hResult);
                m_what += pBuffer;
                FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, hResult, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&lpMsgBuf, 0, NULL);
                if(lpMsgBuf)
                {
                    m_what += lpMsgBuf;
                    LocalFree(lpMsgBuf);
                }
            }
        }
        catch(...)
        {   // If any of the above fail...
            m_what = "Failed in Exception::Exception()";
        }

        if(m_hr == 0) m_hr = E_UNEXPECTED; // Unknown error means unexpected
    }
};


#endif //__EXCEPTION_H__

