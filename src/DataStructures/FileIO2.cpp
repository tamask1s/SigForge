#include <stdio.h>
#include <string.h>
#include "fileio2.h"

void ThrowError(void*, const char* errstr1, const char* errstr2, int junk2)
{
    printf("ERROR: %s %s\n", errstr1, errstr2);
}

bool CheckFile(const char* Filename)
{
    FILE* f = fopen(Filename, "rb");
    if (f)
    {
        fclose(f);
        return true;
    }
    else
        return false;
}

int GetFileLen(const char*Filename)
{
    FILE* f = fopen(Filename, "rb");
    if (f)
    {
        fseek(f, 0, SEEK_END);
        int flength = ftell(f);
        fclose(f);
        return flength;
    }
    else
        return -1;
}

int SaveBuffer(const char Filename[], byte* buffer, int length, int offset, bool rewrite)
{
    int result = 0;
    FILE* f;

    if (rewrite)
        f = fopen(Filename, "wb");
    else
        f = fopen(Filename, "r+b");

    if (!f)
        f = fopen(Filename, "wb");
    if (f)
    {
        result = SaveBuffer(f, buffer, length, offset);
        fclose(f);
        return result;
    }
    else
    {
        ThrowError(NULL, "FileIO::SaveBuffer Message: Write Error. Disk may be full.", Filename, 0);
        return result;
    }
}

int SaveBuffer(FILE*f, byte* buffer, int length, int offset)
{
    int result = 0;
    if (!f)
        ThrowError(NULL, "FileIO::SaveBuffer Message: No file", "FILE", 0);
    else
    {
        if (offset == ENDOFFILE)
            fseek(f, 0, SEEK_END);
        if (offset == CONTINUE)
        {/*DO NOTHING*/}
        if (offset >= 0)
            fseek(f, offset, SEEK_SET);
        result = fwrite(buffer, sizeof(byte), length, f);
    }
    if (result != length)
        ThrowError(NULL, "FileIO::SaveBufferOffset Message: Write Error. Disk may be full.", "FILE", 0);

    return result;
}

byte*  LoadBuffer(const char Filename[], int offset, int* nrofbytesreaded, int nrbytestoread)
{
    byte* buffer = 0;
    FILE* f = fopen(Filename, "rb");
    if (f)
    {
        buffer = LoadBuffer(f, offset, nrofbytesreaded, nrbytestoread);
        fclose(f);
        return buffer;
    }
    else
    {
        if (nrofbytesreaded)
            *nrofbytesreaded = 0;
        ThrowError(NULL, "FileIO::LoadBuffer Message: Disk Error. Can`t read file", Filename, 0);
        return buffer;
    }
}

byte* LoadBuffer(FILE* f, int offset, int* nrofbytesreaded, int nrbytestoread)
{
    byte* buffer = 0;
    if (f)
    {
        if (nrbytestoread == WHOLEFILE)
        {
            fseek(f, 0, SEEK_END);
            int flength = ftell(f);
            nrbytestoread = flength - offset;
        }
        fseek(f, offset, SEEK_SET);
        buffer = new byte[nrbytestoread];
        int tf = fread(buffer, sizeof(byte), nrbytestoread, f);
        if (nrofbytesreaded)
            *nrofbytesreaded = tf;
        if (tf != nrbytestoread)
            ThrowError(NULL, "FileIO::LoadBuffer Message: Reading beyond end of file", "FILE", 0);
    }
    else
    {
        if (nrofbytesreaded)
            *nrofbytesreaded = 0;
        ThrowError(NULL, "FileIO::LoadBuffer Message: No read file", "FILE", 0);
    }
    return buffer;
}

byte LoadByte(FILE* f, int offset)
{
    byte buffer;
    fseek(f, offset, SEEK_SET);
    fread(&buffer,/*sizeof(byte)*/1, 1, f);
    return buffer;
}

int LoadInt(FILE* f, int offset)
{
    int buffer;
    fseek(f, offset, SEEK_SET);
    fread(&buffer,/*sizeof(int)*/4, 1, f);
    return buffer;
}

unsigned int LoadUInt(FILE* f, int offset)
{
    unsigned int buffer;
    fseek(f, offset, SEEK_SET);
    fread(&buffer,/*sizeof(int)*/4, 1, f);
    return buffer;
}

int IFileIO::LoadFromFile     (FILE*f, int offset, int*totalstreamlen)
{
    int result = 1;
    if (!f)
        ThrowError(NULL, "IFileIO::LoadFromFile Message: Disk Error. Can`t read file", "FILE", 0);
    else
    {
        int totallen;
        int additionals;
        int packer;
        int uncompressedlen;
        fseek(f, offset, SEEK_SET);
        fread((byte*)&totallen,/*sizeof(byte)*/1, 4, f);
        fread((byte*)&additionals,/*sizeof(byte)*/1, 4, f);
        fread((byte*)&packer,/*sizeof(byte)*/1, 4, f);
        fread((byte*)&uncompressedlen,/*sizeof(byte)*/1, 4, f);
        byte* buffer = 0;
        if (totallen)
        {
            if (totalstreamlen)
                *totalstreamlen = totallen + additionals;
            buffer = new byte[totallen];
            fread(buffer,/*sizeof(byte)*/1, totallen, f);
            byte* unpackedbuffer = 0;
            byte* buffertodeserialize = buffer;
            result = 2;
            if (totallen)
                Deserialize(buffertodeserialize);
            else
            {
                Deserialize(0);
                result = 0;
            }
            if (unpackedbuffer)
                delete[] unpackedbuffer;
            delete[]buffer;
        }
        else
        {
            if (totalstreamlen)
                *totalstreamlen = totallen + additionals;
            Deserialize(buffer);
            result = 0;
        }
    }
    return result;
}

