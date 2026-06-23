/*
 * wavlib.cpp
 *
 * Sound Forge RIFF/WAVE reading and writing. See wavlib.h for the contract.
 *
 * Sound Forge writes ordinary RIFF/WAVE files plus a few extra chunks that mark
 * cue points and regions, which wavlib uses to recover a loop region:
 *
 *   - A "cue " chunk holds a DWORD count followed by that many cue structures.
 *     Each cue records a cue index and its offset (in samples). The first cue's
 *     offset is taken as the loop start.
 *
 *   - A LIST/"adtl" form holds an "ltxt" chunk per region. The "ltxt" chunk gives
 *     the starting cue index and the region length (in samples), from which the
 *     loop end is derived. (The accompanying "labl" chunk - the region name - is
 *     written but not read back.)
 *
 * MP3 ("MPEGLAYER3") data is decoded to 16-bit PCM through the ACM, which on a
 * standard Windows install resolves to the Fraunhofer IIS decoder.
 */

#include "WaveReader.h"

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
// RIFF chunk identifiers
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
const unsigned long rgn_ID  = CHUNK_NAME('r','g','n',' ');
const unsigned long fact_ID = CHUNK_NAME('f','a','c','t');
const unsigned long trim_ID = CHUNK_NAME('t','r','i','m');

/*
 * Packs the first four characters of 'str' into a little-endian chunk id.
 */
inline unsigned long str2id(char* str)
{
	return
	  (unsigned long) str[0]        +
	(((unsigned long) str[1])<<8)   +
	(((unsigned long) str[2])<<16)  +
	(((unsigned long) str[3])<<24);
}

//
// On-disk chunk layouts
//

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
	DWORD sampleLength; // Number of PCM samples after decode.
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
		char          key[4];  // Set to 'data'.
		unsigned long keyLong;
	};
	DWORD pad[2];  // Both set to 0.
	DWORD offset2; // Same as offset.
};

struct LtxtData
{
	DWORD cueIndex;
	DWORD length;
	union
	{
		char          key[4];  // Set to 'rgn '.
		unsigned long keyLong;
	};
	DWORD pad[2];  // Both set to 0.
};
#pragma pack()

/*
 * Scans 'fs' from 'startPos' for the chunk whose id is 'chunkId', leaving the
 * file positioned just past the chunk header (at the chunk body) on success.
 * Returns true if found.
 */
bool FindRiffChunk(IFileSystem* fs, DWORD chunkId, RiffChunk& chunk, DWORD startPos)
{
	DWORD bytesRead = 0;
	fs->SetFilePointer(0, startPos, nullptr, FILE_BEGIN);
	while (fs->ReadFile(0, &chunk, sizeof(chunk), &bytesRead) && (bytesRead == sizeof(chunk)))
	{
		if (chunk.id == chunkId)
			return true;
		// Skip the current chunk's data.
		fs->SetFilePointer(0, chunk.size, nullptr, FILE_CURRENT);
	}
	return false;
}

/*
 * Scans 'fs' from 'startPos' for the chunk 'chunkId' and reads its body into
 * 'out_chunk'. Returns true if the chunk was found and fully read.
 */
template<typename t_chunk>
bool FindChunk(IFileSystem* fs, DWORD chunkId, t_chunk& out_chunk, DWORD startPos)
{
	DWORD bytesRead = 0;
	fs->SetFilePointer(0, startPos, nullptr, FILE_BEGIN);
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
		// Skip the current chunk's data.
		fs->SetFilePointer(0, chunk.size, nullptr, FILE_CURRENT);
	}
	return false;
}

#include <vector>

/*
 * Loads a WAV (or WAV+ with markers) file from 'fs' into 'data'. Supports PCM
 * (WAVE_FORMAT_PCM) and MPEG Layer-3 (WAVE_FORMAT_MPEGLAYER3); MP3 is decoded to
 * 16-bit PCM through the ACM (the Fraunhofer IIS codec). Returns true on success.
 */
