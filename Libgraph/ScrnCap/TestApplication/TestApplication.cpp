//#define LIBGRAPH_USE_DLL 1

#include "stdafx.h"
using namespace std;
#include <initguid.h>
DEFINE_GUID(CLSID_ScreenCapturer, 0x49634b24, 0x4c8d, 0x451c, 0xbd, 0xca, 0xa5, 0xcc, 0x9, 0x38, 0x8a, 0xfb);

int _tmain(int argc, _TCHAR* argv[])
{
    try
    {
        CGraph g;
        int i;
        vector<string> vc, ac, al;
        g.EnumFiltersByCategory(vc, CLSID_VideoCompressorCategory);
        g.EnumFiltersByCategory(ac, CLSID_AudioCompressorCategory);
        g.AddFilterByCategory(0, "AudioSrc", CLSID_AudioInputDeviceCategory);
        g.RefreshPins("AudioSrc");

        int audioLines = g.GetPinCount("AudioSrc", PINDIR_INPUT);
        for(i = 0; i < audioLines; ++i)
        {
            PIN_INFO pi;
            g.GetPin("AudioSrc", i, PINDIR_INPUT)->QueryPinInfo(&pi);
            _bstr_t name(pi.achName);
            al.push_back((char*)name);
        }

        if(argc < 7)
        {
            cout << "Usage : scrncap <outfilename> <x> <y> <w> <h> <fps> [v-codec] [a-codec] [audioline]\n";
            cout << "\n[rectx, recty, rectw, recth] is the rectangle to be captured\n";
            cout << "[v-codec] is the index of the video codec in the list to use.\n";
            cout << "[a-codec] is the index of the audio codec in the list to use.\n";
            cout << "[audioline] is the index of the audio line in the list to capture from\n";
            cout << "If either codec is unspecified, it defaults to 'Microsoft Video 1' and 'GSM 6.10'\n";
            cout << "If audioline is unspecified, it uses the microphone\n";
            cout << "To capture the currently playing output, select the stereo, mono or wave mix\n";
            cout << "Installed Video Codecs (Note : Not all of them may work)\n";
            int i = 0;
            vector<string>::iterator ii;
            for(ii = vc.begin(); ii != vc.end(); ++ii)
            {
                cout << "    " << i++ << "." << *ii << endl;
            }

            cout << "\nInstalled Audio Codecs (Note : Not all of them may work)\n";
            i = 0;

            for(ii = ac.begin(); ii != ac.end(); ++ii)
            {
                cout << "    " << i++ << "." << *ii << endl;
            }

            i = 0;
            cout << "\nAudio input lines\n";
            for(ii = al.begin(); ii != al.end(); ++ii)
            {
                cout << "    " << i++ << "." << *ii << endl;
            }

            while(!kbhit()) Sleep(100);
            return 1;
        }

        int x, y, w, h, fps;
        unsigned int vCodec, aCodec, vDefCodec = 0, aDefCodec = 0;
        int aLine, defALine = 0;

        vector<string>::iterator dvci = std::find(vc.begin(), vc.end(),"Microsoft Video 1");
        if(dvci != vc.end()) vDefCodec = dvci - vc.begin();

        vector<string>::iterator daci = std::find(ac.begin(), ac.end(),"GSM 6.10");
        if(daci != ac.end()) aDefCodec = daci - ac.begin();

        vector<string>::iterator dali = std::find(al.begin(), al.end(),"Microphone");
        if(dali != al.end()) defALine = dali - al.begin();

        x = atoi(argv[2]);
        y = atoi(argv[3]);
        w = atoi(argv[4]);
        h = atoi(argv[5]);
        fps = atoi(argv[6]);

        if(argc > 7) vCodec = atoi(argv[7]); else vCodec = vDefCodec;
        if(argc > 8) aCodec = atoi(argv[8]); else aCodec = aDefCodec;
        if(argc > 9) aLine = atoi(argv[9]); else aLine = defALine;

        if(vCodec > vc.size() || vCodec < 0) vCodec = vDefCodec;
        if(aCodec > ac.size() || aCodec < 0) aCodec = aDefCodec;

        CComQIPtr<IAMBufferNegotiation> pBufferNeg(g.GetPin("AudioSrc", 0, PINDIR_OUTPUT));
        ALLOCATOR_PROPERTIES AllocProp;
        AllocProp.cbAlign = -1;  // -1 means no preference.
        AllocProp.cbBuffer = 11000;
        AllocProp.cbPrefix = -1;
        AllocProp.cBuffers = -1;
        pBufferNeg->SuggestAllocatorProperties(&AllocProp);

        for(i = 0; i < audioLines; ++i)
        {
            CComQIPtr<IAMAudioInputMixer> pMixer(g.GetPin("AudioSrc", i, PINDIR_INPUT));
            pMixer->put_Enable(i == aLine);
        }

        g.AddFilter(CLSID_ScreenCapturer, "ScrnCap");
        g.AddFilter(CLSID_AviDest, "AVIMux");
        g.AddSinkFilter(CLSID_FileWriter, argv[1], "Writer");

        CComQIPtr<ICaptureConfig> pConfig(g.GetFilter("ScrnCap"));
        pConfig->SetCaptureRect(x, y, w, h, fps);

        cout << "Using audio compressor - " << ac[aCodec] << endl;
        g.AddFilterByCategory(aCodec, "ACompressor", CLSID_AudioCompressorCategory);
        g.Connect("AudioSrc", "ACompressor");

        cout << "Using video compressor - " << vc[vCodec] << endl;
        g.AddFilterByCategory(vCodec, "VCompressor", CLSID_VideoCompressorCategory);
        g.Connect("ScrnCap", "VCompressor");
        g.Connect("ACompressor", "AVIMux");
        g.RefreshPins("AVIMux");
        g.Connect("VCompressor", "AVIMux", 0, 0, 1);

        IBaseFilter *pAVIMux = g.GetFilter("AVIMux");

        CComQIPtr<IConfigInterleaving> pInterleaveConfig(pAVIMux);
        REFERENCE_TIME rtInterleave = UNITS / 2, rtPreroll = 0;
        pInterleaveConfig->put_Mode(INTERLEAVE_CAPTURE);
        pInterleaveConfig->put_Interleaving(&rtInterleave, &rtPreroll);

        CComQIPtr<IConfigAviMux> pMuxConfig(pAVIMux);
        pMuxConfig->SetMasterStream(0);

        g.Connect("AVIMux", "Writer");

        CComPtr<IReferenceClock> pClock;
        pClock.CoCreateInstance(CLSID_SystemClock);

        CComQIPtr<IMediaFilter> pMF(g.GetFilterGraph());
        pMF->SetSyncSource(pClock);
        g.GetMediaControl()->Run();
        i = 0;
        char *spin = "|/-\\";

        printf("Starting capture - Hit a key to stop ...\n");
        while(!kbhit())
        {
            printf("\rEncoding %c",spin[i++]);
            i %= 4;
            Sleep(50);
        }

        printf("\nDone");
        getchar();

    }
    catch(Exception &e)
    {
        cout << "Error : " << endl << e.m_what;
    }

    return 0;
}

