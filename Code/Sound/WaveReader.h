#ifndef WAVLIB_H
#define WAVLIB_H

/*
 * wavlib.h
 *
 * Reads and writes Sound Forge-style RIFF/WAVE files into the SoundManager's
 * in-memory representation. Loading supports uncompressed PCM and MPEG Layer-3
 * (decoded to 16-bit PCM via the ACM), plus the cue/region chunks Sound Forge
 * writes to mark a loop region.
 */

#include <FileSys.h>

/*
 * Describes a sound's data format.
 *
 * NOTE: all sound data is assumed to be unsigned, so the minimum deflection is 0,
 * not a negative value.
 */
struct SoundFormat
{
	unsigned short num_channels;      // Channels per sample.
	unsigned short bytes_per_channel; // 1 for 8-bit, 2 for 16-bit, etc.
	unsigned short samples_per_sec;   // Sample rate, in Hz.
	unsigned short bytes_per_sample;  // num_channels * bytes_per_channel.
};

/*
 * A loaded sound: its format, loop region and decoded sample data.
 *
 * NOTE: 'samples' points to a heap buffer. LoadWAV() allocates it (the caller
 * must delete[] it); SaveWAV() expects it to be valid.
 */
struct SoundFile
{
	SoundFormat   format;       // Format of the sample data.
	unsigned int  loop_start;   // Start of the loop region, in samples.
	unsigned int  loop_end;     // End of the loop region, in samples (inclusive; <= num_samples).
	unsigned int  num_samples;  // Number of samples in the file.
	unsigned int  length;       // Length of the data, in bytes (== num_samples * format.bytes_per_sample).
	unsigned int  start_offset; // Byte offset of the first valid sample (codec trim).
	char*         samples;      // Sample data, 'length' bytes long.
};

/*
 * Loads 'waveFile' as a RIFF/WAVE (or WAVE+ with markers) file into 'data'.
 * Reading begins at the current file position. Returns true on success.
 *
 * WARNING: data.samples is heap-allocated; the caller must delete[] it.
 */
extern bool LoadWAV(IFileSystem* waveFile, SoundFile& data);

/*
 * Writes 'data' to 'waveFile' as a WAVE+ file (with cue/region loop markers).
 * Writing begins at the current file position. Returns true on success.
 */
extern bool SaveWAV(IFileSystem* waveFile, SoundFile& data);

#endif // WAVLIB_H