bool LoadWAV(IFileSystem* fs, SoundFile& data)
{
	// Initialize the SoundFile structure.
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

	// Read the entire format chunk.
	std::vector<char> fmtBuffer(fmtChunk.size);
	if (!fs->ReadFile(0, fmtBuffer.data(), fmtChunk.size, &bytesRead) || (bytesRead != fmtChunk.size))
		return false;

	// The first part of this chunk is a standard WAVEFORMATEX.
	WAVEFORMATEX* pwfx = reinterpret_cast<WAVEFORMATEX*>(fmtBuffer.data());

	// Only PCM and MPEG Layer-3 are supported.
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
		// MP3 decodes to 16-bit PCM.
		data.format.num_channels = pwfx->nChannels;
		data.format.bytes_per_channel = 2; // 16 bits per channel
		data.format.samples_per_sec = pwfx->nSamplesPerSec;
		data.format.bytes_per_sample = data.format.num_channels * data.format.bytes_per_channel;
	}

	// ----- Find the "data" chunk holding the audio samples -----
	fs->SetFilePointer(0, startPos, nullptr, FILE_BEGIN);
	RiffChunk dataChunk;
	if (!FindRiffChunk(fs, data_ID, dataChunk, startPos))
		return false;

	if (pwfx->wFormatTag == WAVE_FORMAT_PCM)
	{
		// PCM: load the sample data directly.
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
		// ----- MP3: load the compressed data and decode it via the ACM -----

		// Read the compressed MP3 data into a temporary buffer.
		char* mp3Buffer = new char[dataChunk.size];
		if (!fs->ReadFile(0, mp3Buffer, dataChunk.size, &bytesRead) || (bytesRead != dataChunk.size))
		{
			delete[] mp3Buffer;
			return false;
		}

		// Destination PCM format.
		WAVEFORMATEX destFormat = { 0 };
		destFormat.wFormatTag = WAVE_FORMAT_PCM;
		destFormat.nChannels = pwfx->nChannels;
		destFormat.nSamplesPerSec = pwfx->nSamplesPerSec;
		destFormat.wBitsPerSample = 16; // decode to 16-bit PCM
		destFormat.nBlockAlign = destFormat.nChannels * (destFormat.wBitsPerSample / 8);
		destFormat.nAvgBytesPerSec = destFormat.nSamplesPerSec * destFormat.nBlockAlign;
		destFormat.cbSize = 0;

		// Open the ACM stream (uses the Fraunhofer IIS MPEG Layer-3 decoder).
		HACMSTREAM hAcmStream = nullptr;
		MMRESULT result = acmStreamOpen(&hAcmStream, nullptr, pwfx, &destFormat, nullptr, 0,
			0, ACM_STREAMOPENF_NONREALTIME);
		if (result != MMSYSERR_NOERROR || hAcmStream == nullptr)
		{
			delete[] mp3Buffer;
			return false;
		}

		// Determine the decoded-PCM buffer size.
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

		// Prepare and run the conversion.
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

		// Release the ACM objects.
		acmStreamUnprepareHeader(hAcmStream, &streamHeader, 0);
		acmStreamClose(hAcmStream, 0);

		// Store the decoded PCM in the SoundFile.
		data.samples = pcmBuffer;
		//data.length = streamHeader.cbDstLengthUsed;
		data.length = pcmSize;
		data.num_samples = data.length / data.format.bytes_per_sample;

		// Free the temporary MP3 buffer.
		delete[] mp3Buffer;
	}

	if (has_fact)
	{
		/*
		 * $todo why are some fact chunks saying that the length is larger than the amount of data?
		 * is the actual buffer length supposed to be larger and the audio is filled with zero at the end?
		 */
		data.length = __min(data.format.bytes_per_sample * fact.sampleLength, data.length);
		data.num_samples = fact.sampleLength;
	}

	if (has_trim)
	{
		data.start_offset = data.format.bytes_per_sample * trim.trim;
	}

	// ----- Process optional cue and LIST chunks for loop markers -----
	fs->SetFilePointer(0, startPos, nullptr, FILE_BEGIN);

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
			fs->SetFilePointer(0, chunk.size, nullptr, FILE_CURRENT);
		}
	}

	if (!cueFound)
	{
		// Default loop markers (whole file).
		data.loop_start = 0;
		data.loop_end = data.num_samples;
	}
	else
	{
		// Locate a LIST chunk that provides the loop length (via an "ltxt" chunk).
		fs->SetFilePointer(0, startPos, nullptr, FILE_BEGIN);
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
					fs->SetFilePointer(0, chunk.size - sizeof(DWORD), nullptr, FILE_CURRENT);
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
						fs->SetFilePointer(0, chunk.size, nullptr, FILE_CURRENT);
					}
				}
				if (foundLtxt)
					break;
			}
			else
			{
				fs->SetFilePointer(0, chunk.size, nullptr, FILE_CURRENT);
			}
		}
	}

	return true;
}

/*
 * Writes 'data' to 'fs' as a WAV+ file: the RIFF/WAVE header, the format and data
 * chunks, and the cue/LIST(adtl) chunks that record the loop region. Returns true
 * on success.
 */
bool SaveWAV(IFileSystem* fs, SoundFile& data)
{
	// *** TODO: validate that the contents of 'data' make sense.

	// The given file system is assumed to be a file.
	COMPTR<IFileSystem> pFile = fs;

	// Build the chunk headers, computing their sizes.
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
	// Write the data.
	//

	DWORD count;

	// RIFF/WAVE header.
	if (!pFile->WriteFile(0, &riffWave, sizeof(riffWave), &count))
	{
		return false;
	}

	// fmt chunk.
	if (!pFile->WriteFile(0, &fmt, sizeof(fmt), &count))
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

		if (!pFile->WriteFile(0, &waveFmt, sizeof(waveFmt), &count))
		{
			return false;
		}
	}

	// data chunk.
	if (!pFile->WriteFile(0, &dataChunk, sizeof(dataChunk), &count))
	{
		return false;
	}

	if (!pFile->WriteFile(0, data.samples, data.length, &count))
	{
		return false;
	}

	// cue chunk.
	if (!pFile->WriteFile(0, &cue, sizeof(cue), &count))
	{
		return false;
	}

	{
		DWORD cueCount = 1;

		if (!pFile->WriteFile(0, &cueCount, sizeof(cueCount), &count))
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

		if (!pFile->WriteFile(0, &cd, sizeof(cd), &count))
		{
			return false;
		}
	}

	// LIST/adtl header.
	if (!pFile->WriteFile(0, &listAdtl, sizeof(listAdtl), &count))
	{
		return false;
	}

	// ltxt chunk.
	if (!pFile->WriteFile(0, &ltxt, sizeof(ltxt), &count))
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

		if (!pFile->WriteFile(0, &ld, sizeof(ld), &count))
		{
			return false;
		}
	}

	// labl chunk.
	if (!pFile->WriteFile(0, &labl, sizeof(labl), &count))
	{
		return false;
	}

	{
		DWORD cueIndex = 1;
		if (!pFile->WriteFile(0, &cueIndex, sizeof(cueIndex), &count))
		{
			return false;
		}

		if (!pFile->WriteFile(0, DEFAULT_REGION_LABEL, strlen(DEFAULT_REGION_LABEL) + 1, &count))
		{
			return false;
		}
	}

	return true;
}