int IFileIO::LoadFromFile     (char Filename[], int offset, int*totalstreamlen)
{
    int result = 0;
    FILE* f = fopen(Filename, "rb");
    if (!f)
        ThrowError(NULL, "IFileIO::LoadFromFile Message: Disk Error. Can`t read file", Filename, 0);
    else
    {
        result = LoadFromFile (f, offset, totalstreamlen);
        fclose(f);
    }
    return result;
}

int IFileIO::SaveToFile       (char Filename[], int offset, bool rewrite, int packer, int additionalbytes)
{
    int result = 0;
    FILE* f;

    if (rewrite)
        f = fopen(Filename, "wb+");
    else
        f = fopen(Filename, "r+b");

    if (!f)
        f = fopen(Filename, "wb+");
    if (!f)
        ThrowError(NULL, "IFileIO::SaveToFile Message: Can`t create file", Filename, 0);
    else
    {
        result = SaveToFile(f, offset, packer, additionalbytes);
        fclose(f);
        if (result <= 0)
            ThrowError(NULL, "IFileIOXX::SaveToFile Message: Write Error. Disk may be full.", Filename, 0);
    }
    return result;
}

int IFileIO::SaveToFile       (FILE*f, int offset, int packer, int additionalbytes)
{
    unsigned int result = 0;
    unsigned int totallen = 0;
    if (!f)
        ThrowError(NULL, "IFileIO::SaveToFile Message: Can`t create file", "FILE", 0);
    else
    {
        if (offset == ENDOFFILE)
            fseek(f, 0, SEEK_END);
        else
            fseek(f, offset, SEEK_SET);

        byte* buffer = Serialize(&totallen);
        int uncompressedlen = 0;
        if (buffer)
        {
            byte* packedbuffer = 0;
            byte* buffertosave = buffer;

            result += fwrite((byte*)&totallen,/*sizeof(byte)*/1, 4, f);
            result += fwrite((byte*)&additionalbytes,/*sizeof(byte)*/1, 4, f);
            result += fwrite((byte*)&packer,/*sizeof(byte)*/1, 4, f);
            result += fwrite((byte*)&uncompressedlen,/*sizeof(byte)*/1, 4, f);
            result += fwrite(buffertosave,/*sizeof(byte)*/1, totallen, f);
            if (packedbuffer)
                delete[] packedbuffer;
            delete[]buffer;
        }
        else
        {
            totallen = 0;
            result += fwrite((byte*)&totallen,/*sizeof(byte)*/1, 4, f);
            result += fwrite((byte*)&additionalbytes,/*sizeof(byte)*/1, 4, f);
            result += fwrite((byte*)&packer,/*sizeof(byte)*/1, 4, f);
            result += fwrite((byte*)&uncompressedlen,/*sizeof(byte)*/1, 4, f);
        }
        if (additionalbytes)
        {
            byte*buffer4 = new byte[additionalbytes];
            for (int i = 0; i < additionalbytes; i++)
                buffer4[i] = 0;
            result += fwrite(buffer4,/*sizeof(byte)*/1, additionalbytes, f);
            delete[]buffer4;
        }
    }

    if (result != totallen + additionalbytes + 16)
    {
        ThrowError(NULL, "IFileIOXX::SaveToFile Message: Write Error. Disk may be full.", "FILE", 0);
        result *= -1;
    }

    return result;
}

void IFileIO::Equal(IFileIO& fioin)
{
    byte* buff = fioin.Serialize();
    Deserialize(buff);
    if (buff)
        delete[]buff;
}

CBuffer::CBuffer()
{
    buffer = 0;
    offset = 0;
    bufferlen = 0;
    buffergetted = false;
}

void CBuffer::Deinitialize()
{
    if (buffer)
        delete[]buffer;
    buffer = 0;
    offset = 0;
    bufferlen = 0;
    buffergetted = false;
}

CBuffer::~CBuffer()
{
    if (!buffergetted)
        Deinitialize();
}

void CBuffer::Create(int nrelements)
{
    Deinitialize();
    buffer = new byte[nrelements];
    bufferlen = nrelements;
}

bool CBuffer::CatBuffer(byte* bufftoadd, int bufftoaddlen)
{
    if ((!buffer) || (!bufferlen))
        return false;
    if (offset + bufftoaddlen > bufferlen)
        return false;
    memcpy(buffer + offset, bufftoadd, bufftoaddlen);
    offset += bufftoaddlen;
    return true;
}

bool CBuffer::CatBuffer(IFileIO* inbuffer)
{
    if (!inbuffer)
        return false;
    int bufftoaddlen = inbuffer->GetSerializedLen ();
    if (offset + bufftoaddlen > bufferlen)
        return false;
    unsigned int bufftoaddlen2 = 0;
    byte* bufftoadd = inbuffer->Serialize(&bufftoaddlen2);
    memcpy(buffer + offset, bufftoadd, bufftoaddlen);
    delete[] bufftoadd;
    offset += bufftoaddlen;
    return true;
}

bool CBuffer::Finished()
{
    if (offset == bufferlen)
        return true;
    else
        return false;
}

byte* CBuffer::GetBuffer()
{
    if (Finished())
    {
        buffergetted = true;
        return buffer;
    }
    else
        return 0;
}
