//
// WavLib.cpp - Sound Forge .WAV file reading and writing library.
//

//
// The Sound Forge .WAV Format (.WAV+)
//      The Sound Forge tool creates regular RIFF .WAV files, but adds some extra chunks to specify cue points
// and regions.
//      The first chunk is direct part of the WAVE form. The id is "cue ", and its data consists of a DWORD count of
// cues, then count instances of a cue structure.
//  struct Cue
//  {
//      DWORD index;   // which cue is this, starting with 1
//      DWORD offset;  // offset of cue point, in samples!
//      char  key[4] = 'data'; // what the hell is this for?
//      DWORD pad[2] = {0,0};  // sound forge specific data goes here?
//      DWORD offset2; // another copy of the offset in samples
//  };
//
//  struct CueChunkData
//  {
//      DWORD count;   // number of subsequent Cue instances
//      Cue   data[count];
//  };
//
//      Sound Forge also adds a LIST form which contains 'ltxt' and 'labl' chunks, one for each region.
// The 'ltxt' chunk contains the index of the cue which starts the region and the length of the region. The 'labl'
// chunk contains the name of the region.  We do not read the 'labl' chunk, but we do write it.
//  struct LtxtChunkData
//  {
//      DWORD cueIndex;  // the index of the cue which starts this region, starting at 1
//      DWORD length;    // the length of the region, in samples
//      char  key[4] = 'rgn '; // what the hell is this for?
//      DWORD pad[2] = {0,0};  // sound forge specific data goes here?
//  };
//  struct LablChunkData
//  {
//      DWORD regionIndex; // the index of this region, starting at 1
//      char  [];          // a null terminated string makes up the rest of the chunk
//  };
//

//
// Include files
//

#include "wavlib.h"

#include <TSmartPointer.h>
#include <FDump.h>
#include <TempStr.h>

#include <Windows.h>
#include <mmreg.h>
#include <mmsystem.h>
#include <msacm.h>


//
// Macros
//

#define CHUNK_NAME(a,b,c,d) \
	  (unsigned long) (a)       + \
	(((unsigned long) (b))<<8)  + \
	(((unsigned long) (c))<<16) + \
	(((unsigned long) (d))<<24)

#define DEFAULT_REGION_LABEL "LoopRegion"

//
// Constants
//

const unsigned long RIFF_ID = CHUNK_NAME('R','I','F','F');
const unsigned long LIST_ID = CHUNK_NAME('L','I','S','T');
const unsigned long WAVE_ID = CHUNK_NAME('W','A','V','E');
const unsigned long fmt_ID  = CHUNK_NAME('f','m','t',' ');
const unsigned long ltxt_ID = CHUNK_NAME('l','t','x','t');
const unsigned long labl_ID = CHUNK_NAME('l','a','b','l');
const unsigned long data_ID = CHUNK_NAME('d','a','t','a');
const unsigned long cue_ID  = CHUNK_NAME('c','u','e',' ');
const unsigned long adtl_ID = CHUNK_NAME('a','d','t','l');
const unsigned long rgn_ID = CHUNK_NAME('r', 'g', 'n', ' ');
const unsigned long fact_ID = CHUNK_NAME('f', 'a', 'c', 't');
const unsigned long trim_ID = CHUNK_NAME('t','r','i','m');

//
// Simple Type Definitions
//

//
// Inline routines
//

inline unsigned long str2id (char *str)
{
	return
	  (unsigned long) str[0]       + 
	(((unsigned long) str[1])<<8)  +
	(((unsigned long) str[2])<<16) +
	(((unsigned long) str[3])<<24);
}

//
// Class and structure definitions
//

// These represent the chunks as they exist in the file
#pragma pack(1)
struct RiffChunk
{
	union
	{
		char          idArray[4];
		unsigned long id;
	};
	unsigned long size;
};

struct RiffForm : public RiffChunk
{
	union
	{
		char          formTypeArray[4];
		unsigned long formType;
	};
};

struct FactChunk
{
    DWORD sampleLength; // number of PCM samples AFTER decode
};

struct TrimChunk
{
    DWORD trim;
};

struct CueData
{
	DWORD index;
	DWORD offset;
	union
	{
		char          key[4];  // set to 'data'
		unsigned long keyLong;
	};
	DWORD pad[2];  // set both to 0
	DWORD offset2; // set same as offset
};

struct LtxtData
{
	DWORD cueIndex;
	DWORD length;
	union
	{
		char          key[4];  // set to 'rgn '
		unsigned long keyLong;
	};
	DWORD pad[2];  // set both to 0
};
#pragma pack()

