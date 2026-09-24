#pragma once

// Shared test utilities: build a deterministic WAV fixture with full ground-truth
// control over the bext (BWF) and iXML chunks, and raw-parse RIFF chunks. Used by
// MetadataRoundTripTest (unit) and SplitPipelineTest (end-to-end) so the fixture and
// RIFF helpers live in one place.

#include <juce_core/juce_core.h>

namespace testwav
{
    using namespace juce;

    // Classic field-recorder iXML (NOT ASWG-namespaced) -- the schema JUCE 8 drops.
    inline const char* iXmlText()
    {
        return
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<BWFXML>"
              "<IXML_VERSION>1.5</IXML_VERSION>"
              "<PROJECT>ZQ Onboarding Project</PROJECT>"
              "<SCENE>SC01</SCENE>"
              "<TAKE>003</TAKE>"
              "<TAPE>TAPE_A</TAPE>"
              "<NOTE>reclamation null test</NOTE>"
              "<CIRCLED>FALSE</CIRCLED>"
              "<SPEED><MASTER_SPEED>48000/1</MASTER_SPEED><TIMECODE_RATE>25/1</TIMECODE_RATE></SPEED>"
              "<TRACK_LIST>"
                "<TRACK_COUNT>2</TRACK_COUNT>"
                "<TRACK><CHANNEL_INDEX>1</CHANNEL_INDEX><INTERLEAVE_INDEX>1</INTERLEAVE_INDEX><NAME>Boom</NAME></TRACK>"
                "<TRACK><CHANNEL_INDEX>2</CHANNEL_INDEX><INTERLEAVE_INDEX>2</INTERLEAVE_INDEX><NAME>Lav1</NAME></TRACK>"
              "</TRACK_LIST>"
            "</BWFXML>";
    }

    inline const String& bextDescription() { static const String s ("ZQ TEST DESCRIPTION 12345"); return s; }
    inline const String& bextOriginator()  { static const String s ("ZQ-RECORDER"); return s; }

    // Classic iXML markers expected to survive preservation.
    inline StringArray classicFields()
    {
        return { "<PROJECT>", "<SCENE>", "<TAKE>", "<TAPE>", "<TRACK_LIST>", "Boom", "Lav1" };
    }

    //==============================================================================
    // Little-endian RIFF assembly helpers (no JUCE writer -> true ground truth).
    inline void appendU32 (MemoryBlock& mb, uint32 v)
    {
        const uint8 b[4] = { (uint8) v, (uint8) (v >> 8), (uint8) (v >> 16), (uint8) (v >> 24) };
        mb.append (b, 4);
    }

    inline void appendU16 (MemoryBlock& mb, uint16 v)
    {
        const uint8 b[2] = { (uint8) v, (uint8) (v >> 8) };
        mb.append (b, 2);
    }

    inline void appendU64 (MemoryBlock& mb, uint64 v)
    {
        uint8 b[8];
        for (int i = 0; i < 8; ++i) b[i] = (uint8) (v >> (8 * i));
        mb.append (b, 8);
    }

    inline void appendFixed (MemoryBlock& mb, const String& s, int len)
    {
        std::vector<char> buf ((size_t) len, 0);
        const char* utf = s.toRawUTF8();
        const int n = jmin (len, (int) strlen (utf));
        memcpy (buf.data(), utf, (size_t) n);
        mb.append (buf.data(), (size_t) len);
    }

    inline void appendZeros (MemoryBlock& mb, int len)
    {
        std::vector<char> buf ((size_t) len, 0);
        mb.append (buf.data(), (size_t) len);
    }

    inline void appendChunk (MemoryBlock& mb, const char* id, const MemoryBlock& payload)
    {
        mb.append (id, 4);
        appendU32 (mb, (uint32) payload.getSize());
        mb.append (payload.getData(), payload.getSize());
        if ((payload.getSize() & 1) != 0)   // RIFF chunks are word-aligned
            appendZeros (mb, 1);
    }

    // Build a complete `numFrames`-frame, 2-channel, 16-bit, 48k WAV with fmt/bext/iXML/data.
    inline MemoryBlock buildFixtureWav (int numFrames = 16)
    {
        MemoryBlock fmtP;
        appendU16 (fmtP, 1);                 // PCM
        appendU16 (fmtP, 2);                 // channels
        appendU32 (fmtP, 48000);             // sample rate
        appendU32 (fmtP, 48000u * 2u * 2u);  // byte rate
        appendU16 (fmtP, (uint16) (2 * 2));  // block align
        appendU16 (fmtP, 16);                // bits per sample

        MemoryBlock bextP;
        appendFixed (bextP, bextDescription(), 256);
        appendFixed (bextP, bextOriginator(), 32);
        appendFixed (bextP, "ZQREF-0001", 32);
        appendFixed (bextP, "2026-06-28", 10);
        appendFixed (bextP, "12:00:00", 8);
        appendU32 (bextP, 123456);           // TimeReference low
        appendU32 (bextP, 0);                // TimeReference high
        appendU16 (bextP, 1);                // Version
        appendZeros (bextP, 64);             // UMID
        appendU16 (bextP, 0); appendU16 (bextP, 0); appendU16 (bextP, 0);
        appendU16 (bextP, 0); appendU16 (bextP, 0);   // 5 loudness fields
        appendZeros (bextP, 180);            // Reserved
        jassert (bextP.getSize() == 602);

        MemoryBlock ixmlP;
        ixmlP.append (iXmlText(), strlen (iXmlText()));

        MemoryBlock dataP;
        for (int i = 0; i < numFrames; ++i)
        {
            appendU16 (dataP, (uint16) (int16) (i * 100));
            appendU16 (dataP, (uint16) (int16) (-i * 100));
        }

        MemoryBlock body;
        body.append ("WAVE", 4);
        appendChunk (body, "fmt ", fmtP);
        appendChunk (body, "bext", bextP);
        appendChunk (body, "iXML", ixmlP);
        appendChunk (body, "data", dataP);

        MemoryBlock file;
        file.append ("RIFF", 4);
        appendU32 (file, (uint32) body.getSize());
        file.append (body.getData(), body.getSize());
        return file;
    }