//
// Global functions
//
// FOURCC definitions for convenience
//
// RIFF and chunk ID constants (use appropriate FOURCC values for your environment)
//
//const DWORD RIFF_ID = mmioFOURCC('R', 'I', 'F', 'F');
//const DWORD WAVE_ID = mmioFOURCC('W', 'A', 'V', 'E');
//const DWORD fmt_ID = mmioFOURCC('f', 'm', 't', ' ');
//const DWORD data_ID = mmioFOURCC('d', 'a', 't', 'a');
//const DWORD cue_ID = mmioFOURCC('c', 'u', 'e', ' ');
//const DWORD LIST_ID = mmioFOURCC('L', 'I', 'S', 'T');
//const DWORD adtl_ID = mmioFOURCC('a', 'd', 't', 'l'); // adtl list type
//const DWORD ltxt_ID = mmioFOURCC('l', 't', 'x', 't'); // loop text chunk

//
// Helper function: Scan for a RIFF chunk given its ID starting at position "startPos".
//
bool FindRiffChunk(IFileSystem* fs, DWORD chunkId, RiffChunk& chunk, DWORD startPos)
{
    DWORD bytesRead = 0;
    fs->SetFilePointer(0, startPos, NULL, FILE_BEGIN);
    while (fs->ReadFile(0, &chunk, sizeof(chunk), &bytesRead) && (bytesRead == sizeof(chunk)))
    {
        if (chunk.id == chunkId)
            return true;
        // Skip current chunk’s data.
        fs->SetFilePointer(0, chunk.size, NULL, FILE_CURRENT);
    }
    return false;
}

template<typename t_chunk>
bool FindChunk(IFileSystem* fs, DWORD chunkId, t_chunk& out_chunk, DWORD startPos)
{
    DWORD bytesRead = 0;
    fs->SetFilePointer(0, startPos, NULL, FILE_BEGIN);
    RiffChunk chunk;
    while (fs->ReadFile(0, &chunk, sizeof(chunk), &bytesRead) && (bytesRead == sizeof(chunk)))
    {
        if (chunk.id == chunkId)
        {
            if (fs->ReadFile(0, &out_chunk, sizeof(out_chunk), &bytesRead) && (bytesRead == sizeof(out_chunk)))
            {
                return true;
            }
        }
        // Skip current chunk’s data.
        fs->SetFilePointer(0, chunk.size, NULL, FILE_CURRENT);
    }
    return false;
}

#include <vector>