    // Build a small but spec-valid RF64/BW64 mono WAV (ds64 + fmt + data, NO iXML/bext).
    // Mimics a JUCE >4 GB output without the size: ds64 carries the real 64-bit sizes while
    // the bytes-4..7 and data-chunk size fields hold the 0xFFFFFFFF sentinel.
    inline MemoryBlock buildRf64NoMetaWav (int numFrames = 2000)
    {
        const uint32 sentinel = 0xFFFFFFFFu;
        const uint64 dataBytes = (uint64) numFrames * 1u * 2u; // mono, 16-bit

        MemoryBlock fmtP;
        appendU16 (fmtP, 1);                 // PCM
        appendU16 (fmtP, 1);                 // channels (mono)
        appendU32 (fmtP, 48000);             // sample rate
        appendU32 (fmtP, 48000u * 1u * 2u);  // byte rate
        appendU16 (fmtP, (uint16) (1 * 2));  // block align
        appendU16 (fmtP, 16);                // bits

        MemoryBlock pcm;
        for (int i = 0; i < numFrames; ++i)
            appendU16 (pcm, (uint16) (int16) (i * 50));

        // File size = RF64 hdr(12) + ds64(8+28) + fmt(8+16) + data hdr(8) + dataBytes.
        const uint64 fileSize = 12 + (8 + 28) + (8 + 16) + 8 + dataBytes;
        const uint64 riffSize = fileSize - 8;

        MemoryBlock ds64;                    // 28-byte payload, no chunk-size table
        appendU64 (ds64, riffSize);
        appendU64 (ds64, dataBytes);         // real data size
        appendU64 (ds64, (uint64) numFrames);// sample count
        appendU32 (ds64, 0);                 // table length

        MemoryBlock file;
        file.append ("RF64", 4);
        appendU32 (file, sentinel);          // size sentinel -> see ds64
        file.append ("WAVE", 4);
        appendChunk (file, "ds64", ds64);
        appendChunk (file, "fmt ", fmtP);
        // data chunk written manually so its size field is the RF64 sentinel.
        file.append ("data", 4);
        appendU32 (file, sentinel);
        file.append (pcm.getData(), pcm.getSize());
        jassert ((uint64) file.getSize() == fileSize);
        return file;
    }

    //==============================================================================
    inline uint32 readU32LE (const uint8* p)
    {
        return (uint32) p[0] | ((uint32) p[1] << 8) | ((uint32) p[2] << 16) | ((uint32) p[3] << 24);
    }

    inline uint64 readU64LE (const uint8* p)
    {
        uint64 v = 0;
        for (int i = 0; i < 8; ++i) v |= (uint64) p[i] << (8 * i);
        return v;
    }

    // RF64-aware data size: returns the real `data` byte count from the ds64 chunk if the
    // file is RF64/BW64 (where the data chunk header carries the 0xFFFFFFFF sentinel), else 0.
    inline uint64 rf64DataSize (const uint8* d, size_t n)
    {
        if (n < 28) return 0;
        if (memcmp (d, "RF64", 4) != 0 && memcmp (d, "BW64", 4) != 0) return 0;
        if (memcmp (d + 12, "ds64", 4) != 0) return 0;
        return readU64LE (d + 28); // ds64 payload: riffSize(8), dataSize(8) -> at offset 20+8
    }

    // Walk RIFF/RF64 chunks (resolving the RF64 data-size sentinel) and return a payload as text.
    inline String findChunkText (const MemoryBlock& mb, const char* id)
    {
        const uint8* d = (const uint8*) mb.getData();
        const size_t n = mb.getSize();
        if (n < 12) return {};
        const uint64 dataReal = rf64DataSize (d, n);

        size_t pos = 12; // skip magic + size + "WAVE"
        while (pos + 8 <= n)
        {
            char cid[4]; memcpy (cid, d + pos, 4);
            uint64 sz = readU32LE (d + pos + 4);
            if (sz == 0xFFFFFFFFu && memcmp (cid, "data", 4) == 0 && dataReal > 0)
                sz = dataReal; // RF64 sentinel -> real size from ds64
            const size_t payloadPos = pos + 8;
            if (memcmp (cid, id, 4) == 0)
            {
                const size_t avail = n - payloadPos;
                return String::fromUTF8 ((const char*) (d + payloadPos), (int) jmin ((size_t) sz, avail));
            }
            pos = payloadPos + (size_t) sz + (size_t) (sz & 1);
        }
        return {};
    }

    inline bool hasChunk (const MemoryBlock& mb, const char* id)
    {
        const uint8* d = (const uint8*) mb.getData();
        const size_t n = mb.getSize();
        if (n < 12) return false;
        const uint64 dataReal = rf64DataSize (d, n);

        size_t pos = 12;
        while (pos + 8 <= n)
        {
            char cid[4]; memcpy (cid, d + pos, 4);
            if (memcmp (cid, id, 4) == 0) return true;
            uint64 sz = readU32LE (d + pos + 4);
            if (sz == 0xFFFFFFFFu && memcmp (cid, "data", 4) == 0 && dataReal > 0)
                sz = dataReal;
            pos = pos + 8 + (size_t) sz + (size_t) (sz & 1);
        }
        return false;
    }

    inline bool ixmlFieldPresent (const String& ixml, const String& token)
    {
        return ixml.contains (token);
    }
}