//
// LoadWAV: Loads a WAV (or WAV+ [with markers]) file from the provided IFileSystem.
// Supports both PCM (WAVE_FORMAT_PCM) and MPEG Layer-3 (WAVE_FORMAT_MPEGLAYER3).
// For MP3, uses ACM (which should employ the Fraunhofer IIS MPEG Layer-3 Codec) to decode into
// 16-bit PCM.
//------------------------------------------------------------------------------
bool LoadWAV(IFileSystem* fs, SoundFile& data)
{
    // Initialize SoundFile structure.
    memset(&data, 0, sizeof(SoundFile));

    DWORD bytesRead = 0;
    RiffForm riffHeader;
    if (!fs->ReadFile(0, &riffHeader, sizeof(riffHeader), &bytesRead) || (bytesRead != sizeof(riffHeader)))
    {
        return false;
    }
    if (riffHeader.id != RIFF_ID || riffHeader.formType != WAVE_ID)
    {
        return false;
    }

    // Save the starting position for later chunk scanning.
    DWORD startPos = fs->GetFilePosition(0);

    TrimChunk trim = {};
    bool has_trim = FindChunk(fs, trim_ID, trim, startPos);

    FactChunk fact = {};
    bool has_fact = FindChunk(fs, fact_ID, fact, startPos);

    if (has_fact || has_trim)
    {
        debug_point;
    }

    // ----- Locate and read the "fmt " chunk -----
    RiffChunk fmtChunk;
    if (!FindRiffChunk(fs, fmt_ID, fmtChunk, startPos))
        return false;

    // Allocate a buffer to read the entire format chunk.
    std::vector<char> fmtBuffer(fmtChunk.size);
    if (!fs->ReadFile(0, fmtBuffer.data(), fmtChunk.size, &bytesRead) || (bytesRead != fmtChunk.size))
        return false;

    // The first part of this chunk is a standard WAVEFORMATEX structure.
    WAVEFORMATEX* pwfx = reinterpret_cast<WAVEFORMATEX*>(fmtBuffer.data());

    // Only support PCM and MPEG Layer-3 formats.
    if (pwfx->wFormatTag != WAVE_FORMAT_PCM && pwfx->wFormatTag != WAVE_FORMAT_MPEGLAYER3)
        return false;

    // Set up the output format.
    if (pwfx->wFormatTag == WAVE_FORMAT_PCM)
    {
        data.format.num_channels = pwfx->nChannels;
        data.format.bytes_per_channel = pwfx->wBitsPerSample / 8;
        data.format.samples_per_sec = pwfx->nSamplesPerSec;
        data.format.bytes_per_sample = pwfx->nBlockAlign;
    }
    else if (pwfx->wFormatTag == WAVE_FORMAT_MPEGLAYER3)
    {
        // When decoding MP3, our output will be PCM, and we choose 16-bit sample depth.
        data.format.num_channels = pwfx->nChannels;
        data.format.bytes_per_channel = 2; // 16 bits per channel
        data.format.samples_per_sec = pwfx->nSamplesPerSec;
        data.format.bytes_per_sample = data.format.num_channels * data.format.bytes_per_channel;
    }

    // ----- Find the "data" chunk that contains the audio samples -----
    fs->SetFilePointer(0, startPos, NULL, FILE_BEGIN);
    RiffChunk dataChunk;
    if (!FindRiffChunk(fs, data_ID, dataChunk, startPos))
        return false;

    if (pwfx->wFormatTag == WAVE_FORMAT_PCM)
    {
        // For PCM, simply allocate the buffer and load the sample data.
        data.samples = new char[dataChunk.size];
        if (!fs->ReadFile(0, data.samples, dataChunk.size, &bytesRead) || (bytesRead != dataChunk.size))
        {
            delete[] reinterpret_cast<char*>(data.samples);
            data.samples = nullptr;
            return false;
        }
        data.length = dataChunk.size;
        data.num_samples = data.length / data.format.bytes_per_sample;
    }
    else if (pwfx->wFormatTag == WAVE_FORMAT_MPEGLAYER3)
    {
        // ----- For MP3: Load compressed data and decode via ACM -----

        // Read the compressed MP3 data into a temporary buffer.
        char* mp3Buffer = new char[dataChunk.size];
        if (!fs->ReadFile(0, mp3Buffer, dataChunk.size, &bytesRead) || (bytesRead != dataChunk.size))
        {
            delete[] mp3Buffer;
            return false;
        }

        // Set up a destination PCM format structure.
        WAVEFORMATEX destFormat = { 0 };
        destFormat.wFormatTag = WAVE_FORMAT_PCM;
        destFormat.nChannels = pwfx->nChannels;
        destFormat.nSamplesPerSec = pwfx->nSamplesPerSec;
        destFormat.wBitsPerSample = 16; // decode to 16-bit PCM
        destFormat.nBlockAlign = destFormat.nChannels * (destFormat.wBitsPerSample / 8);
        destFormat.nAvgBytesPerSec = destFormat.nSamplesPerSec * destFormat.nBlockAlign;
        destFormat.cbSize = 0;

        // Open the ACM stream. This call will use the Fraunhofer IIS MPEG Layer-3 Codec (decode only).
        HACMSTREAM hAcmStream = nullptr;
        MMRESULT result = acmStreamOpen(&hAcmStream, NULL, pwfx, &destFormat, NULL, 0,
            0, ACM_STREAMOPENF_NONREALTIME);
        if (result != MMSYSERR_NOERROR || hAcmStream == nullptr)
        {
            delete[] mp3Buffer;
            return false;
        }

        // Determine the required buffer size for the decoded PCM data.
        DWORD pcmSize = 0;
        result = acmStreamSize(hAcmStream, dataChunk.size, &pcmSize, ACM_STREAMSIZEF_SOURCE);
        if (result != MMSYSERR_NOERROR)
        {
            acmStreamClose(hAcmStream, 0);
            delete[] mp3Buffer;
            return false;
        }

        // Allocate the output PCM buffer.
        char* pcmBuffer = new char[pcmSize];
        ACMSTREAMHEADER streamHeader = { 0 };
        streamHeader.cbStruct = sizeof(ACMSTREAMHEADER);
        streamHeader.pbSrc = reinterpret_cast<LPBYTE>(mp3Buffer);
        streamHeader.cbSrcLength = dataChunk.size;
        streamHeader.pbDst = reinterpret_cast<LPBYTE>(pcmBuffer);
        streamHeader.cbDstLength = pcmSize;

        // Prepare and perform the conversion.
        result = acmStreamPrepareHeader(hAcmStream, &streamHeader, 0);
        if (result != MMSYSERR_NOERROR)
        {
            acmStreamClose(hAcmStream, 0);
            delete[] mp3Buffer;
            delete[] pcmBuffer;
            return false;
        }

        result = acmStreamConvert(hAcmStream, &streamHeader, ACM_STREAMCONVERTF_BLOCKALIGN);
        if (result != MMSYSERR_NOERROR)
        {
            acmStreamUnprepareHeader(hAcmStream, &streamHeader, 0);
            acmStreamClose(hAcmStream, 0);
            delete[] mp3Buffer;
            delete[] pcmBuffer;
            return false;
        }

        // Clean up the ACM objects.
        acmStreamUnprepareHeader(hAcmStream, &streamHeader, 0);
        acmStreamClose(hAcmStream, 0);

        // Store the decoded PCM data in the SoundFile structure.
        data.samples = pcmBuffer;
        data.length = streamHeader.cbDstLengthUsed;
        data.num_samples = data.length / data.format.bytes_per_sample;

        // Free the temporary MP3 buffer.
        delete[] mp3Buffer;
    }

    if (has_fact)
    {
        data.length = data.format.bytes_per_sample * fact.sampleLength;
        data.num_samples = fact.sampleLength;
    }
    
    if (has_trim)
    {
        data.start_offset = data.format.bytes_per_sample * trim.trim;
    }

    // ----- Process optional cue and LIST chunks for loop markers -----
    fs->SetFilePointer(0, startPos, NULL, FILE_BEGIN);

    bool cueFound = false;
    RiffChunk chunk;
    while (fs->ReadFile(0, &chunk, sizeof(chunk), &bytesRead) && (bytesRead == sizeof(chunk)))
    {
        if (chunk.id == cue_ID)
        {
            DWORD cueCount = 0;
            if (fs->ReadFile(0, &cueCount, sizeof(DWORD), &bytesRead) &&
                (bytesRead == sizeof(DWORD)) && (cueCount >= 1))
            {
                CueData cue;
                if (fs->ReadFile(0, &cue, sizeof(CueData), &bytesRead) &&
                    (bytesRead == sizeof(CueData)))
                {
                    data.loop_start = cue.offset;
                    cueFound = true;
                    break;
                }
            }
        }
        else
        {
            fs->SetFilePointer(0, chunk.size, NULL, FILE_CURRENT);
        }
    }

    if (!cueFound)
    {
        // Set default loop markers (from beginning to end).
        data.loop_start = 0;
        data.loop_end = data.num_samples;
    }
    else
    {
        // Try to locate a LIST chunk that provides loop length (via an 'ltxt' chunk).
        fs->SetFilePointer(0, startPos, NULL, FILE_BEGIN);
        while (fs->ReadFile(0, &chunk, sizeof(chunk), &bytesRead) && (bytesRead == sizeof(chunk)))
        {
            if (chunk.id == LIST_ID)
            {
                RiffForm listForm;
                listForm.id = chunk.id;
                listForm.size = chunk.size;
                if (!fs->ReadFile(0, &listForm.formType, sizeof(DWORD), &bytesRead))
                    break;
                if (listForm.formType != adtl_ID)
                {
                    fs->SetFilePointer(0, chunk.size - sizeof(DWORD), NULL, FILE_CURRENT);
                    continue;
                }
                bool foundLtxt = false;
                while (fs->ReadFile(0, &chunk, sizeof(chunk), &bytesRead) && (bytesRead == sizeof(chunk)))
                {
                    if (chunk.id == ltxt_ID)
                    {
                        LtxtData ltxt;
                        if (fs->ReadFile(0, &ltxt, sizeof(ltxt), &bytesRead) && (bytesRead == sizeof(ltxt)))
                        {
                            data.loop_end = data.loop_start + ltxt.length;
                            foundLtxt = true;
                            break;
                        }
                    }
                    else
                    {
                        fs->SetFilePointer(0, chunk.size, NULL, FILE_CURRENT);
                    }
                }
                if (foundLtxt)
                    break;
            }
            else
            {
                fs->SetFilePointer(0, chunk.size, NULL, FILE_CURRENT);
            }
        }
    }

    return true;
}

// Attempts to save the given sound data as a .WAV+ file in the given file system.
bool SaveWAV (IFileSystem *fs, SoundFile &data)
{
	// *** TODO: Make sure that the data stored in 'data' makes sense.

	// We assume here that the given file system is a file.

	COMPTR<IFileSystem> pFile = fs;
	// DWORD startPos = pFile->GetFilePosition (0);

	// Build the chunk structures, calculating their sizes.

	RiffForm riffWave;
	RiffChunk fmt;
	RiffChunk dataChunk;
	RiffChunk cue;
	RiffForm listAdtl;
	RiffChunk ltxt;
	RiffChunk labl;

	ltxt.id = ltxt_ID;
	ltxt.size = sizeof(LtxtData);
	labl.id = labl_ID;
	labl.size = sizeof(DWORD) + strlen(DEFAULT_REGION_LABEL) + 1;
	listAdtl.id = LIST_ID;
	listAdtl.formType = adtl_ID;
	listAdtl.size = sizeof(DWORD) + sizeof(RiffChunk) * 2 + ltxt.size + labl.size;

	fmt.id = fmt_ID;
	fmt.size = sizeof(WAVEFORMATEX);
	dataChunk.id = data_ID;
	dataChunk.size = data.length;
	cue.id = cue_ID;
	cue.size = sizeof(DWORD) + sizeof(CueData);
	riffWave.id = RIFF_ID;
	riffWave.formType = WAVE_ID;
	riffWave.size = sizeof(DWORD) + 4 * sizeof(RiffChunk) + fmt.size + dataChunk.size + cue.size + listAdtl.size;

	//
	// Write out the data.
	//

	DWORD count;

	// RIFF/WAVE header
	if (!pFile->WriteFile (0, &riffWave, sizeof(riffWave), &count))
	{
		return false;
	}

	// fmt chunk
	if (!pFile->WriteFile (0, &fmt, sizeof(fmt), &count))
	{
		return false;
	}

	{
		WAVEFORMATEX waveFmt;
		waveFmt.wFormatTag = WAVE_FORMAT_PCM;
		waveFmt.nChannels = data.format.num_channels;
		waveFmt.wBitsPerSample = data.format.bytes_per_channel * 8;
		waveFmt.nSamplesPerSec = data.format.samples_per_sec;
		waveFmt.nBlockAlign = data.format.num_channels * data.format.bytes_per_channel;
		waveFmt.nAvgBytesPerSec = waveFmt.nSamplesPerSec * waveFmt.nBlockAlign;
		waveFmt.cbSize = 0;

		if (!pFile->WriteFile (0, &waveFmt, sizeof(waveFmt), &count))
		{
			return false;
		}
	}

	// data chunk.

	if (!pFile->WriteFile (0, &dataChunk, sizeof(dataChunk), &count))
	{
		return false;
	}

	if (!pFile->WriteFile (0, data.samples, data.length, &count))
	{
		return false;
	}

	// cue chunk

	if (!pFile->WriteFile (0, &cue, sizeof(cue), &count))
	{
		return false;
	}

	{
		DWORD cueCount = 1;

		if (!pFile->WriteFile (0, &cueCount, sizeof(cueCount), &count))
		{
			return false;
		}

		CueData cd;
		cd.index = 1;
		cd.offset = data.loop_start;
		cd.keyLong = data_ID;
		cd.pad[0] = 0;
		cd.pad[1] = 0;
		cd.offset2 = cd.offset;

		if (!pFile->WriteFile (0, &cd, sizeof(cd), &count))
		{
			return false;
		}
	}

	// LIST/adtl header

	if (!pFile->WriteFile (0, &listAdtl, sizeof(listAdtl), &count))
	{
		return false;
	}

	// ltxt chunk

	if (!pFile->WriteFile (0, &ltxt, sizeof(ltxt), &count))
	{
		return false;
	}

	{
		LtxtData ld;

		ld.cueIndex = 1;
		ld.length = data.loop_end - data.loop_start;
		ld.keyLong = rgn_ID;
		ld.pad[0] = 0;
		ld.pad[1] = 0;


		if (!pFile->WriteFile (0, &ld, sizeof(ld), &count))
		{
			return false;
		}
	}

	// labl chunk

	if (!pFile->WriteFile (0, &labl, sizeof(labl), &count))
	{
		return false;
	}

	{
		DWORD cueIndex = 1;
		if (!pFile->WriteFile (0, &cueIndex, sizeof(cueIndex), &count))
		{
			return false;
		}

		if (!pFile->WriteFile (0, DEFAULT_REGION_LABEL, strlen(DEFAULT_REGION_LABEL)+1, &count))
		{
			return false;
		}
	}

	// All is well, so return true.
	return true;
}
